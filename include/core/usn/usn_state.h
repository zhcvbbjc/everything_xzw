#pragma once

#include <cstdint>

/*
 * UsnState
 * -----------------------------------------
 * USN Journal 同步状态
 *
 * 用于：
 * - 程序重启恢复
 * - Journal 回绕检测
 */
struct UsnState {
    uint64_t journal_id = 0;  // USN Journal ID
    uint64_t next_usn = 0;    // 下次读取起点
    uint64_t last_sync_time = 0; // 可选（调试/统计）

    bool valid() const {
        return journal_id != 0;
    };

    void reset() {
        journal_id = 0;
        next_usn = 0;
        last_sync_time = 0;
    };
};