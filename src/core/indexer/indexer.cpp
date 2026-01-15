#include "core/indexer/indexer.h"

#include "core/mft/mft_reader.h"
#include "core/usn/usn_watcher.h"
#include "utils/logger.h"

#include <shared_mutex>

namespace {

// 枚举所有 NTFS 卷（Everything 同款）
std::vector<std::wstring> enumerate_ntfs_volumes() {
    std::vector<std::wstring> volumes;

    DWORD mask = GetLogicalDrives();
    for (wchar_t c = L'A'; c <= L'Z'; ++c) {
        if (!(mask & (1 << (c - L'A')))) {
            continue;
        }

        std::wstring root{ c, L':', L'\\' };
        wchar_t fs_name[16] = {};

        if (!GetVolumeInformationW(
                root.c_str(),
                nullptr, 0,
                nullptr, nullptr, nullptr,
                fs_name, _countof(fs_name))) {
            continue;
        }

        if (wcscmp(fs_name, L"NTFS") == 0) {
            volumes.emplace_back(L"\\\\.\\" + std::wstring(1, c) + L":");
        }
    }
    return volumes;
}

} // namespace

Indexer::Indexer()
    : m_active_volumes(enumerate_ntfs_volumes()),
      m_path_builder(),
      m_update(*this,m_active_volumes),
      m_running(false) {}

Indexer::Indexer(std::vector<std::wstring> volumes)
    : m_active_volumes(std::move(volumes)),
      m_path_builder(),
      m_update(*this,m_active_volumes),
      m_running(false) {}

Indexer::~Indexer() {
    stop_usn_monitor();
}

void Indexer::build_initial_index() {
    std::unique_lock lock(m_mutex);

    m_records.clear();
    m_path_builder.clear();

    for (const auto& volume : m_active_volumes) {
        LOG_INFO(L"Indexer: scanning MFT on " + volume);

        MFTReader reader(volume);

        reader.enumerate([this](const MFTReader::Entry& e) {
            FileRecord record;
            record.file_id        = e.file_id;
            record.parent_file_id = e.parent_file_id;
            record.name           = e.name;
            record.is_directory   = e.is_directory;

            m_records.emplace(record.file_id, std::move(record));
            return true; // continue
        });
    }

    // 构建路径缓存（可懒，但这里一次性构建更利于搜索）
    for (auto& [_, record] : m_records) {
        rebuild_path(record);
    }

    LOG_INFO(L"Indexer: initial index built, count = " +
             std::to_wstring(m_records.size()));
}

// ===== USN 监听 =====

void Indexer::start_usn_monitor() {
    if (m_running.exchange(true)) {
        return;
    }

    LOG_INFO(L"Indexer: start USN monitor");
    m_update.start();
}

void Indexer::stop_usn_monitor() {
    if (!m_running.exchange(false)) {
        return;
    }

    LOG_INFO(L"Indexer: stop USN monitor");
    m_update.stop();
}

// ===== 查询 =====

std::vector<const FileRecord*> Indexer::search(const std::wstring& keyword) const {
    std::shared_lock lock(m_mutex);

    std::vector<const FileRecord*> result;
    result.reserve(128);

    for (const auto& [_, record] : m_records) {
        if (record.full_path.find(keyword) != std::wstring::npos) {
            result.push_back(&record);
        }
    }
    return result;
}

size_t Indexer::file_count() const {
    std::shared_lock lock(m_mutex);
    return m_records.size();
}

// ===== USN 回调 =====

void Indexer::on_file_created(const FileRecord& record) {
    std::unique_lock lock(m_mutex);

    m_records[record.file_id] = record;
    rebuild_path(m_records[record.file_id]);
}

void Indexer::on_file_deleted(uint64_t file_id) {
    std::unique_lock lock(m_mutex);

    m_records.erase(file_id);
    m_path_builder.invalidate(file_id);
}

void Indexer::on_file_renamed(uint64_t file_id,
                              const std::wstring& new_name,
                              uint64_t new_parent_id) {
    std::unique_lock lock(m_mutex);

    auto it = m_records.find(file_id);
    if (it == m_records.end()) {
        return;
    }

    it->second.name = new_name;
    it->second.parent_file_id = new_parent_id;

    m_path_builder.invalidate(file_id);
    rebuild_path(it->second);
}

IndexSnapshot Indexer::snapshot() const {
    std::shared_lock lock(m_mutex);

    std::vector<FileRecord> records;
    records.reserve(m_records.size());

    for (const auto& [_, record] : m_records) {
        records.push_back(record);
    }
    return IndexSnapshot(std::move(records));
}

void Indexer::rebuild_path(FileRecord& record) {
    record.full_path = m_path_builder.build_path(record.file_id, m_records);
}
