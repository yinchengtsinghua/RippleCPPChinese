
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

#ifndef RIPPLE_OVERLAY_PEER_H_INCLUDED
#define RIPPLE_OVERLAY_PEER_H_INCLUDED

#include <ripple/overlay/Message.h>
#include <ripple/basics/base_uint.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/beast/net/IPEndpoint.h>

namespace ripple {

namespace Resource {
class Charge;
}

//抓取碎片时要尝试的最大跃距。cs=爬行碎片
static constexpr std::uint32_t csHopLimit = 3;

/*表示覆盖中的对等连接。*/
class Peer
{
public:
    using ptr = std::shared_ptr<Peer>;

    /*唯一标识对等机。
        这可以存储在表中，以便稍后查找对等机。呼叫者
        可以发现对等端是否不再连接，并使
        根据需要进行调整。
    **/

    using id_t = std::uint32_t;

    virtual ~Peer() = default;

//
//网络
//

    virtual
    void
    send (Message::pointer const& m) = 0;

    virtual
    beast::IP::Endpoint
    getRemoteAddress() const = 0;

    /*根据施加的负载类型调整此对等机的负载平衡。*/
    virtual
    void
    charge (Resource::Charge const& fee) = 0;

//
//身份
//

    virtual
    id_t
    id() const = 0;

    /*如果此连接是群集的成员，则返回“true”。*/
    virtual
    bool
    cluster() const = 0;

    virtual
    bool
    isHighLatency() const = 0;

    virtual
    int
    getScore (bool) const = 0;

    virtual
    PublicKey const&
    getNodePublic() const = 0;

    virtual
    Json::Value json() = 0;

//
//分类帐
//

    virtual uint256 const& getClosedLedgerHash () const = 0;
    virtual bool hasLedger (uint256 const& hash, std::uint32_t seq) const = 0;
    virtual void ledgerRange (std::uint32_t& minSeq, std::uint32_t& maxSeq) const = 0;
    virtual bool hasShard (std::uint32_t shardIndex) const = 0;
    virtual bool hasTxSet (uint256 const& hash) const = 0;
    virtual void cycleStatus () = 0;
    virtual bool supportsVersion (int version) = 0;
    virtual bool hasRange (std::uint32_t uMin, std::uint32_t uMax) = 0;
};

}

#endif
