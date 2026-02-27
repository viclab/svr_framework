/// @file context_mgr.h
/// @brief 当前线程上下文管理器（C++20 重写版）
#pragma once

#include <cstdint>

namespace ua
{

struct ServerContext;

/// 线程局部的当前事务 ServerContext 管理器
class ContextMgr
{
public:
    [[nodiscard]] static ServerContext* GetCurrServerContext() noexcept { return curr_context_; }
    static void SetCurrServerContext(ServerContext* ctx) noexcept { curr_context_ = ctx; }
    [[nodiscard]] static bool IsNull() noexcept { return curr_context_ == nullptr; }
    [[nodiscard]] static uint64_t GetContextId() noexcept;

private:
    static inline thread_local ServerContext* curr_context_ = nullptr;
};

}  // namespace ua
