#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

#include "core/indexer/indexer_types.h"

/*
 * TokenIndex
 * -----------------------------------------
 * 单词级倒排索引
 *
 * example:
 *   "read" -> { file1, file7 }
 *   "log"  -> { file3 }
 */
class TokenIndex {
public:
    void clear();

    void add(const FileRecord& record);
    void remove(uint64_t file_id);

    // 查询多个 token 的交集
    std::vector<const FileRecord*>
    query(const std::vector<std::wstring>& tokens) const;

private:
    // token -> set<file_id>
    std::unordered_map<std::wstring, std::unordered_set<uint64_t>> m_index;
};