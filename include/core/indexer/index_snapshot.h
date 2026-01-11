#pragma once

#include <string>
#include <vector>

#include "indexer_types.h"

/*
 * IndexSnapshot
 * ------------------------------------
 * 用于：
 *  - UI 渲染
 *  - 序列化到磁盘
 *  - IPC / 插件系统
 *
 * 注意：
 * 这是只读结构
 */
class IndexSnapshot {
public:
    IndexSnapshot() = default;

    explicit IndexSnapshot(std::vector<FileRecord>&& records);

    const std::vector<FileRecord>& records() const;

private:
    std::vector<FileRecord> m_records;
};