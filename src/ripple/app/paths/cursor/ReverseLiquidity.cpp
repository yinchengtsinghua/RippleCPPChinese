
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

#include <ripple/app/paths/cursor/PathCursor.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/View.h>
#include <tuple>

namespace ripple {
namespace path {

//计算一个节点及其以前的节点。最终目标是确定1
//我们需要多少投入货币来满足
//输出。
//
//从目的地向源反向工作计算多少
//必须要求。当我们向后移动时，单个节点可能会进一步限制
//可用流动性的数量。
//
//这只是一个控制循环，它将事情设置好，然后移交工作。
//关到反向流动性或帐户或
//反向流动。
//
//稍后，这一结果将被用于进一步研究，找出
//实际上可以交付很多。
//
//<--resultcode:tessuccess或tecpath-dry
//<>节点：
//-->[结束]sawanted.mamount
//-->[全部]sawanted.mcurrency
//-->[全部]帐户
//<->[0]sawanted.mamount:-->limit，<--actual

TER PathCursor::reverseLiquidity () const
{
//每个账户的发行都有一个转账率。

//明天：账户费用
//第三方转让该账户本身的发行时的费用。

//缓存此节点的输出传输速率。
    node().transferRate_ = transferRate (view(), node().issue_.account);

    if (node().isAccount ())
        return reverseLiquidityForAccount ();

//否则，节点就是一个报价。
    if (isXRP (nextNode().account_))
    {
        JLOG (j_.trace())
            << "reverseLiquidityForOffer: "
            << "OFFER --> offer: nodeIndex_=" << nodeIndex_;
        return tesSUCCESS;

//此控件结构确保仅为
//在一系列优惠中最合适的优惠-这意味着
//delivernodereverse必须考虑所有这些报价。
    }

//接下来是一个帐户节点，解析当前提供节点的交付。
    STAmount saDeliverAct;

    JLOG (j_.trace())
        << "reverseLiquidityForOffer: OFFER --> account:"
        << " nodeIndex_=" << nodeIndex_
        << " saRevDeliver=" << node().saRevDeliver;

//下一个节点希望当前节点提供这么多：
    return deliverNodeReverse (
        nextNode().account_,
        node().saRevDeliver,
        saDeliverAct);
}

} //路径
} //涟漪
