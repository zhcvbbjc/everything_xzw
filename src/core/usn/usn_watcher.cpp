#pragma once

#include <cstdint>
#include <string>
#include <functional>

#include "core/indexer.h"

// ====================
// USN 事件类型
// ====================
enum class UsnEventType {
    Create,
    Delete,
    Rename
};

// ====================
// USN 事件
// ====================
struct UsnEvent {
    UsnEventType type;
    uint64_t file_id;
    std::wstring name;
    uint64_t parent_file_id;
    bool is_directory;
};

// =====================
// USN journal 监听器
// =====================
class UsnWatcher {
public:
    UsnWatcher() = default;
    ~UsnWatcher() = default;

    UsnWatcher(const UsnWatcher&) = delete;
    UsnWatcher& operator=(const UsnWatcher&) = delete;

    // 阻塞运行（通常在后台线程）
    void run(const std::function<void(const UsnEvent&)>& callback);
};