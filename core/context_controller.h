/// @file context_controller.h
/// @brief 上下文控制器（C++20 重写版）
/// @note 支持协程/异步双模式上下文切换
///       改进: Init 参数改为 ICoroutine*，避免隐式 bool 转换
#pragma once

#include <unordered_map>
#include "common/timeout_queue.h"
#include "context.h"

namespace ua
{

class ICoroutine;

class ContextController
{
public:
    /// 改进: 参数改为 ICoroutine*，内部判断是否启用协程
    bool Init(ICoroutine* coroutine) noexcept;
    /// 处理定时器超时
    uint32_t ProcTimeOut(uint64_t now);
    /// 挂起当前上下文
    /// 改进: insert 失败时返回错误而非 SUCCESS
    int32_t Pending(uint64_t seq_id, uint32_t timeout, ClientContext* client_ctx, const AsyncTask& task);
    /// 唤醒挂起的上下文
    ClientContext* Awake(uint64_t seq_id, int32_t ret_code);
    /// 是否使用协程模式
    [[nodiscard]] bool UseCoroutine() const noexcept;
    /// 挂起的上下文数量
    [[nodiscard]] size_t PendingContextNum() const noexcept;
    /// 挂起的协程数量
    [[nodiscard]] size_t PendingCoroutineNum() const noexcept;

private:
    TimeoutQueue timeout_queue_;
    std::unordered_map<uint64_t, ClientContext*> context_cache_;
    bool init_ = false;
    bool use_coroutine_ = false;
};

}  // namespace ua
