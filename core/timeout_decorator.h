/// @file timeout_decorator.h
/// @brief 定时事件装饰器（C++20 重写版）
/// @note 将超时事件包装成类似 Service 的上下文处理
///       支持协程和异步两种模式
///       支持调度器集成
#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include "common/timeout_queue.h"

namespace ua
{

struct ServerContext;
class IScheduler;

class TimeoutDecorator
{
public:
    using FinishWatchFunc = std::function<void(const ServerContext&, uint64_t gid)>;
    using TimeoutTask = std::function<int32_t()>;

    void Init(bool use_coroutine);

    /// 设置调度器，special_transport_type 指定调度器识别的唯一标识
    void SetReqScheduler(IScheduler* scheduler, uint32_t special_transport_type);

    /// 添加定时事件
    uint64_t AddEvent(uint64_t gid, TimeoutTask callback, uint64_t expire_time, uint32_t interval_time = 0);
    /// 删除定时事件
    bool DelEvent(uint64_t timer_id);

    /// 定时器循环处理
    uint32_t ProcTimeOut(uint64_t now);

    /// 处理超时事件（从调度器或直接调用）
    bool DealEvent(const char* data, uint32_t len);

    void SetFinishWatch(FinishWatchFunc watch_func) { watch_func_ = std::move(watch_func); }

private:
    void Run(ServerContext* context, const TimeoutTask& task, uint64_t gid);
    void EventFinish(ServerContext* context, uint64_t gid);

private:
    bool use_coroutine_ = true;
    uint32_t special_transport_type_ = 0;
    IScheduler* scheduler_ = nullptr;
    std::unordered_map<uint64_t, TimeoutTask> timeout_event_;
    TimeoutQueue timeout_mgr_;
    FinishWatchFunc watch_func_ = nullptr;
};

}  // namespace ua
