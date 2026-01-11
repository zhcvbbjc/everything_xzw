#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "core/indexer/indexer_types.h"
#include "core/indexer/index_snapshot.h"

/*
 * IndexStore
 * -----------------------------------------
 * 索引持久化管理器
 *
 * 职责：
 * - 保存完整索引快照
 * - 从磁盘恢复索引
 * - 处理版本兼容
 */
class IndexStore {
public:
    explicit IndexStore(const std::wstring& store_dir);
    ~IndexStore();

    IndexStore(const IndexStore&) = delete;
    IndexStore& operator=(const IndexStore&) = delete;

    // 保存完整索引（退出/定期）
    bool save_snapshot(const IndexSnapshot& snapshot);

    // 加载索引（启动）
    bool load_snapshot(const IndexSnapshot& out_snapshot);

    // 清空索引存储（版本不兼容）
    void clear();

private:
    std::wstring m_store_dir;

    std::wstring snapshot_path() const;
    std::wstring metadata_path() const;
};