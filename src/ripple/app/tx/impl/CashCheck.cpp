
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
    版权所有（c）2017 Ripple Labs Inc.

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

#include <ripple/app/tx/impl/CashCheck.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/paths/Flow.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/TxFlags.h>

#include <algorithm>

namespace ripple {

NotTEC
CashCheck::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled (featureChecks))
        return temDISABLED;

    NotTEC const ret {preflight1 (ctx)};
    if (! isTesSuccess (ret))
        return ret;

    if (ctx.tx.getFlags() & tfUniversalMask)
    {
//目前还没有现金支票的标志（通用标志除外）。
        JLOG(ctx.j.warn()) << "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

//必须正好存在金额或delivermin中的一个。
    auto const optAmount = ctx.tx[~sfAmount];
    auto const optDeliverMin = ctx.tx[~sfDeliverMin];

    if (static_cast<bool>(optAmount) == static_cast<bool>(optDeliverMin))
    {
        JLOG(ctx.j.warn()) << "Malformed transaction: "
            "does not specify exactly one of Amount and DeliverMin.";
        return temMALFORMED;
    }

//确保金额有效。
    STAmount const value {optAmount ? *optAmount : *optDeliverMin};
    if (!isLegalNet (value) || value.signum() <= 0)
    {
        JLOG(ctx.j.warn()) << "Malformed transaction: bad amount: "
            << value.getFullText();
        return temBAD_AMOUNT;
    }

    if (badCurrency() == value.getCurrency())
    {
        JLOG(ctx.j.warn()) <<"Malformed transaction: Bad currency.";
        return temBAD_CURRENCY;
    }

    return preflight2 (ctx);
}

