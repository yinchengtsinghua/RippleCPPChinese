
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
    版权所有（c）2014 Ripple Labs Inc.

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

#include <ripple/app/tx/impl/SetSignerList.h>

#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STArray.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/STTx.h>
#include <algorithm>
#include <cstdint>

namespace ripple {

//我们已经准备好将来有多个签名者列表，
//但我们还不需要它们。所以目前我们是手动的
//在所有情况下，将sfsignerListed设置为零。
static std::uint32_t const defaultSignerListID_ = 0;

std::tuple<NotTEC, std::uint32_t,
    std::vector<SignerEntries::SignerEntry>,
        SetSignerList::Operation>
SetSignerList::determineOperation(STTx const& tx,
    ApplyFlags flags, beast::Journal j)
{
//检查法定人数。非零仲裁意味着我们正在创建或替换
//名单。法定人数为零意味着我们正在销毁名单。
    auto const quorum = tx[sfSignerQuorum];
    std::vector<SignerEntries::SignerEntry> sign;
    Operation op = unknown;

    bool const hasSignerEntries(tx.isFieldPresent(sfSignerEntries));
    if (quorum && hasSignerEntries)
    {
        auto signers = SignerEntries::deserialize(tx, j, "transaction");

        if (signers.second != tesSUCCESS)
            return std::make_tuple(signers.second, quorum, sign, op);

        std::sort(signers.first.begin(), signers.first.end());

//保存反序列化列表供以后使用。
        sign = std::move(signers.first);
        op = set;
    }
    else if ((quorum == 0) && !hasSignerEntries)
    {
        op = destroy;
    }

    return std::make_tuple(tesSUCCESS, quorum, sign, op);
}

NotTEC
SetSignerList::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featureMultiSign))
        return temDISABLED;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto const result = determineOperation(ctx.tx, ctx.flags, ctx.j);
    if (std::get<0>(result) != tesSUCCESS)
        return std::get<0>(result);

    if (std::get<3>(result) == unknown)
    {
//既不是布景，也不是毁灭。畸形的
        JLOG(ctx.j.trace()) <<
            "Malformed transaction: Invalid signer set list format.";
        return temMALFORMED;
    }

    if (std::get<3>(result) == set)
    {
//验证我们的设置。
        auto const account = ctx.tx.getAccountID(sfAccount);
        NotTEC const ter =
            validateQuorumAndSignerEntries(std::get<1>(result),
                std::get<2>(result), account, ctx.j);
        if (ter != tesSUCCESS)
        {
            return ter;
        }
    }

    return preflight2 (ctx);
}

TER
SetSignerList::doApply ()
{
//执行决定的操作precompute（）。
    switch (do_)
    {
    case set:
        return replaceSignerList ();

    case destroy:
        return destroySignerList ();

    default:
        break;
    }
assert (false); //不可能到这里。
    return temMALFORMED;
}

void
SetSignerList::preCompute()
{
//获取仲裁和操作信息。
    auto result = determineOperation(ctx_.tx, view().flags(), j_);
    assert(std::get<0>(result) == tesSUCCESS);
    assert(std::get<3>(result) != unknown);

    quorum_ = std::get<1>(result);
    signers_ = std::get<2>(result);
    do_ = std::get<3>(result);

    return Transactor::preCompute();
}

