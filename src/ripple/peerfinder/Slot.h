
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

#ifndef RIPPLE_PEERFINDER_SLOT_H_INCLUDED
#define RIPPLE_PEERFINDER_SLOT_H_INCLUDED

#include <ripple/protocol/PublicKey.h>
#include <ripple/beast/net/IPEndpoint.h>
#include <boost/optional.hpp>
#include <memory>

namespace ripple {
namespace PeerFinder {

/*与对等覆盖连接关联的属性和状态。*/
class Slot
{
public:
    using ptr = std::shared_ptr <Slot>;

    enum State
    {
        accept,
        connect,
        connected,
        active,
        closing
    };

    virtual ~Slot () = 0;

    /*如果这是入站连接，则返回“true”。*/
    virtual bool inbound () const = 0;

    /*如果是固定连接，则返回“true”。
        如果连接的远程端点位于
        固定连接的远程终结点。
    **/

    virtual bool fixed () const = 0;

    /*如果这是群集连接，则返回“true”。
        只有在握手完成后才能知道这一点。
    **/

    virtual bool cluster () const = 0;

    /*返回连接的状态。*/
    virtual State state () const = 0;

    /*套接字的远程端点。*/
    virtual beast::IP::Endpoint const& remote_endpoint () const = 0;

    /*已知时套接字的本地终结点。*/
    virtual boost::optional <beast::IP::Endpoint> const& local_endpoint () const = 0;

    virtual boost::optional<std::uint16_t> listening_port () const = 0;

    /*对等方的公钥（如果知道）。
        公钥在握手完成时建立。
    **/

    virtual boost::optional <PublicKey> const& public_key () const = 0;
};

}
}

#endif
