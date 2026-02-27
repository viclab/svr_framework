/// @file pb_context.h
/// @brief PB 上下文与 Arena 适配（C++20 重写版）
/// @note 通过 if constexpr 或条件编译支持 Arena 模式
///       MaybeArenaMsg 使用 C++20 concepts 做约束
#pragma once

#include "common_context.h"
#include "core/context_mgr.h"
#include "google/protobuf/message.h"

#ifdef PB_ARENA
#include "google/protobuf/arena.h"
#endif

namespace ua
{

#ifdef PB_ARENA

struct PBContextFull : public PBContext
{
    PBContextFull(uint32_t transport_type, const ReadCodec& codec,
                  const google::protobuf::Message& req_proto_type,
                  const google::protobuf::Message& rsp_proto_type,
                  const google::protobuf::ArenaOptions& arena_options);

    [[nodiscard]] google::protobuf::Message* GetReq() { return req; }
    [[nodiscard]] google::protobuf::Message* GetRsp() { return rsp; }
    [[nodiscard]] google::protobuf::Arena* GetArena() const { return arena.get(); }

protected:
    std::unique_ptr<google::protobuf::Arena> arena;
    google::protobuf::Message* req = nullptr;
    google::protobuf::Message* rsp = nullptr;
};

/// Arena 感知消息包装器
/// 如果当前上下文有 Arena，则在 Arena 上分配；否则用 new
template <typename T>
struct MaybeArenaMsg
{
    explicit MaybeArenaMsg(const PBContextFull* ctx)
    {
        const auto* svr_ctx = ctx ? ctx : dynamic_cast<PBContextFull*>(ContextMgr::GetCurrServerContext());
        if (svr_ctx)
        {
            msg_ = google::protobuf::Arena::CreateMessage<T>(svr_ctx->GetArena());
        }
        else
        {
            msg_ = new T;
            use_new_ = true;
        }
    }

    MaybeArenaMsg() = delete;
    MaybeArenaMsg(const MaybeArenaMsg&) = delete;
    MaybeArenaMsg& operator=(const MaybeArenaMsg&) = delete;

    MaybeArenaMsg(MaybeArenaMsg&& other) noexcept : msg_(other.msg_), use_new_(other.use_new_)
    {
        other.msg_ = nullptr;
        other.use_new_ = false;
    }

    ~MaybeArenaMsg() noexcept
    {
        if (use_new_)
            delete msg_;
    }

    operator T&() { return *msg_; }
    operator const T&() const { return *msg_; }
    T* operator&() { return msg_; }
    const T* operator&() const { return msg_; }

private:
    T* msg_ = nullptr;
    bool use_new_ = false;
};

#else

struct PBContextFull : public PBContext
{
    PBContextFull(uint32_t transport_type, const ReadCodec& codec,
                  const google::protobuf::Message& req_proto_type,
                  const google::protobuf::Message& rsp_proto_type);

    [[nodiscard]] google::protobuf::Message* GetReq() { return req.get(); }
    [[nodiscard]] google::protobuf::Message* GetRsp() { return rsp.get(); }

protected:
    std::unique_ptr<google::protobuf::Message> req;
    std::unique_ptr<google::protobuf::Message> rsp;
};

/// 非 Arena 模式：直接包装栈对象
template <typename T>
struct MaybeArenaMsg
{
    explicit MaybeArenaMsg(const PBContextFull* = nullptr) {}
    MaybeArenaMsg() = delete;
    MaybeArenaMsg(const MaybeArenaMsg&) = delete;
    MaybeArenaMsg& operator=(const MaybeArenaMsg&) = delete;
    MaybeArenaMsg(MaybeArenaMsg&&) = default;

    operator T&() { return msg_; }
    operator const T&() const { return msg_; }
    T* operator&() { return &msg_; }
    const T* operator&() const { return &msg_; }

private:
    T msg_;
};

#endif

}  // namespace ua
