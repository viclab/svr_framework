/// @file transport.cpp
/// @brief Transport 实现（C++20 重写版）
#include "transport.h"
#include <cassert>
#include "interface/channel_interface.h"
#include "interface/codec_interface.h"
#include "interface/routing_interface.h"
#include "rpc_error.h"

namespace ua
{

int32_t TransportInfo::Send() const
{
    return Send(*send_codec, send_codec->GetDst());
}

int32_t TransportInfo::Send(uint32_t dst) const
{
    return Send(*send_codec, dst);
}

int32_t TransportInfo::Send(const ReadCodec& codec, uint32_t dst) const
{
    assert(channel);
    if (routing)
    {
        dst = routing->GetSendDest(codec.GetSvrType(), codec.GetGid(), dst, codec.GetVersion());
        if (dst == 0)
            return RPC_ROUTER_FIND_DST_ERR;
    }

    uint32_t send_len = 0;
    const char* send_data = codec.GetRawData(send_len);
    if (!send_data)
        return RPC_SYS_ERR;

    int32_t ret = channel->Send(dst, send_data, send_len);
    return ret != 0 ? RPC_CHANNEL_SEND_ERR : RPC_SUCCESS;
}

int32_t TransportInfo::Broadcast() const
{
    return Broadcast(*send_codec, send_codec->GetDst());
}

int32_t TransportInfo::Broadcast(uint32_t dst) const
{
    return Broadcast(*send_codec, dst);
}

int32_t TransportInfo::Broadcast(const ReadCodec& codec, uint32_t dst) const
{
    assert(channel);
    if (!routing)
        return RPC_ROUTER_FIND_DST_ERR;

    const auto& all_inst = routing->GetAllSendDest(codec.GetSvrType(), dst, 0);
    if (all_inst.empty())
        return RPC_ROUTER_FIND_DST_ERR;

    uint32_t send_len = 0;
    const char* send_data = codec.GetRawData(send_len);
    if (!send_data)
        return RPC_SYS_ERR;

    for (uint32_t dest_id : all_inst)
        channel->Send(dest_id, send_data, send_len);

    return RPC_SUCCESS;
}

}  // namespace ua
