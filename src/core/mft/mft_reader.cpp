#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include "core/file_record.h"

// ====================
// MFTReader：NTES 读取器
// ====================
class MFTReader {
public:
    MFTReader() = default;
    ~MFTReader() = default;

    // 禁止拷贝
    MFTReader(const MFTReader&) = delete;
    MFTReader& operator=(const MFTReader&) = delete;

    // 枚举整个 MFT
    // callback 每条记录都会被调用
    void enumerate(const std::function<void(const FileRecord&)>& callback);
};