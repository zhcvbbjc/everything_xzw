# pragma once

#include <string>
#include <cstdint>

/*
 * FileRecord
 *
 * 内存中的“文件索引记录”
 * 所有搜索、USN 更新、路径构建都基于它
 */
struct FileRecord {
    // NTFS File Reference Number (FRN)
    uint64_t file_id = 0;

    // 父目录的 FRN（根目录为 0）
    uint64_t parent_file_id = 0;

    // 文件名（不含路径）
    std::wstring name;

    // 完整路径缓存（延迟或按需构建）
    std::wstring full_path;

    // 属性
    bool is_directory = false;

    // 文件大小（目录为0）
    uint64_t file_size = 0;

    // NTFS 时间戳（FILETIME 原始值）
    uint64_t create_time = 0;
    uint64_t last_write_time = 0;

    // 标志位（供未来拓展）
    uint64_t flags = 0;
};