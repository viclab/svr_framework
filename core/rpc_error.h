/// @file rpc_error.h
/// @brief RPC 错误码定义（C++20 重写版）
/// @note 使用 enum class 替代裸 const int32_t，提供类型安全
#pragma once

#include <cstdint>
#include <string_view>

namespace ua
{

/// RPC 错误码，强类型枚举
enum class RpcError : int32_t
{
    Success = 0,
    SystemError = -1,
    ChannelSendError = -2,
    Timeout = -3,
    SendMsgTooLong = -4,
    MsgSerializeError = -5,
    RecvMsgTooLong = -6,
    MsgParseError = -7,
    RouterFindDstError = -8,
};

/// 判断是否成功
[[nodiscard]] constexpr bool IsSuccess(RpcError err) noexcept
{
    return err == RpcError::Success;
}

/// 获取错误码的原始整数值（兼容旧接口）
[[nodiscard]] constexpr int32_t ToInt(RpcError err) noexcept
{
    return static_cast<int32_t>(err);
}

/// 从整数值构造错误码（兼容旧接口）
[[nodiscard]] constexpr RpcError FromInt(int32_t val) noexcept
{
    return static_cast<RpcError>(val);
}

/// 获取错误码的字符串描述
[[nodiscard]] constexpr std::string_view ErrorName(RpcError err) noexcept
{
    switch (err)
    {
        case RpcError::Success: return "Success";
        case RpcError::SystemError: return "SystemError";
        case RpcError::ChannelSendError: return "ChannelSendError";
        case RpcError::Timeout: return "Timeout";
        case RpcError::SendMsgTooLong: return "SendMsgTooLong";
        case RpcError::MsgSerializeError: return "MsgSerializeError";
        case RpcError::RecvMsgTooLong: return "RecvMsgTooLong";
        case RpcError::MsgParseError: return "MsgParseError";
        case RpcError::RouterFindDstError: return "RouterFindDstError";
        default: return "Unknown";
    }
}

// ========== 旧版兼容常量（推荐逐步迁移到 RpcError 枚举） ==========
inline constexpr int32_t RPC_SUCCESS = ToInt(RpcError::Success);
inline constexpr int32_t RPC_SYS_ERR = ToInt(RpcError::SystemError);
inline constexpr int32_t RPC_CHANNEL_SEND_ERR = ToInt(RpcError::ChannelSendError);
inline constexpr int32_t RPC_TIME_OUT = ToInt(RpcError::Timeout);
inline constexpr int32_t RPC_SEND_MSG_TOO_LONG = ToInt(RpcError::SendMsgTooLong);
inline constexpr int32_t RPC_MSG_SERIALIZE_ERR = ToInt(RpcError::MsgSerializeError);
inline constexpr int32_t RPC_RECV_MSG_TOO_LONG = ToInt(RpcError::RecvMsgTooLong);
inline constexpr int32_t RPC_MSG_PARSE_ERR = ToInt(RpcError::MsgParseError);
inline constexpr int32_t RPC_ROUTER_FIND_DST_ERR = ToInt(RpcError::RouterFindDstError);

}  // namespace ua
