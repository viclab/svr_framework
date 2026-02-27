/// @file intercepter.h
/// @brief 拦截器体系（C++20 重写版）
/// @note 使用可变参数模板多继承实现拦截器管理器
#pragma once

#include <deque>
#include <utility>

namespace ua
{

/// 收包拦截器：返回 true 表示包被拦截
template <typename FUNC>
struct TRecvIntercepter
{
    using Type = FUNC;
    void AddRecvIntercepter(FUNC func) { intercepter_queue_.emplace_back(std::move(func)); }
    [[nodiscard]] const std::deque<FUNC>& GetAllRecvIntercepter() const { return intercepter_queue_; }
private:
    std::deque<FUNC> intercepter_queue_;
};

/// 请求反序列化后的拦截器
template <typename FUNC>
struct TReqIntercepter
{
    using Type = FUNC;
    void AddReqIntercepter(FUNC func) { intercepter_queue_.emplace_back(std::move(func)); }
    [[nodiscard]] const std::deque<FUNC>& GetAllReqIntercepter() const { return intercepter_queue_; }
private:
    std::deque<FUNC> intercepter_queue_;
};

/// 回包序列化前的拦截器（洋葱模型：push_front）
template <typename FUNC>
struct TRspIntercepter
{
    using Type = FUNC;
    void AddRspIntercepter(FUNC func) { intercepter_queue_.emplace_front(std::move(func)); }
    [[nodiscard]] const std::deque<FUNC>& GetAllRspIntercepter() const { return intercepter_queue_; }
private:
    std::deque<FUNC> intercepter_queue_;
};

/// 发包序列化后的拦截器（洋葱模型：push_front）
template <typename FUNC>
struct TSendIntercepter
{
    using Type = FUNC;
    void AddSendIntercepter(FUNC func) { intercepter_queue_.emplace_front(std::move(func)); }
    [[nodiscard]] const std::deque<FUNC>& GetAllSendIntercepter() const { return intercepter_queue_; }
private:
    std::deque<FUNC> intercepter_queue_;
};

/// 主调拦截器
template <typename FUNC>
struct TCallIntercepter
{
    using Type = FUNC;
    void AddCallIntercepter(FUNC func) { intercepter_queue_.emplace_back(std::move(func)); }
    [[nodiscard]] const std::deque<FUNC>& GetAllCallIntercepter() const { return intercepter_queue_; }
private:
    std::deque<FUNC> intercepter_queue_;
};

/// 主调回包拦截器（洋葱模型：push_front）
template <typename FUNC>
struct TReplyIntercepter
{
    using Type = FUNC;
    void AddReplyIntercepter(FUNC func) { intercepter_queue_.emplace_front(std::move(func)); }
    [[nodiscard]] const std::deque<FUNC>& GetAllReplyIntercepter() const { return intercepter_queue_; }
private:
    std::deque<FUNC> intercepter_queue_;
};

/// 多继承聚合（替代原版 TEmptyBase）
template <typename... Args>
struct IntercepterMgr : public Args...
{
};

}  // namespace ua
