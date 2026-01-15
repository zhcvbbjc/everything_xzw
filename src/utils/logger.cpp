#include "utils/logger.h"

#include <windows.h>
#include <cstdarg>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::log(LogLevel level, const std::wstring& message) {
    std::lock_guard lock(m_mutex);

    const wchar_t* prefix = L"[INFO] ";

    switch (level) {
    case LogLevel::Warning:
        prefix = L"[WARN] ";
        break;
    case LogLevel::Error:
        prefix = L"[ERROR]";
        break;
    case LogLevel::Debug:
        prefix = L"[DEBUG]";
        break;
    default:
        break;
    }

    std::wcout << prefix << L" " << message << std::endl;
}

template<typename... Args>
void Logger::logf(LogLevel level, const wchar_t* fmt, Args&&... args) {
    wchar_t buffer[1024]{};
    _snwprintf_s(buffer, _TRUNCATE, fmt, std::forward<Args>(args)...);
    log(level, buffer);
}

// 显式实例化（避免链接问题）
template void Logger::logf<>(LogLevel, const wchar_t*);
