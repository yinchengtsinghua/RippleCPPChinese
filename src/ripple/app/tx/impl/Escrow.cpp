
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

#include <ripple/app/tx/impl/Escrow.h>

#include <ripple/app/misc/HashRouter.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/conditions/Condition.h>
#include <ripple/conditions/Fulfillment.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/XRPAmount.h>

//在escrowfinish期间，事务必须同时指定
//条件和满足。我们会追踪
//满足匹配并验证条件。
#define SF_CF_INVALID  SF_PRIVATE5
#define SF_CF_VALID    SF_PRIVATE6

namespace ripple {

/*
    托管
    不等于

    托管是xrp分类账的一个功能，允许您发送有条件的
    XRP付款。这些有条件的支付，称为escrows，留出了xrp和
    在满足某些条件后再交付。成功的条件
    完成一个托管包括基于时间的解锁和加密条件。托管
    如果没有及时完成，也可以设置为过期。

    在托管中预留的XRP被锁定。没有人可以使用或破坏
    直到托管成功完成或取消。之前
    到期时间，只有预期的接收者才能获得xrp。后
    到期时间，xrp只能返回给发送方。

    有关托管的更多详细信息，包括示例、图表和更多信息，请
    访问https://ripple.com/build/escrow/escrow

    有关特定事务的详细信息，包括字段和验证规则
    请参阅：

    “EcthReCube”
    --------------
        请参阅：https://ripple.com/build/transactions/escrowcreate

    “尾饰”
    --------------
        请参阅：https://ripple.com/build/transactions/escrowfinish

    “托管取消”
    --------------
        请参阅：https://ripple.com/build/transactions/escrowcancel
**/


//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*指定的时间过去了吗？

    @param现在是当前时间
    @param标记截止点
    @return true，如果\a now严格引用标记后的时间，则返回false，否则返回false。
**/

static inline bool after (NetClock::time_point now, std::uint32_t mark)
{
    return now.time_since_epoch().count() > mark;
}

XRPAmount
EscrowCreate::calculateMaxSpend(STTx const& tx)
{
    return tx[sfAmount].xrp();
}

NotTEC
EscrowCreate::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featureEscrow))
        return temDISABLED;

    if (ctx.rules.enabled(fix1543) && ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    if (! isXRP(ctx.tx[sfAmount]))
        return temBAD_AMOUNT;

    if (ctx.tx[sfAmount] <= beast::zero)
        return temBAD_AMOUNT;

//必须至少指定一个超时值
    if (! ctx.tx[~sfCancelAfter] && ! ctx.tx[~sfFinishAfter])
            return temBAD_EXPIRATION;

//如果同时指定了完成时间和取消时间，则取消时间必须
//严格在完成时间之后。
    if (ctx.tx[~sfCancelAfter] && ctx.tx[~sfFinishAfter] &&
            ctx.tx[sfCancelAfter] <= ctx.tx[sfFinishAfter])
        return temBAD_EXPIRATION;

    if (ctx.rules.enabled(fix1571))
    {
//如果没有FinishAfter，则可以完成托管
//立即，这可能会令人困惑。创建托管时，
//我们要确保finishAfter时间
//指定或附加完成条件。
        if (! ctx.tx[~sfFinishAfter] && ! ctx.tx[~sfCondition])
            return temMALFORMED;
    }

    if (auto const cb = ctx.tx[~sfCondition])
    {
        using namespace ripple::cryptoconditions;

        std::error_code ec;

        auto condition = Condition::deserialize(*cb, ec);
        if (!condition)
        {
            JLOG(ctx.j.debug()) <<
                "Malformed condition during escrow creation: " << ec.message();
            return temMALFORMED;
        }

//PrefixSha256以外的条件要求
//“密码条件套件”修正案：
        if (condition->type != Type::preimageSha256 &&
                !ctx.rules.enabled(featureCryptoConditionsSuite))
            return temDISABLED;
    }

    return preflight2 (ctx);
}

