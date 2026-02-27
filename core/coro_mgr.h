/// @file coro_mgr.h
/// @brief 协程全局管理器（C++20 重写版）
#pragma once

#include <cassert>
#include "interface/coroutine_interface.h"

namespace ua
{

class CoroMgr
{
public:
    static void SetCoroutine(ICoroutine* coroutine) noexcept { coroutine_ = coroutine; }

    [[nodiscard]] static ICoroutine& GetInst() noexcept
    {
        assert(coroutine_);
        return *coroutine_;
    }

    [[nodiscard]] static Coro* ThisCoro() noexcept
    {
        return coroutine_ ? coroutine_->ThisCoro() : nullptr;
    }

    [[nodiscard]] static size_t GetRunningCoro() noexcept
    {
        return coroutine_ ? coroutine_->GetRunningCoro() : 0;
    }

private:
    inline static ICoroutine* coroutine_ = nullptr;
};

}  // namespace ua
