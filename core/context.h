/// @file context.h
/// @brief 上下文体系（C++20 重写版）
/// @note 改进: Context::auto_counter 使用 std::atomic 保证线程安全
///       改进: id 使用 uint64_t 避免回绕
///       改进: svr_version 修正拼写
#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include "context_mgr.h"

namespace ua
{

/// 上下文基类
struct Context
{
    /// 自定义回调处理函数
    using Callback = std::function<void(int32_t)>;
    /// 自定义回收函数
    using RecycleFun = std::function<void()>;

    Context() noexcept : id(auto_counter_.fetch_add(1, std::memory_order_relaxed)) {}
    virtual ~Context() = default;

    void SetCallback(const Callback& cb, const RecycleFun& fun = nullptr)
    {
        callback_ = cb;
        recycle_ = fun;
    }

    void SetCallback(Callback&& cb, RecycleFun&& fun = nullptr) noexcept
    {
        callback_ = std::move(cb);
        recycle_ = std::move(fun);
    }

    void Run()
    {
        assert(callback_);
        callback_(ret_code);
        if (recycle_)
            recycle_();
    }

    /// 改进: 使用 uint64_t 避免回绕
    const uint64_t id;
    int32_t ret_code = 0;

protected:
    Callback callback_;
    RecycleFun recycle_;

private:
    /// 改进: 使用 atomic 保证线程安全
    inline static std::atomic<uint64_t> auto_counter_{1};
};

/// 服务端上下文 —— 被调侧，一个请求对应一个
struct ServerContext : public Context
{
    [[nodiscard]] uint32_t Duration() const noexcept
    {
        return static_cast<uint32_t>(end_time - start_time);
    }

    [[nodiscard]] bool IsFinish() const noexcept
    {
        return ret_code || !to_be_continue;
    }

    ~ServerContext() override = default;

    bool to_be_continue = false;
    uint64_t start_time = 0;
    uint64_t end_time = 0;
    uint64_t gid = 0;
    uint16_t pkg_flag = 0;
    uint32_t svr_version = 0;  // 改进: 修正拼写 svr_verison -> svr_version
};

/// 客户端上下文 —— 主调侧，一个 RPC 对应一个
struct ClientContext : public Context
{
    ClientContext() noexcept
    {
        server_ctx = ContextMgr::GetCurrServerContext();
    }
    ~ClientContext() override = default;

    uint32_t timer_id = 0;
    ServerContext* server_ctx = nullptr;
};

/// 异步任务封装：回调 + 回收 + 阻塞（三合一）
struct AsyncTask
{
    using RpcCallback = std::function<void(int32_t, ServerContext*)>;
    using BlockingCallBack = std::function<void()>;
    using RecycleCallBack = Context::RecycleFun;

    AsyncTask(std::nullptr_t) noexcept {}

    template <typename Callable, typename Recycle = RecycleCallBack, typename Blocking = BlockingCallBack>
    AsyncTask(const Callable& cb, const Recycle& recycle = nullptr, const Blocking& blocking = nullptr)
        : callback(cb), recycle_fun(recycle), blocking_fun(blocking)
    {
    }

    RpcCallback callback;
    RecycleCallBack recycle_fun;
    BlockingCallBack blocking_fun;
};

}  // namespace ua
