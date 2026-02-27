/// @file pkg_flag_type.h
/// @brief 包标志位定义（C++20 重写版）
#pragma once

#include <cstdint>

namespace ua
{

// 使用 inline constexpr 替代裸 constexpr
inline constexpr uint16_t FLAG_RSP_PKG = 0x0001;         // 回包标记
inline constexpr uint16_t FLAG_DONT_RSP = 0x0002;        // 不需要回包
inline constexpr uint16_t FLAG_FROM_TCONND = 0x0004;     // 来自接入层（客户端）
inline constexpr uint16_t FLAG_IS_BROADCAST = 0x0008;    // 广播包
inline constexpr uint16_t FLAG_CLIENT_KEY_PKG = 0x0010;  // 客户端关键包
inline constexpr uint16_t FLAG_SVR_KEY_PKG = 0x0020;     // 服务器关键包
inline constexpr uint16_t FLAG_IS_FORWARD_PKG = 0x0040;  // 同服务转发包

}  // namespace ua
