/// @file timeout_queue.h
/// @brief 优先队列定时器（C++20 重写版）
/// @note 改进: timer_id 使用 uint64_t 避免回绕
#pragma once

#include <cstdint>
#include <functional>
#include <set>
#include <unordered_map>

namespace ua
{

class TimeoutQueue
{
public:
    /// 改进: timer_id 使用 uint64_t 避免 32 位回绕
    using Task = std::function<void(uint64_t timer_id, uint32_t interval_time)>;

    /// 添加定时器，返回唯一 timer_id，失败返回 0
    [[nodiscard]] uint64_t Add(Task task, uint64_t expire_time, uint32_t interval_time = 0);
    /// 取消定时器
    bool Cancel(uint64_t timer_id);
    /// 处理所有超时的定时器，返回处理数量
    uint32_t TimeOut(uint64_t now);
    /// 判断定时器是否存在
    [[nodiscard]] bool Exist(uint64_t timer_id) const;
    /// 清空所有定时器
    void Clear();

private:
    [[nodiscard]] uint64_t GenerateID() noexcept;

    struct Timer
    {
        uint64_t seq_id = 0;
        uint32_t interval_time = 0;
        uint64_t expire_time = 0;
        Task task;

        friend auto operator<=>(const Timer& left, const Timer& right) noexcept
        {
            if (auto cmp = left.expire_time <=> right.expire_time; cmp != 0)
                return cmp;
            return left.seq_id <=> right.seq_id;
        }

        friend bool operator==(const Timer& left, const Timer& right) noexcept
        {
            return left.expire_time == right.expire_time && left.seq_id == right.seq_id;
        }
    };

    std::set<Timer> timer_queue_;
    std::unordered_map<uint64_t, uint64_t> timer_id_index_;
    uint64_t base_id_ = 0;
};

}  // namespace ua
