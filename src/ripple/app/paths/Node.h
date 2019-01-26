
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

#ifndef RIPPLE_APP_PATHS_NODE_H_INCLUDED
#define RIPPLE_APP_PATHS_NODE_H_INCLUDED

#include <ripple/app/paths/NodeDirectory.h>
#include <ripple/app/paths/Types.h>
#include <ripple/protocol/Rate.h>
#include <ripple/protocol/UintTypes.h>
#include <boost/optional.hpp>

namespace ripple {
namespace path {

struct Node
{
    explicit Node() = default;

    using List = std::vector<Node>;

    inline bool isAccount() const
    {
        return (uFlags & STPathElement::typeAccount);
    }

    Json::Value getJson () const;

    bool operator == (Node const&) const;

std::uint16_t uFlags;       //>从路径。

AccountID account_;         //-->账户：接收/发送账户。

Issue issue_;               //-->accounts:接收和发送，offers:发送。
//——下一个报价的货币已经用完了。

boost::optional<Rate> transferRate_;         //发行人的转移率。

//反向计算。
STAmount saRevRedeem;        //<--兑换到下一个的金额。
STAmount saRevIssue;         //<--发行给下一个的金额，受限于
//信用证和欠条。问题
//报价中没有使用。
STAmount saRevDeliver;       //<--无论
//费用。

//由正向计算。
STAmount saFwdRedeem;        //<--金额节点将兑现到下一个。
STAmount saFwdIssue;         //<--金额节点下发。
//优惠不使用问题。
STAmount saFwdDeliver;       //<--无论
//费用。

//提供：
    boost::optional<Rate> rateMax;

//节点被划分成一个称为“目录”的存储桶。
//
//每个“目录”包含完全相同“质量”的节点（意思是
//一个正确性与下一个正确性之间的转换率）。
//
//“目录”按“增加”“质量”值排序，其中
//表示第一个“目录”具有“最佳”（即数字最少）
//“质量”。
//https://ripple.com/wiki/ledger_-format优先排序_a_-continuous_-key_-space

    NodeDirectory directory;

STAmount saOfrRate;          //正确的比率。

//支付节点
bool bEntryAdvance;          //需要提前进入。
    unsigned int uEntry;
    uint256 offerIndex_;
    SLE::pointer sleOffer;
    AccountID offerOwnerAccount_;

//我们需要更新SaofferFunds、Satakerpays和Satakergets吗？
    bool bFundsDirty;
    STAmount saOfferFunds;
    STAmount saTakerPays;
    STAmount saTakerGets;

    /*清除输入和输出量。*/
    void clear()
    {
        saRevRedeem.clear ();
        saRevIssue.clear ();
        saRevDeliver.clear ();
        saFwdDeliver.clear ();
    }
};

} //路径
} //涟漪

#endif
