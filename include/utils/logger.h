#pragma once

#include <string>
#include <mutex>
#include <iostream>

enum class LogLevel {
    Info,
    Warning,
    Error,
    Debug
};

class Logger {
public:
    static Logger& instance();

    void log(LogLevel level, const std::wstring& message);

    // printf 风格（可选）
    template<typename... Args>
    void info(const wchar_t* fmt, Args&&... args) {
        logf(LogLevel::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const wchar_t* fmt, Args&&... args) {
        logf(LogLevel::Warning, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const wchar_t* fmt, Args&&... args) {
        logf(LogLevel::Error, fmt, std::forward<Args>(args)...);
    }

private:
    Logger() = default;

    template<typename... Args>
    void logf(LogLevel level, const wchar_t* fmt, Args&&... args);

private:
    std::mutex m_mutex;
};

// 方便使用的宏（Everything 风格）
#define LOG_INFO(msg)    Logger::instance().log(LogLevel::Info, msg)
#define LOG_WARN(msg)    Logger::instance().log(LogLevel::Warning, msg)
#define LOG_ERROR(msg)   Logger::instance().log(LogLevel::Error, msg)
