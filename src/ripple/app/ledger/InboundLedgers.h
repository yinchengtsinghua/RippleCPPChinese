
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

#ifndef RIPPLE_APP_LEDGER_INBOUNDLEDGERS_H_INCLUDED
#define RIPPLE_APP_LEDGER_INBOUNDLEDGERS_H_INCLUDED

#include <ripple/app/ledger/InboundLedger.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/core/Stoppable.h>
#include <memory>

namespace ripple {

/*管理入站分类帐的生存期。

    @参见InboundLedger
**/

class InboundLedgers
{
public:
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

    virtual ~InboundLedgers() = 0;

//vfalc todo应该称为findoradd吗？
//
    virtual
    std::shared_ptr<Ledger const>
    acquire (uint256 const& hash,
        std::uint32_t seq, InboundLedger::Reason) = 0;

    virtual std::shared_ptr<InboundLedger> find (LedgerHash const& hash) = 0;

//vvalco todo删除对对等对象的依赖关系。
//
    virtual bool gotLedgerData (LedgerHash const& ledgerHash,
        std::shared_ptr<Peer>,
        std::shared_ptr <protocol::TMLedgerData>) = 0;

    virtual void doLedgerData (LedgerHash hash) = 0;

    virtual void gotStaleData (
        std::shared_ptr <protocol::TMLedgerData> packet) = 0;

    virtual int getFetchCount (int& timeoutCount) = 0;

    virtual void logFailure (uint256 const& h, std::uint32_t seq) = 0;

    virtual bool isFailure (uint256 const& h) = 0;

    virtual void clearFailures() = 0;

    virtual Json::Value getInfo() = 0;

    /*返回每分钟提取历史分类帐的速率。*/
    virtual std::size_t fetchRate() = 0;

    /*获取完整分类帐时调用。*/
    virtual void onLedgerFetched() = 0;

    virtual void gotFetchPack () = 0;
    virtual void sweep () = 0;

    virtual void onStop() = 0;
};

std::unique_ptr<InboundLedgers>
make_InboundLedgers (Application& app,
    InboundLedgers::clock_type& clock, Stoppable& parent,
    beast::insight::Collector::ptr const& collector);


} //涟漪

#endif
