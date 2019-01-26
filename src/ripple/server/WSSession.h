
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

#ifndef RIPPLE_SERVER_WSSESSION_H_INCLUDED
#define RIPPLE_SERVER_WSSESSION_H_INCLUDED

#include <ripple/server/Handoff.h>
#include <ripple/server/Port.h>
#include <ripple/server/Writer.h>
#include <boost/beast/core/buffers_prefix.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/logic/tribool.hpp>
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace ripple {

class WSMsg
{
public:
    WSMsg() = default;
    WSMsg(WSMsg const&) = delete;
    WSMsg& operator=(WSMsg const&) = delete;
    virtual ~WSMsg() = default;

    /*检索消息数据。

        返回指示是否
        数据可用，并且constbuffer序列
        表示数据。

        TrimoOL值：
            可能：数据还没准备好
            错误：数据可用
            对：数据可用，并且
                        它是最后一块字节。

        不知道数据何时
        结束（例如，返回
        paged database query）可能返回“true”和
        空向量。
    **/

    virtual
    std::pair<boost::tribool,
        std::vector<boost::asio::const_buffer>>
    prepare(std::size_t bytes,
        std::function<void(void)> resume) = 0;
};

template<class Streambuf>
class StreambufWSMsg : public WSMsg
{
    Streambuf sb_;
    std::size_t n_ = 0;

public:
    StreambufWSMsg(Streambuf&& sb)
        : sb_(std::move(sb))
    {
    }

    std::pair<boost::tribool,
        std::vector<boost::asio::const_buffer>>
    prepare(std::size_t bytes,
        std::function<void(void)>) override
    {
        if (sb_.size() == 0)
            return{true, {}};
        sb_.consume(n_);
        boost::tribool done;
        if (bytes < sb_.size())
        {
            n_ = bytes;
            done = false;
        }
        else
        {
            n_ = sb_.size();
            done = true;
        }
        auto const pb = boost::beast::buffers_prefix(n_, sb_.data());
        std::vector<boost::asio::const_buffer> vb (
            std::distance(pb.begin(), pb.end()));
        std::copy(pb.begin(), pb.end(), std::back_inserter(vb));
        return{done, vb};
    }
};

struct WSSession
{
    std::shared_ptr<void> appDefined;

    virtual ~WSSession () = default;
    WSSession() = default;
    WSSession(WSSession const&) = delete;
    WSSession& operator=(WSSession const&) = delete;

    virtual
    void
    run() = 0;

    virtual
    Port const&
    port() const = 0;

    virtual
    http_request_type const&
    request() const = 0;

    virtual
    boost::asio::ip::tcp::endpoint const&
    remote_endpoint() const = 0;

    /*发送WebSockets消息。*/
    virtual
    void
    send(std::shared_ptr<WSMsg> w) = 0;

    virtual
    void
    close() = 0;

    /*指示响应已完成。
        处理程序应在完成写入后调用它
        反应。
    **/

    virtual
    void
    complete() = 0;
};

} //涟漪

#endif
