/// @file channel_interface.h
/// @brief 收发包通道抽象接口（C++20 重写版）
/// @note 使用 std::move_only_function 替代 std::function（可选）
#pragma once

#include <cstdint>
#include <functional>

namespace ua
{

/// 通信通道抽象接口
class IChannel
{
public:
    /// 收包回调函数类型
    /// @param data 数据指针
    /// @param data_len 数据长度
    /// @param recv_id 来源端点 ID
    /// @param arrived_time 到达时间戳（毫秒）
    using RecvCallBack = std::function<int32_t(const char*, size_t, uint32_t, uint64_t)>;

    void SetCallback(RecvCallBack callback) noexcept { recv_callback_ = std::move(callback); }

    /// 获取当前端点 ID
    [[nodiscard]] virtual uint32_t MyID() const = 0;
    /// 发送数据
    virtual int32_t Send(uint32_t dest_id, const char* buff, size_t buff_len) = 0;
    /// 收包驱动循环，返回处理的包数量
    virtual size_t Loop(uint32_t max_recv_count) = 0;

    virtual ~IChannel() = default;

protected:
    RecvCallBack recv_callback_;
};

}  // namespace ua
