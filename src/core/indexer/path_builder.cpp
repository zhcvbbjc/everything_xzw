#include "core/indexer/path_builder.h"

std::wstring PathBuilder::build_path(
    uint64_t file_id,
    const std::unordered_map<uint64_t, FileRecord>& records) {

    auto it = m_cache.find(file_id);
    if (it != m_cache.end()) {
        return it->second;
    }

    std::wstring path = build_recursive(file_id, records);
    m_cache[file_id] = path;
    return path;
}

void PathBuilder::invalidate(uint64_t file_id) {
    m_cache.erase(file_id);
}

void PathBuilder::clear() {
    m_cache.clear();
}

std::wstring PathBuilder::build_recursive(
    uint64_t file_id,
    const std::unordered_map<uint64_t, FileRecord>& records) {

    auto it = records.find(file_id);
    if (it == records.end()) {
        return L"";
    }

    const FileRecord& rec = it->second;

    if (rec.parent_file_id == 0) {
        return rec.name;
    }

    auto pit = records.find(rec.parent_file_id);
    if (pit == records.end()) {
        return rec.name;
    }

    std::wstring parent_path = build_path(rec.parent_file_id, records);

    if (parent_path.empty()) {
        return rec.name;
    }

    return parent_path + L"\\" + rec.name;
}
