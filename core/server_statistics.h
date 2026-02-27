/// @file server_statistics.h
/// @brief 运行时统计（C++20 重写版）
/// @note 改进: 去掉 UA_DEF_MEMBER 等宏，使用普通成员变量
///       改进: 使用 = {} 清零代替 memset
#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>
#include "patterns/singleton.h"

namespace ua
{

// 耗时分桶边界（毫秒）
inline const std::vector<uint32_t> kLowerCostTime = {0, 50, 100, 500, 1000, 3000, 5000, 60000};

/// 每个统计周期清零的统计数据
struct ServerStatisticsSt
{
    uint32_t recv_pkg_num = 0;
    uint64_t recv_byte_num = 0;
    uint32_t recv_error_pkg_num = 0;
    uint32_t send_pkg_num = 0;
    uint64_t send_byte_num = 0;
    uint32_t send_error_pkg_num = 0;
    uint32_t send_pkg_size_max = 0;
    uint32_t recv_pkg_size_max = 0;
    uint32_t log_error_num = 0;
    uint32_t log_warn_num = 0;
    uint32_t log_info_num = 0;
    uint32_t log_debug_num = 0;
    uint32_t log_trace_num = 0;
    uint32_t rpc_time_out_num = 0;
    uint32_t coro_num_max = 0;
    uint32_t coro_pending_num_max = 0;
    uint32_t on_proc_num = 0;
    uint32_t on_idle_num = 0;
    uint32_t proc_timeout_0 = 0;
    uint32_t proc_timeout_1 = 0;
    uint32_t proc_timeout_2 = 0;
    uint32_t proc_total_timeout = 0;
    uint32_t proc_deal_time_0 = 0;
    uint32_t proc_deal_time_1 = 0;
    uint32_t proc_deal_time_2 = 0;
    uint32_t tick_timeout = 0;
    uint32_t tick_deal_time = 0;

    // 便利的递增/最大值方法
    void inc_recv_pkg_num(uint32_t n = 1) { recv_pkg_num += n; }
    void inc_send_pkg_num(uint32_t n = 1) { send_pkg_num += n; }
    void inc_recv_error_pkg_num(uint32_t n = 1) { recv_error_pkg_num += n; }
    void inc_send_error_pkg_num(uint32_t n = 1) { send_error_pkg_num += n; }
    void inc_log_error_num(uint32_t n = 1) { log_error_num += n; }
    void inc_log_warn_num(uint32_t n = 1) { log_warn_num += n; }
    void inc_log_info_num(uint32_t n = 1) { log_info_num += n; }
    void inc_log_debug_num(uint32_t n = 1) { log_debug_num += n; }
    void inc_log_trace_num(uint32_t n = 1) { log_trace_num += n; }
    void inc_rpc_time_out_num(uint32_t n = 1) { rpc_time_out_num += n; }
    void inc_on_proc_num(uint32_t n = 1) { on_proc_num += n; }
    void inc_on_idle_num(uint32_t n = 1) { on_idle_num += n; }
    void inc_proc_timeout_0(uint32_t n = 1) { proc_timeout_0 += n; }
    void inc_proc_timeout_1(uint32_t n = 1) { proc_timeout_1 += n; }
    void inc_proc_timeout_2(uint32_t n = 1) { proc_timeout_2 += n; }
    void inc_proc_total_timeout(uint32_t n = 1) { proc_total_timeout += n; }
    void inc_tick_timeout(uint32_t n = 1) { tick_timeout += n; }

    void set_max_proc_deal_time_0(uint32_t v) { proc_deal_time_0 = std::max(proc_deal_time_0, v); }
    void set_max_proc_deal_time_1(uint32_t v) { proc_deal_time_1 = std::max(proc_deal_time_1, v); }
    void set_max_proc_deal_time_2(uint32_t v) { proc_deal_time_2 = std::max(proc_deal_time_2, v); }
    void set_max_tick_deal_time(uint32_t v) { tick_deal_time = std::max(tick_deal_time, v); }

