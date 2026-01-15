#include "core/search/token_index.h"

void TokenIndex::clear() {
    m_index.clear();
}

void TokenIndex::add(const FileRecord& record) {
    std::wstring token;
    size_t start = 0;
    for (size_t i = 0; i <= record.name.size(); ++i) {
        if (i == record.name.size() || record.name[i] == L'_' || record.name[i] == L'.') {
            if (i > start) {
                token = record.name.substr(start, i - start);
                m_index[token].insert(record.file_id);
            }
            start = i + 1;
        }
    }
}

void TokenIndex::remove(uint64_t file_id) {
    for (auto& [token, set_ids] : m_index) {
        set_ids.erase(file_id);
    }
}

std::vector<const FileRecord*>
TokenIndex::query(const std::vector<std::wstring>& tokens) const {
    if (tokens.empty()) return {};

    std::vector<const FileRecord*> results;
    std::unordered_set<uint64_t> intersection;

    bool first = true;
    for (auto& token : tokens) {
        auto it = m_index.find(token);
        if (it == m_index.end()) return {}; // 任意一个 token 不存在，结果为空

        if (first) {
            intersection = it->second;
            first = false;
        } else {
            std::unordered_set<uint64_t> tmp;
            for (auto id : intersection) {
                if (it->second.count(id)) tmp.insert(id);
            }
            intersection = std::move(tmp);
        }
    }

    // 转成 FileRecord*（注意：这里只返回 file_id，需要外部映射 FileRecord）
    for (auto id : intersection) {
        results.push_back(reinterpret_cast<const FileRecord*>(id)); // 由上层 SearchEngine 维护映射
    }

    return results;
}
