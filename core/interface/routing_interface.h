/// @file routing_interface.h
/// @brief 路由选址抽象接口（C++20 重写版）
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace ua
{

class IRouting
{
public:
    /// 添加路由节点
    virtual void AddRoute(uint32_t svr_type, uint32_t node_id, int32_t version) = 0;
    /// 删除路由节点
    virtual void DelRoute(uint32_t svr_type, uint32_t node_id) = 0;
    /// 节点是否存在
    [[nodiscard]] virtual bool IsNodeExist(uint32_t node_id, uint32_t svr_type) const = 0;
    /// 获取服务的节点数
    [[nodiscard]] virtual size_t GetNodeNum(uint32_t svr_type, int32_t version) const = 0;
    /// 返回最终要发送的目标 ID
    [[nodiscard]] virtual uint32_t GetSendDest(uint32_t svr_type, uint64_t gid,
                                                uint32_t expect_dest_id, uint32_t version) const = 0;
    /// 返回所有的 dest_id（广播场景）
    [[nodiscard]] virtual const std::vector<uint32_t>& GetAllSendDest(uint32_t svr_type, uint32_t world_id,
                                                                      uint32_t version) const = 0;
    /// 清空所有路由信息
    virtual void Clear() = 0;

    /// 节点上下线回调
    using OnlineStateCallBack = std::function<void(uint32_t svr_type, uint32_t id, bool is_online)>;
    virtual void AddOnlineStateCallback(OnlineStateCallBack callback) {}
    virtual void ClearCallback() {}

    virtual ~IRouting() = default;
};

}  // namespace ua
