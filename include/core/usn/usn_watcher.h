#pragma once

#include <windows.h>
#include <atomic>
#include <string>
#include <functional>

#include "usn_event.h"
#include "usn_state.h"

/*
 * UsnWatcher
 * -----------------------------------------
 * 负责：
 * - 打开 NTFS 卷
 * - 查询 USN Journal
 * - 持续读取 USN_RECORD
 * - 交给 UsnParser
 */
class UsnWatcher {
public:
    using EventCallback = std::function<void(const UsnEvent&)>;

public:
    explicit UsnWatcher(const std::wstring& volume_name);
    ~UsnWatcher();

    UsnWatcher(const UsnWatcher&) = delete;
    UsnWatcher& operator=(const UsnWatcher&) = delete;

    // 启动监听（通常在独立线程）
    void run(EventCallback callback);

    // 请求停止
    void stop();

    // 状态管理（持久化用）
    UsnState get_state() const;
    void set_state(const UsnState& state);

private:
    void open_volume();
    void query_journal();

private:
    std::wstring m_volume_name;
    HANDLE m_volume = INVALID_HANDLE_VALUE;

    std::atomic<bool> m_running{false};
    UsnState m_state;
};