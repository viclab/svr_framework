/// @file server_core.h
/// @brief 服务核心生命周期管理（C++20 重写版）
/// @note 改进: SvrOption 参数分组为嵌套结构体
///       改进: protected 区域最小化
///       改进: OnInit/OnTick/OnProc/OnFinish 签名与原版兼容
#pragma once

#include <cstdint>
#include <string>
#include "context_controller.h"
#include "system_mgr.h"
#include "timeout_decorator.h"

namespace ua
{

class PBService;
class IRouting;
struct TransportInfo;
class IServiceMesh;
class IScheduler;
class ICoroutine;

class ServerCore : public SystemMgr
{
public:
    [[nodiscard]] uint32_t GetID() const;
    /// 获取默认 transport
    [[nodiscard]] const TransportInfo* DefaultTransportInfo() const;
    /// 获取对应类型的 transport
    [[nodiscard]] const TransportInfo* FindTransportInfo(uint32_t transport_type) const;
    /// 添加一个 transport，可指定为默认
    bool AddTransportInfo(uint32_t transport_type, const TransportInfo& info, bool is_default = false);
    /// 获取路由管理器
    [[nodiscard]] const IRouting* DefaultRouting() const;
    [[nodiscard]] IRouting* DefaultRouting();
    /// 获取服务网格组件
    [[nodiscard]] const IServiceMesh* GetServiceMesh() const;
    [[nodiscard]] IServiceMesh* GetServiceMesh();
    /// 设置服务网格组件，返回旧值
    IServiceMesh* SetServiceMesh(IServiceMesh* service_mesh);
    /// 获取 PBService 指针
    [[nodiscard]] const PBService* GetPBService() const;
    [[nodiscard]] PBService* GetPBService();
    /// 设置调度器
    bool SetScheduler(IScheduler* new_scheduler);

    /// 改进: 配置参数分组为嵌套结构体
    struct SvrOption
    {
        // 协程插件（nullptr 表示非协程模式）
        ICoroutine* coroutine = nullptr;
        // PBService（nullptr 表示不使用）
        PBService* pb_service = nullptr;
        // 单个服务最大协程数
        uint32_t max_coro_num = 0;

        // 时间片控制
        struct FrameTimeLimit
        {
            uint32_t max_proc_ms = 0;         // 单个 proc 最大时长 (ms)
            uint32_t max_ctx_proc_ms = 0;      // 处理上下文/定时器时间上限 (ms)
            uint32_t min_on_proc_ms = 0;       // 自定义逻辑最小处理时间 (ms)
        };
        FrameTimeLimit frame{};

        // 自适应流控
        struct FlowControl
        {
            uint32_t max_deal_pkg_num = 0;     // 初始最大处理包个数
            uint32_t max_num = 0;              // 能处理的最大包数
            uint32_t min_num = 0;              // 能处理的最小包数
            uint32_t inc_delta = 0;            // 增加的单位
            uint32_t dec_delta = 0;            // 减少的单位（快减）
            uint32_t judge_range_ms = 0;       // 差异判断区间 (ms)
        };
        FlowControl flow_ctrl{};

        // tick 处理最大超时时间 (ms)
        uint32_t max_tick_ms = 1000;
    };

    /// 服务初始化
    bool SvrInit(const SvrOption& option);
    /// 服务 tick 调用
    void SvrTick(uint64_t now_ms, uint64_t tick_count);
    /// 服务主循环
    size_t SvrProc(uint64_t now_ms);
    /// 服务退出前调用
    bool SvrFinish();
    /// 通知服务准备退出
    void SvrNtfQuit();
    /// 检查是否可以安全退出
    [[nodiscard]] bool SvrStopReady() const;
    /// 是否正在停止
    [[nodiscard]] bool IsStoping() const noexcept { return stop_; }

    using TimeoutCallback = TimeoutDecorator::TimeoutTask;
    /// 添加定时事件
    uint64_t AddTimer(uint64_t gid, TimeoutCallback&& callback, uint64_t expire_time, uint32_t interval_time = 0);
    /// 取消定时事件
    bool CancelTimer(uint64_t timer_id);

    virtual ~ServerCore() = default;

protected:
    virtual bool OnInit() { return true; }
    virtual void OnTick(uint64_t now_ms, uint64_t tick_count) {}
    virtual size_t OnProc(uint64_t now_ms, uint64_t remain_ms, bool stop) { return 0; }
    virtual bool OnFinish() { return true; }

private:
    [[nodiscard]] bool CheckOption(const SvrOption& option) const;
    bool InitPBService();
    void AdjustParam(uint64_t remain_ms, uint64_t used_ms);

protected:
    bool stop_ = false;
    uint32_t default_transport_ = 0;
    ContextController context_ctrl_;
    TimeoutDecorator timeout_decorator_;
    IScheduler* req_scheduler_ = nullptr;
    IServiceMesh* service_mesh_ = nullptr;
    SvrOption option_;
};

}  // namespace ua
