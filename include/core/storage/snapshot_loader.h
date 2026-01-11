#pragma once

#include "core/indexer/indexer.h"
#include "core/indexer/index_snapshot.h"
#include "mmap_file.h"

/*
 * SnapshotLoader
 * -----------------------------------------
 * 从磁盘快照恢复 Indexer
 *
 * 职责：
 * - mmap 索引文件
 * - 校验结构
 * - 重建运行时索引
 */
class SnapshotLoader {
public:
    SnapshotLoader() = default;

    SnapshotLoader(const SnapshotLoader&) = delete;
    SnapshotLoader& operator=(const SnapshotLoader&) = delete;

    // 从 snapshot 构建 Indexer
    bool load(Indexer& indexer, IndexSnapshot& snapshot);

private:
    bool validate(const IndexSnapshot& snapshot) const;
    bool rebuild_runtime_structures(Indexer& indexer, const IndexSnapshot& snapshot);
};