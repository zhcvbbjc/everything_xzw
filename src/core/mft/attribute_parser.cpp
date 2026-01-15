#include "core/mft/attribute_parser.h"
#include <windows.h>
#include <vector>
#include <iostream>
#include <cstring>

static constexpr uint32_t MAX_ATTRIBUTES = 50;  // 防止死循环
static constexpr uint32_t MAX_NAME_LENGTH = 255;

static inline bool is_valid_attribute_type(uint32_t type) {
    return type != 0xFFFFFFFF; // ATTRIBUTE_END
}

const NTFS_ATTRIBUTE_HEADER*
AttributeParser::next_attribute(const NTFS_ATTRIBUTE_HEADER* attr) {
    if (!attr) return nullptr;
    if (attr->length == 0) return nullptr;
    if (attr->length < 24) return nullptr; // 最小长度
    
    const uint8_t* next_ptr = reinterpret_cast<const uint8_t*>(attr) + attr->length;
    
    // 8字节对齐
    uintptr_t aligned_next = (reinterpret_cast<uintptr_t>(next_ptr) + 7) & ~7;
    
    return reinterpret_cast<const NTFS_ATTRIBUTE_HEADER*>(aligned_next);
}

const NTFS_FILE_NAME_ATTRIBUTE*
AttributeParser::parse_file_name_attr(const NTFS_ATTRIBUTE_HEADER* attr) {
    if (!attr) return nullptr;
    if (attr->type != 0x30) return nullptr;
    if (attr->non_resident != 0) return nullptr;
    if (attr->length < 24 + sizeof(NTFS_FILE_NAME_ATTRIBUTE)) return nullptr;
    
    const uint8_t* content = reinterpret_cast<const uint8_t*>(attr) + 24;
    return reinterpret_cast<const NTFS_FILE_NAME_ATTRIBUTE*>(content);
}

std::vector<AttributeParser::FileNameInfo>
AttributeParser::parse_file_name(const NTFS_FILE_RECORD_HEADER* record) {
    std::vector<FileNameInfo> result;

    if (!record) return result;
    if (memcmp(record->signature, "FILE", 4) != 0) return result;

    const uint8_t* record_base = reinterpret_cast<const uint8_t*>(record);
    
    // 关键修复：检查偏移量有效性
    if (record->first_attribute_offset < sizeof(NTFS_FILE_RECORD_HEADER) ||
        record->first_attribute_offset >= 1024) {
        return result;
    }

    const NTFS_ATTRIBUTE_HEADER* attr = 
        reinterpret_cast<const NTFS_ATTRIBUTE_HEADER*>(
            record_base + record->first_attribute_offset
        );

    // 添加循环计数器
    uint32_t attr_count = 0;
    
    while (attr && attr_count < MAX_ATTRIBUTES) {
        attr_count++;
        
        // 检查指针是否有效
        if (reinterpret_cast<const uint8_t*>(attr) < record_base ||
            reinterpret_cast<const uint8_t*>(attr) > record_base + 1000) {
            break;
        }
        
        if (attr->type == 0xFFFFFFFF) {
            break;
        }
        
        // 关键修复：检查属性长度有效性
        if (attr->length == 0 || attr->length < 24) {
            break;
        }
        
        // 检查是否超出记录边界
        const uint8_t* attr_end = reinterpret_cast<const uint8_t*>(attr) + attr->length;
        if (attr_end > record_base + 1024) {
            break;
        }
        
        if (attr->type == 0x30) {
            const NTFS_FILE_NAME_ATTRIBUTE* fn = parse_file_name_attr(attr);
            if (fn && fn->name_length > 0 && fn->name_length <= MAX_NAME_LENGTH) {
                // 检查名字是否在属性范围内
                const wchar_t* name_end = fn->name + fn->name_length;
                const uint8_t* name_bytes_end = reinterpret_cast<const uint8_t*>(name_end);
                if (name_bytes_end <= attr_end) {
                    FileNameInfo info{};
                    info.parent_frn = fn->parent_directory & 0x0000FFFFFFFFFFFFULL;
                    info.flags = fn->flags;
                    info.name.assign(fn->name, fn->name + fn->name_length);
                    result.push_back(std::move(info));
                }
            }
        }
        
        const NTFS_ATTRIBUTE_HEADER* next = next_attribute(attr);
        if (!next || next == attr) {  // 防止死循环
            break;
        }
        attr = next;
    }

    return result;
}

bool AttributeParser::is_directory(const NTFS_FILE_RECORD_HEADER* record) {
    if (!record) return false;
    return (record->flags & 0x02) != 0;
}