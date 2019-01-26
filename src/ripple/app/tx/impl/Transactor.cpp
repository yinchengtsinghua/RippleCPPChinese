
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

#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/tx/apply.h>
#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/app/tx/impl/SignerEntries.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/json/to_string.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/protocol/Protocol.h>

namespace ripple {

/*对txid执行早期健全性检查*/
NotTEC
preflight0(PreflightContext const& ctx)
{
    auto const txID = ctx.tx.getTransactionID();

    if (txID == beast::zero)
    {
        JLOG(ctx.j.warn()) <<
            "applyTransaction: transaction id may not be zero";
        return temINVALID;
    }

    return tesSUCCESS;
}

/*对帐户和费用字段执行早期健全性检查*/
NotTEC
preflight1 (PreflightContext const& ctx)
{
    auto const ret = preflight0(ctx);
    if (!isTesSuccess(ret))
        return ret;

    auto const id = ctx.tx.getAccountID(sfAccount);
    if (id == beast::zero)
    {
        JLOG(ctx.j.warn()) << "preflight1: bad account id";
        return temBAD_SRC_ACCOUNT;
    }

//如果交易费格式不正确，就没必要再进一步了。
    auto const fee = ctx.tx.getFieldAmount (sfFee);
    if (!fee.native () || fee.negative () || !isLegalAmount (fee.xrp ()))
    {
        JLOG(ctx.j.debug()) << "preflight1: invalid fee";
        return temBAD_FEE;
    }

    auto const spk = ctx.tx.getSigningPubKey();

    if (!spk.empty () && !publicKeyType (makeSlice (spk)))
    {
        JLOG(ctx.j.debug()) << "preflight1: invalid signing key";
        return temBAD_SIGNATURE;
    }

    return tesSUCCESS;
}

/*检查签名是否有效*/
NotTEC
preflight2 (PreflightContext const& ctx)
{
    auto const sigValid = checkValidity(ctx.app.getHashRouter(),
        ctx.tx, ctx.rules, ctx.app.config());
    if (sigValid.first == Validity::SigBad)
    {
        JLOG(ctx.j.debug()) <<
            "preflight2: bad signature. " << sigValid.second;
        return temINVALID;
    }
    return tesSUCCESS;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

PreflightContext::PreflightContext(Application& app_, STTx const& tx_,
    Rules const& rules_, ApplyFlags flags_,
        beast::Journal j_)
    : app(app_)
    , tx(tx_)
    , rules(rules_)
    , flags(flags_)
    , j(j_)
{
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Transactor::Transactor(
    ApplyContext& ctx)
    : ctx_ (ctx)
    , j_ (ctx.journal)
{
}

std::uint64_t Transactor::calculateBaseFee (
    ReadView const& view,
    STTx const& tx)
{
//以费用单位返回费用。

//计算分为两部分：
//*基本费用，大多数交易相同。
//*交易中每个多重签名的额外成本。
    std::uint64_t baseFee = view.fees().units;

//每个签名者在所需的最低费用中再加一个基本费用
//交易记录。
    std::uint32_t signerCount = 0;
    if (tx.isFieldPresent (sfSigners))
        signerCount = tx.getFieldArray (sfSigners).size();

    return baseFee + (signerCount * baseFee);
}

XRPAmount
Transactor::calculateFeePaid(STTx const& tx)
{
    return tx[sfFee].xrp();
}

XRPAmount
Transactor::minimumFee (Application& app, std::uint64_t baseFee,
    Fees const& fees, ApplyFlags flags)
{
    return scaleFeeLoad (baseFee, app.getFeeTrack (),
        fees, flags & tapUNLIMITED);
}

XRPAmount
Transactor::calculateMaxSpend(STTx const& tx)
{
    return beast::zero;
}

TER
Transactor::checkFee (PreclaimContext const& ctx,
    std::uint64_t baseFee)
{
    auto const feePaid = calculateFeePaid(ctx.tx);
    if (!isLegalAmount (feePaid) || feePaid < beast::zero)
        return temBAD_FEE;

    auto const feeDue = minimumFee(ctx.app,
        baseFee, ctx.view.fees(), ctx.flags);

//只有在分类帐打开时支票费用才足够。
    if (ctx.view.open() && feePaid < feeDue)
    {
        JLOG(ctx.j.trace()) << "Insufficient fee paid: " <<
            to_string (feePaid) << "/" << to_string (feeDue);
        return telINSUF_FEE_P;
    }

    if (feePaid == beast::zero)
        return tesSUCCESS;

    auto const id = ctx.tx.getAccountID(sfAccount);
    auto const sle = ctx.view.read(
        keylet::account(id));
    auto const balance = (*sle)[sfBalance].xrp();

    if (balance < feePaid)
    {
        JLOG(ctx.j.trace()) << "Insufficient balance:" <<
            " balance=" << to_string(balance) <<
            " paid=" << to_string(feePaid);

        if ((balance > beast::zero) && !ctx.view.open())
        {
//关闭的分类帐，非零余额，低于费用
            return tecINSUFF_FEE;
        }

        return terINSUF_FEE_B;
    }

    return tesSUCCESS;
}

TER Transactor::payFee ()
{
    auto const feePaid = calculateFeePaid(ctx_.tx);

    auto const sle = view().peek(
        keylet::account(account_));

//扣除费用，因此在交易期间不可用。
//只有在交易成功时才会回写帐户。

    mSourceBalance -= feePaid;
    sle->setFieldAmount (sfBalance, mSourceBalance);

//vfalc我们也应该在这里调用view（）.rawdestroyxrp（）？

    return tesSUCCESS;
}

NotTEC
Transactor::checkSeq (PreclaimContext const& ctx)
{
    auto const id = ctx.tx.getAccountID(sfAccount);

    auto const sle = ctx.view.read(
        keylet::account(id));

    if (!sle)
    {
        JLOG(ctx.j.trace()) <<
            "applyTransaction: delay: source account does not exist " <<
            toBase58(ctx.tx.getAccountID(sfAccount));
        return terNO_ACCOUNT;
    }

    std::uint32_t const t_seq = ctx.tx.getSequence ();
    std::uint32_t const a_seq = sle->getFieldU32 (sfSequence);

    if (t_seq != a_seq)
    {
        if (a_seq < t_seq)
        {
            JLOG(ctx.j.trace()) <<
                "applyTransaction: has future sequence number " <<
                "a_seq=" << a_seq << " t_seq=" << t_seq;
            return terPRE_SEQ;
        }

        if (ctx.view.txExists(ctx.tx.getTransactionID ()))
            return tefALREADY;

        JLOG(ctx.j.trace()) << "applyTransaction: has past sequence number " <<
            "a_seq=" << a_seq << " t_seq=" << t_seq;
        return tefPAST_SEQ;
    }

    if (ctx.tx.isFieldPresent (sfAccountTxnID) &&
            (sle->getFieldH256 (sfAccountTxnID) != ctx.tx.getFieldH256 (sfAccountTxnID)))
        return tefWRONG_PRIOR;

    if (ctx.tx.isFieldPresent (sfLastLedgerSequence) &&
            (ctx.view.seq() > ctx.tx.getFieldU32 (sfLastLedgerSequence)))
        return tefMAX_LEDGER;

    return tesSUCCESS;
}

void
Transactor::setSeq ()
{
    auto const sle = view().peek(
        keylet::account(account_));

    std::uint32_t const t_seq = ctx_.tx.getSequence ();

    sle->setFieldU32 (sfSequence, t_seq + 1);

    if (sle->isFieldPresent (sfAccountTxnID))
        sle->setFieldH256 (sfAccountTxnID, ctx_.tx.getTransactionID ());
}

//在你费心锁账本之前检查一下东西
void Transactor::preCompute ()
{
    account_ = ctx_.tx.getAccountID(sfAccount);
    assert(account_ != beast::zero);
}

TER Transactor::apply ()
{
    preCompute();

//如果交易人要求有效的账户，而交易没有
//首先，飞行前已经有一个故障标志。
    auto const sle = view().peek (keylet::account(account_));

//SLE必须存在，交易除外
//允许零账户。
    assert(sle != nullptr || account_ == beast::zero);

    if (sle)
    {
        mPriorBalance   = STAmount ((*sle)[sfBalance]).xrp ();
        mSourceBalance  = mPriorBalance;

        setSeq();

        auto result = payFee ();

        if (result  != tesSUCCESS)
            return result;

        view().update (sle);
    }

    return doApply ();
}

NotTEC
Transactor::checkSign (PreclaimContext const& ctx)
{
//在检查多重签名之前，请确保启用多重签名。
    if (ctx.view.rules().enabled(featureMultiSign))
    {
//如果pk为空，则必须进行多重签名。
        if (ctx.tx.getSigningPubKey().empty ())
            return checkMultiSign (ctx);
    }

    return checkSingleSign (ctx);
}

NotTEC
Transactor::checkSingleSign (PreclaimContext const& ctx)
{
    auto const id = ctx.tx.getAccountID(sfAccount);

    auto const sle = ctx.view.read(
        keylet::account(id));
    auto const hasAuthKey     = sle->isFieldPresent (sfRegularKey);

//一致性：检查签名并验证事务的签名
//公钥被授权签名。
    auto const spk = ctx.tx.getSigningPubKey();
    if (!publicKeyType (makeSlice (spk)))
    {
        JLOG(ctx.j.trace()) <<
            "checkSingleSign: signing public key type is unknown";
return tefBAD_AUTH; //修正：应该是更好的错误！
    }

    auto const pkAccount = calcAccountID (
        PublicKey (makeSlice (spk)));

    if (pkAccount == id)
    {
//授权继续。
        if (sle->isFlag(lsfDisableMaster))
            return tefMASTER_DISABLED;
    }
    else if (hasAuthKey &&
        (pkAccount == sle->getAccountID (sfRegularKey)))
    {
//授权继续。
    }
    else if (hasAuthKey)
    {
        JLOG(ctx.j.trace()) <<
            "checkSingleSign: Not authorized to use account.";
        return tefBAD_AUTH;
    }
    else
    {
        JLOG(ctx.j.trace()) <<
            "checkSingleSign: Not authorized to use account.";
        return tefBAD_AUTH_MASTER;
    }

    return tesSUCCESS;
}

NotTEC
Transactor::checkMultiSign (PreclaimContext const& ctx)
{
    auto const id = ctx.tx.getAccountID(sfAccount);
//获取mtxnaccountid的签名列表和仲裁。
    std::shared_ptr<STLedgerEntry const> sleAccountSigners =
        ctx.view.read (keylet::signers(id));
//如果签名者列表不存在，则帐户不是多重签名。
    if (!sleAccountSigners)
    {
        JLOG(ctx.j.trace()) <<
            "applyTransaction: Invalid: Not a multi-signing account.";
        return tefNOT_MULTI_SIGNING;
    }

//我们计划在未来支持多个签名者。这个
//SignerListed字段的存在和默认值将启用此功能。
    assert (sleAccountSigners->isFieldPresent (sfSignerListID));
    assert (sleAccountSigners->getFieldU32 (sfSignerListID) == 0);

    auto accountSigners =
        SignerEntries::deserialize (*sleAccountSigners, ctx.j, "ledger");
    if (accountSigners.second != tesSUCCESS)
        return accountSigners.second;

//获取事务签名者数组。
    STArray const& txSigners (ctx.tx.getFieldArray (sfSigners));

//让会计签名者进行各种检查，看看是否
//达到法定人数。

//多签名者和帐户签名者都按帐户排序。所以
//将多个签名者与帐户签名者匹配应该是一个简单的
//线性行走。*所有*签名者必须有效或事务失败。
    std::uint32_t weightSum = 0;
    auto iter = accountSigners.first.begin ();
    for (auto const& txSigner : txSigners)
    {
        AccountID const txSignerAcctID = txSigner.getAccountID (sfAccount);

//尝试将签名者与签名者匹配；
        while (iter->account < txSignerAcctID)
        {
            if (++iter == accountSigners.first.end ())
            {
                JLOG(ctx.j.trace()) <<
                    "applyTransaction: Invalid SigningAccount.Account.";
                return tefBAD_SIGNATURE;
            }
        }
        if (iter->account != txSignerAcctID)
        {
//SigningAccount不在Signerentries中。
            JLOG(ctx.j.trace()) <<
                "applyTransaction: Invalid SigningAccount.Account.";
            return tefBAD_SIGNATURE;
        }

//我们在有效签名者列表中找到了签名帐户。现在我们
//需要计算与签名者的
//公钥。
        auto const spk = txSigner.getFieldVL (sfSigningPubKey);

        if (!publicKeyType (makeSlice(spk)))
        {
            JLOG(ctx.j.trace()) <<
                "checkMultiSign: signing public key type is unknown";
            return tefBAD_SIGNATURE;
        }

        AccountID const signingAcctIDFromPubKey =
            calcAccountID(PublicKey (makeSlice(spk)));

//验证SigningAccTid和SigningAccTidFromPubKey
//属于一起。规则如下：
//
//1。虚拟账户：不在分类账中的账户。
//a.如果signingacctid==signingacctidfrompubkey和
//签名CCTID不在分类帐中，那么我们有一个幻影
//帐户。
//B.虚拟账户总是允许作为多个签名人。
//
//2。主密钥
//a.signingAccTid==signingAccTidFromPubKey和signingAccTid
//在分类帐中。
//B.如果分类帐中的签名CCID没有
//asfdisablemaster标志设置，则允许签名。
//
//3。常规键
//A.签署CCTID！=SigningAccTidFromPubKey和SigningAccTid
//在分类帐中。
//b.如果signingacctidfrompubkey==signingacctid.regularkey（从
//分类帐）然后允许签名。
//
//不允许其他签名。（2015年1月）

//在这些情况下，我们需要知道账户是否在
//分类帐。现在就确定。
        auto sleTxSignerRoot =
            ctx.view.read (keylet::account(txSignerAcctID));

        if (signingAcctIDFromPubKey == txSignerAcctID)
        {
//幻影或大师。幻影自动通过。
            if (sleTxSignerRoot)
            {
//万能钥匙。帐户不能设置为asfdisablemaster。
                std::uint32_t const signerAccountFlags =
                    sleTxSignerRoot->getFieldU32 (sfFlags);

                if (signerAccountFlags & lsfDisableMaster)
                {
                    JLOG(ctx.j.trace()) <<
                        "applyTransaction: Signer:Account lsfDisableMaster.";
                    return tefMASTER_DISABLED;
                }
            }
        }
        else
        {
//可能是普通钥匙。让我们来查一下。
//公钥必须哈希到帐户的常规密钥。
            if (!sleTxSignerRoot)
            {
                JLOG(ctx.j.trace()) <<
                    "applyTransaction: Non-phantom signer lacks account root.";
                return tefBAD_SIGNATURE;
            }

            if (!sleTxSignerRoot->isFieldPresent (sfRegularKey))
            {
                JLOG(ctx.j.trace()) <<
                    "applyTransaction: Account lacks RegularKey.";
                return tefBAD_SIGNATURE;
            }
            if (signingAcctIDFromPubKey !=
                sleTxSignerRoot->getAccountID (sfRegularKey))
            {
                JLOG(ctx.j.trace()) <<
                    "applyTransaction: Account doesn't match RegularKey.";
                return tefBAD_SIGNATURE;
            }
        }
//签名人是合法的。把他们的体重加到法定人数上。
        weightSum += iter->weight;
    }

//如果未满足仲裁，则无法执行事务。
    if (weightSum < sleAccountSigners->getFieldU32 (sfSignerQuorum))
    {
        JLOG(ctx.j.trace()) <<
            "applyTransaction: Signers failed to meet quorum.";
        return tefBAD_QUORUM;
    }

//达到法定人数。继续。
    return tesSUCCESS;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

static
void removeUnfundedOffers (ApplyView& view, std::vector<uint256> const& offers, beast::Journal viewJ)
{
    int removed = 0;

    for (auto const& index : offers)
    {
        if (auto const sleOffer = view.peek (keylet::offer (index)))
        {
//报价没有资金
            offerDelete (view, sleOffer, viewJ);
            if (++removed == unfundedOfferRemoveLimit)
                return;
        }
    }
}

/*重置上下文，放弃所做的任何更改并调整费用*/
XRPAmount
Transactor::reset(XRPAmount fee)
{
    ctx_.discard();

    auto const txnAcct = view().peek(
        keylet::account(ctx_.tx.getAccountID(sfAccount)));

    auto const balance = txnAcct->getFieldAmount (sfBalance).xrp ();

//余额应该已经在支票费/飞行前检查过了。
    assert(balance != beast::zero && (!view().open() || balance >= fee));

//如果帐户余额为零或
//根据未结分类账申请，余额低于费用
    if (fee > balance)
        fee = balance;

//因为我们重置了上下文，所以我们需要收取费用并更新
//帐户的序列号。
    txnAcct->setFieldAmount (sfBalance, balance - fee);
    txnAcct->setFieldU32 (sfSequence, ctx_.tx.getSequence() + 1);

    view().update (txnAcct);

    return fee;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
std::pair<TER, bool>
Transactor::operator()()
{
    JLOG(j_.trace()) << "apply: " << ctx_.tx.getTransactionID ();

#ifdef BEAST_DEBUG
    {
        Serializer ser;
        ctx_.tx.add (ser);
        SerialIter sit(ser.slice());
        STTx s2 (sit);

        if (! s2.isEquivalent(ctx_.tx))
        {
            JLOG(j_.fatal()) <<
                "Transaction serdes mismatch";
            JLOG(j_.info()) << to_string(ctx_.tx.getJson (0));
            JLOG(j_.fatal()) << s2.getJson (0);
            assert (false);
        }
    }
#endif

    auto result = ctx_.preclaimResult;
    if (result == tesSUCCESS)
        result = apply();

//任何事务都不能从Apply返回TemUnknown，
//而且它不能从一个预索赔中被传递进来。
    assert(result != temUNKNOWN);

    if (auto stream = j_.trace())
        stream << "preclaim result: " << transToken(result);

    bool applied = isTesSuccess (result);
    auto fee = ctx_.tx.getFieldAmount(sfFee).xrp ();

    if (ctx_.size() > oversizeMetaDataCap)
        result = tecOVERSIZE;

    if ((result == tecOVERSIZE) || (result == tecKILLED) ||
        (isTecClaimHardFail (result, view().flags())))
    {
        JLOG(j_.trace()) << "reapplying because of " << transToken(result);

        std::vector<uint256> removedOffers;

        if ((result == tecOVERSIZE) || (result == tecKILLED))
        {
            ctx_.visit (
                [&removedOffers](
                    uint256 const& index,
                    bool isDelete,
                    std::shared_ptr <SLE const> const& before,
                    std::shared_ptr <SLE const> const& after)
                {
                    if (isDelete)
                    {
                        assert (before && after);
                        if (before && after &&
                            (before->getType() == ltOFFER) &&
                            (before->getFieldAmount(sfTakerPays) == after->getFieldAmount(sfTakerPays)))
                        {
//撤销已发现或未提供资金的要约
                            removedOffers.push_back (index);
                        }
                    }
                });
        }

//重置上下文，可能调整费用
        fee = reset(fee);

//如有必要，删除在处理过程中发现没有资金的任何报价。
        if ((result == tecOVERSIZE) || (result == tecKILLED))
            removeUnfundedOffers (view(), removedOffers, ctx_.app.journal ("View"));

        applied = true;
    }

    if (applied)
    {
//检查不变量：如果没有返回'tecinvariant_failed'，我们可以
//继续应用Tx
        result = ctx_.checkInvariants(result, fee);

        if (result == tecINVARIANT_FAILED)
        {
//如果不变量检查再次失败，请重置上下文并
//尝试只收取费用。
            fee = reset(fee);

//再次检查不变量以确保费用申请不会
//违反不变量。
            result = ctx_.checkInvariants(result, fee);
        }

//我们运行了不变量检查器，在某些情况下，
//返回TEF错误代码。在这种情况下不要应用事务。
        if (!isTecClaim(result) && !isTesSuccess(result))
            applied = false;
    }

    if (applied)
    {
//事务完全成功或（不允许重试，并且
//交易可能收取费用）

//事务处理程序和不变的检查程序保证
//*从不*触发，但如果它发生，不知何故，不允许发送
//那要负费用。
        if (fee < beast::zero)
            Throw<std::logic_error> ("fee charged is negative!");

//收取他们规定的任何费用。费用已经
//从发出的账户余额中扣除
//交易。我们只需要把它记在分类帐上就行了
//标题。
        if (!view().open() && fee != beast::zero)
            ctx_.destroyXRP (fee);

//一旦调用Apply，我们将无法再查看View（）。
        ctx_.apply(result);
    }

    JLOG(j_.trace()) << (applied ? "applied" : "not applied") << transToken(result);

    return { result, applied };
}

}
