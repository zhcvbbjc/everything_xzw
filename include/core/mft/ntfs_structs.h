#pragma once

#include <cstdint>

#pragma pack(push,1)

/*
 * NTFS FILE_RECORD_HEADER
 * 对应 $MFT 中的每一条记录
 */
struct NTFS_FILE_RECORD_HEADER {
    char      signature[4];            // 固定为"FILE"
    uint16_t  fixup_offset;            // Fixup 数组在记录中的偏移
    uint16_t  fixup_entries;           // 需要修复的扇区数量
    uint64_t  lsn;                     // Log Sequence Number，表示这条记录最后一次修改的日志序号
    uint16_t  sequence_number;         // 用于检测 FRN 是否被重用
    uint16_t  hard_link_count;         // 硬链接数量，1 表示同一文件有多个路径
    uint16_t  first_attribute_offset;  // 指向 第一个属性结构 的偏移
    uint16_t  flags;                   // 0x01=InUse, 0x02=Directory
    uint32_t  used_size;               // 实际使用的字节数（属性结束位置）
    uint32_t  allocated_size;          // 为该记录分配的空间（通常 1024 字节）
    uint64_t  base_file_record;        // 用于 扩展记录
    uint16_t  next_attribute_id;       // NTFS 内部使用，分配新属性时用
};

/* 
 * NTFS ATTRIBUTE HEADER (通用) 
 */
struct NTFS_ATTRIBUTE_HEADER {
    // ATTRIBUTE_TYPE 
    // 常见值：0x30 → $FILE_NAME、0x80 → $DATA、0x90 → $INDEX_ROOT、0xA0 → $INDEX_ALLOCATION
    uint32_t type;

    // 当前属性的总长度
    uint32_t length;

    // 0：驻留（数据在 MFT 内）
    // 1：非驻留（数据在磁盘其他位置）
    // $FILE_NAME 永远是驻留的
    uint8_t non_resident;

    // 属性名字
    uint8_t name_length;
    uint16_t name_offset;

    // 属性标志（压缩/加密/稀疏）
    uint16_t flags;

    // 属性编号
    uint16_t attribute_id;
};

/* 
 * $FILE_NAME 属性（最重要） 
 */
struct NTFS_FILE_NAME_ATTRIBUTE {
    // 父目录 FRN
    uint64_t parent_directory;

    // 时间戳（FILETIME）
    uint64_t creation_time;
    uint64_t last_mod_time;
    uint64_t mft_mod_time;
    uint64_t last_access_time;

    // 磁盘占用
    uint64_t allocate_size;
    // 文件真实大小
    uint64_t real_size;

    // 文件属性：只读/隐藏/系统/目录
    uint32_t flags;

    // Reparse Point，用于符号链接、Junction、挂载点
    uint32_t reparse;

    // 文件名长度（字符数）
    uint8_t name_length;

    // 文件名类型：Win32、DOS(8.3)
    uint8_t name_type;

    // UTF-16 文件名
    wchar_t  name[1];          // 可变长
};

#pragma pack(pop)