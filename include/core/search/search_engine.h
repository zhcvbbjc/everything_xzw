#pragma once

#include <vector>
#include <string>
#include <memory>

#include "core/indexer/indexer_types.h"
#include "token_index.h"
#include "prefix_index.h"
#include "result_ranker.h"

/*
 * SearchEngine
 * -----------------------------------------
 * Everything 搜索子系统的入口
 *
 * - 不持有文件系统
 * - 不关心 NTFS
 * - 只对 FileRecord 做索引
 */
class SearchEngine {
public:
    SearchEngine();

    SearchEngine(const SearchEngine&) = delete;
    SearchEngine& operator=(const SearchEngine&) = delete;

    // 全量重建（程序启动/索引重建）
    void build(const std::vector<const FileRecord*>& records);

    // 增量更新
    void add(const FileRecord& record);
    void remove(uint64_t file_id);
    void update(const FileRecord& record);

    // 搜索入口（UI 每次输入）
    std::vector<const FileRecord*> 
    search(const std::wstring& query, size_t limit = 100) const;

private:
    PrefixIndex m_prefix_index;
    TokenIndex m_token_index;
    ResultRanker m_ranker;
};