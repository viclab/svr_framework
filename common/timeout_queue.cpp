/// @file timeout_queue.cpp
/// @brief 优先队列定时器实现（C++20 重写版）
#include "timeout_queue.h"

namespace ua
{

uint64_t TimeoutQueue::Add(Task task, uint64_t expire_time, uint32_t interval_time)
{
    uint64_t new_id = GenerateID();

    auto [_, ok1] = timer_id_index_.emplace(new_id, expire_time);
    if (!ok1)
        return 0;

    auto [__, ok2] = timer_queue_.emplace(Timer{new_id, interval_time, expire_time, std::move(task)});
    if (!ok2)
    {
        timer_id_index_.erase(new_id);
        return 0;
    }

    return new_id;
}

bool TimeoutQueue::Cancel(uint64_t timer_id)
{
    auto iter = timer_id_index_.find(timer_id);
    if (iter == timer_id_index_.end())
        return false;

    timer_queue_.erase(Timer{timer_id, 0, iter->second, nullptr});
    timer_id_index_.erase(iter);
    return true;
}

uint32_t TimeoutQueue::TimeOut(uint64_t now)
{
    uint32_t count = 0;
    while (!timer_queue_.empty())
    {
        auto iter = timer_queue_.begin();
        if (iter->expire_time > now)
            break;

        // 拷贝一份，先删再执行，防止 task 中操作定时器
        Timer tmp = *iter;
        timer_id_index_.erase(iter->seq_id);
        timer_queue_.erase(iter);

        // 循环定时器：重新加入
        if (tmp.interval_time > 0)
        {
            tmp.expire_time += tmp.interval_time;
            if (auto [_, ok] = timer_id_index_.emplace(tmp.seq_id, tmp.expire_time); ok)
            {
                timer_queue_.insert(tmp);
            }
        }

        tmp.task(tmp.seq_id, tmp.interval_time);
        ++count;
    }
    return count;
}

bool TimeoutQueue::Exist(uint64_t timer_id) const
{
    return timer_id_index_.contains(timer_id);
}

void TimeoutQueue::Clear()
{
    timer_queue_.clear();
    timer_id_index_.clear();
}

uint64_t TimeoutQueue::GenerateID() noexcept
{
    ++base_id_;
    if (base_id_ == 0)
        ++base_id_;
    return base_id_;
}

}  // namespace ua
