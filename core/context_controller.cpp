/// @file context_controller.cpp
/// @brief 上下文控制器实现（C++20 重写版）
/// @note 改进: insert 失败时取消定时器并返回错误
///       改进: Init 接受 ICoroutine* 参数
#include "context_controller.h"
#include "common/clock.h"
#include "common/id_generator.h"
#include "coro_mgr.h"
#include "logger.h"
#include "rpc_error.h"
#include "server_statistics.h"

namespace ua
{

bool ContextController::Init(ICoroutine* coroutine) noexcept
{
    if (!init_)
    {
        use_coroutine_ = (coroutine != nullptr);
        init_ = true;
    }
    return true;
}

uint32_t ContextController::ProcTimeOut(uint64_t now)
{
    return timeout_queue_.TimeOut(now);
}

ClientContext* ContextController::Awake(uint64_t seq_id, int32_t ret_code)
{
    auto iter = context_cache_.find(seq_id);
    if (iter == context_cache_.end())
    {
        UA_LOG_WARN(0, "cache can not find seq_id(%lu), ret(%d)", seq_id, ret_code);
        return nullptr;
    }

    auto* client_ctx = iter->second;
    assert(client_ctx);
    context_cache_.erase(iter);

    if (ret_code != RPC_TIME_OUT)
    {
        timeout_queue_.Cancel(client_ctx->timer_id);
    }
    else
    {
        ServerStatistics::GetInst().statistics().inc_rpc_time_out_num();
    }

    UA_LOG_TRACE(0, "seq_id(%lu) awake, timer_id(%u), ret(%d)", seq_id, client_ctx->timer_id, ret_code);

    client_ctx->ret_code = ret_code;
    client_ctx->timer_id = 0;
    return client_ctx;
}

int32_t ContextController::Pending(uint64_t seq_id, uint32_t timeout, ClientContext* client_ctx, const AsyncTask& task)
{
    if (!client_ctx)
    {
        UA_LOG_ERROR(0, "params error");
        return RPC_SYS_ERR;
    }

    if (seq_id == 0)
        seq_id = IDGenerator::GetInst().GenerateSeqID();

    uint64_t expire_time = Clock::GetInst().CurrentMilliSec() + timeout;
    uint64_t timer_id = timeout_queue_.Add(
        [this, seq_id](uint64_t /*timer_id*/, uint32_t /*interval_time*/) {
            auto* tmp_context = Awake(seq_id, RPC_TIME_OUT);
            if (tmp_context)
                tmp_context->Run();
        },
        expire_time);

    if (timer_id == 0)
    {
        UA_LOG_ERROR(0, "add context timer error seq_id(%lu)", seq_id);
        return RPC_SYS_ERR;
    }

    client_ctx->timer_id = static_cast<uint32_t>(timer_id);

    auto [_, ok] = context_cache_.emplace(seq_id, client_ctx);
    if (!ok)
    {
        // 改进: insert 失败时取消刚添加的定时器，返回错误而非 SUCCESS
        timeout_queue_.Cancel(timer_id);
        UA_LOG_ERROR(0, "context_cache insert error, seq_id(%lu)", seq_id);
        return RPC_SYS_ERR;
    }

    UA_LOG_TRACE(0, "seq_id(%lu) pending, timer_id(%lu), expire_time(%lu)", seq_id, timer_id, expire_time);
    ServerStatistics::GetInst().statistics().save_max_coro_pending_num_max(
        static_cast<uint32_t>(PendingContextNum()));

    if (UseCoroutine())
    {
        auto* server_ctx = client_ctx->server_ctx;
        // 协程模式：有自定义 blocking 操作
        if (task.callback && task.blocking_fun)
        {
            client_ctx->SetCallback(
                [=, cb = task.callback](int32_t ret_code) { cb(ret_code, server_ctx); },
                task.recycle_fun);
            ContextMgr::SetCurrServerContext(nullptr);
            task.blocking_fun();
        }
        else
        {
            // 协程模式：Yield/Resume
            Coro* coro = CoroMgr::ThisCoro();
            assert(coro);
            client_ctx->SetCallback(
                [=, cb = task.callback](int32_t ret_code) {
                    if (cb)
                        cb(ret_code, server_ctx);
                },
                [coro]() { coro->Resume(); });
            ContextMgr::SetCurrServerContext(nullptr);
            coro->Yield();
        }
        ContextMgr::SetCurrServerContext(server_ctx);
    }
    else
    {
        // 异步模式
        auto* server_ctx = client_ctx->server_ctx;
        assert(server_ctx);
        client_ctx->SetCallback(
            [=, cb = task.callback](int32_t ret_code) {
                server_ctx->to_be_continue = false;
                ContextMgr::SetCurrServerContext(server_ctx);
                if (cb)
                    cb(ret_code, server_ctx);
                if (server_ctx->IsFinish())
                    server_ctx->Run();
            },
            task.recycle_fun);

        server_ctx->to_be_continue = true;
        ContextMgr::SetCurrServerContext(nullptr);
    }

    return RPC_SUCCESS;
}

bool ContextController::UseCoroutine() const noexcept
{
    return use_coroutine_;
}

size_t ContextController::PendingContextNum() const noexcept
{
    return context_cache_.size();
}

size_t ContextController::PendingCoroutineNum() const noexcept
{
    return UseCoroutine() ? CoroMgr::GetRunningCoro() : 0;
}

}  // namespace ua
