
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

#ifndef RIPPLE_OVERLAY_MESSAGE_H_INCLUDED
#define RIPPLE_OVERLAY_MESSAGE_H_INCLUDED

#include <ripple/protocol/messages.h>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>

namespace ripple {

//vfalc注意，如果我们转发声明消息并写出共享的指针
//我们可以移除整个
//来自主集管的Ripple.pb.h。
//

//将消息打包成长度/类型的预结束缓冲区
//准备传输。
//
//消息实现了简单的协议“打包”，将消息缓冲到
//由指定消息长度的头所加的字符串。
//messagetype应该是protobuf编译器生成的message类。
//

class Message : public std::enable_shared_from_this <Message>
{
public:
    using pointer = std::shared_ptr<Message>;

public:
    /*消息头中的字节数。

        除非收到完整的邮件头，否则不会处理邮件，并且
        不会序列化小于此值的消息。
    **/

    static std::size_t constexpr kHeaderBytes = 6;

    /*消息的最大大小。

        发送大小超过此值的消息可能会导致连接
        被丢弃。将来可能支持更大的邮件大小，或者
        作为协议升级的一部分进行协商。
     **/

    static std::size_t constexpr kMaxMessageSize = 64 * 1024 * 1024;

    Message (::google::protobuf::Message const& message, int type);

    /*检索打包的消息数据。*/
    std::vector <uint8_t> const&
    getBuffer () const
    {
        return mBuffer;
    }

    /*获取流量类别*/
    int
    getCategory () const
    {
        return mCategory;
    }

    /*按相等的顺序确定。*/
    bool operator == (Message const& other) const;

    /*计算打包消息的长度。*/
    /*@ {*/
    static unsigned getLength (std::vector <uint8_t> const& buf);

    template <class FwdIter>
    static
    std::enable_if_t<std::is_same<typename
        FwdIter::value_type, std::uint8_t>::value, std::size_t>
    size (FwdIter first, FwdIter last)
    {
        if (std::distance(first, last) <
                Message::kHeaderBytes)
            return 0;
        std::size_t n;
        n  = std::size_t{*first++} << 24;
        n += std::size_t{*first++} << 16;
        n += std::size_t{*first++} <<  8;
        n += std::size_t{*first};
        return n;
    }

    template <class BufferSequence>
    static
    std::size_t
    size (BufferSequence const& buffers)
    {
        return size(buffers_begin(buffers),
            buffers_end(buffers));
    }
    /*@ }*/

    /*确定打包消息的类型。*/
    /*@ {*/
    static int getType (std::vector <uint8_t> const& buf);

    template <class FwdIter>
    static
    std::enable_if_t<std::is_same<typename
        FwdIter::value_type, std::uint8_t>::value, int>
    type (FwdIter first, FwdIter last)
    {
        if (std::distance(first, last) <
                Message::kHeaderBytes)
            return 0;
        return (int{*std::next(first, 4)} << 8) |
            *std::next(first, 5);
    }

    template <class BufferSequence>
    static
    int
    type (BufferSequence const& buffers)
    {
        return type(buffers_begin(buffers),
            buffers_end(buffers));
    }
    /*@ }*/

private:
    template <class BufferSequence, class Value = std::uint8_t>
    static
    boost::asio::buffers_iterator<BufferSequence, Value>
    buffers_begin (BufferSequence const& buffers)
    {
        return boost::asio::buffers_iterator<
            BufferSequence, Value>::begin (buffers);
    }

    template <class BufferSequence, class Value = std::uint8_t>
    static
    boost::asio::buffers_iterator<BufferSequence, Value>
    buffers_end (BufferSequence const& buffers)
    {
        return boost::asio::buffers_iterator<
            BufferSequence, Value>::end (buffers);
    }

//对大小进行编码，并在buf开头将其键入页眉
//
    void encodeHeader (unsigned size, int type);

    std::vector <uint8_t> mBuffer;

    int mCategory;
};

}

#endif
