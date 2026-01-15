#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ntfs_structs.h"

/*
 * AttributeParser
 * ------------------------------------------------
 * 职责：
 * 1. 遍历 FILE_RECORD 中的 NTFS 属性链
 * 2. 提取 FILE_NAME（核心）
 * 3. 判断是否目录
 * 4. 支持多个 FILE_NAME（DOS / Win32）
 */
class AttributeParser {
public:
    struct FileNameInfo {
        uint64_t parent_frn;
        std::wstring name;
        uint32_t flags;
    };

public:
    AttributeParser() = default;
    
    // 解析FILE_RECORD 中的所有 FILE_NAME 属性
    static std::vector<FileNameInfo> parse_file_name(const NTFS_FILE_RECORD_HEADER* record);

    // 判断是否目录
    static bool is_directory(const NTFS_FILE_RECORD_HEADER* record);

private:
    static const NTFS_ATTRIBUTE_HEADER* next_attribute(const NTFS_ATTRIBUTE_HEADER* attr);

    static const NTFS_FILE_NAME_ATTRIBUTE* parse_file_name_attr(const NTFS_ATTRIBUTE_HEADER* attr);
};