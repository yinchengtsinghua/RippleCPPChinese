
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
#include <tuple>

namespace ripple {
namespace path {

TER PathCursor::forwardLiquidity () const
{
    if (node().isAccount())
        return forwardLiquidityForAccount ();

//否则，节点就是一个报价。
    if (previousNode().account_ == beast::zero)
        return tesSUCCESS;

//前一个是帐户节点，解析其传递。
    STAmount saInAct;
    STAmount saInFees;

    auto resultCode = deliverNodeForward (
        previousNode().account_,
previousNode().saFwdDeliver, //前一个是发送这么多。
        saInAct,
        saInFees,
        false);

    assert (resultCode != tesSUCCESS ||
            previousNode().saFwdDeliver == saInAct + saInFees);
    return resultCode;
}

} //路径
} //涟漪

//原始评论：

//调用以从链中的第一个提供节点驱动。
//
//-报价输入是在发行人/边缘。
//-当前使用的报价。
//-借记当前报价所有者。
//-贷记给发行人的转让费。
//-支付给发行人或林博。
//-设定交付，不收取转让费。
