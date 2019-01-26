
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
    版权所有（c）2012-2014 Ripple Labs Inc.

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

#include <ripple/app/main/Application.h>
#include <ripple/app/paths/RippleState.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {

//查询：
//1）指定要查询的分类帐。
//2）在“账户”字段中指定发卡机构账户（冷钱包）。
//3）指定保存网关资产的帐户（如热钱包）
//使用“HotWallet”字段，该字段应为字符串（如果只是
//一个钱包）或一组字符串（如果不止一个）。

//回应：
//1）数组“义务”，表示
//每种货币的网关。对指定热钱包的义务
//这里不算。
//2）对象“余额”，表示每个账户的余额。
//拥有网关资产。（那些在“Hotwallet”中指定的）
//字段）
//3）“资产”的对象，表示欠网关的帐户。
//（网关通常不保持正平衡。这是不寻常的。）

//网关余额

Json::Value doGatewayBalances (RPC::Context& context)
{
    auto& params = context.params;

//获取当前分类帐
    std::shared_ptr<ReadView const> ledger;
    auto result = RPC::lookupLedger (ledger, context);

    if (!ledger)
        return result;

    if (!(params.isMember (jss::account) || params.isMember (jss::ident)))
        return RPC::missing_field_error (jss::account);

    std::string const strIdent (params.isMember (jss::account)
        ? params[jss::account].asString ()
        : params[jss::ident].asString ());

    bool const bStrict = params.isMember (jss::strict) &&
            params[jss::strict].asBool ();

//获取有关帐户的信息。
    AccountID accountID;
    auto jvAccepted = RPC::accountFromString (accountID, strIdent, bStrict);

    if (jvAccepted)
        return jvAccepted;

    context.loadType = Resource::feeHighBurdenRPC;

    result[jss::account] = context.app.accountIDCache().toBase58 (accountID);

//分析指定的热钱包（如果有）
    std::set <AccountID> hotWallets;

    if (params.isMember (jss::hotwallet))
    {

        auto addHotWallet = [&hotWallets](Json::Value const& j)
        {
            if (j.isString())
            {
                auto const pk = parseBase58<PublicKey>(
                    TokenType::AccountPublic,
                    j.asString ());
                if (pk)
                {
                    hotWallets.insert(calcAccountID(*pk));
                    return true;
                }

                auto const id = parseBase58<AccountID>(j.asString());

                if (id)
                {
                    hotWallets.insert(*id);
                    return true;
                }
            }

            return false;
        };

        Json::Value const& hw = params[jss::hotwallet];
        bool valid = true;

//空值被视为有效的0大小的HotWallet数组
        if (hw.isArrayOrNull())
        {
            for (unsigned i = 0; i < hw.size(); ++i)
                valid &= addHotWallet (hw[i]);
        }
        else if (hw.isString())
        {
            valid &= addHotWallet (hw);
        }
        else
        {
            valid = false;
        }

        if (! valid)
        {
            result[jss::error]   = "invalidHotWallet";
            return result;
        }

    }

    std::map <Currency, STAmount> sums;
    std::map <AccountID, std::vector <STAmount>> hotBalances;
    std::map <AccountID, std::vector <STAmount>> assets;
    std::map <AccountID, std::vector <STAmount>> frozenBalances;

//穿过冷钱包的信任线
    {
        forEachItem(*ledger, accountID,
            [&](std::shared_ptr<SLE const> const& sle)
            {
                auto rs = RippleState::makeItem (accountID, sle);

                if (!rs)
                    return;

                int balSign = rs->getBalance().signum();
                if (balSign == 0)
                    return;

                auto const& peer = rs->getAccountIDPeer();

//这里，负余额意味着冷钱包欠（正常）
//正余额意味着冷钱包有资产（不寻常）

                if (hotWallets.count (peer) > 0)
                {
//这是一个特别的热钱包
                    hotBalances[peer].push_back (-rs->getBalance ());
                }
                else if (balSign > 0)
                {
//这是一个网关资产
                    assets[peer].push_back (rs->getBalance ());
                }
                else if (rs->getFreeze())
                {
//关口冻结的一项义务
                    frozenBalances[peer].push_back (-rs->getBalance ());
                }
                else
                {
//正常负余额，对客户的义务
                    auto& bal = sums[rs->getBalance().getCurrency()];
                    if (bal == beast::zero)
                    {
//这是正确设置货币代码所必需的
                        bal = -rs->getBalance();
                    }
                    else
                        bal -= rs->getBalance();
                }
            });
    }

    if (! sums.empty())
    {
        Json::Value j;
        for (auto const& e : sums)
        {
            j[to_string (e.first)] = e.second.getText ();
        }
        result [jss::obligations] = std::move (j);
    }

    auto populate = [](
        std::map <AccountID, std::vector <STAmount>> const& array,
        Json::Value& result,
        Json::StaticString const& name)
        {
            if (!array.empty())
            {
                Json::Value j;
                for (auto const& account : array)
                {
                    Json::Value balanceArray;
                    for (auto const& balance : account.second)
                    {
                        Json::Value entry;
                        entry[jss::currency] = to_string (balance.issue ().currency);
                        entry[jss::value] = balance.getText();
                        balanceArray.append (std::move (entry));
                    }
                    j [to_string (account.first)] = std::move (balanceArray);
                }
                result [name] = std::move (j);
            }
        };

    populate (hotBalances, result, jss::balances);
    populate (frozenBalances, result, jss::frozen_balances);
    populate (assets, result, jss::assets);

    return result;
}

} //涟漪
