#pragma once

#include <atomic>
#include <thread>

#include "core/usn/usn_event.h"

/*
 * IndexUpdate
 * ------------------------------------
 * 封装 USN Journal 监听逻辑
 * 将 NTFS 变更事件转换为高层回调
 */
class Indexer; // 前向声明

class IndexUpdate {
public:
    explicit IndexUpdate(Indexer& indexer);
    ~IndexUpdate();

    // 禁止拷贝
    IndexUpdate (const IndexUpdate&) = delete;
    IndexUpdate& operator=(const IndexUpdate&) = delete;

    // 启动 / 停止 USN监听
    void start();
    void stop();

private:
    void worker_thread();

private:
    Indexer& m_indexer;
    std::atomic<bool> m_running;
    std::thread m_thread;
};