
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

#include <ripple/app/paths/AccountCurrencies.h>

namespace ripple {

hash_set<Currency> accountSourceCurrencies (
    AccountID const& account,
    std::shared_ptr<RippleLineCache> const& lrCache,
    bool includeXRP)
{
    hash_set<Currency> currencies;

//YYY只会在超出保留范围的情况下才会麻烦。
    if (includeXRP)
        currencies.insert (xrpCurrency());

//波纹线列表。
    auto& rippleLines = lrCache->getRippleLines (account);

    for (auto const& item : rippleLines)
    {
        auto rspEntry = (RippleState*) item.get ();
        assert (rspEntry);
        if (!rspEntry)
            continue;

        auto& saBalance = rspEntry->getBalance ();

//滤除非
        if (saBalance > beast::zero
//有借据要发送。
            || (rspEntry->getLimitPeer ()
//同行扩大信用。
&& ((-saBalance) < rspEntry->getLimitPeer ()))) //贷方离开。
        {
            currencies.insert (saBalance.getCurrency ());
        }
    }

    currencies.erase (badCurrency());
    return currencies;
}

hash_set<Currency> accountDestCurrencies (
    AccountID const& account,
    std::shared_ptr<RippleLineCache> const& lrCache,
    bool includeXRP)
{
    hash_set<Currency> currencies;

    if (includeXRP)
        currencies.insert (xrpCurrency());
//即使帐户不存在

//波纹线列表。
    auto& rippleLines = lrCache->getRippleLines (account);

    for (auto const& item : rippleLines)
    {
        auto rspEntry = (RippleState*) item.get ();
        assert (rspEntry);
        if (!rspEntry)
            continue;

        auto& saBalance  = rspEntry->getBalance ();

if (saBalance < rspEntry->getLimit ())                  //可以采取更多
            currencies.insert (saBalance.getCurrency ());
    }

    currencies.erase (badCurrency());
    return currencies;
}

} //涟漪
