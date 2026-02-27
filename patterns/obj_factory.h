/// @file obj_factory.h
/// @brief 通用对象工厂模板类（C++20 重写版）
/// @note 支持 Array/Map 两种存储策略，支持 Singleton/非 Singleton 模式
///       使用 C++20 concepts 和 std::conditional_t
#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <unordered_map>
#include "singleton.h"

namespace ua
{

/// 数组存储策略，O(1) 查找
template <typename TNode, size_t MaxNodeNum>
struct ArrayStorage
{
    using ValueType = TNode;
    using KeyType = size_t;

    bool Insert(KeyType id, TNode&& node)
    {
        assert(id < MaxNodeNum);
        nodes_[id] = std::forward<TNode>(node);
        if (max_num_ < id)
            max_num_ = id;
        return true;
    }

    [[nodiscard]] TNode* Get(KeyType id)
    {
        assert(id <= max_num_);
        return &(nodes_[id]);
    }

    [[nodiscard]] const auto& GetAll() const noexcept { return nodes_; }
    [[nodiscard]] KeyType GetMaxNum() const noexcept { return max_num_; }

private:
    KeyType max_num_ = 0;
    std::array<TNode, MaxNodeNum> nodes_{};
};

/// Map 存储策略，灵活 key
template <typename TNode, typename Key = size_t>
struct MapStorage
{
    using ValueType = TNode;
    using KeyType = Key;

    bool Insert(KeyType id, TNode&& node)
    {
        auto [_, ok] = nodes_.emplace(id, std::forward<TNode>(node));
        return ok;
    }

    [[nodiscard]] TNode* Get(KeyType id)
    {
        auto iter = nodes_.find(id);
        return iter == nodes_.end() ? nullptr : &(iter->second);
    }

    [[nodiscard]] const auto& GetAll() const noexcept { return nodes_; }
    [[nodiscard]] auto GetMaxNum() const noexcept { return nodes_.size(); }

private:
    std::unordered_map<Key, TNode> nodes_;
};

/// 工厂核心实现
template <typename TAbstract, typename TStorage>
struct FactoryImpl : public TStorage
{
    using CreateType = typename TStorage::ValueType;

    bool Register(typename TStorage::KeyType id, CreateType&& creator)
    {
        return TStorage::Insert(id, std::forward<CreateType>(creator));
    }

    template <typename... Args>
    [[nodiscard]] TAbstract Create(typename TStorage::KeyType id, Args&&... args)
    {
        auto* creator = TStorage::Get(id);
        if (creator)
            return (*creator)(std::forward<Args>(args)...);
        return nullptr;
    }
};

/// 工厂模板类，通过 is_singleton 模板参数控制是否使用单例
template <typename TAbstract, typename TStorage, bool IsSingleton, typename TDerived = void>
class TFactory;

/// 单例版本
template <typename TAbstract, typename TStorage, typename TDerived>
class TFactory<TAbstract, TStorage, true, TDerived>
    : public FactoryImpl<TAbstract, TStorage>,
      public Singleton<std::conditional_t<std::is_void_v<TDerived>,
                                          TFactory<TAbstract, TStorage, true, void>,
                                          TDerived>>
{
};

/// 非单例版本
template <typename TAbstract, typename TStorage>
class TFactory<TAbstract, TStorage, false, void> : public FactoryImpl<TAbstract, TStorage>
{
};

// ========== 常用类型别名 ==========
template <typename TAbstract, typename TCreator, size_t MaxNodeNum = 128, typename TDerived = void>
using TArraySingletonFactory = TFactory<TAbstract, ArrayStorage<TCreator, MaxNodeNum>, true, TDerived>;

template <typename TAbstract, typename TCreator, typename Key = size_t, typename TDerived = void>
using TMapSingletonFactory = TFactory<TAbstract, MapStorage<TCreator, Key>, true, TDerived>;

template <typename TAbstract, typename TCreator, size_t MaxNodeNum = 128>
using TArrayFactory = TFactory<TAbstract, ArrayStorage<TCreator, MaxNodeNum>, false>;

template <typename TAbstract, typename TCreator, typename Key = size_t>
using TMapFactory = TFactory<TAbstract, MapStorage<TCreator, Key>, false>;

}  // namespace ua
