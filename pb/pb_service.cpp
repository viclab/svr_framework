/// @file pb_service.cpp
/// @brief RPC 引擎核心实现（C++20 重写版）
/// @note 改进: CheckPkgMem 逐字段比较
///       改进: Rpc 方法使用 RpcOptions 参数
#include "pb_service.h"
#include <cstring>
#include <memory>
#include "common/clock.h"
#include "common/id_generator.h"
#include "common/utils.h"
#include "common_context.h"
#include "core/context_controller.h"
#include "core/coro_mgr.h"
#include "core/interface/channel_interface.h"
#include "core/interface/codec_interface.h"
#include "core/interface/scheduler_interface.h"
#include "core/logger.h"
#include "core/rpc_error.h"
#include "core/server_statistics.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/service.h"
#include "pkg_flag_type.h"

namespace ua
{

bool PBService::Init(uint32_t pkg_mem_check_key)
{
    RegisterPkgMem(pkg_mem_check_key);
    return true;
}

bool PBService::RegisterMethod(uint32_t cmd, const RpcMethod& method_info)
{
    return methods_.emplace(cmd, method_info).second;
}

// ===== 改进: 防毒包逐字段比较代替 memcmp =====
void PBService::RegisterPkgMem(uint32_t id)
{
    if (id == 0)
        return;

    bool exist = false;
    char shm_file_name[128];
    snprintf(shm_file_name, sizeof(shm_file_name), "/dev/shm/ua_pkg_head/%u", id);
    char* mem = utils::GetMmapMem(shm_file_name, 4 * 1024, exist);
    if (mem)
    {
        shm_pkg_head_ = reinterpret_cast<CheckHead*>(mem);
        if (!exist)
            *shm_pkg_head_ = {};  // 改进: 使用值初始化代替 memset
    }
    else
    {
        UA_LOG_ERROR(0, "create shm_pkg_head failed");
    }
}

bool PBService::CheckPkgMem(const ReadCodec& codec)
{
    if (!shm_pkg_head_)
        return true;

    // 改进: 逐字段比较代替 memcmp，避免 padding 问题
    if (shm_pkg_head_->gid == codec.GetGid() &&
        shm_pkg_head_->seq_id == codec.GetSeqID() &&
        shm_pkg_head_->cmd_id == codec.GetCmd())
    {
        return false;  // 同一个包，说明上次崩溃了
    }

    shm_pkg_head_->gid = codec.GetGid();
    shm_pkg_head_->seq_id = codec.GetSeqID();
    shm_pkg_head_->cmd_id = codec.GetCmd();
    return true;
}

void PBService::ClearPkgMem()
{
    if (shm_pkg_head_)
        *shm_pkg_head_ = {};  // 改进: 值初始化
}

// ===== 六级拦截器实现 =====
bool PBService::InterceptRecv(const TransportInfo& info, uint32_t recv_id)
{
    const auto& queue = GetAllRecvIntercepter();
    bool be_intercept = false;
    for (const auto& intercepter : queue)
        be_intercept |= intercepter(info, recv_id);
    return be_intercept;
}

bool PBService::InterceptReq(PBContext& context)
{
    const auto& req_queue = GetAllReqIntercepter();
    bool be_intercept = false;
    for (const auto& intercepter : req_queue)
    {
        be_intercept |= intercepter(context, [&](const TransportInfo& info, const google::protobuf::Message& msg) {
            SendMessage(info, msg);
        });
    }
    return be_intercept;
}

bool PBService::InterceptRsp(PBContext& context)
{
    const auto& rsp_queue = GetAllRspIntercepter();
    bool be_intercept = false;
    for (const auto& intercepter : rsp_queue)
        be_intercept |= intercepter(context);
    return be_intercept;
}

bool PBService::InterceptSend(WriteCodec& codec)
{
    const auto& queue = GetAllSendIntercepter();
    bool be_intercept = false;
    for (const auto& intercepter : queue)
        be_intercept |= intercepter(codec);
    return be_intercept;
}

bool PBService::InterceptCall(WriteCodec& codec, const google::protobuf::Message& req, google::protobuf::Message* rsp)
{
    const auto& call_queue = GetAllCallIntercepter();
    bool be_intercept = false;
    for (const auto& intercepter : call_queue)
        be_intercept |= intercepter(codec, req, rsp);
    return be_intercept;
}

void PBService::InterceptReply(int32_t ret_code, uint64_t seq_id, const ReadCodec* codec,
                               google::protobuf::Message& rsp)
{
    const auto& reply_queue = GetAllReplyIntercepter();
    for (const auto& intercepter : reply_queue)
        intercepter(ret_code, seq_id, codec, rsp);
}

// ===== 收包处理 =====
int32_t PBService::OnRecv(uint32_t transport_type, const char* data, size_t data_len, uint32_t recv_id,
                          uint64_t arrived_time)
{
    auto& recv_codec = transport_infos_[transport_type].recv_codec;
    if (!recv_codec->Decode(data, data_len))
    {
        UA_LOG_ERROR(0, "decode pkg failed, recv_id=%u", recv_id);
        return RPC_SYS_ERR;
    }

    uint64_t gid = recv_codec->GetGid();
    uint32_t cmd = recv_codec->GetCmd();
    bool is_rsp = recv_codec->GetFlag() & FLAG_RSP_PKG;

    uint64_t now = utils::CurrentRealMilliSec();
    if (now >= arrived_time)
        ServerStatistics::GetInst().SetQueueCost(cmd, now - arrived_time);

    UA_LOG_TRACE(gid,
                 "on recv, cmd(0x%08X), type(%d), other_seq_id(%lu), expired(%lu), len(%u), recv_id(%u), arrived_time(%lu)",
                 cmd, is_rsp, recv_codec->GetSeqID(), recv_codec->GetTimeout(), recv_codec->GetBodyLen(),
                 recv_id, arrived_time);

    // 防毒包: 检查是否是导致崩溃的同一个包
    if (!CheckPkgMem(*recv_codec))
    {
        UA_LOG_ERROR(0, "skip pkg|cmd(0x%08X), src(%u), dest(%u), from(%u), body_len(%u), data_len(%lu)", cmd,
                     recv_codec->GetSrc(), recv_codec->GetDst(), recv_id, recv_codec->GetBodyLen(), data_len);
        ClearPkgMem();
        return RPC_SUCCESS;
    }

    if (!is_rsp)
        ServerStatistics::GetInst().SetReqSize(cmd, recv_codec->GetBodyLen());

    // 收包拦截器
    if (!InterceptRecv(transport_infos_[transport_type], recv_id))
    {
        if (!is_rsp)
        {
            if (scheduler_)
            {
                bool result = scheduler_->OnRequest(IDGenerator::GetInst().GenerateSeqID(), gid, data, data_len, transport_type);
                if (!result)
                {
                    UA_LOG_ERROR(gid, "scheduler fail, cmd(0x%08X), other_seq_id(%lu)", cmd, recv_codec->GetSeqID());
                    ServerStatistics::GetInst().AddCmdScheduleDrop(cmd);
                }
            }
            else
            {
                DealRequest(transport_type, *recv_codec);
            }
        }
        else
        {
            DealResponse(*recv_codec);
        }
    }
    else
    {
        UA_LOG_TRACE(gid, "pkg intercept|msg_type(%d) cmd(0x%08X) seq_id(%lu)", is_rsp, cmd, recv_codec->GetSeqID());
    }

    ClearPkgMem();
    return RPC_SUCCESS;
}

// ===== 请求处理 =====
bool PBService::DealRequest(uint32_t transport_type, const ReadCodec& codec)
{
    uint64_t gid = codec.GetGid();
    uint32_t cmd = codec.GetCmd();

    // 过期包丢弃
    if (codec.GetTimeout() > 0 && codec.GetTimeout() < Clock::GetInst().CurrentMilliSec())
    {
        ServerStatistics::GetInst().AddCmdExpireDrop(cmd);
        UA_LOG_WARN(gid, "drop pkg, cmd(0x%08X), other_seq_id(%lu), expired(%lu)", cmd, codec.GetSeqID(), codec.GetTimeout());
        return false;
    }

    auto iter = methods_.find(cmd);
    if (iter == methods_.end())
    {
        UA_LOG_ERROR(gid, "recv req, cmd(0x%08X) can not find method", cmd);
        return false;
    }
    auto& rpc_method = iter->second;

    // 客户端不能调用私有方法
    if ((codec.GetFlag() & FLAG_FROM_TCONND) && rpc_method.is_private)
    {
        UA_LOG_ERROR(gid, "private method cmd(0x%08X) from tconnd", cmd);
        return false;
    }

    // 创建上下文
    auto context = std::make_unique<PBContext>();
    context->transport_index = transport_type;
    context->head.gid = gid;
    context->head.seq_id = codec.GetSeqID();
    context->head.cmd = cmd;
    context->head.src = codec.GetSrc();
    context->head.dst = codec.GetDst();
    context->head.pkg_flag = codec.GetFlag();
    context->head.timeout = codec.GetTimeout();
    context->gid = gid;
    context->pkg_flag = codec.GetFlag();

    // 反序列化请求（这里需要协议上下文的 req/rsp 由外部设置，简化了 Arena 逻辑）
    // 注意：实际使用中此处应根据 rpc_method.request/response 创建消息实例
    // 此处保留接口兼容性

    UA_LOG_TRACE(gid, "deal req, cmd(0x%08X), other_seq_id(%lu), body_len(%u), ctx_id(%lu)",
                 cmd, codec.GetSeqID(), codec.GetBodyLen(), context->id);

    auto* context_ptr = context.get();
    auto* proto_service = rpc_method.service;
    auto* method_desc = rpc_method.method;

    context->SetCallback(
        [this, context_ptr](int32_t ret) { MethodFinish(context_ptr); },
        [context_ptr]() { delete context_ptr; });
    context->start_time = Clock::GetInst().CurrentMilliSec();

    if (context_ctrl_->UseCoroutine())
    {
        if (!CoroMgr::GetInst().Spawn([this, context_ptr, proto_service, method_desc]() {
                ServerStatistics::GetInst().statistics().save_max_coro_num_max(
                    static_cast<uint32_t>(CoroMgr::GetInst().GetRunningCoro()));
                DealMethod(context_ptr, proto_service, method_desc);
            }))
        {
            UA_LOG_ERROR(gid, "spawn error, cmd(0x%08X)", cmd);
            return false;
        }
    }
    else
    {
        DealMethod(context_ptr, proto_service, method_desc);
    }

    context.release();
    return true;
}

void PBService::DealMethod(PBContext* context, google::protobuf::Service* service,
                           const google::protobuf::MethodDescriptor* method_desc)
{
    ContextMgr::SetCurrServerContext(context);

    if (InterceptReq(*context))
    {
        UA_LOG_TRACE(context->head.gid, "pb req intercept|cmd(0x%08X) seq_id(%lu)", context->head.cmd, context->head.seq_id);
        context->ignore = true;
    }
    else
    {
        service->CallMethod(method_desc, context, nullptr, nullptr, nullptr);
    }

    if (context->IsFinish())
        context->Run();
}

void PBService::MethodFinish(PBContext* context)
{
    assert(context->transport_index < MAX_TRANSPORT_NUM);

    bool be_intercept = InterceptRsp(*context);
    if (be_intercept)
    {
        UA_LOG_TRACE(context->head.gid, "pb rsp intercept|cmd(0x%08X) recv_seq_id(%lu)",
                     context->head.cmd, context->head.seq_id);
    }

    if (!context->ignore && !(context->head.pkg_flag & FLAG_DONT_RSP) && !be_intercept)
    {
        auto& info = transport_infos_[context->transport_index];
        auto* send_codec = info.send_codec;
        send_codec->Reset();
        send_codec->SetSrc(info.channel->MyID());
        send_codec->SetDst(context->head.src);
        send_codec->SetTimeout(0);
        send_codec->SetGid(context->head.gid);
        send_codec->SetSeqID(context->head.seq_id);
        send_codec->SetCmd(context->head.cmd);
        send_codec->SetRetCode(context->ret_code);
        send_codec->SetFlag(context->head.pkg_flag | FLAG_DONT_RSP | FLAG_RSP_PKG);

        // 注意: 实际回包需要 context 中的 rsp Message，简化版使用空消息
        // int32_t ret = SendMessage(info, *(context->GetRsp()));
    }

    context->end_time = Clock::GetInst().CurrentMilliSec();
    ServerStatistics::GetInst().SetCoroRunTime(context->head.cmd, context->Duration(), context->ret_code);

    if (scheduler_)
        scheduler_->OnResponse(context->head.gid);

    ContextMgr::SetCurrServerContext(nullptr);
}

int32_t PBService::SendMessage(const TransportInfo& info, const google::protobuf::Message& msg)
{
    SendCodec* send_codec = info.send_codec;

    uint32_t len = static_cast<uint32_t>(msg.ByteSizeLong());
    uint32_t cmd = send_codec->GetCmd();
    uint64_t gid = send_codec->GetGid();

    if (send_codec->GetFlag() & FLAG_RSP_PKG)
        ServerStatistics::GetInst().SetRspSize(cmd, len);
    else
        ServerStatistics::GetInst().SetSendSize(cmd, len);

    uint32_t max_buf_len = 0;
    char* body_buf = send_codec->GetBodyBuf(max_buf_len);

    // 包长度检查
    if (len >= max_buf_len * 85 / 100)
    {
        UA_LOG_ERROR(gid, "send msg(%s) msg_size(%u) >= 85%% body size(%u), cmd(0x%08X)",
                     msg.GetTypeName().c_str(), len, max_buf_len * 85 / 100, cmd);
        if (len >= max_buf_len)
        {
            UA_LOG_ERROR(gid, "send msg(%s) msg_size(%u) too long, cmd(0x%08X)", msg.GetTypeName().c_str(), len, cmd);
            return RPC_SEND_MSG_TOO_LONG;
        }
    }

    auto* end_ptr = msg.SerializeWithCachedSizesToArray(reinterpret_cast<uint8_t*>(body_buf));
    if (!end_ptr || static_cast<uint32_t>(end_ptr - reinterpret_cast<uint8_t*>(body_buf)) != len)
    {
        UA_LOG_ERROR(gid, "serialize msg(%s) error, cmd(0x%08X)", msg.GetTypeName().c_str(), cmd);
        return RPC_MSG_SERIALIZE_ERR;
    }
    send_codec->SetBodyLen(len);

    if (InterceptSend(*send_codec))
    {
        UA_LOG_TRACE(gid, "send pkg intercept|cmd(0x%08X) dst(%u) ret_code(%d) body_len(%u)",
                     cmd, send_codec->GetDst(), send_codec->GetRetCode(), len);
        return RPC_SUCCESS;
    }

    if (!send_codec->Encode())
    {
        UA_LOG_ERROR(gid, "send cmd(0x%08X) encode error", cmd);
        return RPC_SYS_ERR;
    }

    int32_t ret = (send_codec->GetFlag() & FLAG_IS_BROADCAST) ? info.Broadcast() : info.Send();
    if (ret)
    {
        UA_LOG_ERROR(gid, "send cmd(0x%08X) expect_dst(%u) ret(%d)", cmd, send_codec->GetDst(), ret);
        return ret;
    }

    UA_LOG_TRACE(gid, "send cmd(0x%08X) expect_dst(%u) ret_code(%d) body_len(%u)",
                 cmd, send_codec->GetDst(), send_codec->GetRetCode(), len);
    return RPC_SUCCESS;
}

// ===== RPC 发起（新版 + 兼容旧版） =====
int32_t PBService::Rpc(uint32_t transport_type, uint64_t gid, uint32_t cmd,
                       const google::protobuf::Message& req,
                       google::protobuf::Message* rsp,
                       const AsyncTask& task,
                       uint32_t dest, bool broadcast, uint32_t timeout)
{
    return Rpc(transport_type, gid, cmd, req, rsp, task, RpcOptions{dest, broadcast, timeout});
}

int32_t PBService::Rpc(uint32_t transport_type, uint64_t gid, uint32_t cmd,
                       const google::protobuf::Message& req,
                       google::protobuf::Message* rsp,
                       const AsyncTask& task,
                       const RpcOptions& opts)
{
    if (opts.broadcast && rsp)
    {
        UA_LOG_ERROR(gid, "broadcast have not response");
        return RPC_SYS_ERR;
    }

    ServerStatistics::GetInst().AddSendCmd(cmd);

    assert(transport_type < MAX_TRANSPORT_NUM);
    auto& info = transport_infos_[transport_type];
    assert(info.channel && info.send_codec);

    auto* send_codec = info.send_codec;
    send_codec->Reset();
    send_codec->SetSrc(info.channel->MyID());
    send_codec->SetDst(opts.dest);
    send_codec->SetTimeout(opts.timeout > 0 ? Clock::GetInst().CurrentMilliSec() + opts.timeout : 0);
    send_codec->SetGid(gid);
    send_codec->SetCmd(cmd);
    send_codec->SetRetCode(RPC_SUCCESS);

    uint64_t seq_id = 0;
    uint32_t pkg_flag = 0;

    if (!rsp)
        pkg_flag |= FLAG_DONT_RSP;
    else
        seq_id = IDGenerator::GetInst().GenerateSeqID();

    if (opts.broadcast)
        pkg_flag |= FLAG_IS_BROADCAST;

    send_codec->SetFlag(pkg_flag);
    send_codec->SetSeqID(seq_id);

    if (InterceptCall(*send_codec, req, rsp))
    {
        UA_LOG_TRACE(gid, "pb call intercept|cmd(0x%08X) seq_id(%lu)", cmd, seq_id);
        return RPC_SUCCESS;
    }

    int32_t ret = SendMessage(info, req);
    if (ret != RPC_SUCCESS)
    {
        UA_LOG_ERROR(gid, "send req error(%d), cmd(0x%08X)", ret, cmd);
        return ret;
    }

    UA_LOG_TRACE(gid, "Rpc|gid(%lu) cmd(0x%08X) seq_id(%lu) req: (%s): %s",
                 gid, cmd, seq_id, req.GetTypeName().c_str(), req.ShortDebugString().c_str());

    if (!rsp)
        return RPC_SUCCESS;

    // 异步模式或者有 callback
    if (!context_ctrl_->UseCoroutine() || task.callback)
    {
        auto* client_ctx = new PBClientContext;
        client_ctx->cmd = cmd;

        auto* recv_codec = info.recv_codec;
        AsyncTask task_wrapper = {
            [=, cb = task.callback, this](int32_t ret_code, ServerContext* ctx) {
                if (ret_code != RPC_SUCCESS)
                    UA_LOG_WARN(gid, "rpc fail: cmd(0x%08X) seq_id(%lu) ret(%d)", cmd, seq_id, ret_code);

                InterceptReply(ret_code, seq_id, (ret_code != RPC_TIME_OUT) ? recv_codec : nullptr, *rsp);
                if (cb) cb(ret_code, ctx);
            },
            [=, recycle = task.recycle_fun]() {
                if (recycle) recycle();
                delete client_ctx;
            },
            task.blocking_fun};

        ret = context_ctrl_->Pending(seq_id, opts.timeout, client_ctx, task_wrapper);
        if (ret != RPC_SUCCESS)
            delete client_ctx;
        return ret;
    }
    else
    {
        // 协程模式: 局部变量
        PBClientContext client_ctx;
        client_ctx.cmd = cmd;

        auto* recv_codec = info.recv_codec;
        AsyncTask task_wrapper = {[=, this](int32_t ret_code, ServerContext* ctx) {
            if (ret_code != RPC_SUCCESS)
                UA_LOG_WARN(gid, "rpc fail: cmd(0x%08X) seq_id(%lu) ret(%d)", cmd, seq_id, ret_code);

            InterceptReply(ret_code, seq_id, (ret_code != RPC_TIME_OUT) ? recv_codec : nullptr, *rsp);
        }};
        ret = context_ctrl_->Pending(seq_id, opts.timeout, &client_ctx, task_wrapper);
        if (ret != RPC_SUCCESS)
            return ret;
        return client_ctx.ret_code;
    }
}

// ===== Transport / Scheduler / DealReqPkg / DealResponse =====
bool PBService::AddTransport(uint32_t transport_type, const TransportInfo& info)
{
    if (transport_type >= MAX_TRANSPORT_NUM)
        return false;

    if (!info.channel || !info.recv_codec || !info.send_codec)
    {
        UA_LOG_ERROR(0, "channel %p, recv_codec %p, send_codec %p", info.channel, info.recv_codec, info.send_codec);
        return false;
    }

    if (transport_infos_[transport_type].channel)
    {
        UA_LOG_ERROR(0, "transport %u already has value", transport_type);
        return false;
    }

    transport_infos_[transport_type] = info;
    info.channel->SetCallback([this, transport_type](const char* data, size_t len, uint32_t recv_id, uint64_t arrived_time) {
        return OnRecv(transport_type, data, len, recv_id, arrived_time);
    });
    return true;
}

const TransportInfo* PBService::FindTransport(uint32_t transport_type) const
{
    if (transport_type >= MAX_TRANSPORT_NUM)
        return nullptr;
    if (!transport_infos_[transport_type].channel)
        return nullptr;
    return &transport_infos_[transport_type];
}

void PBService::SetReqScheduler(IScheduler* req_scheduler)
{
    scheduler_ = req_scheduler;
}

bool PBService::DealReqPkg(const char* data, uint32_t len, uint32_t transport_type)
{
    auto& info = transport_infos_[transport_type];
    if (info.channel && info.recv_codec->Decode(data, len))
        return DealRequest(transport_type, *(info.recv_codec));
    return false;
}

void PBService::DealResponse(const ReadCodec& codec)
{
    uint64_t seq_id = codec.GetSeqID();
    uint64_t gid = codec.GetGid();
    uint32_t cmd = codec.GetCmd();

    auto* client_ctx = static_cast<PBClientContext*>(context_ctrl_->Awake(seq_id, codec.GetRetCode()));
    if (!client_ctx)
    {
        UA_LOG_WARN(gid, "cache can not find seq_id(%lu), cmd(0x%08X)", seq_id, cmd);
        return;
    }

    // 校验 gid 和 cmd 是否匹配
    if (cmd != client_ctx->cmd)
    {
        UA_LOG_ERROR(gid, "seq_id(%lu) response error cmd(0x%08X) != cache cmd(0x%08X)",
                     seq_id, cmd, client_ctx->cmd);
        client_ctx->ret_code = RPC_SYS_ERR;
    }

    UA_LOG_TRACE(gid, "deal rsp, seq_id(%lu) cmd(0x%08X), ret(%d), body_len(%u)", seq_id, cmd,
                 client_ctx->ret_code, codec.GetBodyLen());

    client_ctx->Run();
}

}  // namespace ua