TER
EscrowCreate::doApply()
{
    auto const closeTime = ctx_.view ().info ().parentCloseTime;

//在Fix1571之前，取消和完成时间可能更大
//大于或等于父分类帐的关闭时间。
//
//对于FIX1571，我们要求它们都严格地更大
//比父分类帐的关闭时间短。
    if (ctx_.view ().rules().enabled(fix1571))
    {
        if (ctx_.tx[~sfCancelAfter] && after(closeTime, ctx_.tx[sfCancelAfter]))
            return tecNO_PERMISSION;

        if (ctx_.tx[~sfFinishAfter] && after(closeTime, ctx_.tx[sfFinishAfter]))
            return tecNO_PERMISSION;
    }
    else
    {
        if (ctx_.tx[~sfCancelAfter])
        {
            auto const cancelAfter = ctx_.tx[sfCancelAfter];

            if (closeTime.time_since_epoch().count() >= cancelAfter)
                return tecNO_PERMISSION;
        }

        if (ctx_.tx[~sfFinishAfter])
        {
            auto const finishAfter = ctx_.tx[sfFinishAfter];

            if (closeTime.time_since_epoch().count() >= finishAfter)
                return tecNO_PERMISSION;
        }
    }

    auto const account = ctx_.tx[sfAccount];
    auto const sle = ctx_.view().peek(
        keylet::account(account));

//检查准备金和资金可用性
    {
        auto const balance = STAmount((*sle)[sfBalance]).xrp();
        auto const reserve = ctx_.view().fees().accountReserve(
            (*sle)[sfOwnerCount] + 1);

        if (balance < reserve)
            return tecINSUFFICIENT_RESERVE;

        if (balance < reserve + STAmount(ctx_.tx[sfAmount]).xrp())
            return tecUNFUNDED;
    }

//检查目标帐户
    {
        auto const sled = ctx_.view().read(
            keylet::account(ctx_.tx[sfDestination]));
        if (! sled)
            return tecNO_DST;
        if (((*sled)[sfFlags] & lsfRequireDestTag) &&
                ! ctx_.tx[~sfDestinationTag])
            return tecDST_TAG_NEEDED;

//遵守lsfdissalowxrp标志是一个错误。背上
//FeatureDepositauth删除错误。
        if (! ctx_.view().rules().enabled(featureDepositAuth) &&
                ((*sled)[sfFlags] & lsfDisallowXRP))
            return tecNO_TARGET;
    }

//在分类帐中创建托管
    auto const slep = std::make_shared<SLE>(
        keylet::escrow(account, (*sle)[sfSequence] - 1));
    (*slep)[sfAmount] = ctx_.tx[sfAmount];
    (*slep)[sfAccount] = account;
    (*slep)[~sfCondition] = ctx_.tx[~sfCondition];
    (*slep)[~sfSourceTag] = ctx_.tx[~sfSourceTag];
    (*slep)[sfDestination] = ctx_.tx[sfDestination];
    (*slep)[~sfCancelAfter] = ctx_.tx[~sfCancelAfter];
    (*slep)[~sfFinishAfter] = ctx_.tx[~sfFinishAfter];
    (*slep)[~sfDestinationTag] = ctx_.tx[~sfDestinationTag];

    ctx_.view().insert(slep);

//将托管添加到发件人的所有者目录
    {
        auto page = dirAdd(ctx_.view(), keylet::ownerDir(account), slep->key(),
            false, describeOwnerDir(account), ctx_.app.journal ("View"));
        if (!page)
            return tecDIR_FULL;
        (*slep)[sfOwnerNode] = *page;
    }

//如果不是自发送，请将托管添加到收件人的所有者目录。
    if (ctx_.view ().rules().enabled(fix1523))
    {
        auto const dest = ctx_.tx[sfDestination];

        if (dest != ctx_.tx[sfAccount])
        {
            auto page = dirAdd(ctx_.view(), keylet::ownerDir(dest), slep->key(),
                false, describeOwnerDir(dest), ctx_.app.journal ("View"));
            if (!page)
                return tecDIR_FULL;
            (*slep)[sfDestinationNode] = *page;
        }
    }

//扣除所有者余额，增加所有者计数
    (*sle)[sfBalance] = (*sle)[sfBalance] - ctx_.tx[sfAmount];
    adjustOwnerCount(ctx_.view(), sle, 1, ctx_.journal);
    ctx_.view().update(sle);

    return tesSUCCESS;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

static
bool
checkCondition (Slice f, Slice c)
{
    using namespace ripple::cryptoconditions;

    std::error_code ec;

    auto condition = Condition::deserialize(c, ec);
    if (!condition)
        return false;

    auto fulfillment = Fulfillment::deserialize(f, ec);
    if (!fulfillment)
        return false;

    return validate (*fulfillment, *condition);
}

NotTEC
EscrowFinish::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featureEscrow))
        return temDISABLED;

    if (ctx.rules.enabled(fix1543) && ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    {
        auto const ret = preflight1 (ctx);
        if (!isTesSuccess (ret))
            return ret;
    }

    auto const cb = ctx.tx[~sfCondition];
    auto const fb = ctx.tx[~sfFulfillment];

//如果指定条件，则还必须指定
//履行。
    if (static_cast<bool>(cb) != static_cast<bool>(fb))
        return temMALFORMED;

//验证事务签名。如果不起作用
//那就别再干了。
    {
        auto const ret = preflight2 (ctx);
        if (!isTesSuccess (ret))
            return ret;
    }

    if (cb && fb)
    {
        auto& router = ctx.app.getHashRouter();

        auto const id = ctx.tx.getTransactionID();
        auto const flags = router.getFlags (id);

//如果我们没有检查过情况，就检查一下
//现在。是否通过并不重要
//在飞行前。
        if (!(flags & (SF_CF_INVALID | SF_CF_VALID)))
        {
            if (checkCondition (*fb, *cb))
                router.setFlags (id, SF_CF_VALID);
            else
                router.setFlags (id, SF_CF_INVALID);
        }
    }

    return tesSUCCESS;
}

