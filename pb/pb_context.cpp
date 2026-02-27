/// @file pb_context.cpp
/// @brief PB 上下文实现（C++20 重写版）
#include "pb_context.h"
#include "core/interface/codec_interface.h"

namespace ua
{

#ifdef PB_ARENA

PBContextFull::PBContextFull(uint32_t transport_type, const ReadCodec& codec,
                             const google::protobuf::Message& req_proto_type,
                             const google::protobuf::Message& rsp_proto_type,
                             const google::protobuf::ArenaOptions& arena_options)
{
    arena = std::make_unique<google::protobuf::Arena>(arena_options);
    req = req_proto_type.New(arena.get());
    rsp = rsp_proto_type.New(arena.get());

    this->transport_index = transport_type;
    this->head.gid = codec.GetGid();
    this->head.seq_id = codec.GetSeqID();
    this->head.cmd = codec.GetCmd();
    this->head.src = codec.GetSrc();
    this->head.dst = codec.GetDst();
    this->head.pkg_flag = static_cast<uint16_t>(codec.GetFlag());
    this->head.timeout = codec.GetTimeout();
    this->gid = codec.GetGid();
    this->pkg_flag = static_cast<uint16_t>(codec.GetFlag());
}

#else

PBContextFull::PBContextFull(uint32_t transport_type, const ReadCodec& codec,
                             const google::protobuf::Message& req_proto_type,
                             const google::protobuf::Message& rsp_proto_type)
{
    req.reset(req_proto_type.New());
    rsp.reset(rsp_proto_type.New());

    this->transport_index = transport_type;
    this->head.gid = codec.GetGid();
    this->head.seq_id = codec.GetSeqID();
    this->head.cmd = codec.GetCmd();
    this->head.src = codec.GetSrc();
    this->head.dst = codec.GetDst();
    this->head.pkg_flag = static_cast<uint16_t>(codec.GetFlag());
    this->head.timeout = codec.GetTimeout();
    this->gid = codec.GetGid();
    this->pkg_flag = static_cast<uint16_t>(codec.GetFlag());
}

#endif

}  // namespace ua
