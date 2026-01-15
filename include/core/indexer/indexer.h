#pragma once

#include "core/indexer/indexer_types.h"
#include "core/indexer/path_builder.h"
#include "core/indexer/index_update.h"
#include "core/indexer/index_snapshot.h"

#include <unordered_map>
#include <shared_mutex>
#include <atomic>
#include <vector>
#include <string>

/*
 * Indexer
 * ------------------------------------
 * 1. 持有全量文件索引（FRN -> FileRecord）
 * 2. 负责 MFT 初始扫描（可多盘）
 * 3. 接收 USN 增量更新
 * 4. 提供搜索 / 快照接口
 *
 * 设计要点：
 *  - 默认索引所有 NTFS 盘
 *  - 可指定只索引某些盘
 *  - Indexer 不关心 NTFS / USN 细节
 */
class Indexer {
public:
    // 默认：索引所有 NTFS 卷
    Indexer();

    // 指定卷（如 { L"\\\\.\\C:" }）
    explicit Indexer(std::vector<std::wstring> volumes);

    ~Indexer();

    Indexer(const Indexer&) = delete;
    Indexer& operator=(const Indexer&) = delete;
    Indexer(Indexer&&) = delete;
    Indexer& operator=(Indexer&&) = delete;

    // ===== 生命周期 =====
    void build_initial_index();
    void start_usn_monitor();
    void stop_usn_monitor();

    // ===== 查询接口 =====
    std::vector<const FileRecord*> search(const std::wstring& keyword) const;
    size_t file_count() const;

    // ===== USN 回调 =====
    void on_file_created(const FileRecord& record);
    void on_file_deleted(uint64_t file_id);
    void on_file_renamed(uint64_t file_id,
                         const std::wstring& new_name,
                         uint64_t new_parent_id);

    // ===== 快照 =====
    IndexSnapshot snapshot() const;

private:
    void rebuild_path(FileRecord& record);

private:
    // 活跃卷列表（\\.\C:）
    std::vector<std::wstring> m_active_volumes;

    // 主索引
    std::unordered_map<uint64_t, FileRecord> m_records;

    // 路径构建器
    PathBuilder m_path_builder;

    // USN 增量更新器
    IndexUpdate m_update;

    // 并发控制
    mutable std::shared_mutex m_mutex;

    std::atomic<bool> m_running;
};
