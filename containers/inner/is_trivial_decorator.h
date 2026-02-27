/// @file is_trivial_decorator.h
/// @brief Trivially Copyable 类型装饰器（C++20 重写版）
/// @note 改进: 用 if constexpr 替代偏特化
///       修复: operator= 返回 *this
#pragma once

#include <type_traits>

namespace ua::inner
{

/// 对于 trivially_copyable 的类型不需要析构操作
/// 对于非 trivially_copyable 的类型需要在析构时调用 clear() 和 placement new 拷贝构造
template <template <typename, size_t, typename, typename> class SET,
          typename T, size_t MAX_SIZE, typename HASH, typename IS_EQUAL>
struct IsTrivialDecorator : public SET<T, MAX_SIZE, HASH, IS_EQUAL>
{
    using BaseType = SET<T, MAX_SIZE, HASH, IS_EQUAL>;

    IsTrivialDecorator() = default;

    ~IsTrivialDecorator()
    {
        if constexpr (!std::is_trivially_copyable_v<T>)
            BaseType::clear();
    }

    IsTrivialDecorator(const IsTrivialDecorator& other) : BaseType(other)
    {
        if constexpr (!std::is_trivially_copyable_v<T>)
        {
            auto beg = BaseType::begin();
            auto o_beg = other.begin();
            while (beg != BaseType::end())
            {
                new (&(*beg)) T(*o_beg);
                ++beg;
                ++o_beg;
            }
        }
    }

    IsTrivialDecorator& operator=(const IsTrivialDecorator& other)
    {
        if (this == &other) return *this;

        if constexpr (!std::is_trivially_copyable_v<T>)
        {
            BaseType::clear();
            BaseType::operator=(other);
            auto beg = BaseType::begin();
            auto o_beg = other.begin();
            while (beg != BaseType::end())
            {
                new (&(*beg)) T(*o_beg);
                ++beg;
                ++o_beg;
            }
        }
        else
        {
            BaseType::operator=(other);
        }
        return *this;  // 修复: 原版缺少返回值
    }

    IsTrivialDecorator(IsTrivialDecorator&&) = default;
    IsTrivialDecorator& operator=(IsTrivialDecorator&&) = default;
};

}  // namespace ua::inner
