/// @file logger.h
/// @brief 框架日志系统（C++20 重写版）
/// @note 改进: 使用 thread_local buffer，天然线程安全
///       改进: 去掉成员变量 buffer，避免多线程竞争
#pragma once

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include "context_mgr.h"
#include "patterns/singleton.h"
#include "server_statistics.h"

namespace ua
{

class Logger : public Singleton<Logger>
{
public:
    enum PriorityType
    {
        PRIORITY_NULL = 0,
        PRIORITY_ERROR = 1,
        PRIORITY_WARN = 2,
        PRIORITY_INFO = 3,
        PRIORITY_DEBUG = 4,
        PRIORITY_TRACE = 5,
        PRIORITY_MAX,
    };

    /// 改进: 使用 thread_local buffer，线程安全
    uint32_t Format(const char* fmt, ...) __attribute__((format(printf, 2, 3)))
    {
        va_list args;
        va_start(args, fmt);
        int32_t write_len = vsnprintf(GetBuffer(), kBufferSize, fmt, args);
        va_end(args);
        if (write_len < 0)
            return 0;
        uint32_t len = static_cast<uint32_t>(write_len);
        if (len >= kBufferSize)
            len = kBufferSize - 1;
        GetBuffer()[len] = '\0';
        return len;
    }

    [[nodiscard]] const char* GetBuff() const noexcept { return GetBuffer(); }

    [[nodiscard]] bool CanOutput(PriorityType priority) const
    {
        return can_output_func_ && can_output_func_(priority);
    }

    void Output(PriorityType priority, const char* msg, uint32_t len)
    {
        if (output_func_)
            output_func_(priority, msg, len);
    }

    using CanOutputFunc = std::function<bool(PriorityType)>;
    void SetCanOutputFunc(CanOutputFunc func) { can_output_func_ = std::move(func); }

    using OutputFunc = std::function<void(PriorityType, const char*, uint32_t)>;
    void SetOutputFunc(OutputFunc func) { output_func_ = std::move(func); }

private:
    friend class Singleton<Logger>;
    Logger() = default;

    /// 改进: thread_local buffer，每个线程独立的 4K 缓冲区
    static constexpr uint32_t kBufferSize = 4 * 1024;
    static char* GetBuffer() noexcept
    {
        static thread_local char buffer[kBufferSize] = {};
        return buffer;
    }

    CanOutputFunc can_output_func_;
    OutputFunc output_func_;
};

}  // namespace ua

// ========== 日志宏 ==========
#define _FILE_NAME_ ((__builtin_strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)

#define _UA_LOG_(priority, level, format, ...)                                                      \
    {                                                                                               \
        if (ua::Logger::GetInst().CanOutput(priority))                                              \
        {                                                                                           \
            ua::ServerStatistics::GetInst().statistics().inc_log_##level##_num();                    \
            uint32_t _fmt_str_len_ = ua::Logger::GetInst().Format(format, ##__VA_ARGS__);           \
            ua::Logger::GetInst().Output(priority, ua::Logger::GetInst().GetBuff(), _fmt_str_len_); \
        }                                                                                           \
    }

#define UA_LOG_TRACE(uid, format, ...)                                                                            \
    {                                                                                                             \
        _UA_LOG_(ua::Logger::PRIORITY_TRACE, trace, "[TRACE]|%lu|%lu|%s:%d:%s|" format,                           \
                 ua::ContextMgr::GetContextId(), static_cast<uint64_t>(uid), _FILE_NAME_, __LINE__, __FUNCTION__, \
                 ##__VA_ARGS__);                                                                                  \
    }

#define UA_LOG_DEBUG(uid, format, ...)                                                                            \
    {                                                                                                             \
        _UA_LOG_(ua::Logger::PRIORITY_DEBUG, debug, "[DEBUG]|%lu|%lu|%s:%d:%s|" format,                           \
                 ua::ContextMgr::GetContextId(), static_cast<uint64_t>(uid), _FILE_NAME_, __LINE__, __FUNCTION__, \
                 ##__VA_ARGS__);                                                                                  \
    }

#define UA_LOG_INFO(uid, format, ...)                                                                                \
    {                                                                                                                \
        _UA_LOG_(ua::Logger::PRIORITY_INFO, info, "[INFO]|%lu|%lu|%s:%d:%s|" format, ua::ContextMgr::GetContextId(), \
                 static_cast<uint64_t>(uid), _FILE_NAME_, __LINE__, __FUNCTION__, ##__VA_ARGS__);                    \
    }

#define UA_LOG_WARN(uid, format, ...)                                                                                \
    {                                                                                                                \
        _UA_LOG_(ua::Logger::PRIORITY_WARN, warn, "[WARN]|%lu|%lu|%s:%d:%s|" format, ua::ContextMgr::GetContextId(), \
                 static_cast<uint64_t>(uid), _FILE_NAME_, __LINE__, __FUNCTION__, ##__VA_ARGS__);                    \
    }

#define UA_LOG_ERROR(uid, format, ...)                                                                            \
    {                                                                                                             \
        _UA_LOG_(ua::Logger::PRIORITY_ERROR, error, "[ERROR]|%lu|%lu|%s:%d:%s|" format,                           \
                 ua::ContextMgr::GetContextId(), static_cast<uint64_t>(uid), _FILE_NAME_, __LINE__, __FUNCTION__, \
                 ##__VA_ARGS__);                                                                                  \
    }