TER
CashCheck::preclaim (PreclaimContext const& ctx)
{
    auto const sleCheck = ctx.view.read (keylet::check (ctx.tx[sfCheckID]));
    if (! sleCheck)
    {
        JLOG(ctx.j.warn()) << "Check does not exist.";
        return tecNO_ENTRY;
    }

//只兑现以这个帐户为目的地的支票。
    AccountID const dstId {(*sleCheck)[sfDestination]};
    if (ctx.tx[sfAccount] != dstId)
    {
        JLOG(ctx.j.warn()) << "Cashing a check with wrong Destination.";
        return tecNO_PERMISSION;
    }
    AccountID const srcId {(*sleCheck)[sfAccount]};
    if (srcId == dstId)
    {
//他们给自己开了一张支票。这个应该在什么时候抓到
//支票已创建，但迟交总比不交好。
        JLOG(ctx.j.error()) << "Malformed transaction: Cashing check to self.";
        return tecINTERNAL;
    }
    {
        auto const sleSrc = ctx.view.read (keylet::account (srcId));
        auto const sleDst = ctx.view.read (keylet::account (dstId));
        if (!sleSrc || !sleDst)
        {
//如果检查存在，则不应发生这种情况。
            JLOG(ctx.j.warn())
                << "Malformed transaction: source or destination not in ledger";
            return tecNO_ENTRY;
        }

        if ((sleDst->getFlags() & lsfRequireDestTag) &&
            !sleCheck->isFieldPresent (sfDestinationTag))
        {
//标签基本上是特定于帐户的信息，我们没有
//明白，但我们可以要求有人填写。
            JLOG(ctx.j.warn())
                << "Malformed transaction: DestinationTag required in check.";
            return tecDST_TAG_NEEDED;
        }
    }
    {
        using duration = NetClock::duration;
        using timepoint = NetClock::time_point;
        auto const optExpiry = (*sleCheck)[~sfExpiration];

//到期是根据父级的关闭时间定义的。
//Ledger，因为我们明确知道它关闭的时间，但是
//我们不知道下面的分类帐的结算时间
//建设。
        if (optExpiry &&
            (ctx.view.parentCloseTime() >= timepoint {duration {*optExpiry}}))
        {
            JLOG(ctx.j.warn()) << "Cashing a check that has already expired.";
            return tecEXPIRED;
        }
    }
    {
//飞行前确认确实存在一个数量或交付时间。
//确保要求的金额合理。
        STAmount const value {[] (STTx const& tx)
            {
                auto const optAmount = tx[~sfAmount];
                return optAmount ? *optAmount : tx[sfDeliverMin];
            } (ctx.tx)};

        STAmount const sendMax {(*sleCheck)[sfSendMax]};
        Currency const currency {value.getCurrency()};
        if (currency != sendMax.getCurrency())
        {
            JLOG(ctx.j.warn()) << "Check cash does not match check currency.";
            return temMALFORMED;
        }
        AccountID const issuerId {value.getIssuer()};
        if (issuerId != sendMax.getIssuer())
        {
            JLOG(ctx.j.warn()) << "Check cash does not match check issuer.";
            return temMALFORMED;
        }
        if (value > sendMax)
        {
            JLOG(ctx.j.warn()) << "Check cashed for more than check sendMax.";
            return tecPATH_PARTIAL;
        }

//确保检查所有者至少持有值。如果他们有
//低于该值，支票无法兑现。
        {
            STAmount availableFunds {accountFunds (ctx.view,
                (*sleCheck)[sfAccount], value, fhZERO_IF_FROZEN, ctx.j)};

//请注意，SRC将拥有一个额外的XRP储备价值。
//一旦支票兑现，因为支票的储备金将
//需要更长的时间。所以，如果我们在做XRP，我们就加一个
//准备金对可用资金的价值。
            if (value.native())
                availableFunds += XRPAmount (ctx.view.fees().increment);

            if (value > availableFunds)
            {
                JLOG(ctx.j.warn())
                    << "Check cashed for more than owner's balance.";
                return tecPATH_PARTIAL;
            }
        }

//发行人总是可以接受自己的货币。
        if (! value.native() && (value.getIssuer() != dstId))
        {
            auto const sleTrustLine = ctx.view.read (
                keylet::line (dstId, issuerId, currency));
            if (! sleTrustLine)
            {
                JLOG(ctx.j.warn())
                    << "Cannot cash check for IOU without trustline.";
                return tecNO_LINE;
            }

            auto const sleIssuer = ctx.view.read (keylet::account (issuerId));
            if (! sleIssuer)
            {
                JLOG(ctx.j.warn())
                    << "Can't receive IOUs from non-existent issuer: "
                    << to_string (issuerId);
                return tecNO_ISSUER;
            }

            if ((*sleIssuer)[sfFlags] & lsfRequireAuth)
            {
//条目具有规范表示，由
//字典编纂的“大于”比较采用严格的
//弱序确定我们需要访问哪个条目。
                bool const canonical_gt (dstId > issuerId);

                bool const is_authorized ((*sleTrustLine)[sfFlags] &
                    (canonical_gt ? lsfLowAuth : lsfHighAuth));

                if (! is_authorized)
                {
                    JLOG(ctx.j.warn())
                        << "Can't receive IOUs from issuer without auth.";
                    return tecNO_AUTH;
                }
            }

//从源到颁发者的信任线不需要
//因为我们已经证实
//来源有足够的非冻结资金可用。

//但是，从目的地到颁发者的信任线可能不会
//被冻僵了。
            if (isFrozen (ctx.view, dstId, currency, issuerId))
            {
                JLOG(ctx.j.warn())
                    << "Cashing a check to a frozen trustline.";
                return tecFROZEN;
            }
        }
    }
    return tesSUCCESS;
}

