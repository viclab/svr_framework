/// @file context_mgr.cpp
/// @brief ContextMgr 实现（C++20 重写版）
#include "context_mgr.h"
#include "context.h"

namespace ua
{

uint64_t ContextMgr::GetContextId() noexcept
{
    auto* ctx = GetCurrServerContext();
    return ctx ? ctx->gid : 0;
}

}  // namespace ua
