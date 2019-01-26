
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

#include <ripple/app/misc/CanonicalTXSet.h>
#include <boost/range/adaptor/transformed.hpp>

namespace ripple {

bool CanonicalTXSet::Key::operator< (Key const& rhs) const
{
    if (mAccount < rhs.mAccount) return true;

    if (mAccount > rhs.mAccount) return false;

    if (mSeq < rhs.mSeq) return true;

    if (mSeq > rhs.mSeq) return false;

    return mTXid < rhs.mTXid;
}

bool CanonicalTXSet::Key::operator> (Key const& rhs) const
{
    if (mAccount > rhs.mAccount) return true;

    if (mAccount < rhs.mAccount) return false;

    if (mSeq > rhs.mSeq) return true;

    if (mSeq < rhs.mSeq) return false;

    return mTXid > rhs.mTXid;
}

bool CanonicalTXSet::Key::operator<= (Key const& rhs) const
{
    if (mAccount < rhs.mAccount) return true;

    if (mAccount > rhs.mAccount) return false;

    if (mSeq < rhs.mSeq) return true;

    if (mSeq > rhs.mSeq) return false;

    return mTXid <= rhs.mTXid;
}

bool CanonicalTXSet::Key::operator>= (Key const& rhs)const
{
    if (mAccount > rhs.mAccount) return true;

    if (mAccount < rhs.mAccount) return false;

    if (mSeq > rhs.mSeq) return true;

    if (mSeq < rhs.mSeq) return false;

    return mTXid >= rhs.mTXid;
}

uint256 CanonicalTXSet::accountKey (AccountID const& account)
{
    uint256 ret = beast::zero;
    memcpy (
        ret.begin (),
        account.begin (),
        account.size ());
    ret ^= salt_;
    return ret;
}

void CanonicalTXSet::insert (std::shared_ptr<STTx const> const& txn)
{
    map_.insert (
        std::make_pair (
            Key (
                accountKey (txn->getAccountID(sfAccount)),
                txn->getSequence (),
                txn->getTransactionID ()),
            txn));
}

std::vector<std::shared_ptr<STTx const>>
CanonicalTXSet::prune(AccountID const& account,
    std::uint32_t const seq)
{
    auto effectiveAccount = accountKey (account);

    Key keyLow(effectiveAccount, seq, beast::zero);
    Key keyHigh(effectiveAccount, seq+1, beast::zero);

    auto range = boost::make_iterator_range(
        map_.lower_bound(keyLow),
        map_.lower_bound(keyHigh));
    auto txRange = boost::adaptors::transform(range,
        [](auto const& p) { return p.second; });

    std::vector<std::shared_ptr<STTx const>> result(
        txRange.begin(), txRange.end());

    map_.erase(range.begin(), range.end());
    return result;
}

} //涟漪