TER
CashCheck::doApply ()
{
//流要求我们在PaymentSandbox上操作，而不是
//直接在视图上。
    PaymentSandbox psb (&ctx_.view());

    uint256 const checkKey {ctx_.tx[sfCheckID]};
    auto const sleCheck = psb.peek (keylet::check (checkKey));
    if (! sleCheck)
    {
        JLOG(j_.fatal())
            << "Precheck did not verify check's existence.";
        return tecFAILED_PROCESSING;
    }

    AccountID const srcId {sleCheck->getAccountID (sfAccount)};
    auto const sleSrc = psb.peek (keylet::account (srcId));
    auto const sleDst = psb.peek (keylet::account (account_));

    if (!sleSrc || !sleDst)
    {
        JLOG(ctx_.journal.fatal())
            << "Precheck did not verify source or destination's existence.";
        return tecFAILED_PROCESSING;
    }

//预索赔已检查来源至少有
//基金。
//
//因此，如果这是一张写给自己的支票，（不应该是）
//我们知道他们有足够的资金支付支票。既然它们是
//从他们自己的口袋里把钱放回去
//口袋里的东西不会改变。
//
//如果这不是对自己的检查（应该是这样），那么
//工作要做…
    auto viewJ = ctx_.app.journal ("View");
    auto const optDeliverMin = ctx_.tx[~sfDeliverMin];
    bool const doFix1623 {ctx_.view().rules().enabled (fix1623)};
    if (srcId != account_)
    {
        STAmount const sendMax {sleCheck->getFieldAmount (sfSendMax)};

//流（）不执行从xrp到xrp的传输。
        if (sendMax.native())
        {
//这里我们需要计算xrp slesrc可以发送的数量。
//他们的可用金额是他们的余额减去他们的
//准备金。
//
//因为（如果我们成功的话）我们要删除一个条目
//从SRC的目录，我们允许他们发送额外的
//转账中的增量准备金金额。因此-- 1
//争论。
            STAmount const srcLiquid {xrpLiquid (psb, srcId, -1, viewJ)};

//现在，他们需要多少才能成功？
            STAmount const xrpDeliver {optDeliverMin ?
                std::max (*optDeliverMin, std::min (sendMax, srcLiquid)) :
                ctx_.tx.getFieldAmount (sfAmount)};

            if (srcLiquid < xrpDeliver)
            {
//投反对票。但是，如果适用，交易可能会成功。
//以不同的顺序。
                JLOG(j_.trace()) << "Cash Check: Insufficient XRP: "
                    << srcLiquid.getFullText()
                    << " < " << xrpDeliver.getFullText();
                return tecUNFUNDED_PAYMENT;
            }

            if (optDeliverMin && doFix1623)
//设置DeliveredAmount元数据。
                ctx_.deliver (xrpDeliver);

//源帐户有足够的XRP，因此请更改分类帐。
            transferXRP (psb, srcId, account_, xrpDeliver, viewJ);
        }
        else
        {
//让Flow（）在检查IOU时执行重载提升。
//
//注意，对于delivermin，我们不知道具体有多少
//我们希望流通的货币。我们不能要求
//最大可能货币，因为可能存在网关
//将利率转移到帐户。因为转移率不能
//超过200%，我们使用1/2最大值作为限制。
            STAmount const flowDeliver {optDeliverMin ?
                STAmount { optDeliverMin->issue(),
                    STAmount::cMaxValue / 2, STAmount::cMaxOffset } :
                static_cast<STAmount>(ctx_.tx[sfAmount])};

//调用支付引擎的流（）来完成实际工作。
            auto const result = flow (psb, flowDeliver, srcId, account_,
                STPathSet{},
true,                              //缺省路径
static_cast<bool>(optDeliverMin),  //部分支付
true,                              //业主支付转让费
false,                             //要约路口
                boost::none,
                sleCheck->getFieldAmount (sfSendMax),
                viewJ);

            if (result.result() != tesSUCCESS)
            {
                JLOG(ctx_.journal.warn())
                    << "flow failed when cashing check.";
                return result.result();
            }

//确保delivermin满意。
            if (optDeliverMin)
            {
                if (result.actualAmountOut < *optDeliverMin)
                {
                    JLOG(ctx_.journal.warn()) << "flow did not produce DeliverMin.";
                    return tecPATH_PARTIAL;
                }
                if (doFix1623)
//设置已交付的金额元数据。
                    ctx_.deliver (result.actualAmountOut);
            }
        }
    }

//支票已兑现。如果不是自动发送（不应该是），请删除
//检查目标目录的链接。
    if (srcId != account_)
    {
        std::uint64_t const page {(*sleCheck)[sfDestinationNode]};
        if (! ctx_.view().dirRemove(
                keylet::ownerDir(account_), page, checkKey, true))
        {
            JLOG(j_.warn()) << "Unable to delete check from destination.";
            return tefBAD_LEDGER;
        }
    }
//从检查所有者目录中删除检查。
    {
        std::uint64_t const page {(*sleCheck)[sfOwnerNode]};
        if (! ctx_.view().dirRemove(
                keylet::ownerDir(srcId), page, checkKey, true))
        {
            JLOG(j_.warn()) << "Unable to delete check from owner.";
            return tefBAD_LEDGER;
        }
    }
//如果成功，请更新支票所有者的保留。
    adjustOwnerCount (psb, sleSrc, -1, viewJ);

//从分类帐中删除支票。
    psb.erase (sleCheck);

    psb.apply (ctx_.rawView());
    return tesSUCCESS;
}

} //命名空间波纹
