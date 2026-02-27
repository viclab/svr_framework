/// @file unfixed_ring_buf.h
/// @brief 不定长数据块环形队列（C++20 重写版）
/// @note 改进: [[nodiscard]] + 内联实现
#pragma once

#include <cassert>
#include <cstring>
#include <functional>
#include "inner/ring_buf_data.h"

namespace ua
{

template <size_t MAX_SIZE = 0>
class UnfixedRingBuf : public UnfixedRingBufData<MAX_SIZE>
{
    using Data = UnfixedRingBufData<MAX_SIZE>;
    using IntType = typename Data::IntType;

public:
    using PopCallback = std::function<void(const uint8_t* data, size_t len)>;

    void clear()
    {
        Data::set_start(0);
        Data::set_end(0);
        Data::set_used_size(0);
        Data::set_item_num(0);
    }

    [[nodiscard]] bool empty() const { return Data::get_used_size() == 0 && Data::get_size() > 0; }
    [[nodiscard]] bool full() const { return Data::get_used_size() >= Data::get_size(); }
    [[nodiscard]] size_t size() const { return Data::get_used_size(); }
    [[nodiscard]] size_t capacity() const { return Data::get_size(); }
    [[nodiscard]] size_t get_num() const { return Data::get_item_num(); }

    bool push(const uint8_t* data, size_t len, bool over_write = false, const PopCallback& cb = nullptr)
    {
        struct iovec iov[1];
        iov[0].iov_base = const_cast<void*>(reinterpret_cast<const void*>(data));
        iov[0].iov_len = len;
        return push(iov, 1, over_write, cb);
    }

    bool push(const struct iovec* iov, size_t iov_cnt, bool over_write = false, const PopCallback& cb = nullptr)
    {
        size_t total_len = 0;
        for (size_t i = 0; i < iov_cnt; ++i)
            total_len += iov[i].iov_len;

        size_t need_len = total_len + sizeof(ItemHeader);
        if (need_len > Data::get_size())
            return false;
        return push_impl(iov, iov_cnt, total_len, need_len, over_write, cb);
    }

    void pop(const PopCallback& cb = nullptr)
    {
        if (empty()) return;

        assert(Data::get_used_size() >= sizeof(ItemHeader));
        auto* item_header = reinterpret_cast<ItemHeader*>(Data::m_buf + Data::get_start());
        assert(item_header->m_flag == 0);

        if (cb)
        {
            auto* data = Data::m_buf + Data::get_start() + sizeof(ItemHeader);
            cb(data, item_header->m_len);
        }

        Data::set_start((Data::get_start() + sizeof(ItemHeader) + item_header->m_len) % Data::get_size());
        assert(Data::get_used_size() >= (sizeof(ItemHeader) + item_header->m_len));
        Data::set_used_size(Data::get_used_size() - (sizeof(ItemHeader) + item_header->m_len));
        assert(Data::get_item_num() > 0);
        Data::set_item_num(Data::get_item_num() - 1);

        IntType skip_bytes = need_skip_bytes(Data::get_start());
        if (skip_bytes > 0)
        {
            Data::set_start((Data::get_start() + skip_bytes) % Data::get_size());
            assert(Data::get_used_size() >= skip_bytes);
            Data::set_used_size(Data::get_used_size() - skip_bytes);
        }
        else if (!empty())
        {
            pop_padding();
        }

        if (empty())
        {
            Data::set_start(0);
            Data::set_end(0);
        }
    }

    [[nodiscard]] const uint8_t* front(size_t& len, size_t index = 0) const
    {
        IntType item_start = find_start(index);
        if (item_start == Data::get_size()) return nullptr;
        auto* h = reinterpret_cast<const ItemHeader*>(Data::m_buf + item_start);
        len = h->m_len;
        return reinterpret_cast<const uint8_t*>(h + 1);
    }

    [[nodiscard]] uint8_t* front(size_t& len, size_t index = 0)
    {
        IntType item_start = find_start(index);
        if (item_start == Data::get_size()) return nullptr;
        auto* h = reinterpret_cast<ItemHeader*>(Data::m_buf + item_start);
        len = h->m_len;
        return reinterpret_cast<uint8_t*>(h + 1);
    }

private:
    struct ItemHeader
    {
        uint8_t m_flag = 0;
        IntType m_len = 0;
    };

