/// @file pb_service.h
/// @brief RPC 引擎核心（C++20 重写版）
/// @note 改进: 不再使用 Singleton 继承，方便测试
///       改进: Rpc 方法参数改为结构体 Options，避免 9 参数函数
///       改进: CheckPkgMem 使用逐字段比较代替 memcmp
#pragma once

#include <array>
#include <functional>
#include <unordered_map>
#include <vector>
#include "core/transport.h"
#include "intercepter.h"
#include "patterns/singleton.h"
#include "rpc_methods_info.h"

namespace google::protobuf
{
class Service;
class Message;
class MethodDescriptor;
}  // namespace google::protobuf

namespace ua
{

struct PBContext;
struct AsyncTask;
class IScheduler;
class ReadCodec;
class WriteCodec;
class ContextController;

// ========== 拦截器类型定义 ==========
using PBRecvIntercepter = TRecvIntercepter<std::function<bool(const TransportInfo&, uint32_t recv_id)>>;
using PBSendIntercepter = TSendIntercepter<std::function<bool(WriteCodec&)>>;
using PBSender = std::function<void(const TransportInfo&, const google::protobuf::Message&)>;
using PBReqIntercepter = TReqIntercepter<std::function<bool(PBContext&, const PBSender&)>>;
using PBRspIntercepter = TRspIntercepter<std::function<bool(PBContext&)>>;
using PBCallIntercepter =
    TCallIntercepter<std::function<bool(WriteCodec&, const google::protobuf::Message&, google::protobuf::Message*)>>;
using PBReplyIntercepter =
    TReplyIntercepter<std::function<void(int32_t, uint64_t, const ReadCodec*, google::protobuf::Message&)>>;

class PBService : public IntercepterMgr<PBRecvIntercepter, PBSendIntercepter, PBReqIntercepter, PBRspIntercepter,
                                         PBCallIntercepter, PBReplyIntercepter>,
                  public Singleton<PBService>
{
public:
    static constexpr uint32_t MAX_TRANSPORT_NUM = 10;

    bool Init(uint32_t pkg_mem_check_key = 0);

    void SetContextCtrl(ContextController* context_ctrl) { context_ctrl_ = context_ctrl; }

    bool RegisterMethod(uint32_t cmd, const RpcMethod& method_info);

    /// 改进: Rpc 调用选项结构体，替代 9 参数函数
    struct RpcOptions
    {
        uint32_t dest = 0;
        bool broadcast = false;
        uint32_t timeout = 3000;  // 默认3秒
    };

    /// 发起 RPC 调用
    int32_t Rpc(uint32_t transport_type, uint64_t gid, uint32_t cmd,
                const google::protobuf::Message& req,
                google::protobuf::Message* rsp,
                const AsyncTask& task,
                const RpcOptions& opts = {});

    /// 兼容旧版 9 参数签名
    int32_t Rpc(uint32_t transport_type, uint64_t gid, uint32_t cmd,
                const google::protobuf::Message& req,
                google::protobuf::Message* rsp,
                const AsyncTask& task,
                uint32_t dest, bool broadcast, uint32_t timeout);

    bool AddTransport(uint32_t transport_type, const TransportInfo& info);
    [[nodiscard]] const TransportInfo* FindTransport(uint32_t transport_type) const;
    void SetReqScheduler(IScheduler* req_scheduler);
    bool DealReqPkg(const char* data, uint32_t len, uint32_t transport_type);

private:
    void RegisterPkgMem(uint32_t id);
    [[nodiscard]] bool CheckPkgMem(const ReadCodec& codec);
    void ClearPkgMem();

    int32_t OnRecv(uint32_t transport_type, const char* data, size_t data_len, uint32_t recv_id, uint64_t arrived_time);
    bool DealRequest(uint32_t transport_type, const ReadCodec& codec);
    void DealResponse(const ReadCodec& codec);
    void DealMethod(PBContext* context, google::protobuf::Service* service,
                    const google::protobuf::MethodDescriptor* method_desc);
    void MethodFinish(PBContext* context);
    int32_t SendMessage(const TransportInfo& info, const google::protobuf::Message& msg);

    bool InterceptRecv(const TransportInfo& info, uint32_t recv_id);
    bool InterceptSend(WriteCodec& codec);
    bool InterceptReq(PBContext& context);
    bool InterceptRsp(PBContext& context);
    bool InterceptCall(WriteCodec& codec, const google::protobuf::Message& req, google::protobuf::Message* rsp);
    void InterceptReply(int32_t ret_code, uint64_t seq_id, const ReadCodec* codec, google::protobuf::Message& rsp);

private:
    friend class Singleton<PBService>;
    PBService() = default;
    ~PBService() = default;

    std::unordered_map<uint32_t, RpcMethod> methods_;
    ContextController* context_ctrl_ = nullptr;
    std::array<TransportInfo, MAX_TRANSPORT_NUM> transport_infos_{};
    IScheduler* scheduler_ = nullptr;

    /// 改进: 逐字段比较代替 memcmp
    struct CheckHead
    {
        uint64_t gid = 0;
        uint64_t seq_id = 0;
        uint32_t cmd_id = 0;
    };
    CheckHead* shm_pkg_head_ = nullptr;
};

}  // namespace ua
