#include "core/search/search_engine.h"
#include <algorithm>

SearchEngine::SearchEngine() {}

void SearchEngine::build(const std::vector<const FileRecord*>& records) {
    m_token_index.clear();
    m_prefix_index.clear();

    for (auto* rec : records) {
        m_token_index.add(*rec);
        m_prefix_index.add(*rec);
    }
}

void SearchEngine::add(const FileRecord& record) {
    m_token_index.add(record);
    m_prefix_index.add(record);
}

void SearchEngine::remove(uint64_t file_id) {
    m_token_index.remove(file_id);
    m_prefix_index.remove(file_id);
}

void SearchEngine::update(const FileRecord& record) {
    m_token_index.remove(record.file_id);
    m_token_index.add(record);

    m_prefix_index.update(record);
}

std::vector<const FileRecord*> 
SearchEngine::search(const std::wstring& query, size_t limit) const {
    if (query.empty()) return {};

    // 1️⃣ 前缀搜索
    auto prefix_results = m_prefix_index.query(query);

    // 2️⃣ Token 搜索
    std::vector<std::wstring> tokens;
    size_t start = 0;
    for (size_t i = 0; i <= query.size(); ++i) {
        if (i == query.size() || query[i] == L' ') {
            if (i > start) {
                tokens.emplace_back(query.substr(start, i - start));
            }
            start = i + 1;
        }
    }
    auto token_results = m_token_index.query(tokens);

    // 3️⃣ 合并结果（简单去重）
    std::unordered_set<const FileRecord*> combined;
    for (auto* r : prefix_results) combined.insert(r);
    for (auto* r : token_results) combined.insert(r);

    std::vector<const FileRecord*> result(combined.begin(), combined.end());

    // 4️⃣ 排序
    result = m_ranker.rank(std::move(result), query);

    if (result.size() > limit) result.resize(limit);

    return result;
}
