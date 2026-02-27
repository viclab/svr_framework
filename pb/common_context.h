/// @file common_context.h
/// @brief 通用的 PB 上下文（C++20 重写版）
#pragma once

#include <cstdint>
#include "core/context.h"

namespace ua
{

/// PB 上下文头部（从 Codec 中解析出来的公共字段）
/// 改进: 把字段集中在一个 head 结构中避免重复
struct PBContextHead
{
    uint64_t gid = 0;
    uint64_t seq_id = 0;
    uint32_t cmd = 0;
    uint32_t src = 0;
    uint32_t dst = 0;
    uint16_t pkg_flag = 0;
    uint64_t timeout = 0;
    int32_t ret_code = 0;
    uint32_t svr_version = 0;
};

/// PB 服务端上下文
struct PBContext : public ServerContext
{
    PBContextHead head{};
    uint32_t transport_index = 0;
    bool ignore = false;
    void* arena = nullptr;  // arena 分配器（用于 protobuf）

    ~PBContext() override = default;
};

/// PB 客户端上下文
struct PBClientContext : public ClientContext
{
    uint32_t cmd = 0;
    uint32_t transport_index = 0;
    ~PBClientContext() override = default;
};

}  // namespace ua
