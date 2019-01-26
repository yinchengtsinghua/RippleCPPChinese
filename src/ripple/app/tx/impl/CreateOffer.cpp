
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

#include <ripple/app/tx/impl/CreateOffer.h>
#include <ripple/app/ledger/OrderBookDB.h>
#include <ripple/app/paths/Flow.h>
#include <ripple/ledger/CashDiff.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/Quality.h>
#include <ripple/beast/utility/WrappedSink.h>

namespace ripple {

XRPAmount
CreateOffer::calculateMaxSpend(STTx const& tx)
{
    auto const& saTakerGets = tx[sfTakerGets];

    return saTakerGets.native() ?
        saTakerGets.xrp() : beast::zero;
}

NotTEC
CreateOffer::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto& tx = ctx.tx;
    auto& j = ctx.j;

    std::uint32_t const uTxFlags = tx.getFlags ();

    if (uTxFlags & tfOfferCreateMask)
    {
        JLOG(j.debug()) <<
            "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

    bool const bImmediateOrCancel (uTxFlags & tfImmediateOrCancel);
    bool const bFillOrKill (uTxFlags & tfFillOrKill);

    if (bImmediateOrCancel && bFillOrKill)
    {
        JLOG(j.debug()) <<
            "Malformed transaction: both IoC and FoK set.";
        return temINVALID_FLAG;
    }

    bool const bHaveExpiration (tx.isFieldPresent (sfExpiration));

    if (bHaveExpiration && (tx.getFieldU32 (sfExpiration) == 0))
    {
        JLOG(j.debug()) <<
            "Malformed offer: bad expiration";
        return temBAD_EXPIRATION;
    }

    bool const bHaveCancel (tx.isFieldPresent (sfOfferSequence));

    if (bHaveCancel && (tx.getFieldU32 (sfOfferSequence) == 0))
    {
        JLOG(j.debug()) <<
            "Malformed offer: bad cancel sequence";
        return temBAD_SEQUENCE;
    }

    STAmount saTakerPays = tx[sfTakerPays];
    STAmount saTakerGets = tx[sfTakerGets];

    if (!isLegalNet (saTakerPays) || !isLegalNet (saTakerGets))
        return temBAD_AMOUNT;

    if (saTakerPays.native () && saTakerGets.native ())
    {
        JLOG(j.debug()) <<
            "Malformed offer: redundant (XRP for XRP)";
        return temBAD_OFFER;
    }
    if (saTakerPays <= beast::zero || saTakerGets <= beast::zero)
    {
        JLOG(j.debug()) <<
            "Malformed offer: bad amount";
        return temBAD_OFFER;
    }

    auto const& uPaysIssuerID = saTakerPays.getIssuer ();
    auto const& uPaysCurrency = saTakerPays.getCurrency ();

    auto const& uGetsIssuerID = saTakerGets.getIssuer ();
    auto const& uGetsCurrency = saTakerGets.getCurrency ();

    if (uPaysCurrency == uGetsCurrency && uPaysIssuerID == uGetsIssuerID)
    {
        JLOG(j.debug()) <<
            "Malformed offer: redundant (IOU for IOU)";
        return temREDUNDANT;
    }
//我们不允许非本国货币使用货币代码XRP。
    if (badCurrency() == uPaysCurrency || badCurrency() == uGetsCurrency)
    {
        JLOG(j.debug()) <<
            "Malformed offer: bad currency";
        return temBAD_CURRENCY;
    }

    if (saTakerPays.native () != !uPaysIssuerID ||
        saTakerGets.native () != !uGetsIssuerID)
    {
        JLOG(j.warn()) <<
            "Malformed offer: bad issuer";
        return temBAD_ISSUER;
    }

    return preflight2(ctx);
}

TER
CreateOffer::preclaim(PreclaimContext const& ctx)
{
    auto const id = ctx.tx[sfAccount];

    auto saTakerPays = ctx.tx[sfTakerPays];
    auto saTakerGets = ctx.tx[sfTakerGets];

    auto const& uPaysIssuerID = saTakerPays.getIssuer();
    auto const& uPaysCurrency = saTakerPays.getCurrency();

    auto const& uGetsIssuerID = saTakerGets.getIssuer();

    auto const cancelSequence = ctx.tx[~sfOfferSequence];

    auto const sleCreator = ctx.view.read(keylet::account(id));

    std::uint32_t const uAccountSequence = sleCreator->getFieldU32(sfSequence);

    auto viewJ = ctx.app.journal("View");

    if (isGlobalFrozen(ctx.view, uPaysIssuerID) ||
        isGlobalFrozen(ctx.view, uGetsIssuerID))
    {
        JLOG(ctx.j.info()) <<
            "Offer involves frozen asset";

        return tecFROZEN;
    }
    else if (accountFunds(ctx.view, id, saTakerGets,
        fhZERO_IF_FROZEN, viewJ) <= beast::zero)
    {
        JLOG(ctx.j.debug()) <<
            "delay: Offers must be at least partially funded.";

        return tecUNFUNDED_OFFER;
    }
//这可以简化以确保取消序列
//在事务序列号之前。
    else if (cancelSequence && (uAccountSequence <= *cancelSequence))
    {
        JLOG(ctx.j.debug()) <<
            "uAccountSequenceNext=" << uAccountSequence <<
            " uOfferSequence=" << *cancelSequence;

        return temBAD_SEQUENCE;
    }

    using d = NetClock::duration;
    using tp = NetClock::time_point;
    auto const expiration = ctx.tx[~sfExpiration];

//到期是根据父分类账的关闭时间定义的，
//因为我们确实知道它关闭的时间，但我们不知道
//了解正在施工的分类帐的结算时间。
    if (expiration &&
        (ctx.view.parentCloseTime() >= tp{d{*expiration}}))
    {
//请注意，这将在applyguts中再次检查，但它会保存
//我们打电话来检查接受情况，可能是假阴性。
//
//为了方便起见，将返回代码更改附加到功能检查中。
//这一变化不够大，不值得自己进行修正。
        return ctx.view.rules().enabled(featureDepositPreauth)
            ? TER {tecEXPIRED}
            : TER {tesSUCCESS};
    }

//确保我们有权持有承购人将支付给我们的款项。
    if (!saTakerPays.native())
    {
        auto result = checkAcceptAsset(ctx.view, ctx.flags,
            id, ctx.j, Issue(uPaysCurrency, uPaysIssuerID));
        if (result != tesSUCCESS)
            return result;
    }

    return tesSUCCESS;
}

TER
CreateOffer::checkAcceptAsset(ReadView const& view,
    ApplyFlags const flags, AccountID const id,
        beast::Journal const j, Issue const& issue)
{
//仅对自定义货币有效
    assert (!isXRP (issue.currency));

    auto const issuerAccount = view.read(
        keylet::account(issue.account));

    if (!issuerAccount)
    {
        JLOG(j.warn()) <<
            "delay: can't receive IOUs from non-existent issuer: " <<
            to_string (issue.account);

        return (flags & tapRETRY)
            ? TER {terNO_ACCOUNT}
            : TER {tecNO_ISSUER};
    }

//本规范附于《存款授权修正案》，内容如下：
//方便。这一变化不够显著，不值得
//自己的修正案。
    if (view.rules().enabled(featureDepositPreauth) && (issue.account == id))
//帐户总是可以接受自己的发行。
        return tesSUCCESS;

    if ((*issuerAccount)[sfFlags] & lsfRequireAuth)
    {
        auto const trustLine = view.read(
            keylet::line(id, issue.account, issue.currency));

        if (!trustLine)
        {
            return (flags & tapRETRY)
                ? TER {terNO_LINE}
                : TER {tecNO_LINE};
        }

//条目具有规范表示，由
//采用严格弱比较法的词典编纂“大于”比较
//排序。确定我们需要访问哪个条目。
        bool const canonical_gt (id > issue.account);

        bool const is_authorized ((*trustLine)[sfFlags] &
            (canonical_gt ? lsfLowAuth : lsfHighAuth));

        if (!is_authorized)
        {
            JLOG(j.debug()) <<
                "delay: can't receive IOUs from issuer without auth.";

            return (flags & tapRETRY)
                ? TER {terNO_AUTH}
                : TER {tecNO_AUTH};
        }
    }

    return tesSUCCESS;
}

bool
CreateOffer::dry_offer (ApplyView& view, Offer const& offer)
{
    if (offer.fully_consumed ())
        return true;
    auto const amount = accountFunds(view, offer.owner(),
        offer.amount().out, fhZERO_IF_FROZEN, ctx_.app.journal ("View"));
    return (amount <= beast::zero);
}

std::pair<bool, Quality>
CreateOffer::select_path (
    bool have_direct, OfferStream const& direct,
    bool have_bridge, OfferStream const& leg1, OfferStream const& leg2)
{
//如果我们没有任何可行的途径，为什么我们在这里？！
    assert (have_direct || have_bridge);

//如果没有桥接路径，则默认情况下直接路径是最佳路径。
    if (!have_bridge)
        return std::make_pair (true, direct.tip ().quality ());

    Quality const bridged_quality (composed_quality (
        leg1.tip ().quality (), leg2.tip ().quality ()));

    if (have_direct)
    {
//我们比较了桥的组成质量
//报价，并将其与直接报价进行比较，以选出最佳报价。
        Quality const direct_quality (direct.tip ().quality ());

        if (bridged_quality < direct_quality)
            return std::make_pair (true, direct_quality);
    }

//要么没有直接报价，要么质量不好
//而不是桥。
    return std::make_pair (false, bridged_quality);
}

bool
CreateOffer::reachedOfferCrossingLimit (Taker const& taker) const
{
    auto const crossings =
        taker.get_direct_crossings () +
        (2 * taker.get_bridge_crossings ());

//交叉限制是Ripple协议的一部分，并且
//更改它是事务处理更改。
    return crossings >= 850;
}

std::pair<TER, Amounts>
CreateOffer::bridged_cross (
    Taker& taker,
    ApplyView& view,
    ApplyView& view_cancel,
    NetClock::time_point const when)
{
    auto const& takerAmount = taker.original_offer ();

    assert (!isXRP (takerAmount.in) && !isXRP (takerAmount.out));

    if (isXRP (takerAmount.in) || isXRP (takerAmount.out))
        Throw<std::logic_error> ("Bridging with XRP and an endpoint.");

    OfferStream offers_direct (view, view_cancel,
        Book (taker.issue_in (), taker.issue_out ()),
            when, stepCounter_, j_);

    OfferStream offers_leg1 (view, view_cancel,
        Book (taker.issue_in (), xrpIssue ()),
        when, stepCounter_, j_);

    OfferStream offers_leg2 (view, view_cancel,
        Book (xrpIssue (), taker.issue_out ()),
        when, stepCounter_, j_);

    TER cross_result = tesSUCCESS;

//注意这里的细微区别：在
//桥牌被抢走了，但在直书中遇到了自我报价。
//不是。
    bool have_bridge = offers_leg1.step () && offers_leg2.step ();
    bool have_direct = step_account (offers_direct, taker);
    int count = 0;

    auto viewJ = ctx_.app.journal ("View");

//修改循环中操作的顺序或逻辑将导致
//破坏协议的变化。
    while (have_direct || have_bridge)
    {
        bool leg1_consumed = false;
        bool leg2_consumed = false;
        bool direct_consumed = false;

        Quality quality;
        bool use_direct;

        std::tie (use_direct, quality) = select_path (
            have_direct, offers_direct,
            have_bridge, offers_leg1, offers_leg2);


//我们一直在寻找最好的质量；我们已经完成了
//一旦我们越过质量界限就要穿过。
        if (taker.reject(quality))
            break;

        count++;

        if (use_direct)
        {
            if (auto stream = j_.debug())
            {
                stream << count << " Direct:";
                stream << "  offer: " << offers_direct.tip ();
                stream << "     in: " << offers_direct.tip ().amount().in;
                stream << "    out: " << offers_direct.tip ().amount ().out;
                stream << "  owner: " << offers_direct.tip ().owner ();
                stream << "  funds: " << accountFunds(view,
                    offers_direct.tip ().owner (),
                    offers_direct.tip ().amount ().out,
                    fhIGNORE_FREEZE, viewJ);
            }

            cross_result = taker.cross(offers_direct.tip ());

            JLOG (j_.debug()) << "Direct Result: " << transToken (cross_result);

            if (dry_offer (view, offers_direct.tip ()))
            {
                direct_consumed = true;
                have_direct = step_account (offers_direct, taker);
            }
        }
        else
        {
            if (auto stream = j_.debug())
            {
                auto const owner1_funds_before = accountFunds(view,
                    offers_leg1.tip ().owner (),
                    offers_leg1.tip ().amount ().out,
                    fhIGNORE_FREEZE, viewJ);

                auto const owner2_funds_before = accountFunds(view,
                    offers_leg2.tip ().owner (),
                    offers_leg2.tip ().amount ().out,
                    fhIGNORE_FREEZE, viewJ);

                stream << count << " Bridge:";
                stream << " offer1: " << offers_leg1.tip ();
                stream << "     in: " << offers_leg1.tip ().amount().in;
                stream << "    out: " << offers_leg1.tip ().amount ().out;
                stream << "  owner: " << offers_leg1.tip ().owner ();
                stream << "  funds: " << owner1_funds_before;
                stream << " offer2: " << offers_leg2.tip ();
                stream << "     in: " << offers_leg2.tip ().amount ().in;
                stream << "    out: " << offers_leg2.tip ().amount ().out;
                stream << "  owner: " << offers_leg2.tip ().owner ();
                stream << "  funds: " << owner2_funds_before;
            }

            cross_result = taker.cross(offers_leg1.tip (), offers_leg2.tip ());

            JLOG (j_.debug()) << "Bridge Result: " << transToken (cross_result);

            if (view.rules().enabled (fixTakerDryOfferRemoval))
            {
//只有在下一轮比赛中
//两种产品都不干燥。
                leg1_consumed = dry_offer (view, offers_leg1.tip());
                if (leg1_consumed)
                    have_bridge &= offers_leg1.step();

                leg2_consumed = dry_offer (view, offers_leg2.tip());
                if (leg2_consumed)
                    have_bridge &= offers_leg2.step();
            }
            else
            {
//这种老行为可能会在书中留下一个空提议
//第二回合。
                if (dry_offer (view, offers_leg1.tip ()))
                {
                    leg1_consumed = true;
                    have_bridge = (have_bridge && offers_leg1.step ());
                }
                if (dry_offer (view, offers_leg2.tip ()))
                {
                    leg2_consumed = true;
                    have_bridge = (have_bridge && offers_leg2.step ());
                }
            }
        }

        if (cross_result != tesSUCCESS)
        {
            cross_result = tecFAILED_PROCESSING;
            break;
        }

        if (taker.done())
        {
            JLOG(j_.debug()) << "The taker reports he's done during crossing!";
            break;
        }

        if (reachedOfferCrossingLimit (taker))
        {
            JLOG(j_.debug()) << "The offer crossing limit has been exceeded!";
            break;
        }

//后置条件：如果我们不这样做，那么我们肯定在
//至少有一个报价是完全的。
        assert (direct_consumed || leg1_consumed || leg2_consumed);

        if (!direct_consumed && !leg1_consumed && !leg2_consumed)
            Throw<std::logic_error> ("bridged crossing: nothing was fully consumed.");
    }

    return std::make_pair(cross_result, taker.remaining_offer ());
}

std::pair<TER, Amounts>
CreateOffer::direct_cross (
    Taker& taker,
    ApplyView& view,
    ApplyView& view_cancel,
    NetClock::time_point const when)
{
    OfferStream offers (
        view, view_cancel,
        Book (taker.issue_in (), taker.issue_out ()),
        when, stepCounter_, j_);

    TER cross_result (tesSUCCESS);
    int count = 0;

    bool have_offer = step_account (offers, taker);

//修改循环中操作的顺序或逻辑将导致
//破坏协议的变化。
    while (have_offer)
    {
        bool direct_consumed = false;
        auto& offer (offers.tip());

//一旦越过质量界限，我们就完成了跨越。
        if (taker.reject (offer.quality()))
            break;

        count++;

        if (auto stream = j_.debug())
        {
            stream << count << " Direct:";
            stream << "  offer: " << offer;
            stream << "     in: " << offer.amount ().in;
            stream << "    out: " << offer.amount ().out;
            stream << "quality: " << offer.quality();
            stream << "  owner: " << offer.owner ();
            stream << "  funds: " << accountFunds(view,
                offer.owner (), offer.amount ().out, fhIGNORE_FREEZE,
                ctx_.app.journal ("View"));
        }

        cross_result = taker.cross (offer);

        JLOG (j_.debug()) << "Direct Result: " << transToken (cross_result);

        if (dry_offer (view, offer))
        {
            direct_consumed = true;
            have_offer = step_account (offers, taker);
        }

        if (cross_result != tesSUCCESS)
        {
            cross_result = tecFAILED_PROCESSING;
            break;
        }

        if (taker.done())
        {
            JLOG(j_.debug()) << "The taker reports he's done during crossing!";
            break;
        }

        if (reachedOfferCrossingLimit (taker))
        {
            JLOG(j_.debug()) << "The offer crossing limit has been exceeded!";
            break;
        }

//后置条件：如果我们不这样做，那么我们*一定*消耗了
//全力以赴！
        assert (direct_consumed);

        if (!direct_consumed)
            Throw<std::logic_error> ("direct crossing: nothing was fully consumed.");
    }

    return std::make_pair(cross_result, taker.remaining_offer ());
}

//越走越长越好，跳过任何报价
//这是从接受者或跨越接受者的门槛。
//如果书中没有报价，则返回“假”，否则返回“真”。
bool
CreateOffer::step_account (OfferStream& stream, Taker const& taker)
{
    while (stream.step ())
    {
        auto const& offer = stream.tip ();

//小费处的出价超过了接受方的门槛。我们完了。
        if (taker.reject (offer.quality ()))
            return true;

//小费上的报价不是由承购人提供的。我们完了。
        if (offer.owner () != taker.account ())
            return true;
    }

//我们没有报价了。不能前进。
    return false;
}

//通过消费优惠尽可能多地填写优惠
//已经在书上了。返回状态和
//对左派的提议还没有完成。
std::pair<TER, Amounts>
CreateOffer::takerCross (
    Sandbox& sb,
    Sandbox& sbCancel,
    Amounts const& takerAmount)
{
    NetClock::time_point const when{ctx_.view().parentCloseTime()};

    beast::WrappedSink takerSink (j_, "Taker ");

    Taker taker (cross_type_, sb, account_, takerAmount,
        ctx_.tx.getFlags(), beast::Journal (takerSink));

//如果接线员在我们开始穿越前没有资金
//没什么可做的-只是返回一个错误。
//
//我们在preclaim中检查这个，但是在销售xrp时
//收费可能导致用户的可用余额
//转到0（使其低于储备）
//所以我们再次检查这个案子。
    if (taker.unfunded ())
    {
        JLOG (j_.debug()) <<
            "Not crossing: taker is unfunded.";
        return { tecUNFUNDED_OFFER, takerAmount };
    }

    try
    {
        if (cross_type_ == CrossType::IouToIou)
            return bridged_cross (taker, sb, sbCancel, when);

        return direct_cross (taker, sb, sbCancel, when);
    }
    catch (std::exception const& e)
    {
        JLOG (j_.error()) <<
            "Exception during offer crossing: " << e.what ();
        return { tecINTERNAL, taker.remaining_offer () };
    }
}

std::pair<TER, Amounts>
CreateOffer::flowCross (
    PaymentSandbox& psb,
    PaymentSandbox& psbCancel,
    Amounts const& takerAmount)
{
    try
    {
//如果接线员在我们开始穿越之前没有资金支持，那就什么也没有了。
//做-只返回一个错误。
//
//我们在预索赔中对此进行检查，但在销售XRP时，收费可以
//使用户的可用余额变为0（通过使其下降
//所以我们再次检查这个案子。
        STAmount const inStartBalance = accountFunds (
            psb, account_, takerAmount.in, fhZERO_IF_FROZEN, j_);
        if (inStartBalance <= beast::zero)
        {
//账户余额甚至不能支付部分报价。
            JLOG (j_.debug()) <<
                "Not crossing: taker is unfunded.";
            return { tecUNFUNDED_OFFER, takerAmount };
        }

//如果网关有传输速率，请考虑到这一点。这个
//网关未经
//提供接受者。设置sendmax以允许网关的切割。
        Rate gatewayXferRate {QUALITY_ONE};
        STAmount sendMax = takerAmount.in;
        if (! sendMax.native() && (account_ != sendMax.getIssuer()))
        {
            gatewayXferRate = transferRate (psb, sendMax.getIssuer());
            if (gatewayXferRate.value != QUALITY_ONE)
            {
                sendMax = multiplyRound (takerAmount.in,
                    gatewayXferRate, takerAmount.in.issue(), true);
            }
        }

//付款流代码比较传输率为
//包括。因为传输速率是合并的，所以计算阈值。
        Quality threshold { takerAmount.out, sendMax };

//如果我们创造一个被动的报价，调整门槛，这样我们
//交叉报价的质量比这个好。
        std::uint32_t const txFlags = ctx_.tx.getFlags();
        if (txFlags & tfPassive)
            ++threshold;

//不要超过我们的余额。
        if (sendMax > inStartBalance)
            sendMax = inStartBalance;

//始终使用默认路径调用Flow（）。但是，如果两者都没有
//在承购方金额中，货币是xrp，然后我们穿过
//以xrp作为两本书中间的附加路径。
//第二条道路我们必须建立自己。
        STPathSet paths;
        if (!takerAmount.in.native() & !takerAmount.out.native())
        {
            STPath path;
            path.emplace_back (boost::none, xrpCurrency(), boost::none);
            paths.emplace_back (std::move(path));
        }
//Tfsell标志的特殊处理。
        STAmount deliver = takerAmount.out;
        if (txFlags & tfSell)
        {
//我们正在销售，所以我们接受比报价更多的价格。
//明确规定。因为我们不知道他们能提供多少，
//我们允许尽可能多的交货。
            if (deliver.native())
                deliver = STAmount { STAmount::cMaxNative };
            else
//我们不能在这里使用最大可能的货币，因为
//帐户可能有网关传输率。
//由于传输率不能超过200%，我们使用1/2
//极限的最大值。
                deliver = STAmount { takerAmount.out.issue(),
                    STAmount::cMaxValue / 2, STAmount::cMaxOffset };
        }

//调用支付引擎的流（）来完成实际工作。
        auto const result = flow (psb, deliver, account_, account_,
            paths,
true,                       //缺省路径
! (txFlags & tfFillOrKill), //部分支付
true,                       //业主支付转让费
true,                       //要约路口
            threshold,
            sendMax, j_);

//如果发现陈旧的报价，将其删除。
        for (auto const& toRemove : result.removableOffers)
        {
            if (auto otr = psb.peek (keylet::offer (toRemove)))
                offerDelete (psb, otr, j_);
            if (auto otr = psbCancel.peek (keylet::offer (toRemove)))
                offerDelete (psbCancel, otr, j_);
        }

//穿越后确定最终报价的大小。
auto afterCross = takerAmount; //如果！Tesuccess优惠不变
        if (isTesSuccess (result.result()))
        {
            STAmount const takerInBalance = accountFunds (
                psb, account_, takerAmount.in, fhZERO_IF_FROZEN, j_);

            if (takerInBalance <= beast::zero)
            {
//如果报价已用完，账户资金不
//创建产品。
                afterCross.in.clear();
                afterCross.out.clear();
            }
            else
            {
                STAmount const rate {
                    Quality{takerAmount.out, takerAmount.in}.rate() };

                if (txFlags & tfSell)
                {
//如果出售，则根据
//我们在过境时卖了很多东西。这保留了报价
//质量，

//按划线金额减少报价。
//注意，我们必须忽略
//实际消耗量
//网关的传输速率。
                    STAmount nonGatewayAmountIn = result.actualAmountIn;
                    if (gatewayXferRate.value != QUALITY_ONE)
                        nonGatewayAmountIn = divideRound (result.actualAmountIn,
                            gatewayXferRate, takerAmount.in.issue(), true);

                    afterCross.in -= nonGatewayAmountIn;

//有可能是除法会导致减法
//稍微负一点。所以把aftercross.in限制为零。
                    if (afterCross.in < beast::zero)
//我们应该确认差异*很小，但是
//什么是好的检查门槛？
                        afterCross.in.clear();

                    afterCross.out = divRound (afterCross.in,
                        rate, takerAmount.out.issue(), true);
                }
                else
                {
//如果不出售，我们根据
//剩余输出。这也保留了报价
//质量。
                    afterCross.out -= result.actualAmountOut;
                    assert (afterCross.out >= beast::zero);
                    if (afterCross.out < beast::zero)
                        afterCross.out.clear();
                    afterCross.in = mulRound (afterCross.out,
                        rate, takerAmount.in.issue(), true);
                }
            }
        }

//返回还剩多少报价。
        return { tesSUCCESS, afterCross };
    }
    catch (std::exception const& e)
    {
        JLOG (j_.error()) <<
            "Exception during offer crossing: " << e.what ();
    }
    return { tecINTERNAL, takerAmount };
}

enum class SBoxCmp
{
    same,
    dustDiff,
    offerDelDiff,
    xrpRound,
    diff
};

static std::string to_string (SBoxCmp c)
{
    switch (c)
    {
    case SBoxCmp::same:
        return "same";
    case SBoxCmp::dustDiff:
        return "dust diffs";
    case SBoxCmp::offerDelDiff:
        return "offer del diffs";
    case SBoxCmp::xrpRound:
        return "XRP round to zero";
    case SBoxCmp::diff:
        return "different";
    }
    return {};
}

static SBoxCmp compareSandboxes (char const* name, ApplyContext const& ctx,
    detail::ApplyViewBase const& viewTaker, detail::ApplyViewBase const& viewFlow,
    beast::Journal j)
{
    SBoxCmp c = SBoxCmp::same;
    CashDiff diff = cashFlowDiff (
        CashFilter::treatZeroOfferAsDeletion, viewTaker,
        CashFilter::none, viewFlow);

    if (diff.hasDiff())
    {
        using namespace beast::severities;
//有一种特殊情况是，在一方提供XRP，其中
//xrp四舍五入为零。看起来像是灰尘
//差异。如果我们以前找过它，就更容易发现了
//消除灰尘差异。
        if (int const side = diff.xrpRoundToZero())
        {
            char const* const whichSide = side > 0 ? "; Flow" : "; Taker";
            j.stream (kWarning) << "FlowCross: " << name << " different" <<
                whichSide << " XRP rounded to zero.  tx: " <<
                ctx.tx.getTransactionID();
            return SBoxCmp::xrpRound;
        }

        c = SBoxCmp::dustDiff;
        Severity s = kInfo;
        std::string diffDesc = ", but only dust.";
        diff.rmDust();
        if (diff.hasDiff())
        {
//从这里开始，我们要注意不同的事务ID。
            std::stringstream txIdSs;
            txIdSs << ".  tx: " << ctx.tx.getTransactionID();
            auto txID = txIdSs.str();

//有时一个版本会删除另一个版本不提供的
//删除。没关系，但要注意。
            c = SBoxCmp::offerDelDiff;
            s = kWarning;
            int sides = diff.rmLhsDeletedOffers() ? 1 : 0;
            sides    |= diff.rmRhsDeletedOffers() ? 2 : 0;
            if (!diff.hasDiff())
            {
                char const* t = "";
                switch (sides)
                {
                case 1: t = "; Taker deleted more offers"; break;
                case 2: t = "; Flow deleted more offers"; break;
                case 3: t = "; Taker and Flow deleted different offers"; break;
                default: break;
                }
                diffDesc = std::string(t) + txID;
            }
            else
            {
//没有广泛分类的区别…
                c = SBoxCmp::diff;
                std::stringstream ss;
                ss << "; common entries: " << diff.commonCount()
                    << "; Taker unique: " << diff.lhsOnlyCount()
                    << "; Flow unique: " << diff.rhsOnlyCount() << txID;
                diffDesc = ss.str();
            }
        }
        j.stream (s) << "FlowCross: " << name << " different" << diffDesc;
    }
    return c;
}

std::pair<TER, Amounts>
CreateOffer::cross (
    Sandbox& sb,
    Sandbox& sbCancel,
    Amounts const& takerAmount)
{
    using beast::zero;

//流量提供交叉和比较结果有一些特点
//在接线员和水流之间提供交叉口。把它们变成酒。
    bool const useFlowCross { sb.rules().enabled (featureFlowCross) };
    bool const doCompare { sb.rules().enabled (featureCompareTakerFlowCross) };

    Sandbox sbTaker { &sb };
    Sandbox sbCancelTaker { &sbCancel };
    auto const takerR = (!useFlowCross || doCompare)
        ? takerCross (sbTaker, sbCancelTaker, takerAmount)
        : std::make_pair (tecINTERNAL, takerAmount);

    PaymentSandbox psbFlow { &sb };
    PaymentSandbox psbCancelFlow { &sbCancel };
    auto const flowR = (useFlowCross || doCompare)
        ? flowCross (psbFlow, psbCancelFlow, takerAmount)
        : std::make_pair (tecINTERNAL, takerAmount);

    if (doCompare)
    {
        SBoxCmp c = SBoxCmp::same;
        if (takerR.first != flowR.first)
        {
            c = SBoxCmp::diff;
            j_.warn() << "FlowCross: Offer cross tec codes different.  tx: "
                << ctx_.tx.getTransactionID();
        }
        else if ((takerR.second.in  == zero && flowR.second.in  == zero) ||
           (takerR.second.out == zero && flowR.second.out == zero))
        {
            c = compareSandboxes ("Both Taker and Flow fully crossed",
                ctx_, sbTaker, psbFlow, j_);
        }
        else if (takerR.second.in == zero && takerR.second.out == zero)
        {
            char const * crossType = "Taker fully crossed, Flow partially crossed";
            if (flowR.second.in == takerAmount.in &&
                flowR.second.out == takerAmount.out)
                    crossType = "Taker fully crossed, Flow not crossed";

            c = compareSandboxes (crossType, ctx_, sbTaker, psbFlow, j_);
        }
        else if (flowR.second.in == zero && flowR.second.out == zero)
        {
            char const * crossType =
                "Taker partially crossed, Flow fully crossed";
            if (takerR.second.in == takerAmount.in &&
                takerR.second.out == takerAmount.out)
                    crossType = "Taker not crossed, Flow fully crossed";

            c = compareSandboxes (crossType, ctx_, sbTaker, psbFlow, j_);
        }
        else if (ctx_.tx.getFlags() & tfFillOrKill)
        {
            c = compareSandboxes (
                "FillOrKill offer", ctx_, sbCancelTaker, psbCancelFlow, j_);
        }
        else if (takerR.second.in  == takerAmount.in &&
                 flowR.second.in   == takerAmount.in &&
                 takerR.second.out == takerAmount.out &&
                 flowR.second.out  == takerAmount.out)
        {
            char const * crossType = "Neither Taker nor Flow crossed";
            c = compareSandboxes (crossType, ctx_, sbTaker, psbFlow, j_);
        }
        else if (takerR.second.in == takerAmount.in &&
                 takerR.second.out == takerAmount.out)
        {
            char const * crossType = "Taker not crossed, Flow partially crossed";
            c = compareSandboxes (crossType, ctx_, sbTaker, psbFlow, j_);
        }
        else if (flowR.second.in == takerAmount.in &&
                 flowR.second.out == takerAmount.out)
        {
            char const * crossType = "Taker partially crossed, Flow not crossed";
            c = compareSandboxes (crossType, ctx_, sbTaker, psbFlow, j_);
        }
        else
        {
            c = compareSandboxes (
                "Partial cross offer", ctx_, sbTaker, psbFlow, j_);

//如果我们能做到这一点，那么返回的金额就很重要了。
            if (c <= SBoxCmp::dustDiff && takerR.second != flowR.second)
            {
                c = SBoxCmp::dustDiff;
                using namespace beast::severities;
                Severity s = kInfo;
                std::string onlyDust = ", but only dust.";
                if (! diffIsDust (takerR.second.in, flowR.second.in) ||
                    (! diffIsDust (takerR.second.out, flowR.second.out)))
                {
                    char const* outSame = "";
                    if (takerR.second.out == flowR.second.out)
                        outSame = " but outs same";

                    c = SBoxCmp::diff;
                    s = kWarning;
                    std::stringstream ss;
                    ss << outSame
                        << ".  Taker in: " << takerR.second.in.getText()
                        << "; Taker out: " << takerR.second.out.getText()
                        << "; Flow in: " << flowR.second.in.getText()
                        << "; Flow out: " << flowR.second.out.getText()
                        << ".  tx: " << ctx_.tx.getTransactionID();
                    onlyDust = ss.str();
                }
                j_.stream (s) << "FlowCross: Partial cross amounts different"
                    << onlyDust;
            }
        }
        j_.error() << "FlowCross cmp result: " << to_string (c);
    }

//基于修正返回一个结果或另一个结果。
    if (useFlowCross)
    {
        psbFlow.apply (sb);
        psbCancelFlow.apply (sbCancel);
        return flowR;
    }

    sbTaker.apply (sb);
    sbCancelTaker.apply (sbCancel);
    return takerR;
}

std::string
CreateOffer::format_amount (STAmount const& amount)
{
    std::string txt = amount.getText ();
    txt += "/";
    txt += to_string (amount.issue().currency);
    return txt;
}

void
CreateOffer::preCompute()
{
    cross_type_ = CrossType::IouToIou;
    bool const pays_xrp =
        ctx_.tx.getFieldAmount (sfTakerPays).native ();
    bool const gets_xrp =
        ctx_.tx.getFieldAmount (sfTakerGets).native ();
    if (pays_xrp && !gets_xrp)
        cross_type_ = CrossType::IouToXrp;
    else if (gets_xrp && !pays_xrp)
        cross_type_ = CrossType::XrpToIou;

    return Transactor::preCompute();
}

std::pair<TER, bool>
CreateOffer::applyGuts (Sandbox& sb, Sandbox& sbCancel)
{
    using beast::zero;

    std::uint32_t const uTxFlags = ctx_.tx.getFlags ();

    bool const bPassive (uTxFlags & tfPassive);
    bool const bImmediateOrCancel (uTxFlags & tfImmediateOrCancel);
    bool const bFillOrKill (uTxFlags & tfFillOrKill);
    bool const bSell (uTxFlags & tfSell);

    auto saTakerPays = ctx_.tx[sfTakerPays];
    auto saTakerGets = ctx_.tx[sfTakerGets];

    auto const cancelSequence = ctx_.tx[~sfOfferSequence];

//Fixme理解为什么我们使用SequenceNext而不是当前事务
//确定事务的顺序。为什么报价顺序是
//数字不够？
    auto const uSequence = ctx_.tx.getSequence ();

//这是报价的原价，也是
//即使交叉报价改变了
//以书告终。
    auto uRate = getRate (saTakerGets, saTakerPays);

    auto viewJ = ctx_.app.journal("View");

    TER result = tesSUCCESS;

//处理与报价一起传递的取消请求。
    if (cancelSequence)
    {
        auto const sleCancel = sb.peek(
            keylet::offer(account_, *cancelSequence));

//找不到取消的提议不是错误：它可能
//被消耗或移除。但是，如果找到它，那是一个错误
//删除失败。
        if (sleCancel)
        {
            JLOG(j_.debug()) << "Create cancels order " << *cancelSequence;
            result = offerDelete (sb, sleCancel, viewJ);
        }
    }

    auto const expiration = ctx_.tx[~sfExpiration];
    using d = NetClock::duration;
    using tp = NetClock::time_point;

//到期是根据父分类账的关闭时间定义的，
//因为我们确实知道它关闭的时间，但我们不知道
//了解正在施工的分类帐的结算时间。
    if (expiration &&
        (ctx_.view().parentCloseTime() >= tp{d{*expiration}}))
    {
//如果报价已过期，则交易已成功
//什么都没做，所以这里短路了。
//
//返回代码更改作为
//方便。更改不够大，不值得使用修复代码。
        TER const ter {ctx_.view().rules().enabled(
            featureDepositPreauth) ? TER {tecEXPIRED} : TER {tesSUCCESS}};
        return{ ter, true };
    }

    bool const bOpenLedger = ctx_.view().open();
    bool crossed = false;

    if (result == tesSUCCESS)
    {
//如果勾号大小适用，请将报价舍入到勾号大小
        auto const& uPaysIssuerID = saTakerPays.getIssuer ();
        auto const& uGetsIssuerID = saTakerGets.getIssuer ();

        std::uint8_t uTickSize = Quality::maxTickSize;
        if (!isXRP (uPaysIssuerID))
        {
            auto const sle =
                sb.read(keylet::account(uPaysIssuerID));
            if (sle && sle->isFieldPresent (sfTickSize))
                uTickSize = std::min (uTickSize,
                    (*sle)[sfTickSize]);
        }
        if (!isXRP (uGetsIssuerID))
        {
            auto const sle =
                sb.read(keylet::account(uGetsIssuerID));
            if (sle && sle->isFieldPresent (sfTickSize))
                uTickSize = std::min (uTickSize,
                    (*sle)[sfTickSize]);
        }
        if (uTickSize < Quality::maxTickSize)
        {
            auto const rate =
                Quality{saTakerGets, saTakerPays}.round
                    (uTickSize).rate();

//我们绕着不准确的一边，
//就好像要约正好执行
//以稍微好一点的（砂矿）速度
            if (bSell)
            {
//这是一个卖出，买单者付
                saTakerPays = multiply (
                    saTakerGets, rate, saTakerPays.issue());
            }
            else
            {
//这是买来的，拿圆的
                saTakerGets = divide (
                    saTakerPays, rate, saTakerGets.issue());
            }
            if (! saTakerGets || ! saTakerPays)
            {
                JLOG (j_.debug()) <<
                    "Offer rounded to zero";
                return { result, true };
            }

            uRate = getRate (saTakerGets, saTakerPays);
        }

//我们是因为在过马路的时候，我们是在拿着钱和钱。
        Amounts const takerAmount (saTakerGets, saTakerPays);

//交叉后未完成的报价金额
//进行。可能等于原始金额（未交叉）
//空的（完全交叉的），或介于两者之间的东西。
        Amounts place_offer;

        JLOG(j_.debug()) << "Attempting cross: " <<
            to_string (takerAmount.in.issue ()) << " -> " <<
            to_string (takerAmount.out.issue ());

        if (auto stream = j_.trace())
        {
            stream << "   mode: " <<
                (bPassive ? "passive " : "") <<
                (bSell ? "sell" : "buy");
            stream <<"     in: " << format_amount (takerAmount.in);
            stream << "    out: " << format_amount (takerAmount.out);
        }

        std::tie(result, place_offer) = cross (sb, sbCancel, takerAmount);

//我们期待着Cross的成功实施
//或者给出TEC。
        assert(result == tesSUCCESS || isTecClaim(result));

        if (auto stream = j_.trace())
        {
            stream << "Cross result: " << transToken (result);
            stream << "     in: " << format_amount (place_offer.in);
            stream << "    out: " << format_amount (place_offer.out);
        }

        if (result == tecFAILED_PROCESSING && bOpenLedger)
            result = telFAILED_PROCESSING;

        if (result != tesSUCCESS)
        {
            JLOG (j_.debug()) << "final result: " << transToken (result);
            return { result, true };
        }

        assert (saTakerGets.issue () == place_offer.in.issue ());
        assert (saTakerPays.issue () == place_offer.out.issue ());

        if (takerAmount != place_offer)
            crossed = true;

//报价交叉后我们需要提出的报价应该
//不要消极。如果是的话，一定是出了很大的问题。
        if (place_offer.in < zero || place_offer.out < zero)
        {
            JLOG(j_.fatal()) << "Cross left offer negative!" <<
                "     in: " << format_amount (place_offer.in) <<
                "    out: " << format_amount (place_offer.out);
            return { tefINTERNAL, true };
        }

        if (place_offer.in == zero || place_offer.out == zero)
        {
            JLOG(j_.debug()) << "Offer fully crossed!";
            return { result, true };
        }

//我们现在需要调整报价以反映
//十字路口。我们在这里来回倒车，因为在过马路的时候
//是接受者。
        saTakerPays = place_offer.out;
        saTakerGets = place_offer.in;
    }

    assert (saTakerPays > zero && saTakerGets > zero);

    if (result != tesSUCCESS)
    {
        JLOG (j_.debug()) << "final result: " << transToken (result);
        return { result, true };
    }

    if (auto stream = j_.trace())
    {
        stream << "Place" << (crossed ? " remaining " : " ") << "offer:";
        stream << "    Pays: " << saTakerPays.getFullText ();
        stream << "    Gets: " << saTakerGets.getFullText ();
    }

//对于“填充或杀死”提议，未能完全交叉意味着
//整个操作应中止，只需支付费用。
    if (bFillOrKill)
    {
        JLOG (j_.trace()) << "Fill or Kill: offer killed";
        if (sb.rules().enabled (fix1578))
            return { tecKILLED, false };
        return { tesSUCCESS, false };
    }

//对于“立即或取消”的报价，剩余的金额不会得到
//放置-它被取消，操作成功。
    if (bImmediateOrCancel)
    {
        JLOG (j_.trace()) << "Immediate or cancel: offer canceled";
        return { tesSUCCESS, true };
    }

    auto const sleCreator = sb.peek (keylet::account(account_));
    {
        XRPAmount reserve = ctx_.view().fees().accountReserve(
            sleCreator->getFieldU32 (sfOwnerCount) + 1);

        if (mPriorBalance < reserve)
        {
//如果我们在这里，签字账户的准备金不足。
//*在处理之前。如果有什么东西真的错了，那么
//我们允许这样做；否则，我们只要求付费。
            if (!crossed)
                result = tecINSUF_RESERVE_OFFER;

            if (result != tesSUCCESS)
            {
                JLOG (j_.debug()) <<
                    "final result: " << transToken (result);
            }

            return { result, true };
        }
    }

//我们需要把报价的其余部分放入它的订货簿。
    auto const offer_index = getOfferIndex (account_, uSequence);

//将报价添加到所有者目录。
    auto const ownerNode = dirAdd(sb, keylet::ownerDir (account_),
        offer_index, false, describeOwnerDir (account_), viewJ);

    if (!ownerNode)
    {
        JLOG (j_.debug()) <<
            "final result: failed to add offer to owner's directory";
        return { tecDIR_FULL, true };
    }

//更新所有者计数。
    adjustOwnerCount(sb, sleCreator, 1, viewJ);

    JLOG (j_.trace()) <<
        "adding to book: " << to_string (saTakerPays.issue ()) <<
        " : " << to_string (saTakerGets.issue ());

    Book const book { saTakerPays.issue(), saTakerGets.issue() };

//使用原始价格向订单簿添加报价
//在任何穿越发生之前。
    auto dir = keylet::quality (keylet::book (book), uRate);
    bool const bookExisted = static_cast<bool>(sb.peek (dir));

    auto const bookNode = dirAdd (sb, dir, offer_index, true,
        [&](SLE::ref sle)
        {
            sle->setFieldH160 (sfTakerPaysCurrency,
                saTakerPays.issue().currency);
            sle->setFieldH160 (sfTakerPaysIssuer,
                saTakerPays.issue().account);
            sle->setFieldH160 (sfTakerGetsCurrency,
                saTakerGets.issue().currency);
            sle->setFieldH160 (sfTakerGetsIssuer,
                saTakerGets.issue().account);
            sle->setFieldU64 (sfExchangeRate, uRate);
        }, viewJ);

    if (!bookNode)
    {
        JLOG (j_.debug()) <<
            "final result: failed to add offer to book";
        return { tecDIR_FULL, true };
    }

    auto sleOffer = std::make_shared<SLE>(ltOFFER, offer_index);
    sleOffer->setAccountID (sfAccount, account_);
    sleOffer->setFieldU32 (sfSequence, uSequence);
    sleOffer->setFieldH256 (sfBookDirectory, dir.key);
    sleOffer->setFieldAmount (sfTakerPays, saTakerPays);
    sleOffer->setFieldAmount (sfTakerGets, saTakerGets);
    sleOffer->setFieldU64 (sfOwnerNode, *ownerNode);
    sleOffer->setFieldU64 (sfBookNode, *bookNode);
    if (expiration)
        sleOffer->setFieldU32 (sfExpiration, *expiration);
    if (bPassive)
        sleOffer->setFlag (lsfPassive);
    if (bSell)
        sleOffer->setFlag (lsfSell);
    sb.insert(sleOffer);

    if (!bookExisted)
        ctx_.app.getOrderBookDB().addOrderBook(book);

    JLOG (j_.debug()) << "final result: success";

    return { tesSUCCESS, true };
}

TER
CreateOffer::doApply()
{
//这是我们要处理的分类帐视图。应用事务
//当我们继续处理事务时。
    Sandbox sb (&ctx_.view());

//这是一个只支付费用的分类账，没有资金或到期。
//我们遇到的提议被取消了。它用于处理填充或终止报价，
//如果不能下订单，避免浪费我们所做的工作。
    Sandbox sbCancel (&ctx_.view());

    auto const result = applyGuts(sb, sbCancel);
    if (result.second)
        sb.apply(ctx_.rawView());
    else
        sbCancel.apply(ctx_.rawView());
    return result.first;
}

}
