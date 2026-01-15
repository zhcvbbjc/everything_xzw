#include "core/search/prefix_index.h"

void PrefixIndex::clear() {
    m_index.clear();
}

void PrefixIndex::add(const FileRecord& record) {
    for (size_t len = 1; len <= record.name.size(); ++len) {
        auto prefix = record.name.substr(0, len);
        m_index[prefix].push_back(record.file_id);
    }
}

void PrefixIndex::remove(uint64_t file_id) {
    for (auto it = m_index.begin(); it != m_index.end(); ++it) {
        auto& vec = it->second;
        vec.erase(std::remove(vec.begin(), vec.end(), file_id), vec.end());
    }
}

void PrefixIndex::update(const FileRecord& record) {
    remove(record.file_id);
    add(record);
}

std::vector<const FileRecord*>
PrefixIndex::query(const std::wstring& prefix) const {
    auto it = m_index.find(prefix);
    if (it == m_index.end()) return {};

    std::vector<const FileRecord*> results;
    for (auto id : it->second) {
        results.push_back(reinterpret_cast<const FileRecord*>(id)); // 上层映射
    }

    return results;
}
