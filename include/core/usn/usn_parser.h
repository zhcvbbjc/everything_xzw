#pragma once

#include <windows.h>

#include "usn_event.h"

/*
 * UsnParser
 * -----------------------------------------
 * 将 NTFS USN_RECORD 翻译为 UsnEvent
 *
 * ✔ 处理 reason 位掩码
 * ✔ 合并 Rename Old/New
 * ✔ 隐藏 NTFS 细节
 */
class UsnParser {
public:
    UsnParser() = default;

    UsnParser(const UsnParser&) = delete;
    UsnParser& operator=(const UsnParser&) = delete;

    // 是否是我们关心的 USN_RECORD
    static bool is_relevant(const USN_RECORD* record);

    // 解析单条 USN_RECORD
    static UsnEvent parse(const USN_RECORD* record);

private:
    // 从 USN_REASON_* 翻译为 UsnAction
    static UsnAction decode_action(DWORD reason);

    // 判断是否为目录
    static bool decode_is_directory(const USN_RECORD* record);
};