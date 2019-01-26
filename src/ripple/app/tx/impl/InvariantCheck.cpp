
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
  版权所有（c）2012-2016 Ripple Labs Inc.

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

#include <ripple/app/tx/impl/InvariantCheck.h>
#include <ripple/basics/Log.h>

namespace ripple {

void
TransactionFeeCheck::visitEntry(
    uint256 const&,
    bool,
    std::shared_ptr<SLE const> const&,
    std::shared_ptr<SLE const> const&)
{
//无事可做
}

bool
TransactionFeeCheck::finalize(
    STTx const& tx,
    TER const result,
    XRPAmount const fee,
    beast::Journal const& j)
{
//我们不应该收取负费用
    if (fee.drops() < 0)
    {
        JLOG(j.fatal()) << "Invariant failed: fee paid was negative: " << fee.drops();
        return false;
    }

//我们不应该收取大于或等于
//整个XRP供应。
    if (fee.drops() >= SYSTEM_CURRENCY_START)
    {
        JLOG(j.fatal()) << "Invariant failed: fee paid exceeds system limit: " << fee.drops();
        return false;
    }

//我们不应该对交易收取比交易更多的费用。
//授权。在某些情况下可以降低收费。
    if (fee > tx.getFieldAmount(sfFee).xrp())
    {
        JLOG(j.fatal()) << "Invariant failed: fee paid is " << fee.drops() <<
            " exceeds fee specified in transaction.";
        return false;
    }

    return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
XRPNotCreated::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    /*我们浏览所有修改过的分类账条目，只查看账户根目录，
     *托管付款和付款渠道。我们从总数中去掉
     *以前的xrp值，并添加到所有新的xrp值中。网
     *支付渠道余额由两个字段（金额和
     *余额）和删除对于paychan和escrow被忽略，因为
     *金额字段没有针对删除的情况进行调整。
     **/

    if(before)
    {
        switch (before->getType())
        {
        case ltACCOUNT_ROOT:
            drops_ -= (*before)[sfBalance].xrp().drops();
            break;
        case ltPAYCHAN:
            drops_ -= ((*before)[sfAmount] - (*before)[sfBalance]).xrp().drops();
            break;
        case ltESCROW:
            drops_ -= (*before)[sfAmount].xrp().drops();
            break;
        default:
            break;
        }
    }

    if(after)
    {
        switch (after->getType())
        {
        case ltACCOUNT_ROOT:
            drops_ += (*after)[sfBalance].xrp().drops();
            break;
        case ltPAYCHAN:
            if (! isDelete)
                drops_ += ((*after)[sfAmount] - (*after)[sfBalance]).xrp().drops();
            break;
        case ltESCROW:
            if (! isDelete)
                drops_ += (*after)[sfAmount].xrp().drops();
            break;
        default:
            break;
        }
    }
}

bool
XRPNotCreated::finalize(
    STTx const& tx,
    TER const,
    XRPAmount const fee,
    beast::Journal const& j)
{
//净变化不应该是积极的，因为这意味着
//交易在稀薄的空气中创建了XRP。那是不可能的。
    if (drops_ > 0)
    {
        JLOG(j.fatal()) <<
            "Invariant failed: XRP net change was positive: " << drops_;
        return false;
    }

//净变动的负值应等于实际收取的费用。
    if (-drops_ != fee.drops())
    {
        JLOG(j.fatal()) <<
            "Invariant failed: XRP net change of " << drops_ <<
            " doesn't match fee " << fee.drops();
        return false;
    }

    return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
XRPBalanceChecks::visitEntry(
    uint256 const&,
    bool,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    auto isBad = [](STAmount const& balance)
    {
        if (!balance.native())
            return true;

        auto const drops = balance.xrp().drops();

//不能超过实例化的放置数
//在创世纪的分类帐里。
        if (drops > SYSTEM_CURRENCY_START)
            return true;

//不能有负余额（0可以）
        if (drops < 0)
            return true;

        return false;
    };

    if(before && before->getType() == ltACCOUNT_ROOT)
        bad_ |= isBad ((*before)[sfBalance]);

    if(after && after->getType() == ltACCOUNT_ROOT)
        bad_ |= isBad ((*after)[sfBalance]);
}

bool
XRPBalanceChecks::finalize(STTx const&, TER const, XRPAmount const, beast::Journal const& j)
{
    if (bad_)
    {
        JLOG(j.fatal()) << "Invariant failed: incorrect account XRP balance";
        return false;
    }

    return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
NoBadOffers::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    auto isBad = [](STAmount const& pays, STAmount const& gets)
    {
//报价不应该是否定的。
        if (pays < beast::zero)
            return true;

        if (gets < beast::zero)
            return true;

//不能提供从xrp到xrp的优惠：
        return pays.native() && gets.native();
    };

    if(before && before->getType() == ltOFFER)
        bad_ |= isBad ((*before)[sfTakerPays], (*before)[sfTakerGets]);

    if(after && after->getType() == ltOFFER)
        bad_ |= isBad((*after)[sfTakerPays], (*after)[sfTakerGets]);
}

bool
NoBadOffers::finalize(STTx const& tx, TER const, XRPAmount const, beast::Journal const& j)
{
    if (bad_)
    {
        JLOG(j.fatal()) << "Invariant failed: offer with a bad amount";
        return false;
    }

    return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
NoZeroEscrow::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    auto isBad = [](STAmount const& amount)
    {
        if (!amount.native())
            return true;

        if (amount.xrp().drops() <= 0)
            return true;

        if (amount.xrp().drops() >= SYSTEM_CURRENCY_START)
            return true;

        return false;
    };

    if(before && before->getType() == ltESCROW)
        bad_ |= isBad((*before)[sfAmount]);

    if(after && after->getType() == ltESCROW)
        bad_ |= isBad((*after)[sfAmount]);
}

bool
NoZeroEscrow::finalize(STTx const& tx, TER const, XRPAmount const, beast::Journal const& j)
{
    if (bad_)
    {
        JLOG(j.fatal()) << "Invariant failed: escrow specifies invalid amount";
        return false;
    }

    return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
AccountRootsNotDeleted::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const&)
{
    if (isDelete && before && before->getType() == ltACCOUNT_ROOT)
        accountDeleted_ = true;
}

bool
AccountRootsNotDeleted::finalize(STTx const&, TER const, XRPAmount const, beast::Journal const& j)
{
    if (! accountDeleted_)
        return true;

    JLOG(j.fatal()) << "Invariant failed: an account root was deleted";
    return false;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
LedgerEntryTypesMatch::visitEntry(
    uint256 const&,
    bool,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    if (before && after && before->getType() != after->getType())
        typeMismatch_ = true;

    if (after)
    {
        switch (after->getType())
        {
        case ltACCOUNT_ROOT:
        case ltDIR_NODE:
        case ltRIPPLE_STATE:
        case ltTICKET:
        case ltSIGNER_LIST:
        case ltOFFER:
        case ltLEDGER_HASHES:
        case ltAMENDMENTS:
        case ltFEE_SETTINGS:
        case ltESCROW:
        case ltPAYCHAN:
        case ltCHECK:
        case ltDEPOSIT_PREAUTH:
            break;
        default:
            invalidTypeAdded_ = true;
            break;
        }
    }
}

bool
LedgerEntryTypesMatch::finalize(STTx const&, TER const, XRPAmount const, beast::Journal const& j)
{
    if ((! typeMismatch_) && (! invalidTypeAdded_))
        return true;

    if (typeMismatch_)
    {
        JLOG(j.fatal()) << "Invariant failed: ledger entry type mismatch";
    }

    if (invalidTypeAdded_)
    {
        JLOG(j.fatal()) << "Invariant failed: invalid ledger entry type added";
    }

    return false;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
NoXRPTrustLines::visitEntry(
    uint256 const&,
    bool,
    std::shared_ptr <SLE const> const&,
    std::shared_ptr <SLE const> const& after)
{
    if (after && after->getType() == ltRIPPLE_STATE)
    {
//直接在这里检查问题，而不是
//以某种方式依赖.native（）以防本机
//系统性错误
        xrpTrustLine_ =
            after->getFieldAmount (sfLowLimit).issue() == xrpIssue() ||
            after->getFieldAmount (sfHighLimit).issue() == xrpIssue();
    }
}

bool
NoXRPTrustLines::finalize(STTx const&, TER const, XRPAmount const, beast::Journal const& j)
{
    if (! xrpTrustLine_)
        return true;

    JLOG(j.fatal()) << "Invariant failed: an XRP trust line was created";
    return false;
}

}  //涟漪

