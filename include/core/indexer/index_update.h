#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <vector>

class Indexer;

class IndexUpdate {
public:
    IndexUpdate(
        Indexer& indexer,
        std::vector<std::wstring> volumes
    );

    ~IndexUpdate();

    IndexUpdate(const IndexUpdate&) = delete;
    IndexUpdate& operator=(const IndexUpdate&) = delete;

    void start();
    void stop();

private:
    void watcher_thread(std::wstring volume);

private:
    Indexer& m_indexer;
    std::vector<std::wstring> m_volumes; // ← 明确：这是我自己的
    std::atomic<bool> m_running{false};
    std::vector<std::thread> m_threads;
};
