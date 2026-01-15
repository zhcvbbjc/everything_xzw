#include "core/search/result_ranker.h"
#include <algorithm>

std::vector<const FileRecord*>
ResultRanker::rank(std::vector<const FileRecord*>&& candidate, const std::wstring& query) const {
    std::sort(candidate.begin(), candidate.end(), [&](const FileRecord* a, const FileRecord* b) {
        return score(*a, query) > score(*b, query);
    });
    return std::move(candidate);
}

int ResultRanker::score(const FileRecord& record, const std::wstring& query) {
    int s = 0;

    // 1️⃣ 完全匹配文件名
    if (record.name == query) s += 100;

    // 2️⃣ 前缀匹配
    if (record.name.find(query) == 0) s += 50;

    // 3️⃣ 路径深度（目录越浅，分数高）
    size_t depth = std::count(record.full_path.begin(), record.full_path.end(), L'\\');
    s += static_cast<int>(100 - depth);

    // 4️⃣ 文件类型：目录优先
    if (record.is_directory) s += 20;

    return s;
}
