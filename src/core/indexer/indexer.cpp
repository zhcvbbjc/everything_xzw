// core/indexer.h
#pragma once

#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>

#include "core/file_record.h"

// =====================
// 全局文件索引器
// =====================
class Indexer {
public:
    Indexer();
    ~Indexer();

    // 禁止拷贝/移动（全局唯一），否则会占用大量资源
    Indexer(const Indexer&) = delete;
    Indexer& operator=(const Indexer&) = delete;
    Indexer(Indexer&&) = delete;
    Indexer& operator=(Indexer&&) = delete;

    // ====== 全生命周期 ======= //
    void build_initial_index();    // 从 MFT 建立初始索引
    void start_usn_monitor();      // 启动 USD 监听
    void stop_usn_monitor();       // 停止 USD 监听

    // ====== 查询接口（给 GUI 用）====== //
    std::vector<const FileRecord*> search(const std::wstring& keyword) const;

    size_t file_count() const;

    // ====== USD 回调接口 ====== //
    void on_file_created(const FileRecord& record);
    void on_file_deleted(uint64_t file_id);
    void on_file_renamed(uint64_t file_id, const std::wstring& new_name, uint64_t new_parent_id);

private:
    // 内部结构
    void rebuild_full_path(FileRecord& record);
    std::wstring build_path_recursive(uint64_t file_id) const;

private:
    // 主索引：File ID -> FileRecord
    std::unordered_map<uint64_t, FileRecord> m_records;

    // 读多写少：shared_mutex
    mutable std::shared_mutex m_mutex;

    // USD 线程状态
    std::atomic<bool> m_running;
};