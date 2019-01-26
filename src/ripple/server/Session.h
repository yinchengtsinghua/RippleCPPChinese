
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

#ifndef RIPPLE_SERVER_SESSION_H_INCLUDED
#define RIPPLE_SERVER_SESSION_H_INCLUDED

#include <ripple/server/Writer.h>
#include <ripple/server/WSSession.h>
#include <boost/beast/http/message.hpp>
#include <ripple/beast/net/IPEndpoint.h>
#include <ripple/beast/utility/Journal.h>
#include <functional>
#include <memory>
#include <ostream>
#include <vector>

namespace ripple {

/*连接会话的持久状态信息。
    这些值在效率要求之间保留。
    有些字段是输入参数，有些是输出参数，
    所有这些都只在特定的回调过程中被定义。
**/

class Session
{
public:
    Session() = default;
    Session (Session const&) = delete;
    Session& operator=(Session const&) = delete;
    virtual ~Session () = default;

    /*用户可定义的指针。
        初始值始终为零。
        对值的更改在调用之间保持不变。
    **/

    void* tag = nullptr;

    /*返回用于日志记录的日志。*/
    virtual
    beast::Journal
    journal() = 0;

    /*返回此连接的端口设置。*/
    virtual
    Port const&
    port() = 0;

    /*返回连接的远程地址。*/
    virtual
    beast::IP::Endpoint
    remoteAddress() = 0;

    /*返回当前HTTP请求。*/
    virtual
    http_request_type&
    request() = 0;

    /*异步发送数据副本。*/
    /*@ {*/
    void
    write (std::string const& s)
    {
        if (! s.empty())
            write (&s[0],
                std::distance (s.begin(), s.end()));
    }

    template <typename BufferSequence>
    void
    write (BufferSequence const& buffers)
    {
        for (typename BufferSequence::const_iterator iter (buffers.begin());
            iter != buffers.end(); ++iter)
        {
            typename BufferSequence::value_type const& buffer (*iter);
            write (boost::asio::buffer_cast <void const*> (buffer),
                boost::asio::buffer_size (buffer));
        }
    }

    virtual
    void
    write (void const* buffer, std::size_t bytes) = 0;

    virtual
    void
    write (std::shared_ptr <Writer> const& writer,
        bool keep_alive) = 0;

    /*@ }*/

    /*分离会话。
        这将使会话保持打开状态，以便可以发送响应
        异步。调用IO服务：：由服务器运行
        在关闭所有已分离的会话之前不会返回。
    **/

    virtual
    std::shared_ptr<Session>
    detach() = 0;

    /*指示响应已完成。
        处理程序应在完成写入后调用它
        反应。如果连接上显示“保持活动状态”，
        这将触发对下一个请求的读取；否则，
        发送完所有剩余数据后，连接将关闭。
    **/

    virtual
    void
    complete() = 0;

    /*关闭会话。
        这将异步执行。会议将
        在所有挂起的写入完成后正常关闭。
        @param graceful`true`等待所有数据发送完毕。
    **/

    virtual
    void
    close (bool graceful) = 0;

    /*将连接转换为WebSocket。*/
    virtual
    std::shared_ptr<WSSession>
    websocketUpgrade() = 0;
};

}  //涟漪

#endif
