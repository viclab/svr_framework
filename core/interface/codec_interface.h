/// @file codec_interface.h
/// @brief 统一协议编解码接口（C++20 重写版）
/// @note 四层继承体系: ReadCodec -> WriteCodec -> SendCodec
///                      ReadCodec -> RecvCodec
///       使用 C++20 特性: [[nodiscard]], concepts
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include "patterns/obj_factory.h"

namespace ua
{

/// 只读编解码接口 —— 被调侧使用
class ReadCodec
{
public:
    ReadCodec() = default;
    ReadCodec(const ReadCodec&) = delete;
    ReadCodec(ReadCodec&&) = delete;
    ReadCodec& operator=(const ReadCodec&) = delete;

    [[nodiscard]] virtual uint32_t GetCmd() const = 0;
    [[nodiscard]] virtual uint32_t GetSvrType() const = 0;
    [[nodiscard]] virtual uint64_t GetGid() const = 0;
    [[nodiscard]] virtual uint64_t GetSeqID() const = 0;
    [[nodiscard]] virtual uint32_t GetSrc() const = 0;
    [[nodiscard]] virtual uint32_t GetDst() const = 0;
    [[nodiscard]] virtual uint64_t GetTimeout() const { return 0; }
    [[nodiscard]] virtual int32_t GetRetCode() const { return 0; }
    [[nodiscard]] virtual uint32_t GetVersion() const { return 0; }
    [[nodiscard]] virtual uint32_t GetFlag() const { return 0; }
    [[nodiscard]] virtual uint32_t GetBodyLen() const = 0;
    [[nodiscard]] virtual const char* GetBody() const = 0;
    [[nodiscard]] virtual uint32_t GetExtHeadLen() const { return 0; }
    [[nodiscard]] virtual const char* GetExtHead() const { return nullptr; }
    [[nodiscard]] virtual const char* GetExtHead(uint32_t type, uint32_t& len) const { return nullptr; }
    [[nodiscard]] virtual const char* GetRawData(uint32_t& data_len) const = 0;

    virtual void Reset() = 0;
    virtual ~ReadCodec() = default;
};

/// 读写编解码接口 —— 发包侧使用
class WriteCodec : public ReadCodec
{
public:
    virtual void SetCmd(uint32_t cmd) = 0;
    virtual void SetSvrType(uint32_t svr_type) = 0;
    virtual void SetGid(uint64_t gid) = 0;
    virtual void SetSeqID(uint64_t seq_id) = 0;
    virtual void SetSrc(uint32_t id) = 0;
    virtual void SetDst(uint32_t id) = 0;
    virtual void SetTimeout(uint64_t ms_time) {}
    virtual void SetRetCode(int32_t ret_code) {}
    virtual void SetVersion(uint32_t version) {}
    virtual void SetFlag(uint32_t flag) {}
    virtual void SetBodyLen(uint32_t len) = 0;
    [[nodiscard]] virtual char* GetBodyBuf(uint32_t& max_len) = 0;
    virtual bool SetBody(const char* data, uint32_t len) = 0;
    virtual bool SetExtHead(const char* data, uint32_t len) { return false; }
    virtual bool AddExtHead(uint32_t type, const char* data, uint32_t len) { return false; }
    virtual ~WriteCodec() = default;
};

/// 收包反序列化接口
class RecvCodec : public ReadCodec
{
public:
    virtual bool Decode(const char* data, uint32_t data_len) = 0;
    [[nodiscard]] virtual bool HasDecoded() const = 0;
    virtual void Reset() = 0;
};

/// 发包序列化接口
class SendCodec : public WriteCodec
{
public:
    virtual bool Encode(uint32_t* data_len = nullptr) = 0;
    [[nodiscard]] virtual bool HasEncoded() const = 0;
    virtual void Reset() = 0;
};

/// Codec 工厂注册
using RecvCodecFactory = TMapSingletonFactory<RecvCodec*, std::function<RecvCodec*()>>;
using SendCodecFactory = TMapSingletonFactory<SendCodec*, std::function<SendCodec*()>>;

#define REGIST_CODEC(type, codec, recv_or_send) \
    recv_or_send##CodecFactory::GetInst().Register(type, []() { return new codec; });

#define REGIST_RECV_CODEC(type, codec) REGIST_CODEC(type, codec, Recv)
#define REGIST_SEND_CODEC(type, codec) REGIST_CODEC(type, codec, Send)

}  // namespace ua
