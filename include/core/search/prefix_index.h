#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

#include "core/indexer/indexer_types.h"

/*
 * PrefixIndex
 * -----------------------------------------
 * 文件名的前缀索引
 *
 * example:
 *   "rea" -> { readme.txt, reader.cpp }
 */
class PrefixIndex {
public:
    void clear();

    void add(const FileRecord& record);
    void remove(uint64_t file_id);
    void update(const FileRecord& record);

    // 前缀匹配
    std::vector<const FileRecord*>
    query(const std::wstring& prefix) const;

private:
    // prefix -> list<file_id>
    std::unordered_map<std::wstring, std::vector<uint64_t>> m_index;
};