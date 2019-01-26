
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
    版权所有（c）2015 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_PATHS_IMPL_STEP_CHECKS_H_INCLUDED
#define RIPPLE_APP_PATHS_IMPL_STEP_CHECKS_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/UintTypes.h>

namespace ripple {

inline
TER
checkFreeze (
    ReadView const& view,
    AccountID const& src,
    AccountID const& dst,
    Currency const& currency)
{
    assert (src != dst);

//检查冻结
    if (auto sle = view.read (keylet::account (dst)))
    {
        if (sle->isFlag (lsfGlobalFreeze))
        {
            return terNO_LINE;
        }
    }

    if (auto sle = view.read (keylet::line (src, dst, currency)))
    {
        if (sle->isFlag ((dst > src) ? lsfHighFreeze : lsfLowFreeze))
        {
            return terNO_LINE;
        }
    }

    return tesSUCCESS;
}

inline
TER
checkNoRipple (
    ReadView const& view,
    AccountID const& prev,
    AccountID const& cur,
//这是我们要检查其限制条件的帐户
    AccountID const& next,
    Currency const& currency,
    beast::Journal j)
{
//将波纹线从此节点中取出和取出
    auto sleIn = view.read (keylet::line (prev, cur, currency));
    auto sleOut = view.read (keylet::line (cur, next, currency));

    if (!sleIn || !sleOut)
        return terNO_LINE;

    if ((*sleIn)[sfFlags] &
            ((cur > prev) ? lsfHighNoRipple : lsfLowNoRipple) &&
        (*sleOut)[sfFlags] &
            ((cur > next) ? lsfHighNoRipple : lsfLowNoRipple))
    {
        JLOG (j.info()) << "Path violates noRipple constraint between " << prev
                      << ", " << cur << " and " << next;

        return terNO_RIPPLE;
    }
    return tesSUCCESS;
}

}

#endif