std::uint64_t
EscrowFinish::calculateBaseFee (
    ReadView const& view,
    STTx const& tx)
{
    std::uint64_t extraFee = 0;

    if (auto const fb = tx[~sfFulfillment])
    {
        extraFee += view.fees().units *
            (32 + safe_cast<std::uint64_t> (fb->size() / 16));
    }

    return Transactor::calculateBaseFee (view, tx) + extraFee;
}

TER
EscrowFinish::doApply()
{
    auto const k = keylet::escrow(
        ctx_.tx[sfOwner], ctx_.tx[sfOfferSequence]);
    auto const slep = ctx_.view().peek(k);
    if (! slep)
        return tecNO_TARGET;

//如果存在取消时间，则完成操作只能在
//到那个时候。fix157修正了检查中的逻辑错误
//只有在取消时间之后才能完成。
    if (ctx_.view ().rules().enabled(fix1571))
    {
        auto const now = ctx_.view().info().parentCloseTime;

//太早：无法在完成时间之前执行
        if ((*slep)[~sfFinishAfter] && ! after(now, (*slep)[sfFinishAfter]))
            return tecNO_PERMISSION;

//太晚：取消时间后无法执行
        if ((*slep)[~sfCancelAfter] && after(now, (*slep)[sfCancelAfter]))
            return tecNO_PERMISSION;
    }
    else
    {
//太早了？
        if ((*slep)[~sfFinishAfter] &&
            ctx_.view().info().parentCloseTime.time_since_epoch().count() <=
            (*slep)[sfFinishAfter])
            return tecNO_PERMISSION;

//太晚了？
        if ((*slep)[~sfCancelAfter] &&
            ctx_.view().info().parentCloseTime.time_since_epoch().count() <=
            (*slep)[sfCancelAfter])
            return tecNO_PERMISSION;
    }

//检查密码条件满足情况
    {
        auto const id = ctx_.tx.getTransactionID();
        auto flags = ctx_.app.getHashRouter().getFlags (id);

        auto const cb = ctx_.tx[~sfCondition];

//检查结果不太可能
//从散列路由器过期，但如果发生，
//只需重新运行检查。
        if (cb && ! (flags & (SF_CF_INVALID | SF_CF_VALID)))
        {
            auto const fb = ctx_.tx[~sfFulfillment];

            if (!fb)
                return tecINTERNAL;

            if (checkCondition (*fb, *cb))
                flags = SF_CF_VALID;
            else
                flags = SF_CF_INVALID;

            ctx_.app.getHashRouter().setFlags (id, flags);
        }

//如果检查失败，只需返回一个错误
//别再看别的了。
        if (flags & SF_CF_INVALID)
            return tecCRYPTOCONDITION_ERROR;

//对照分类账分录中的条件检查：
        auto const cond = (*slep)[~sfCondition];

//如果在创建期间未指定条件，
//现在不应该包括一个。
        if (!cond && cb)
            return tecCRYPTOCONDITION_ERROR;

//如果在创建期间指定了条件
//暂停付款，相同条件
//必须再次提交。我们不检查
//履行符合我们的条件
//在飞行前。
        if (cond && (cond != cb))
            return tecCRYPTOCONDITION_ERROR;
    }

//注：托管付款不能用于为账户提供资金。
    AccountID const destID = (*slep)[sfDestination];
    auto const sled = ctx_.view().peek(keylet::account(destID));
    if (! sled)
        return tecNO_DST;

    if (ctx_.view().rules().enabled(featureDepositAuth))
    {
//escrowfinished授权了吗？
        if (sled->getFlags() & lsfDepositAuth)
        {
//需要授权的目标帐户有两个
//使escrowfinished进入帐户的方法：
//1。如果account==目的地，或
//2。如果帐户是按目的地预先授权的存款。
            if (account_ != destID)
            {
                if (! view().exists (keylet::depositPreauth (destID, account_)))
                    return tecNO_PERMISSION;
            }
        }
    }

    AccountID const account = (*slep)[sfAccount];

//从所有者目录中删除托管
    {
        auto const page = (*slep)[sfOwnerNode];
        if (! ctx_.view().dirRemove(
                keylet::ownerDir(account), page, k.key, true))
        {
            return tefBAD_LEDGER;
        }
    }

//从收件人的所有者目录中删除托管（如果存在）。
    if (ctx_.view ().rules().enabled(fix1523) && (*slep)[~sfDestinationNode])
    {
        auto const page = (*slep)[sfDestinationNode];
        if (! ctx_.view().dirRemove(keylet::ownerDir(destID), page, k.key, true))
        {
            return tefBAD_LEDGER;
        }
    }

//将金额转移到目的地
    (*sled)[sfBalance] = (*sled)[sfBalance] + (*slep)[sfAmount];
    ctx_.view().update(sled);

//调整源所有者计数
    auto const sle = ctx_.view().peek(
        keylet::account(account));
    adjustOwnerCount(ctx_.view(), sle, -1, ctx_.journal);
    ctx_.view().update(sle);

//从分类帐中删除托管
    ctx_.view().erase(slep);

    return tesSUCCESS;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

NotTEC
EscrowCancel::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featureEscrow))
        return temDISABLED;

    if (ctx.rules.enabled(fix1543) && ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    return preflight2 (ctx);
}

