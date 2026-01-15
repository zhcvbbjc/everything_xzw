#pragma once

#include <windows.h>
#include <cstdint>
#include <string>
#include <functional>

#include "ntfs_structs.h"

/*
 * MFTReader
 * ------------------------------------------------
 * 职责：
 * 1. 打开 NTFS 卷（\\.\C:）
 * 2. 读取 $MFT 文件
 * 3. 枚举 FILE_RECORD
 * 4. 提取 FILE_NAME / DIRECTORY 信息
 *
 * ⚠️ 只读、无写操作
 */
class MFTReader {
public:
    struct Entry {
        uint64_t file_id;    // FRN
        uint64_t parent_file_id;  // 父目录 FRN
        std::wstring name;
        bool is_directory;
    };

public:
    explicit MFTReader(const std::wstring& volume);
    ~MFTReader();

    // 禁止拷贝
    MFTReader(const MFTReader&) = delete;
    MFTReader& operator=(const MFTReader&) = delete;

    // 枚举整个 MFT
    // callback 每条记录都会被调用
    bool enumerate(const std::function<bool(const Entry&)>& callback);

private:
    // 打开卷
    void open_volume();

    // 一次性读取 NTFS 参数
    void load_ntfs_metadata(); 

    // 读取原始 MFT 数据块
    bool read_mft_record(uint64_t record_index, void* buffer);

    // 解析单条 FILE_RECORD
    // bool parse_file_record(const NTFS_FILE_RECORD_HEADER* record, Entry* out);

    // Fixup 修复（NTFS 必需）
    bool apply_fixup(void* record_buffer, uint32_t record_size);

private:
    HANDLE m_volume = INVALID_HANDLE_VALUE;
    std::wstring m_volume_name;

    // 新增：NTFS 上下文状态
    uint32_t m_bytes_per_record = 0;
    uint32_t m_bytes_per_cluster = 0;
    uint64_t m_mft_start_offset = 0;
    uint64_t m_max_records = 0;
};