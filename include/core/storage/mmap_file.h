#pragma once

#include <windows.h>
#include <string>
#include <cstdint>

/*
 * MMapFile
 * -----------------------------------------
 * 内存映射文件封装
 *
 * 用于：
 * - 索引快照
 * - 路径表
 * - 字符串池
 */
class MMapFile {
public:
    MMapFile();
    ~MMapFile();

    MMapFile(const MMapFile&) = delete;
    MMapFile& operator=(const MMapFile&) = delete;

    // 以只读方式映射
    bool open_read(const std::wstring& path);

    // 创建/覆盖并映射
    bool open_write(const std::wstring& path, size_t size);

    // 获取映射内存
    void* data() const { return m_data;}
    size_t size() const { return m_size;}

    // 关闭映射
    void close();

private:
    HANDLE m_file = INVALID_HANDLE_VALUE;
    HANDLE m_mapping = nullptr;
    void* m_data = nullptr;
    size_t m_size = 0;
};