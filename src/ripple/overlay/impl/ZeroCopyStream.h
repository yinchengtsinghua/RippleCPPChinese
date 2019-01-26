
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Rippled的一部分：https://github.com/ripple/rippled
    版权所有（c）2012，2013 Ripple Labs Inc.

    使用、复制、修改和/或分发本软件的权限
    特此授予免费或不收费的目的，前提是
    版权声明和本许可声明出现在所有副本中。

    本软件按“原样”提供，作者不作任何保证。
    关于本软件，包括
    适销性和适用性。在任何情况下，作者都不对
    任何特殊、直接、间接或后果性损害或任何损害
    因使用、数据或利润损失而导致的任何情况，无论是在
    合同行为、疏忽或其他侵权行为
    或与本软件的使用或性能有关。
**/

//==============================================================

#ifndef RIPPLE_OVERLAY_ZEROCOPYSTREAM_H_INCLUDED
#define RIPPLE_OVERLAY_ZEROCOPYSTREAM_H_INCLUDED

#include <google/protobuf/io/zero_copy_stream.h>
#include <boost/asio/buffer.hpp>
#include <cstdint>

namespace ripple {

/*围绕缓冲区序列实现ZeroCopyInputStream。
    @tparam缓冲一个满足constbuforsequence要求的类型。
    @参见https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.io.zero_copy_stream
**/

template <class Buffers>
class ZeroCopyInputStream
    : public ::google::protobuf::io::ZeroCopyInputStream
{
private:
    using iterator = typename Buffers::const_iterator;
    using const_buffer = boost::asio::const_buffer;

    google::protobuf::int64 count_ = 0;
    iterator last_;
iterator first_;    //位置的来源
const_buffer pos_;  //下一个（）将返回什么

public:
    explicit
    ZeroCopyInputStream (Buffers const& buffers);

    bool
    Next (const void** data, int* size) override;

    void
    BackUp (int count) override;

    bool
    Skip (int count) override;

    google::protobuf::int64
    ByteCount() const override
    {
        return count_;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Buffers>
ZeroCopyInputStream<Buffers>::ZeroCopyInputStream (
        Buffers const& buffers)
    : last_ (buffers.end())
    , first_ (buffers.begin())
    , pos_ ((first_ != last_) ?
        *first_ : const_buffer(nullptr, 0))
{
}

template <class Buffers>
bool
ZeroCopyInputStream<Buffers>::Next (
    const void** data, int* size)
{
    *data = boost::asio::buffer_cast<void const*>(pos_);
    *size = boost::asio::buffer_size(pos_);
    if (first_ == last_)
        return false;
    count_ += *size;
    pos_ = (++first_ != last_) ? *first_ :
        const_buffer(nullptr, 0);
    return true;
}

template <class Buffers>
void
ZeroCopyInputStream<Buffers>::BackUp (int count)
{
    --first_;
    pos_ = *first_ +
        (boost::asio::buffer_size(*first_) - count);
    count_ -= count;
}

template <class Buffers>
bool
ZeroCopyInputStream<Buffers>::Skip (int count)
{
    if (first_ == last_)
        return false;
    while (count > 0)
    {
        auto const size =
            boost::asio::buffer_size(pos_);
        if (count < size)
        {
            pos_ = pos_ + count;
            count_ += count;
            return true;
        }
        count_ += size;
        if (++first_ == last_)
            return false;
        count -= size;
        pos_ = *first_;
    }
    return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*围绕streambuf实现ZeroCopyOutputStream。
    streambuf与boost:：asio:：streambuf定义的公共接口匹配。
    @tparam streambuf满足streambuf要求的类型。
**/

template <class Streambuf>
class ZeroCopyOutputStream
    : public ::google::protobuf::io::ZeroCopyOutputStream
{
private:
    using buffers_type = typename Streambuf::mutable_buffers_type;
    using iterator = typename buffers_type::const_iterator;
    using mutable_buffer = boost::asio::mutable_buffer;

    Streambuf& streambuf_;
    std::size_t blockSize_;
    google::protobuf::int64 count_ = 0;
    std::size_t commit_ = 0;
    buffers_type buffers_;
    iterator pos_;

public:
    explicit
    ZeroCopyOutputStream (Streambuf& streambuf,
        std::size_t blockSize);

    ~ZeroCopyOutputStream();

    bool
    Next (void** data, int* size) override;

    void
    BackUp (int count) override;

    google::protobuf::int64
    ByteCount() const override
    {
        return count_;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Streambuf>
ZeroCopyOutputStream<Streambuf>::ZeroCopyOutputStream(
        Streambuf& streambuf, std::size_t blockSize)
    : streambuf_ (streambuf)
    , blockSize_ (blockSize)
    , buffers_ (streambuf_.prepare(blockSize_))
    , pos_ (buffers_.begin())
{
}

template <class Streambuf>
ZeroCopyOutputStream<Streambuf>::~ZeroCopyOutputStream()
{
    if (commit_ != 0)
        streambuf_.commit(commit_);
}

template <class Streambuf>
bool
ZeroCopyOutputStream<Streambuf>::Next(
    void** data, int* size)
{
    if (commit_ != 0)
    {
        streambuf_.commit(commit_);
        count_ += commit_;
    }

    if (pos_ == buffers_.end())
    {
        buffers_ = streambuf_.prepare (blockSize_);
        pos_ = buffers_.begin();
    }

    *data = boost::asio::buffer_cast<void*>(*pos_);
    *size = boost::asio::buffer_size(*pos_);
    commit_ = *size;
    ++pos_;
    return true;
}

template <class Streambuf>
void
ZeroCopyOutputStream<Streambuf>::BackUp (int count)
{
    assert(count <= commit_);
    auto const n = commit_ - count;
    streambuf_.commit(n);
    count_ += n;
    commit_ = 0;
}

} //涟漪

#endif
