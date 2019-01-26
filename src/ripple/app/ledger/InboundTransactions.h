
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
    版权所有（c）2012-2015 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_LEDGER_INBOUNDTRANSACTIONS_H_INCLUDED
#define RIPPLE_APP_LEDGER_INBOUNDTRANSACTIONS_H_INCLUDED

#include <ripple/overlay/Peer.h>
#include <ripple/shamap/SHAMap.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/core/Stoppable.h>
#include <memory>

namespace ripple {

class Application;

/*管理事务集的获取和生存期。
**/


class InboundTransactions
{
public:
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

    InboundTransactions() = default;
    InboundTransactions(InboundTransactions const&) = delete;
    InboundTransactions& operator=(InboundTransactions const&) = delete;

    virtual ~InboundTransactions() = 0;

    /*按哈希检索事务集
    **/

    virtual std::shared_ptr <SHAMap> getSet (
        uint256 const& setHash,
        bool acquire) = 0;

    /*向入站事务集提供数据
    **/

    virtual void gotData (uint256 const& setHash,
        std::shared_ptr <Peer>,
        std::shared_ptr <protocol::TMLedgerData>) = 0;

    /*给容器设置
    **/

    virtual void giveSet (uint256 const& setHash,
        std::shared_ptr <SHAMap> const& set,
        bool acquired) = 0;

    /*如果出现新的共识回合，通知集装箱
    **/

    virtual void newRound (std::uint32_t seq) = 0;

    virtual Json::Value getInfo() = 0;

    virtual void onStop() = 0;
};

std::unique_ptr <InboundTransactions>
make_InboundTransactions (
    Application& app,
    InboundTransactions::clock_type& clock,
    Stoppable& parent,
    beast::insight::Collector::ptr const& collector,
    std::function
        <void (std::shared_ptr <SHAMap> const&,
            bool)> gotSet);


} //涟漪

#endif
