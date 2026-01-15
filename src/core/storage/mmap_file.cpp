#include "core/storage/mmap_file.h"
#include "utils/logger.h"

#include <windows.h>

MMapFile::MMapFile() = default;

MMapFile::~MMapFile() {
    close();
}

bool MMapFile::open_read(const std::wstring& path) {
    close();

    m_file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m_file == INVALID_HANDLE_VALUE) {
        Logger::instance().log(LogLevel::Error, L"Failed to open file for read");
        return false;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(m_file, &size)) {
        Logger::instance().log(LogLevel::Error, L"Failed to get file size");
        CloseHandle(m_file);
        m_file = INVALID_HANDLE_VALUE;
        return false;
    }
    m_size = static_cast<size_t>(size.QuadPart);

    m_mapping = CreateFileMappingW(m_file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!m_mapping) {
        Logger::instance().log(LogLevel::Error, L"Failed to create file mapping");
        CloseHandle(m_file);
        m_file = INVALID_HANDLE_VALUE;
        return false;
    }

    m_data = MapViewOfFile(m_mapping, FILE_MAP_READ, 0, 0, 0);
    if (!m_data) {
        Logger::instance().log(LogLevel::Error, L"Failed to map view of file");
        CloseHandle(m_mapping);
        CloseHandle(m_file);
        m_mapping = nullptr;
        m_file = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
}

bool MMapFile::open_write(const std::wstring& path, size_t size) {
    close();

    m_file = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m_file == INVALID_HANDLE_VALUE) {
        Logger::instance().log(LogLevel::Error, L"Failed to open file for write");
        return false;
    }

    m_mapping = CreateFileMappingW(m_file, nullptr, PAGE_READWRITE, (DWORD)(size >> 32), (DWORD)(size & 0xFFFFFFFF), nullptr);
    if (!m_mapping) {
        Logger::instance().log(LogLevel::Error, L"Failed to create file mapping for write");
        CloseHandle(m_file);
        m_file = INVALID_HANDLE_VALUE;
        return false;
    }

    m_data = MapViewOfFile(m_mapping, FILE_MAP_WRITE, 0, 0, size);
    if (!m_data) {
        Logger::instance().log(LogLevel::Error, L"Failed to map view for write");
        CloseHandle(m_mapping);
        CloseHandle(m_file);
        m_mapping = nullptr;
        m_file = INVALID_HANDLE_VALUE;
        return false;
    }

    m_size = size;
    return true;
}

void MMapFile::close() {
    if (m_data) {
        UnmapViewOfFile(m_data);
        m_data = nullptr;
    }
    if (m_mapping) {
        CloseHandle(m_mapping);
        m_mapping = nullptr;
    }
    if (m_file != INVALID_HANDLE_VALUE) {
        CloseHandle(m_file);
        m_file = INVALID_HANDLE_VALUE;
    }
    m_size = 0;
}
