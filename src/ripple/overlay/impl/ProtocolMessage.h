
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

#ifndef RIPPLE_OVERLAY_PROTOCOLMESSAGE_H_INCLUDED
#define RIPPLE_OVERLAY_PROTOCOLMESSAGE_H_INCLUDED

#include <ripple/protocol/messages.h>
#include <ripple/overlay/Message.h>
#include <ripple/overlay/impl/ZeroCopyStream.h>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/system/error_code.hpp>
#include <cassert>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

namespace ripple {

/*返回给定类型的协议消息的名称。*/
template <class = void>
std::string
protocolMessageName (int type)
{
    switch (type)
    {
    case protocol::mtHELLO:             return "hello";
    case protocol::mtMANIFESTS:         return "manifests";
    case protocol::mtPING:              return "ping";
    case protocol::mtPROOFOFWORK:       return "proof_of_work";
    case protocol::mtCLUSTER:           return "cluster";
    case protocol::mtGET_SHARD_INFO:    return "get_shard_info";
    case protocol::mtSHARD_INFO:        return "shard_info";
    case protocol::mtGET_PEERS:         return "get_peers";
    case protocol::mtPEERS:             return "peers";
    case protocol::mtENDPOINTS:         return "endpoints";
    case protocol::mtTRANSACTION:       return "tx";
    case protocol::mtGET_LEDGER:        return "get_ledger";
    case protocol::mtLEDGER_DATA:       return "ledger_data";
    case protocol::mtPROPOSE_LEDGER:    return "propose";
    case protocol::mtSTATUS_CHANGE:     return "status";
    case protocol::mtHAVE_SET:          return "have_set";
    case protocol::mtVALIDATION:        return "validation";
    case protocol::mtGET_OBJECTS:       return "get_objects";
    default:
        break;
    };
    return "unknown";
}

namespace detail {

template <class T, class Buffers, class Handler>
std::enable_if_t<std::is_base_of<
    ::google::protobuf::Message, T>::value,
        boost::system::error_code>
invoke (int type, Buffers const& buffers,
    Handler& handler)
{
    ZeroCopyInputStream<Buffers> stream(buffers);
    stream.Skip(Message::kHeaderBytes);
    auto const m (std::make_shared<T>());
    if (! m->ParseFromZeroCopyStream(&stream))
        return boost::system::errc::make_error_code(
            boost::system::errc::invalid_argument);
    auto ec = handler.onMessageBegin (type, m,
       Message::kHeaderBytes + Message::size (buffers));
    if (! ec)
    {
        handler.onMessage (m);
        handler.onMessageEnd (type, m);
    }
    return ec;
}

}

/*在传递的缓冲区中调用最多一条协议消息的处理程序。

    如果没有足够的数据生成完整的协议
    消息，对于所用的字节数返回零。

    @返回消耗的字节数，或者返回错误代码（如果有）。
**/

template <class Buffers, class Handler>
std::pair <std::size_t, boost::system::error_code>
invokeProtocolMessage (Buffers const& buffers, Handler& handler)
{
    std::pair<std::size_t,boost::system::error_code> result = { 0, {} };
    boost::system::error_code& ec = result.second;

    auto const bs = boost::asio::buffer_size(buffers);

//如果我们甚至没有足够的字节容纳头部，那就没有意义了
//做任何工作。
    if (bs < Message::kHeaderBytes)
        return result;

    if (bs > Message::kMaxMessageSize)
    {
        result.second = make_error_code(boost::system::errc::message_size);
        return result;
    }

    auto const size = Message::kHeaderBytes + Message::size(buffers);

    if (bs < size)
        return result;

    auto const type = Message::type(buffers);

    switch (type)
    {
    case protocol::mtHELLO:         ec = detail::invoke<protocol::TMHello> (type, buffers, handler); break;
    case protocol::mtMANIFESTS:     ec = detail::invoke<protocol::TMManifests> (type, buffers, handler); break;
    case protocol::mtPING:          ec = detail::invoke<protocol::TMPing> (type, buffers, handler); break;
    case protocol::mtCLUSTER:       ec = detail::invoke<protocol::TMCluster> (type, buffers, handler); break;
    case protocol::mtGET_SHARD_INFO:ec = detail::invoke<protocol::TMGetShardInfo> (type, buffers, handler); break;
    case protocol::mtSHARD_INFO:    ec = detail::invoke<protocol::TMShardInfo>(type, buffers, handler); break;
    case protocol::mtGET_PEERS:     ec = detail::invoke<protocol::TMGetPeers> (type, buffers, handler); break;
    case protocol::mtPEERS:         ec = detail::invoke<protocol::TMPeers> (type, buffers, handler); break;
    case protocol::mtENDPOINTS:     ec = detail::invoke<protocol::TMEndpoints> (type, buffers, handler); break;
    case protocol::mtTRANSACTION:   ec = detail::invoke<protocol::TMTransaction> (type, buffers, handler); break;
    case protocol::mtGET_LEDGER:    ec = detail::invoke<protocol::TMGetLedger> (type, buffers, handler); break;
    case protocol::mtLEDGER_DATA:   ec = detail::invoke<protocol::TMLedgerData> (type, buffers, handler); break;
    case protocol::mtPROPOSE_LEDGER:ec = detail::invoke<protocol::TMProposeSet> (type, buffers, handler); break;
    case protocol::mtSTATUS_CHANGE: ec = detail::invoke<protocol::TMStatusChange> (type, buffers, handler); break;
    case protocol::mtHAVE_SET:      ec = detail::invoke<protocol::TMHaveTransactionSet> (type, buffers, handler); break;
    case protocol::mtVALIDATION:    ec = detail::invoke<protocol::TMValidation> (type, buffers, handler); break;
    case protocol::mtGET_OBJECTS:   ec = detail::invoke<protocol::TMGetObjectByHash> (type, buffers, handler); break;
    default:
        ec = handler.onMessageUnknown (type);
        break;
    }
    if (! ec)
        result.first = size;

    return result;
}

/*将协议消息写入streambuf。*/
template <class Streambuf>
void
write (Streambuf& streambuf,
    ::google::protobuf::Message const& m, int type,
        std::size_t blockBytes)
{
    auto const size = m.ByteSize();
    std::array<std::uint8_t, 6> v;
    v[0] = static_cast<std::uint8_t>((size >> 24) & 0xFF);
    v[1] = static_cast<std::uint8_t>((size >> 16) & 0xFF);
    v[2] = static_cast<std::uint8_t>((size >>  8) & 0xFF);
    v[3] = static_cast<std::uint8_t>( size        & 0xFF);
    v[4] = static_cast<std::uint8_t>((type >>  8) & 0xFF);
    v[5] = static_cast<std::uint8_t>( type        & 0xFF);
    streambuf.commit(boost::asio::buffer_copy(
        streambuf.prepare(Message::kHeaderBytes),
            boost::asio::buffer(v)));
    ZeroCopyOutputStream<Streambuf> stream (
        streambuf, blockBytes);
    m.SerializeToZeroCopyStream(&stream);
}

} //涟漪

#endif
