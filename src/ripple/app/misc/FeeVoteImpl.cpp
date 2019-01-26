
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

#include <ripple/protocol/st.h>
#include <ripple/app/misc/FeeVote.h>
#include <ripple/app/main/Application.h>
#include <ripple/protocol/STValidation.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/beast/utility/Journal.h>

namespace ripple {

namespace detail {

template <typename Integer>
class VotableInteger
{
private:
    using map_type = std::map <Integer, int>;
Integer mCurrent;   //当前设置
Integer mTarget;    //我们想要的环境
    map_type mVoteMap;

public:
    VotableInteger (Integer current, Integer target)
        : mCurrent (current)
        , mTarget (target)
    {
//增加我们的选票
        ++mVoteMap[mTarget];
    }

    void
    addVote(Integer vote)
    {
        ++mVoteMap[vote];
    }

    void
    noVote()
    {
        addVote (mCurrent);
    }

    Integer
    getVotes() const;
};

template <class Integer>
Integer
VotableInteger <Integer>::getVotes() const
{
    Integer ourVote = mCurrent;
    int weight = 0;
    for (auto const& e : mVoteMap)
    {
//取当前值和目标值之间的最大投票值，包括
        if ((e.first <= std::max (mTarget, mCurrent)) &&
                (e.first >= std::min (mTarget, mCurrent)) &&
                (e.second > weight))
        {
            ourVote = e.first;
            weight = e.second;
        }
    }

    return ourVote;
}

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class FeeVoteImpl : public FeeVote
{
private:
    Setup target_;
    beast::Journal journal_;

public:
    FeeVoteImpl (Setup const& setup, beast::Journal journal);

    void
    doValidation (std::shared_ptr<ReadView const> const& lastClosedLedger,
        STValidation::FeeSettings& fees) override;

    void
    doVoting (std::shared_ptr<ReadView const> const& lastClosedLedger,
        std::vector<STValidation::pointer> const& parentValidations,
        std::shared_ptr<SHAMap> const& initialPosition) override;
};

//——————————————————————————————————————————————————————————————

FeeVoteImpl::FeeVoteImpl (Setup const& setup, beast::Journal journal)
    : target_ (setup)
    , journal_ (journal)
{
}

void
FeeVoteImpl::doValidation(
    std::shared_ptr<ReadView const> const& lastClosedLedger,
        STValidation::FeeSettings& fees)
{
    if (lastClosedLedger->fees().base != target_.reference_fee)
    {
        JLOG(journal_.info()) <<
            "Voting for base fee of " << target_.reference_fee;

        fees.baseFee = target_.reference_fee;
    }

    if (lastClosedLedger->fees().accountReserve(0) != target_.account_reserve)
    {
        JLOG(journal_.info()) <<
            "Voting for base reserve of " << target_.account_reserve;

        fees.reserveBase = target_.account_reserve;
    }

    if (lastClosedLedger->fees().increment != target_.owner_reserve)
    {
        JLOG(journal_.info()) <<
            "Voting for reserve increment of " << target_.owner_reserve;

        fees.reserveIncrement = target_.owner_reserve;
    }
}

void
FeeVoteImpl::doVoting(
    std::shared_ptr<ReadView const> const& lastClosedLedger,
    std::vector<STValidation::pointer> const& set,
    std::shared_ptr<SHAMap> const& initialPosition)
{
//LCL必须是标志分类帐
    assert ((lastClosedLedger->info().seq % 256) == 0);

    detail::VotableInteger<std::uint64_t> baseFeeVote (
        lastClosedLedger->fees().base, target_.reference_fee);

    detail::VotableInteger<std::uint32_t> baseReserveVote (
        lastClosedLedger->fees().accountReserve(0).drops(), target_.account_reserve);

    detail::VotableInteger<std::uint32_t> incReserveVote (
        lastClosedLedger->fees().increment, target_.owner_reserve);

    for (auto const& val : set)
    {
        if (val->isTrusted ())
        {
            if (val->isFieldPresent (sfBaseFee))
            {
                baseFeeVote.addVote (val->getFieldU64 (sfBaseFee));
            }
            else
            {
                baseFeeVote.noVote ();
            }

            if (val->isFieldPresent (sfReserveBase))
            {
                baseReserveVote.addVote (val->getFieldU32 (sfReserveBase));
            }
            else
            {
                baseReserveVote.noVote ();
            }

            if (val->isFieldPresent (sfReserveIncrement))
            {
                incReserveVote.addVote (val->getFieldU32 (sfReserveIncrement));
            }
            else
            {
                incReserveVote.noVote ();
            }
        }
    }

//选择我们的位置
    std::uint64_t const baseFee = baseFeeVote.getVotes ();
    std::uint32_t const baseReserve = baseReserveVote.getVotes ();
    std::uint32_t const incReserve = incReserveVote.getVotes ();
    std::uint32_t const feeUnits = target_.reference_fee_units;
    auto const seq = lastClosedLedger->info().seq + 1;

//将交易添加到我们的位置
    if ((baseFee != lastClosedLedger->fees().base) ||
            (baseReserve != lastClosedLedger->fees().accountReserve(0)) ||
            (incReserve != lastClosedLedger->fees().increment))
    {
        JLOG(journal_.warn()) <<
            "We are voting for a fee change: " << baseFee <<
            "/" << baseReserve <<
            "/" << incReserve;

        STTx feeTx (ttFEE,
            [seq,baseFee,baseReserve,incReserve,feeUnits](auto& obj)
            {
                obj[sfAccount] = AccountID();
                obj[sfLedgerSequence] = seq;
                obj[sfBaseFee] = baseFee;
                obj[sfReserveBase] = baseReserve;
                obj[sfReserveIncrement] = incReserve;
                obj[sfReferenceFeeUnits] = feeUnits;
            });

        uint256 txID = feeTx.getTransactionID ();

        JLOG(journal_.warn()) <<
            "Vote: " << txID;

        Serializer s;
        feeTx.add (s);

        auto tItem = std::make_shared<SHAMapItem> (txID, s.peekData ());

        if (!initialPosition->addGiveItem (tItem, true, false))
        {
            JLOG(journal_.warn()) <<
                "Ledger already had fee change";
        }
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

FeeVote::Setup
setup_FeeVote (Section const& section)
{
    FeeVote::Setup setup;
    set (setup.reference_fee, "reference_fee", section);
    set (setup.account_reserve, "account_reserve", section);
    set (setup.owner_reserve, "owner_reserve", section);
    return setup;
}

std::unique_ptr<FeeVote>
make_FeeVote (FeeVote::Setup const& setup, beast::Journal journal)
{
    return std::make_unique<FeeVoteImpl> (setup, journal);
}

} //涟漪
