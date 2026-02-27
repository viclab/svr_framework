/// @file queue_lock_free.h
/// @brief 无锁队列（C++20 重写版）
/// @note 改进: enum class 替代裸 enum
///       改进: 移除 volatile（对 std::atomic 无意义）
///       改进: [[nodiscard]] 标记查询方法
#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace ua
{

enum class LockFreeErr : int32_t
{
    QueueFull = -1,
    Again = -2,      // 数据还没写完，等会儿再试
    TryMax = -3,     // CAS 失败次数过多
};

static constexpr uint8_t MAX_TRY_TIMES = 100;

/// 一读多写的无锁队列
template <typename DATA_TYPE, uint32_t QUEUE_SIZE>
class FreeLockQueue
{
public:
    int Push(const DATA_TYPE& data)
    {
        Index new_tail;
        uint32_t try_times = 0;
        Index old_tail;
        do
        {
            if (try_times >= MAX_TRY_TIMES)
                return static_cast<int>(LockFreeErr::TryMax);

            old_tail = tail_index_.load(std::memory_order_relaxed);
            if (IsFull())
                return static_cast<int>(LockFreeErr::QueueFull);

            new_tail.index = (old_tail.index + 1) % QUEUE_SIZE;
            new_tail.version = old_tail.version + 1;
            ++try_times;
        } while (!tail_index_.compare_exchange_weak(old_tail, new_tail,
                                                     std::memory_order_acq_rel,
                                                     std::memory_order_relaxed));

        data_[old_tail.index] = data;
        data_flag_[old_tail.index].store(true, std::memory_order_release);
        return 0;
    }

    int Pop(DATA_TYPE& data)
    {
        if (IsEmpty())
            return static_cast<int>(LockFreeErr::QueueFull);

        Index old_head = head_index_.load(std::memory_order_acquire);
        if (!data_flag_[old_head.index].load(std::memory_order_acquire))
            return static_cast<int>(LockFreeErr::Again);

        data = data_[old_head.index];
        data_flag_[old_head.index].store(false, std::memory_order_release);

        Index new_head;
        new_head.index = (old_head.index + 1) % QUEUE_SIZE;
        new_head.version = old_head.version + 1;
        head_index_.store(new_head, std::memory_order_release);
        return 0;
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return tail_index_.load(std::memory_order_acquire).index ==
               head_index_.load(std::memory_order_acquire).index;
    }

    [[nodiscard]] bool IsFull() const
    {
        return (tail_index_.load(std::memory_order_acquire).index + 1) % QUEUE_SIZE ==
               head_index_.load(std::memory_order_acquire).index;
    }

private:
    struct Index
    {
        uint32_t version = 0;
        uint32_t index = 0;
    };

    std::array<DATA_TYPE, QUEUE_SIZE> data_{};
    std::array<std::atomic<bool>, QUEUE_SIZE> data_flag_{};
    std::atomic<Index> head_index_{};
    std::atomic<Index> tail_index_{};
};

}  // namespace ua