    void save_max_send_pkg_size_max(uint32_t v) { send_pkg_size_max = std::max(send_pkg_size_max, v); }
    void save_max_recv_pkg_size_max(uint32_t v) { recv_pkg_size_max = std::max(recv_pkg_size_max, v); }
    void save_max_coro_num_max(uint32_t v) { coro_num_max = std::max(coro_num_max, v); }
    void save_max_coro_pending_num_max(uint32_t v) { coro_pending_num_max = std::max(coro_pending_num_max, v); }
};

struct NotClearServerStatisticsSt
{
    uint64_t start_time = 0;
    uint32_t start_cost_time = 0;
    uint64_t last_reload_time = 0;
    uint32_t last_reload_cost_time = 0;
};

struct RecvCmdStatisticsInfo
{
    std::unordered_map<int32_t, uint32_t> error_code_2_num;
    std::map<uint32_t, uint32_t> cost_map;
    std::map<uint32_t, uint32_t> queue_cost_map;
    uint32_t expire_drop = 0;
    uint32_t schedule_drop = 0;
    uint32_t max_req_size = 0;
    uint32_t max_rsp_size = 0;
    uint32_t total_recv_num = 0;
};

struct SendCmdStatisticsInfo
{
    uint32_t total_send_num = 0;
    uint32_t max_send_size = 0;
};

class ServerStatistics : public Singleton<ServerStatistics>
{
public:
    /// 改进: 使用值初始化代替 memset
    void ClearStatistics()
    {
        statistics_ = {};
        recv_cmd_2_info_.clear();
        send_cmd_2_info_.clear();
    }

    [[nodiscard]] const auto& RecvCmd2Info() const noexcept { return recv_cmd_2_info_; }
    [[nodiscard]] const auto& SendCmd2Info() const noexcept { return send_cmd_2_info_; }
    [[nodiscard]] ServerStatisticsSt& statistics() noexcept { return statistics_; }
    [[nodiscard]] NotClearServerStatisticsSt& not_clear_statistics() noexcept { return not_clear_statistics_; }

    [[nodiscard]] uint32_t GetCostBucket(uint32_t duration) const
    {
        if (duration == 0) duration = 1;
        auto iter = std::lower_bound(kLowerCostTime.begin(), kLowerCostTime.end(), duration);
        if (iter == kLowerCostTime.begin()) return 0;
        return *--iter;
    }

    void SetCoroRunTime(uint32_t cmd, uint32_t duration, int32_t ret_code)
    {
        auto& info = recv_cmd_2_info_[cmd];
        info.error_code_2_num[ret_code] += 1;
        info.cost_map[GetCostBucket(duration)] += 1;
    }

    void SetRspSize(uint32_t cmd, uint32_t body_size)
    {
        recv_cmd_2_info_[cmd].max_rsp_size = std::max(recv_cmd_2_info_[cmd].max_rsp_size, body_size);
    }

    void SetReqSize(uint32_t cmd, uint32_t body_size)
    {
        auto& info = recv_cmd_2_info_[cmd];
        info.max_req_size = std::max(info.max_req_size, body_size);
        ++info.total_recv_num;
    }

    void SetSendSize(uint32_t cmd, uint32_t body_size)
    {
        auto& info = send_cmd_2_info_[cmd];
        info.max_send_size = std::max(info.max_send_size, body_size);
    }

    void AddCmdExpireDrop(uint32_t cmd) { recv_cmd_2_info_[cmd].expire_drop++; }
    void AddCmdScheduleDrop(uint32_t cmd) { recv_cmd_2_info_[cmd].schedule_drop++; }
    void AddSendCmd(uint32_t cmd) { send_cmd_2_info_[cmd].total_send_num++; }

    void SetQueueCost(uint32_t cmd, uint32_t duration)
    {
        recv_cmd_2_info_[cmd].queue_cost_map[GetCostBucket(duration)] += 1;
    }

private:
    friend class Singleton<ServerStatistics>;
    ServerStatistics() = default;

    ServerStatisticsSt statistics_{};
    NotClearServerStatisticsSt not_clear_statistics_{};
    std::unordered_map<uint32_t, RecvCmdStatisticsInfo> recv_cmd_2_info_;
    std::unordered_map<uint32_t, SendCmdStatisticsInfo> send_cmd_2_info_;
};

}  // namespace ua