TER
EscrowCancel::doApply()
{
    auto const k = keylet::escrow(ctx_.tx[sfOwner], ctx_.tx[sfOfferSequence]);
    auto const slep = ctx_.view().peek(k);
    if (! slep)
        return tecNO_TARGET;

    if (ctx_.view ().rules().enabled(fix1571))
    {
        auto const now = ctx_.view().info().parentCloseTime;

//未指定取消时间：根本无法执行。
        if (! (*slep)[~sfCancelAfter])
            return tecNO_PERMISSION;

//太早：无法在取消时间之前执行。
        if (! after(now, (*slep)[sfCancelAfter]))
            return tecNO_PERMISSION;
    }
    else
    {
//太早了？
        if (!(*slep)[~sfCancelAfter] ||
            ctx_.view().info().parentCloseTime.time_since_epoch().count() <=
            (*slep)[sfCancelAfter])
            return tecNO_PERMISSION;
    }

    AccountID const account = (*slep)[sfAccount];

//从所有者目录中删除托管
    {
        auto const page = (*slep)[sfOwnerNode];
        if (! ctx_.view().dirRemove(
                keylet::ownerDir(account), page, k.key, true))
        {
            return tefBAD_LEDGER;
        }
    }

//从收件人的所有者目录中删除托管（如果存在）。
    if (ctx_.view ().rules().enabled(fix1523) && (*slep)[~sfDestinationNode])
    {
        auto const page = (*slep)[sfDestinationNode];
        if (! ctx_.view().dirRemove(
                keylet::ownerDir((*slep)[sfDestination]), page, k.key, true))
        {
            return tefBAD_LEDGER;
        }
    }

//将金额转移回所有者，减少所有者计数
    auto const sle = ctx_.view().peek(
        keylet::account(account));
    (*sle)[sfBalance] = (*sle)[sfBalance] + (*slep)[sfAmount];
    adjustOwnerCount(ctx_.view(), sle, -1, ctx_.journal);
    ctx_.view().update(sle);

//从分类帐中删除托管
    ctx_.view().erase(slep);

    return tesSUCCESS;
}

} //涟漪