    IntType find_start(size_t index) const
    {
        if (empty() || index >= Data::get_item_num())
            return Data::get_size();
        if (index == 0)
            return Data::get_start();

        IntType item_start = Data::get_start();
        for (size_t i = 0; i < index; ++i)
        {
            auto* header = reinterpret_cast<const ItemHeader*>(Data::m_buf + item_start);
            item_start = (item_start + sizeof(ItemHeader) + header->m_len) % Data::get_size();

            IntType skip = need_skip_bytes(item_start);
            if (skip > 0)
            {
                item_start = (item_start + skip) % Data::get_size();
            }
            else if (item_start != Data::get_end())
            {
                auto* ph = reinterpret_cast<const ItemHeader*>(Data::m_buf + item_start);
                if (ph->m_flag == 1)
                    item_start = (item_start + sizeof(ItemHeader) + ph->m_len) % Data::get_size();
            }

            if (item_start == Data::get_end())
                return Data::get_size();
        }
        return item_start;
    }

    bool push_impl(const struct iovec* iov, size_t iov_cnt, size_t total_len, size_t need_len,
                   bool over_write, const PopCallback& cb)
    {
        if (full())
        {
            if (!over_write) return false;
            pop(cb);
        }

        if (Data::get_end() >= Data::get_start())
        {
            if (Data::get_end() + need_len <= Data::get_size())
            {
                write_item(iov, iov_cnt, total_len, need_len);
                return true;
            }

            if (!over_write && Data::get_start() < need_len)
                return false;

            push_padding();
            if (over_write)
            {
                while (!empty() && Data::get_start() < need_len)
                    pop(cb);
            }
        }
        else
        {
            if (Data::get_end() + need_len <= Data::get_start())
            {
                write_item_no_wrap(iov, iov_cnt, total_len, need_len);
                return true;
            }

            if (!over_write) return false;

            if (Data::get_end() + need_len <= Data::get_size())
            {
                while (Data::get_end() < Data::get_start() && Data::get_end() + need_len > Data::get_start())
                    pop(cb);
                return push_impl(iov, iov_cnt, total_len, need_len, over_write, cb);
            }

            do { pop(cb); } while (Data::get_end() < Data::get_start());
            push_padding();
            assert(full());
        }

        return push_impl(iov, iov_cnt, total_len, need_len, over_write, cb);
    }

    void write_item(const struct iovec* iov, size_t iov_cnt, size_t total_len, size_t need_len)
    {
        auto* h = reinterpret_cast<ItemHeader*>(Data::m_buf + Data::get_end());
        h->m_len = total_len;
        h->m_flag = 0;
        auto* begin = Data::m_buf + Data::get_end() + sizeof(ItemHeader);
        for (size_t i = 0; i < iov_cnt; ++i)
        {
            std::memcpy(begin, iov[i].iov_base, iov[i].iov_len);
            begin += iov[i].iov_len;
        }
        Data::set_end((Data::get_end() + need_len) % Data::get_size());
        Data::set_used_size(Data::get_used_size() + need_len);
        Data::set_item_num(Data::get_item_num() + 1);

        IntType skip = need_skip_bytes(Data::get_end());
        Data::set_end((Data::get_end() + skip) % Data::get_size());
        Data::set_used_size(Data::get_used_size() + skip);
    }

    void write_item_no_wrap(const struct iovec* iov, size_t iov_cnt, size_t total_len, size_t need_len)
    {
        auto* h = reinterpret_cast<ItemHeader*>(Data::m_buf + Data::get_end());
        h->m_len = total_len;
        h->m_flag = 0;
        auto* begin = Data::m_buf + Data::get_end() + sizeof(ItemHeader);
        for (size_t i = 0; i < iov_cnt; ++i)
        {
            std::memcpy(begin, iov[i].iov_base, iov[i].iov_len);
            begin += iov[i].iov_len;
        }
        Data::set_end(Data::get_end() + need_len);
        Data::set_used_size(Data::get_used_size() + need_len);
        Data::set_item_num(Data::get_item_num() + 1);
    }

    void push_padding()
    {
        assert(Data::get_start() <= Data::get_end());
        auto* h = reinterpret_cast<ItemHeader*>(Data::m_buf + Data::get_end());
        h->m_len = (Data::get_size() - Data::get_end() - sizeof(ItemHeader));
        h->m_flag = 1;
        Data::set_used_size(Data::get_used_size() + (Data::get_size() - Data::get_end()));
        Data::set_end(0);
    }

    void pop_padding()
    {
        assert(Data::get_used_size() >= sizeof(ItemHeader));
        auto* h = reinterpret_cast<ItemHeader*>(Data::m_buf + Data::get_start());
        if (h->m_flag == 1)
        {
            Data::set_start((Data::get_start() + sizeof(ItemHeader) + h->m_len) % Data::get_size());
            Data::set_used_size(Data::get_used_size() - (sizeof(ItemHeader) + h->m_len));
        }
    }

    IntType need_skip_bytes(IntType cur_pos) const
    {
        assert(cur_pos <= Data::get_size());
        if (cur_pos + sizeof(ItemHeader) > Data::get_size())
            return Data::get_size() - cur_pos;
        return 0;
    }
};

}  // namespace ua
