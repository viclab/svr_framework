/// @file rpc_methods_info.h
/// @brief RPC 方法注册信息（C++20 重写版）
#pragma once

namespace google::protobuf
{
class Service;
class Message;
class MethodDescriptor;
}

namespace ua
{

struct RpcMethod
{
    google::protobuf::Service* service = nullptr;
    const google::protobuf::MethodDescriptor* method = nullptr;
    const google::protobuf::Message* request = nullptr;
    const google::protobuf::Message* response = nullptr;
    bool is_private = false;
};

}  // namespace ua
