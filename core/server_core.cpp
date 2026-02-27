/// @file server_core.cpp
/// @brief 服务核心实现（C++20 重写版）
/// @note 保持与原版完全兼容的三阶段时间片调度和自适应流控
#include "server_core.h"
#include <algorithm>
#include <string>
#include <vector>
#include "common/id_generator.h"
#include "common/utils.h"
#include "coro_mgr.h"
#include "interface/channel_interface.h"
#include "interface/routing_interface.h"
#include "interface/scheduler_interface.h"
#include "interface/service_mesh.h"
#include "logger.h"
#include "transport.h"
#ifdef UA_HAS_PROTOBUF
#include "pb/pb_service.h"
#endif
#include "server_statistics.h"

namespace ua
{

#ifdef UA_HAS_PROTOBUF
// 超时定时器通道用了 PBService::MAX_TRANSPORT_NUM 保证和其它 channel 的 index 不会重叠
static constexpr uint32_t TIMEOUT_CHANNEL_INDEX = PBService::MAX_TRANSPORT_NUM;
#else
static constexpr uint32_t TIMEOUT_CHANNEL_INDEX = 32;
#endif

bool ServerCore::SvrInit(const SvrOption& option)
{
    if (!CheckOption(option))
        return false;

    option_ = option;

    // 初始化全局 ID 生成器
    if (!IDGenerator::GetInst().Init())
    {
        UA_LOG_ERROR(0, "id generator init fail");
        return false;
    }

    if (option_.coroutine)
    {
        option_.coroutine->SetMaxCoroNum(option_.max_coro_num);
        CoroMgr::SetCoroutine(option_.coroutine);
    }

    // 初始化上下文控制器
    if (!context_ctrl_.Init(option_.coroutine))
    {
        UA_LOG_ERROR(0, "context controller init fail");
        return false;
    }

    // 定时事件处理
    timeout_decorator_.Init(option_.coroutine != nullptr);

#ifdef UA_HAS_PROTOBUF
    if (!InitPBService())
        return false;
#endif

    // 用户自定义初始化
    if (!OnInit())
        return false;

    // 模块自定义初始化
    if (!SystemInit())
        return false;

    UA_LOG_INFO(0, "ServerCore SvrInit ok");
    return true;
}

bool ServerCore::CheckOption(const SvrOption& option) const
{
    if (option.flow_ctrl.min_num > option.flow_ctrl.max_num)
    {
        UA_LOG_ERROR(0, "min_num(%u) > max_num(%u)", option.flow_ctrl.min_num, option.flow_ctrl.max_num);
        return false;
    }

    if (option.flow_ctrl.max_deal_pkg_num > option.flow_ctrl.max_num ||
        option.flow_ctrl.max_deal_pkg_num < option.flow_ctrl.min_num)
    {
        UA_LOG_ERROR(0, "max_deal_pkg_num(%u) not in range[%u, %u]", option.flow_ctrl.max_deal_pkg_num,
                     option.flow_ctrl.min_num, option.flow_ctrl.max_num);
        return false;
    }

    if (option.frame.min_on_proc_ms > option.frame.max_proc_ms)
    {
        UA_LOG_ERROR(0, "min_on_proc_ms(%u) > max_proc_ms(%u)", option.frame.min_on_proc_ms,
                     option.frame.max_proc_ms);
        return false;
    }

    if (option.frame.max_ctx_proc_ms > option.frame.max_proc_ms)
    {
        UA_LOG_ERROR(0, "max_ctx_proc_ms(%u) > max_proc_ms(%u)", option.frame.max_ctx_proc_ms,
                     option.frame.max_proc_ms);
        return false;
    }

    return true;
}

#ifdef UA_HAS_PROTOBUF
bool ServerCore::InitPBService()
{
    if (option_.pb_service)
    {
        option_.pb_service->SetContextCtrl(&context_ctrl_);
        UA_LOG_INFO(0, "init pb service");
    }
    else
    {
        UA_LOG_INFO(0, "do not use pb service");
    }
    return true;
}
#endif

void ServerCore::SvrTick(uint64_t now_ms, uint64_t tick_count)
{
    uint64_t begin_ms = now_ms;
    OnTick(now_ms, tick_count);
    SystemTick(now_ms, tick_count);
    uint64_t end_ms = utils::CurrentRealMilliSec();

    if (end_ms > begin_ms + option_.max_tick_ms)
    {
        UA_LOG_WARN(0, "end_ms(%lu) - begin_ms(%lu) = %lu > %u", end_ms, begin_ms, end_ms - begin_ms,
                    option_.max_tick_ms);
        ServerStatistics::GetInst().statistics().inc_tick_timeout();
    }

    ServerStatistics::GetInst().statistics().set_max_tick_deal_time(
        static_cast<uint32_t>(end_ms - begin_ms));
}

size_t ServerCore::SvrProc(uint64_t now_ms)
{
    ServerStatistics::GetInst().statistics().inc_on_proc_num();
    uint64_t begin_ms = now_ms;

    // ===== 阶段 0: 处理超时上下文和定时事件 =====
    uint32_t ctx_count = context_ctrl_.ProcTimeOut(now_ms);
    uint32_t timeout_count = stop_ ? 0 : timeout_decorator_.ProcTimeOut(now_ms);

    uint64_t end_ms = utils::CurrentRealMilliSec();
    if (end_ms > begin_ms + option_.frame.max_ctx_proc_ms)
    {
        UA_LOG_WARN(0, "end_ms(%lu) - begin_ms(%lu) = %lu > %u, ctx(%u) timeout(%u)", end_ms, begin_ms,
                    end_ms - begin_ms, option_.frame.max_ctx_proc_ms, ctx_count, timeout_count);
        ServerStatistics::GetInst().statistics().inc_proc_timeout_0();
    }
    ServerStatistics::GetInst().statistics().set_max_proc_deal_time_0(
        static_cast<uint32_t>(end_ms - begin_ms));

    // ===== 阶段 1: 处理子类自定义逻辑和模块逻辑 =====
    // 保证有最小执行时间
    uint64_t remain_ms = (end_ms + option_.frame.min_on_proc_ms) > (option_.frame.max_proc_ms + begin_ms)
                             ? option_.frame.min_on_proc_ms
                             : (option_.frame.max_proc_ms + begin_ms - end_ms);
    uint32_t proc_count = 0;
    proc_count += static_cast<uint32_t>(OnProc(now_ms, remain_ms, stop_));
    proc_count += static_cast<uint32_t>(SystemProc(now_ms, remain_ms, stop_));
    // 如果有服务网格组件，优先调度
    if (service_mesh_)
        proc_count += service_mesh_->Process();

    uint64_t end_ms1 = utils::CurrentRealMilliSec();
    ServerStatistics::GetInst().statistics().set_max_proc_deal_time_1(
        static_cast<uint32_t>(end_ms1 - end_ms));
    if (end_ms1 > end_ms + remain_ms)
    {
        UA_LOG_WARN(0, "end_ms1(%lu) - end_ms(%lu) = %lu > remain_ms(%lu), proc(%u)", end_ms1, end_ms,
                    end_ms1 - end_ms, remain_ms, proc_count);
        ServerStatistics::GetInst().statistics().inc_proc_timeout_1();
    }

    // ===== 阶段 2: 处理数据包 =====
    uint32_t deal_scheduler_count = 0;
    uint32_t deal_pkg_count = 0;
    // 调度器里的包优先处理
    if (req_scheduler_)
    {
        deal_scheduler_count = req_scheduler_->LoopOnce(option_.flow_ctrl.max_deal_pkg_num);
        deal_pkg_count += deal_scheduler_count;
    }

    // 有调度器或者不是 stop 时才收包
    if (req_scheduler_ || !stop_)
    {
        uint32_t one_loop_num = option_.flow_ctrl.min_num;
        if (option_.flow_ctrl.max_deal_pkg_num > deal_pkg_count + option_.flow_ctrl.min_num)
            one_loop_num = option_.flow_ctrl.max_deal_pkg_num - deal_pkg_count;

        auto* transport_info = DefaultTransportInfo();
        if (transport_info && transport_info->channel)
            deal_pkg_count += static_cast<uint32_t>(transport_info->channel->Loop(one_loop_num));
    }

    uint64_t end_ms2 = utils::CurrentRealMilliSec();
    ServerStatistics::GetInst().statistics().set_max_proc_deal_time_2(
        static_cast<uint32_t>(end_ms2 - end_ms1));
    if (end_ms2 > end_ms + remain_ms)
    {
        UA_LOG_WARN(0, "end_ms2(%lu) - end_ms(%lu) = %lu > remain_ms(%lu), scheduler(%u) deal(%u)", end_ms2, end_ms,
                    end_ms2 - end_ms, remain_ms, deal_scheduler_count, deal_pkg_count);
        ServerStatistics::GetInst().statistics().inc_proc_timeout_2();
    }

    // 自适应流控：根据当前是否超时动态调整收包上限
    AdjustParam(remain_ms, end_ms2 - end_ms);

    // 检查单次 proc 是否超时
    end_ms = utils::CurrentRealMilliSec();
    if (end_ms > begin_ms + option_.frame.max_proc_ms)
    {
        UA_LOG_WARN(0, "end_ms(%lu) - begin_ms(%lu) = %lu > %u, ctx(%u) timeout(%u) deal(%u)", end_ms, begin_ms,
                    end_ms - begin_ms, option_.frame.max_proc_ms, ctx_count, timeout_count,
                    proc_count + deal_pkg_count);
        ServerStatistics::GetInst().statistics().inc_proc_total_timeout();
    }

    return ctx_count + timeout_count + proc_count + deal_pkg_count;
}

void ServerCore::AdjustParam(uint64_t remain_ms, uint64_t used_ms)
{
    // 快减慢增策略
    if (used_ms > remain_ms + option_.flow_ctrl.judge_range_ms)
    {
        // 超时了，快速缩减收包量
        if (option_.flow_ctrl.max_deal_pkg_num > option_.flow_ctrl.min_num + option_.flow_ctrl.dec_delta)
            option_.flow_ctrl.max_deal_pkg_num -= option_.flow_ctrl.dec_delta;
        else if (option_.flow_ctrl.max_deal_pkg_num > option_.flow_ctrl.min_num)
            option_.flow_ctrl.max_deal_pkg_num = option_.flow_ctrl.min_num;
    }
    else if (used_ms + option_.flow_ctrl.judge_range_ms * 2 < remain_ms)
    {
        // 空闲了，慢慢增加收包量
        if (option_.flow_ctrl.max_deal_pkg_num + option_.flow_ctrl.inc_delta < option_.flow_ctrl.max_num)
            option_.flow_ctrl.max_deal_pkg_num += option_.flow_ctrl.inc_delta;
        else if (option_.flow_ctrl.max_deal_pkg_num < option_.flow_ctrl.max_num)
            option_.flow_ctrl.max_deal_pkg_num = option_.flow_ctrl.max_num;
    }
}

bool ServerCore::SvrStopReady() const
{
    if (stop_)
    {
        if (context_ctrl_.PendingContextNum() == 0)
            return true;

        // 200ms 打印一次，防止日志过多
        static uint64_t last_log_time = 0;
        uint64_t now = utils::CurrentRealMilliSec();
        if (last_log_time + 200 < now)
        {
            UA_LOG_WARN(0, "pending context(%lu) coroutine(%lu)", context_ctrl_.PendingContextNum(),
                        context_ctrl_.PendingCoroutineNum());
            last_log_time = now;
        }
    }
    return false;
}

bool ServerCore::SvrFinish()
{
    OnFinish();
    SystemFinish();
    UA_LOG_INFO(0, "finish(%d)", stop_);
    return true;
}

void ServerCore::SvrNtfQuit()
{
    if (!stop_)
    {
        stop_ = true;
        if (req_scheduler_)
            req_scheduler_->SetStop(true);
        UA_LOG_INFO(0, "ntf quit, pending ctx num(%lu) coro(%lu)", context_ctrl_.PendingContextNum(),
                    context_ctrl_.PendingCoroutineNum());
    }
}

// ===== Transport / Routing / ServiceMesh / PBService 管理 =====
uint32_t ServerCore::GetID() const
{
    auto* info = DefaultTransportInfo();
    if (info && info->channel)
        return info->channel->MyID();
    return 0;
}

const TransportInfo* ServerCore::DefaultTransportInfo() const
{
    return FindTransportInfo(default_transport_);
}

const TransportInfo* ServerCore::FindTransportInfo(uint32_t transport_type) const
{
#ifdef UA_HAS_PROTOBUF
    if (option_.pb_service)
        return option_.pb_service->FindTransport(transport_type);
#endif
    return nullptr;
}

bool ServerCore::AddTransportInfo(uint32_t transport_type, const TransportInfo& info, bool is_default)
{
#ifdef UA_HAS_PROTOBUF
    if (!option_.pb_service)
        return false;
    if (!option_.pb_service->AddTransport(transport_type, info))
        return false;
    if (is_default)
        default_transport_ = transport_type;
    return true;
#else
    return false;
#endif
}

const IRouting* ServerCore::DefaultRouting() const
{
    auto* info = DefaultTransportInfo();
    return info ? info->routing : nullptr;
}

IRouting* ServerCore::DefaultRouting()
{
    auto* info = DefaultTransportInfo();
    return info ? info->routing : nullptr;
}

const IServiceMesh* ServerCore::GetServiceMesh() const
{
    return service_mesh_;
}

IServiceMesh* ServerCore::GetServiceMesh()
{
    return service_mesh_;
}

IServiceMesh* ServerCore::SetServiceMesh(IServiceMesh* service_mesh)
{
    std::swap(service_mesh, service_mesh_);
    return service_mesh;
}

#ifdef UA_HAS_PROTOBUF
const PBService* ServerCore::GetPBService() const
{
    return option_.pb_service;
}

PBService* ServerCore::GetPBService()
{
    return option_.pb_service;
}
#endif

bool ServerCore::SetScheduler(IScheduler* new_scheduler)
{
    if (req_scheduler_ || !new_scheduler)
    {
        UA_LOG_ERROR(0, "req scheduler already exist or new_scheduler is nullptr");
        return false;
    }

    req_scheduler_ = new_scheduler;
#ifdef UA_HAS_PROTOBUF
    if (option_.pb_service)
        option_.pb_service->SetReqScheduler(req_scheduler_);
#endif
    timeout_decorator_.SetReqScheduler(req_scheduler_, TIMEOUT_CHANNEL_INDEX);

    // 指定调度器处理函数
    req_scheduler_->SetProcFunc([this](uint64_t gid, const char* data, uint32_t len, uint64_t custom_data) {
        uint32_t transport_type = static_cast<uint32_t>(custom_data);
        if (transport_type == TIMEOUT_CHANNEL_INDEX)
            return timeout_decorator_.DealEvent(data, len);
#ifdef UA_HAS_PROTOBUF
        else if (option_.pb_service)
            return option_.pb_service->DealReqPkg(data, len, transport_type);
#endif
        else
            return false;
    });
    return true;
}

uint64_t ServerCore::AddTimer(uint64_t gid, TimeoutCallback&& callback, uint64_t expire_time, uint32_t interval_time)
{
    return timeout_decorator_.AddEvent(gid, std::move(callback), expire_time, interval_time);
}

bool ServerCore::CancelTimer(uint64_t timer_id)
{
    return timeout_decorator_.DelEvent(timer_id);
}

}  // namespace ua
