#include "core/usn/usn_parser.h"
#include "core/usn/usn_watcher.h"
#include <Windows.h>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <functional>
#include <iostream>
#include <winioctl.h>

UsnWatcher::UsnWatcher(const std::wstring& volume_name)
    : m_volume_name(volume_name), 
      m_volume(INVALID_HANDLE_VALUE),
      m_running(false) {
    open_volume();
}

UsnWatcher::~UsnWatcher() {
    stop();
    
    if (m_volume != INVALID_HANDLE_VALUE) {
        CloseHandle(m_volume);
        m_volume = INVALID_HANDLE_VALUE;
    }
}

void UsnWatcher::open_volume() {
    if (m_volume != INVALID_HANDLE_VALUE)
        return;

    std::wcout << L"Opening volume: " << m_volume_name << std::endl;
    
    m_volume = CreateFileW(
        m_volume_name.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    if (m_volume == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        std::wcerr << L"Failed to open volume, error: " << err << std::endl;
        throw std::runtime_error("Failed to open volume");
    }
    
    std::wcout << L"Volume opened successfully" << std::endl;
}

void UsnWatcher::enable_journal() {
    CREATE_USN_JOURNAL_DATA create{};
    create.MaximumSize = 0;
    create.AllocationDelta = 0;
    
    DWORD bytes = 0;
    
    if (!DeviceIoControl(
            m_volume,
            FSCTL_CREATE_USN_JOURNAL,
            &create,
            sizeof(create),
            nullptr,
            0,
            &bytes,
            nullptr)) {
        DWORD err = GetLastError();
        if (err == ERROR_JOURNAL_DELETE_IN_PROGRESS) {
            std::wcout << L"USN Journal delete in progress, waiting..." << std::endl;
            Sleep(1000);
            DeviceIoControl(
                m_volume,
                FSCTL_CREATE_USN_JOURNAL,
                &create,
                sizeof(create),
                nullptr,
                0,
                &bytes,
                nullptr);
        } else if (err != ERROR_JOURNAL_NOT_ACTIVE) {
            std::wcerr << L"Failed to create USN Journal, error: " << err << std::endl;
        }
    } else {
        std::wcout << L"USN Journal created/enabled" << std::endl;
    }
}

void UsnWatcher::query_journal() {
    USN_JOURNAL_DATA journal{};
    DWORD bytes = 0;

    if (!DeviceIoControl(
            m_volume,
            FSCTL_QUERY_USN_JOURNAL,
            nullptr,
            0,
            &journal,
            sizeof(journal),
            &bytes,
            nullptr)) {
        DWORD err = GetLastError();
        if (err == ERROR_JOURNAL_NOT_ACTIVE) {
            std::wcout << L"USN Journal not active, enabling it..." << std::endl;
            enable_journal();
            
            if (!DeviceIoControl(
                    m_volume,
                    FSCTL_QUERY_USN_JOURNAL,
                    nullptr,
                    0,
                    &journal,
                    sizeof(journal),
                    &bytes,
                    nullptr)) {
                std::wcerr << L"Failed to query USN Journal after enabling, error: " 
                          << GetLastError() << std::endl;
                throw std::runtime_error("FSCTL_QUERY_USN_JOURNAL failed");
            }
        } else {
            std::wcerr << L"FSCTL_QUERY_USN_JOURNAL failed, error: " << err << std::endl;
            throw std::runtime_error("FSCTL_QUERY_USN_JOURNAL failed");
        }
    }

    std::wcout << L"USN Journal ID: " << journal.UsnJournalID 
              << L", Next USN: " << journal.NextUsn 
              << L", First USN: " << journal.FirstUsn << std::endl;

    if (!m_state.valid() || m_state.journal_id != journal.UsnJournalID) {
        m_state.journal_id = journal.UsnJournalID;
        
        // ✅ 关键修改：从 NextUsn 开始，跳过所有历史记录
        m_state.next_usn = journal.NextUsn;
        m_state.first_run = true;
        
        std::wcout << L"Starting from latest USN (ignoring history): " << m_state.next_usn << std::endl;
    } else {
        m_state.first_run = false;
        std::wcout << L"Resuming from USN: " << m_state.next_usn << std::endl;
    }
}

void UsnWatcher::run(EventCallback callback) {
    open_volume();
    query_journal();
    
    std::wcout << L"=== Starting USN Monitor ===" << std::endl;
    std::wcout << L"Waiting for file system changes..." << std::endl;
    std::wcout << L"Press Ctrl+C to stop" << std::endl;

    m_running.store(true);

    READ_USN_JOURNAL_DATA read{};
    read.StartUsn = m_state.next_usn;
    read.ReasonMask = 0xFFFFFFFF;          // 所有事件
    read.ReturnOnlyOnClose = FALSE;
    read.Timeout = 0;                      // 阻塞直到有事件
    read.BytesToWaitFor = 0;               // 任意新记录即返回
    read.UsnJournalID = m_state.journal_id;

    std::vector<uint8_t> buffer(64 * 1024); // 64KB 缓冲区

    while (m_running.load()) {
        DWORD bytes_returned = 0;

        // std::wcout << L"Waiting for changes (StartUsn=" << read.StartUsn << L")..." << std::endl;

        BOOL ok = DeviceIoControl(
            m_volume,
            FSCTL_READ_USN_JOURNAL,
            &read,
            sizeof(read),
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            &bytes_returned,
            nullptr);

        DWORD err = GetLastError();

        if (!ok) {
            if (err == ERROR_JOURNAL_DELETE_IN_PROGRESS || err == ERROR_JOURNAL_NOT_ACTIVE) {
                std::wcout << L"Journal issue (error " << err << L"), reinitializing..." << std::endl;
                Sleep(1000);
                query_journal();
                read.StartUsn = m_state.next_usn;
                read.UsnJournalID = m_state.journal_id;
                continue;
            } else {
                std::wcerr << L"FSCTL_READ_USN_JOURNAL failed, error: " << err << std::endl;
                break;
            }
        }

        // 成功读取：解析数据
        if (bytes_returned <= sizeof(USN)) {
            // std::wcout << L"No USN records in this read." << std::endl;
            continue;
        }

        // ✅ 关键修复：第一个 8 字节是“下一个起始 USN”
        USN next_start_usn = *reinterpret_cast<USN*>(buffer.data());
        uint8_t* current = buffer.data() + sizeof(USN);
        uint8_t* end = buffer.data() + bytes_returned;
        int record_count = 0;

        while (current < end) {
            auto* record = reinterpret_cast<USN_RECORD*>(current);
            if (record->RecordLength == 0) {
                break; // 防止无限循环
            }

            if (UsnParser::is_relevant(record)) {
                try {
                    UsnEvent evt = UsnParser::parse(record);
                    std::wcout << L"[Event] File: '" << evt.name 
                              << L"', FRN: " << evt.file_id
                              << L", Parent: " << evt.parent_file_id << std::endl;
                    callback(evt);
                    ++record_count;
                } catch (const std::exception& ex) {
                    std::cerr << "Parse error: " << ex.what() << std::endl;
                }
            }

            current += record->RecordLength;
        }

        // ✅ 更新状态：使用正确的 next_start_usn
        read.StartUsn = next_start_usn;
        m_state.next_usn = next_start_usn;
        m_state.last_sync_time = static_cast<uint64_t>(
            std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())
        );

        if (record_count > 0) {
            std::wcout << L"Processed " << record_count << L" records." << std::endl;
        }

        if (m_state.first_run) {
            m_state.first_run = false;
            std::wcout << L"=== Initial scan complete ===" << std::endl;
        }
    }

    std::wcout << L"USN monitor stopped." << std::endl;
}

void UsnWatcher::stop() {
    if (m_running.exchange(false)) {
        std::wcout << L"Stopping USN monitoring..." << std::endl;
    }
}

UsnState UsnWatcher::get_state() const {
    return m_state;
}

void UsnWatcher::set_state(const UsnState& state) {
    m_state = state;
}