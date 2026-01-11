#pragma once

#include <cstdint>
#include <string>

/*
 * UsnAction
 * 抽象后的文件系统事件类型
 */
enum class UsnAction {
    Create,
    Delete,
    Rename,
    Move,
    Modify,
    Unknown
};

/*
 * UsnEvent
 *
 * 从 USN_RECORD 转换后的“高层事件”
 * 作为 Indexer 的唯一输入
 */
struct UsnEvent {
    UsnAction action = UsnAction::Unknown;

    // 文件 FRN
    uint64_t file_id = 0;

    // 新父目录 FRN（重命名/移动）
    uint64_t parent_file_id = 0;

    // 文件名（USN 中是UTF-16）
    std::wstring name;

    // 是否目录
    bool is_directory = false;

    // 是否来自重建（非实时 USN）
    bool from_rebuild = false;
};