NotTEC
SetSignerList::validateQuorumAndSignerEntries (
    std::uint32_t quorum,
        std::vector<SignerEntries::SignerEntry> const& signers,
            AccountID const& account,
                beast::Journal j)
{
//如果列表中的条目太多或太少，则拒绝。
    {
        std::size_t const signerCount = signers.size ();
        if ((signerCount < STTx::minMultiSigners)
            || (signerCount > STTx::maxMultiSigners))
        {
            JLOG(j.trace()) << "Too many or too few signers in signer list.";
            return temMALFORMED;
        }
    }

//确保没有重复的签名者。
    assert(std::is_sorted(signers.begin(), signers.end()));
    if (std::adjacent_find (
        signers.begin (), signers.end ()) != signers.end ())
    {
        JLOG(j.trace()) << "Duplicate signers in signer list";
        return temBAD_SIGNER;
    }

//确保没有签名者引用此帐户。同时确保
//达到法定人数。
    std::uint64_t allSignersWeight (0);
    for (auto const& signer : signers)
    {
        std::uint32_t const weight = signer.weight;
        if (weight <= 0)
        {
            JLOG(j.trace()) << "Every signer must have a positive weight.";
            return temBAD_WEIGHT;
        }

        allSignersWeight += signer.weight;

        if (signer.account == account)
        {
            JLOG(j.trace()) << "A signer may not self reference account.";
            return temBAD_SIGNER;
        }

//不要验证签名者帐户是否存在。不存在的帐户
//可以是虚拟账户（允许）。
    }
    if ((quorum <= 0) || (allSignersWeight < quorum))
    {
        JLOG(j.trace()) << "Quorum is unreachable";
        return temBAD_QUORUM;
    }
    return tesSUCCESS;
}

TER
SetSignerList::replaceSignerList ()
{
    auto const accountKeylet = keylet::account (account_);
    auto const ownerDirKeylet = keylet::ownerDir (account_);
    auto const signerListKeylet = keylet::signers (account_);

//这可以是创建或替换。抢先删除任何
//旧签名者列表。可能会减少储备，所以这是以前做过的
//检查储备。
    if (TER const ter = removeSignersFromLedger (
        accountKeylet, ownerDirKeylet, signerListKeylet))
            return ter;

    auto const sle = view().peek(accountKeylet);

//计算新准备金。核实该账户有足够的资金来支付准备金。
    std::uint32_t const oldOwnerCount {(*sle)[sfOwnerCount]};

//根据功能多信号保留更改所需的保留…
    int addedOwnerCount {1};
    std::uint32_t flags {lsfOneOwnerCount};
    if (! ctx_.view().rules().enabled(featureMultiSignReserve))
    {
        addedOwnerCount = signerCountBasedOwnerCountDelta (signers_.size ());
        flags = 0;
    }

    XRPAmount const newReserve {
        view().fees().accountReserve(oldOwnerCount + addedOwnerCount)};

//我们根据初始余额核对准备金，因为我们想
//允许动用储备金支付费用。这种行为是一致的
//使用CreateTicket。
    if (mPriorBalance < newReserve)
        return tecINSUFFICIENT_RESERVE;

//一切都是杜基。将ltsigner_列表添加到分类帐。
    auto signerList = std::make_shared<SLE>(signerListKeylet);
    view().insert (signerList);
    writeSignersToSLE (signerList, flags);

    auto viewJ = ctx_.app.journal ("View");
//将签名者列表添加到帐户的目录中。
    auto const page = dirAdd (ctx_.view (), ownerDirKeylet,
        signerListKeylet.key, false, describeOwnerDir (account_), viewJ);

    JLOG(j_.trace()) << "Create signer list for account " <<
        toBase58 (account_) << ": " << (page ? "success" : "failure");

    if (!page)
        return tecDIR_FULL;

    signerList->setFieldU64 (sfOwnerNode, *page);

//如果我们成功，新条目将与
//创造者的储备。
    adjustOwnerCount(view(), sle, addedOwnerCount, viewJ);
    return tesSUCCESS;
}

TER
SetSignerList::destroySignerList ()
{
    auto const accountKeylet = keylet::account (account_);
//只有在主密钥
//已启用或有常规密钥。
    SLE::pointer ledgerEntry = view().peek (accountKeylet);
    if ((ledgerEntry->isFlag (lsfDisableMaster)) &&
        (!ledgerEntry->isFieldPresent (sfRegularKey)))
            return tecNO_ALTERNATIVE_KEY;

    auto const ownerDirKeylet = keylet::ownerDir (account_);
    auto const signerListKeylet = keylet::signers (account_);
    return removeSignersFromLedger(
        accountKeylet, ownerDirKeylet, signerListKeylet);
}

