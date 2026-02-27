/// @file transport.h
/// @brief Transport 组合体（C++20 重写版）
#pragma once

#include <cstdint>

namespace ua
{

class IChannel;
class ReadCodec;
class RecvCodec;
class SendCodec;
class IRouting;

/// Transport = Channel + RecvCodec + SendCodec + Routing 四个策略的组合
struct TransportInfo
{
    /// 单播发送
    int32_t Send() const;
    int32_t Send(uint32_t dst) const;
    int32_t Send(const ReadCodec& codec, uint32_t dst) const;
    /// 广播发送
    int32_t Broadcast() const;
    int32_t Broadcast(uint32_t dst) const;
    int32_t Broadcast(const ReadCodec& codec, uint32_t dst) const;

    IChannel* channel = nullptr;
    RecvCodec* recv_codec = nullptr;
    SendCodec* send_codec = nullptr;
    IRouting* routing = nullptr;
};

}  // namespace ua
