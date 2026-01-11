#pragma once

#include <vector>
#include <string>

#include "core/indexer/indexer_types.h"

/*
 * ResultRanker
 * -----------------------------------------
 * 搜索结果评分与排序
 *
 * 排序规则示例：
 * 1. 文件名完全匹配
 * 2. 前缀匹配
 * 3. 路径深度
 * 4. 文件类型
 */
class ResultRanker {
public:
    ResultRanker() = default;

    std::vector<const FileRecord*>
    rank(std::vector<const FileRecord*>&& condidate, const std::wstring& query) const;

private:
    static int score(const FileRecord& record, const std::wstring& query);
};