TER
SetSignerList::removeSignersFromLedger (Keylet const& accountKeylet,
        Keylet const& ownerDirKeylet, Keylet const& signerListKeylet)
{
//我们必须检查当前的签名者，这样我们才能知道
//减少所有者数量。
    SLE::pointer signers = view().peek (signerListKeylet);

//如果签名者列表不存在，我们已经成功删除了它。
    if (!signers)
        return tesSUCCESS;

//有两种不同的方法可以管理所有者计数。
//如果设置了lsfoneownercount位，则只删除一个所有者计数。
//否则，请使用多信号保留前修正计算。
    int removeFromOwnerCount = -1;
    if ((signers->getFlags() & lsfOneOwnerCount) == 0)
    {
        STArray const& actualList = signers->getFieldArray (sfSignerEntries);
        removeFromOwnerCount =
            signerCountBasedOwnerCountDelta (actualList.size()) * -1;
    }

//从帐户目录中删除节点。
    auto const hint = (*signers)[sfOwnerNode];

    if (! ctx_.view().dirRemove(
            ownerDirKeylet, hint, signerListKeylet.key, false))
    {
        return tefBAD_LEDGER;
    }

    auto viewJ = ctx_.app.journal("View");
    adjustOwnerCount(
        view(), view().peek(accountKeylet), removeFromOwnerCount, viewJ);

    ctx_.view().erase (signers);

    return tesSUCCESS;
}

void
SetSignerList::writeSignersToSLE (
    SLE::pointer const& ledgerEntry, std::uint32_t flags) const
{
//分配仲裁、默认签名列表和标志。
    ledgerEntry->setFieldU32 (sfSignerQuorum, quorum_);
    ledgerEntry->setFieldU32 (sfSignerListID, defaultSignerListID_);
if (flags) //仅当标志非默认时才设置（默认为零）。
        ledgerEntry->setFieldU32 (sfFlags, flags);

//一次创建一个签名者列表。
    STArray toLedger (signers_.size ());
    for (auto const& entry : signers_)
    {
        toLedger.emplace_back(sfSignerEntry);
        STObject& obj = toLedger.back();
        obj.reserve (2);
        obj.setAccountID (sfAccount, entry.account);
        obj.setFieldU16 (sfSignerWeight, entry.weight);
    }

//分配签名人。
    ledgerEntry->setFieldArray (sfSignerEntries, toLedger);
}

//返回类型已签名，因此它与第三个参数兼容
//AdjustownerCount（）（必须签名）。
//
//注意：这种计算与签名者列表关联的所有者计数的方法
//在通过FeatureMultiSignReserve修正之前有效。一旦它
//通过后，只有1个所有者计数与签名者列表关联。
int
SetSignerList::signerCountBasedOwnerCountDelta (std::size_t entryCount)
{
//我们总是计算所有者数量的全部变化，考虑到：
//o事实上，我们正在添加/删除签名列表，并且
//o计算列表中的条目数量。
//我们可以摆脱这一点，因为列表没有递增调整；
//我们添加或删除整个列表。
//
//规则是：
//o仅仅拥有一个签名者就需要花费2个所有者计数单位。
//o列表中的每个签名者需要再花费1个所有者计数单位。
//因此，至少添加一个包含1个条目的签名者列表需要花费3个所有者计数
//单位。带有8个条目的签名者列表将花费10个所有者计数单位。
//
//静态类型转换应该始终是安全的，因为EntryCount应该始终
//在1到8之间。我们有很大的成长空间。
    assert (entryCount >= STTx::minMultiSigners);
    assert (entryCount <= STTx::maxMultiSigners);
    return 2 + static_cast<int>(entryCount);
}

} //命名空间波纹
