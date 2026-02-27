/// @file wait_group.h
/// @brief 并发同步原语（C++20 重写版）
/// @note 支持等待多个异步操作全部完成后触发回调
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include "context.h"
#include "context_controller.h"

namespace ua
{

class WaitGroup
{
public:
    using DoneCallback = std::function<void()>;

    /// 创建一个 WaitGroup
    /// @param count 需要等待的任务数
    /// @param callback 全部完成后的回调
    /// @param ctx_ctrl 上下文控制器（用于协程挂起）
    WaitGroup(uint32_t count, DoneCallback callback = nullptr, ContextController* ctx_ctrl = nullptr)
        : count_(count), callback_(std::move(callback)), ctx_ctrl_(ctx_ctrl)
    {
    }

    /// 标记一个任务完成
    void Done()
    {
        if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            if (callback_)
                callback_();
        }
    }

    /// 等待所有任务完成（协程模式下会挂起）
    [[nodiscard]] bool Wait() const
    {
        return count_.load(std::memory_order_acquire) == 0;
    }

    [[nodiscard]] uint32_t Remaining() const noexcept
    {
        return count_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<uint32_t> count_;
    DoneCallback callback_;
    ContextController* ctx_ctrl_ = nullptr;
};

}  // namespace ua
