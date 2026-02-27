/// @file timeout_decorator.cpp
/// @brief 定时事件装饰器实现（C++20 重写版）
#include "timeout_decorator.h"
#include <memory>
#include "common/clock.h"
#include "common/id_generator.h"
#include "context.h"
#include "context_mgr.h"
#include "coro_mgr.h"
#include "interface/scheduler_interface.h"
#include "logger.h"

namespace ua
{

/// 事件信息（通过调度器传递的内部数据结构）
struct EventInfo
{
    uint64_t event_id = 0;
    uint64_t gid = 0;
    uint64_t timer_id = 0;
    uint32_t interval_time = 0;
};

void TimeoutDecorator::Init(bool use_coroutine)
{
    use_coroutine_ = use_coroutine;
}

void TimeoutDecorator::SetReqScheduler(IScheduler* scheduler, uint32_t special_transport_type)
{
    scheduler_ = scheduler;
    special_transport_type_ = special_transport_type;
}

uint64_t TimeoutDecorator::AddEvent(uint64_t gid, TimeoutTask callback, uint64_t expire_time, uint32_t interval_time)
{
    return timeout_mgr_.Add(
        [this, gid, callback = std::move(callback), expire_time, interval_time](uint64_t timer_id,
                                                                                 uint32_t interval) {
            uint64_t seq_id = IDGenerator::GetInst().GenerateSeqID();
            timeout_event_.emplace(seq_id, std::move(callback));
            EventInfo info{seq_id, gid, timer_id, interval};

            UA_LOG_TRACE(gid, "recv timeout event, seq_id %lu, timer_id %lu, expire %lu, interval %u", seq_id,
                         timer_id, expire_time, interval_time);

            auto* data_ptr = reinterpret_cast<const char*>(&info);
            if (scheduler_)
                scheduler_->OnRequest(seq_id, gid, data_ptr, sizeof(info), special_transport_type_);
            else
                DealEvent(data_ptr, sizeof(info));
        },
        expire_time, interval_time);
}

bool TimeoutDecorator::DelEvent(uint64_t timer_id)
{
    // 对于单次定时器事件：进入 scheduler 后就无法取消了
    // 对于循环定时器事件：取消成功，调度器中的事件也不执行
    return timeout_mgr_.Cancel(timer_id);
}

uint32_t TimeoutDecorator::ProcTimeOut(uint64_t now)
{
    return timeout_mgr_.TimeOut(now);
}

bool TimeoutDecorator::DealEvent(const char* data, uint32_t len)
{
    if (!data || len != sizeof(EventInfo))
    {
        UA_LOG_ERROR(0, "deal timeout event param fail, len %u", len);
        return false;
    }

    const auto* info = reinterpret_cast<const EventInfo*>(data);
    uint64_t event_id = info->event_id;
    uint64_t gid = info->gid;

    auto iter = timeout_event_.find(event_id);
    if (iter == timeout_event_.end())
    {
        UA_LOG_WARN(gid, "find timeout event, event_id %lu", event_id);
        return false;
    }

    // 循环定时器事件：如果已被取消，则不执行
    if (info->interval_time > 0)
    {
        if (!timeout_mgr_.Exist(info->timer_id))
        {
            UA_LOG_INFO(gid, "interval timeout has cancel, event_id %lu, timer_id %lu, interval %u", event_id,
                        info->timer_id, info->interval_time);
            timeout_event_.erase(iter);
            return false;
        }
    }

    // 拷贝出 task 再删除
    TimeoutTask task = std::move(iter->second);
    timeout_event_.erase(iter);

    UA_LOG_TRACE(gid, "timeout event, event_id %lu", event_id);

    auto context = std::make_unique<ServerContext>();
    auto* context_ptr = context.get();

    context->SetCallback(
        [this, context_ptr, gid](int32_t ret) { EventFinish(context_ptr, gid); },
        [context_ptr]() { delete context_ptr; });
    context->start_time = Clock::GetInst().CurrentMilliSec();
    context->gid = gid;

    if (use_coroutine_)
    {
        if (!CoroMgr::GetInst().Spawn([this, task = std::move(task), gid, context_ptr]() {
                Run(context_ptr, task, gid);
            }))
        {
            UA_LOG_ERROR(gid, "spawn error, event_id %lu", event_id);
            return false;
        }
    }
    else
    {
        Run(context_ptr, task, gid);
    }

    // 没出错则内存由 context Run 释放
    context.release();
    return true;
}

void TimeoutDecorator::Run(ServerContext* context, const TimeoutTask& task, uint64_t gid)
{
    ContextMgr::SetCurrServerContext(context);
    context->ret_code = task();
    if (context->IsFinish())
        context->Run();
}

void TimeoutDecorator::EventFinish(ServerContext* context, uint64_t gid)
{
    if (scheduler_)
        scheduler_->OnResponse(gid);

    if (watch_func_)
        watch_func_(*context, gid);

    context->end_time = Clock::GetInst().CurrentMilliSec();
    ContextMgr::SetCurrServerContext(nullptr);
}

}  // namespace ua
