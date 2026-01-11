#pragma once

#include <unordered_map>
#include <string>
#include "indexer_types.h"

/*
 * PathBuilder
 * ------------------------------------
 * 通过 parent_file_id 递归构建完整路径
 * 使用缓存减少重复计算
 */
class PathBuilder {
public:
    PathBuilder() = default;
    // ~PathBuilder() = default;

    // 构建 / 更新单个文件的路径
    std::wstring build_path(uint64_t file_id, const std::unordered_map<uint64_t, FileRecord>& records);

    // 路径失效（如父目录变更）
    void invalidate(uint64_t file_id);

    // 清空缓存
    void clear();

private:
    std::wstring build_recursive(uint64_t file_id, const std::unordered_map<uint64_t, FileRecord>& records);

private:
    // file_id -> full_path
    std::unordered_map<uint64_t, std::wstring> m_cache;
};