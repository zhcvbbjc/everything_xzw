#include "core/mft/mft_reader.h"
#include "core/mft/attribute_parser.h"

#include <windows.h>
#include <winioctl.h>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <iostream>

static constexpr uint32_t MFT_RECORD_SIZE = 1024;

// NTFS Fixup（必须）
bool MFTReader::apply_fixup(void* record, uint32_t record_size) {
    auto* header = reinterpret_cast<NTFS_FILE_RECORD_HEADER*>(record);
    
    // 验证 magic number 先
    if (memcmp(header->signature, "FILE", 4) != 0) {
        return false;
    }
    
    uint16_t fixup_offset  = header->fixup_offset;
    uint16_t fixup_entries = header->fixup_entries;
    
    if (fixup_offset < sizeof(NTFS_FILE_RECORD_HEADER) || 
        fixup_offset >= record_size) {
        return false;
    }
    
    // 这里有问题：fixup_entries 包括 USA 本身
    // 但 fixup_array 大小应该是 fixup_entries
    if (fixup_offset + (fixup_entries * sizeof(uint16_t)) > record_size) {
        return false;
    }
    
    // 检查是否有有效的 fixup 条目
    if (fixup_entries < 2 || fixup_entries > (record_size / 512 + 1)) {
        return false;
    }

    auto* fixup_array = reinterpret_cast<uint16_t*>(
        reinterpret_cast<uint8_t*>(record) + fixup_offset
    );

    uint16_t usa_value = fixup_array[0];

    constexpr uint32_t SECTOR_SIZE = 512;
    uint32_t sector_count = fixup_entries - 1;

    for (uint32_t i = 0; i < sector_count; ++i) {
        uint32_t sector_end = (i + 1) * SECTOR_SIZE - sizeof(uint16_t);

        if (sector_end + sizeof(uint16_t) > record_size) {
            return false;
        }

        auto* check = reinterpret_cast<uint16_t*>(
            reinterpret_cast<uint8_t*>(record) + sector_end
        );

        if (*check != usa_value) {
            // MFT 记录损坏或未完整读取
            return false;
        }

        *check = fixup_array[i + 1];
    }

    return true;
}

MFTReader::MFTReader(const std::wstring& volume)
    : m_volume(INVALID_HANDLE_VALUE),
      m_volume_name(volume) {
    open_volume();
}

MFTReader::~MFTReader() {
    if (m_volume != INVALID_HANDLE_VALUE)
        CloseHandle(m_volume);
}

void MFTReader::open_volume() {
    m_volume = CreateFileW(
        m_volume_name.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        // FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if (m_volume == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open volume");
    }

    NTFS_VOLUME_DATA_BUFFER vol{};
    DWORD bytesReturned = 0;

    if (!DeviceIoControl(
            m_volume,
            FSCTL_GET_NTFS_VOLUME_DATA,
            nullptr, 0,
            &vol, sizeof(vol),
            &bytesReturned,
            nullptr)) {
        CloseHandle(m_volume);
        throw std::runtime_error("FSCTL_GET_NTFS_VOLUME_DATA failed");
    }

    m_bytes_per_cluster = vol.BytesPerCluster;
    m_bytes_per_record  = vol.BytesPerFileRecordSegment;

    m_mft_start_offset =
        static_cast<uint64_t>(vol.MftStartLcn.QuadPart) *
        m_bytes_per_cluster;

    m_max_records =
        static_cast<uint64_t>(
            vol.MftValidDataLength.QuadPart / m_bytes_per_record
        );
    
    std::wcout << L"MFT start LCN: " << vol.MftStartLcn.QuadPart << L"\n";
    std::wcout << L"Bytes per cluster: " << m_bytes_per_cluster << L"\n";
    std::wcout << L"Bytes per record: " << m_bytes_per_record << L"\n";
    std::wcout << L"MFT valid data length: " << vol.MftValidDataLength.QuadPart << L"\n";
    std::wcout << L"Max records: " << m_max_records << L"\n";
}

bool MFTReader::read_mft_record(uint64_t record_index, void* buffer) {
    if (buffer == nullptr)
        return false;

    if (buffer == nullptr)
        return false;
    
    // 调试输出
    if (record_index % 10000 == 0) {
        std::wcout << L"Reading record " << record_index 
                  << L", offset: " 
                  << (m_mft_start_offset + record_index * m_bytes_per_record)
                  << L"\n";
    }
    
    // 检查参数
    if (m_bytes_per_record == 0 || m_bytes_per_record > 65536) {
        return false;
    }

    // 计算 MFT 记录的绝对字节偏移
    uint64_t abs_offset =
        m_mft_start_offset + record_index * m_bytes_per_record;

    // 设置文件指针
    LARGE_INTEGER li;
    li.QuadPart = static_cast<LONGLONG>(abs_offset);

    if (!SetFilePointerEx(
            m_volume,
            li,
            nullptr,
            FILE_BEGIN)) {
        return false;
    }

    DWORD bytesRead = 0;

    BOOL ok = ReadFile(
        m_volume,
        buffer,
        static_cast<DWORD>(m_bytes_per_record),
        &bytesRead,
        nullptr   // 同步 I/O
    );

    if (!ok)
        return false;

    // 必须完整读到一个 MFT record
    if (bytesRead != m_bytes_per_record)
        return false;

    return true;
}

bool MFTReader::enumerate(
    const std::function<bool(const Entry&)>& callback) {

    std::vector<uint8_t> buffer(m_bytes_per_record);
    uint32_t fail_count = 0;
    uint64_t empty_count = 0;

    for (uint64_t i = 0; i < m_max_records; ++i) {
        if (i % 100000 == 0 && i > 0) {
            std::wcout << L"Scanned " << i << L"/" << m_max_records 
                      << L" (" << (i * 100 / m_max_records) << L"%)\n";
        }

        if (!read_mft_record(i, buffer.data())) {
            if (++fail_count > 100) {
                std::wcout << L"Too many read failures at " << i << L"\n";
                break;
            }
            continue;
        }
        fail_count = 0;

        auto* record = reinterpret_cast<NTFS_FILE_RECORD_HEADER*>(buffer.data());

        // 检查签名
        if (memcmp(record->signature, "FILE", 4) != 0) {
            // 关键修改：这里不要直接停止，继续尝试
            empty_count++;
            if (empty_count > 10000) { // 连续10000个空记录才停止
                std::wcout << L"Too many empty records at " << i 
                          << L", stopping enumeration\n";
                break;
            }
            continue;
        }
        empty_count = 0;

        // 应用fixup
        if (!apply_fixup(buffer.data(), 
                        static_cast<uint32_t>(m_bytes_per_record))) {
            // fixup失败可能是因为记录损坏，继续下一个
            continue;
        }

        // 检查是否在使用中
        if (!(record->flags & 0x01)) {
            continue;
        }

        // 解析文件名
        auto names = AttributeParser::parse_file_name(record);
        if (names.empty()) {
            continue;
        }

        bool is_dir = AttributeParser::is_directory(record);
        uint64_t frn = i & 0x0000FFFFFFFFFFFFULL;

        for (const auto& n : names) {
            Entry e{};
            e.file_id = frn;
            e.parent_file_id = n.parent_frn;
            e.name = n.name;
            e.is_directory = is_dir;

            if (!callback(e)) {
                return false;
            }
        }
    }

    std::wcout << L"Enumeration completed.\n";
    return true;
}