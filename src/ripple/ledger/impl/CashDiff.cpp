
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
    版权所有（c）2016 Ripple Labs Inc.

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

#include <ripple/ledger/CashDiff.h>
#include <ripple/ledger/detail/ApplyStateTable.h>
#include <ripple/protocol/st.h>
#include <boost/container/static_vector.hpp>
#include <cassert>
#include <cstdlib>                                  //性病：：

namespace ripple {
namespace detail {

//在单个应用程序表中汇总现金变化的数据结构。
struct CashSummary
{
    explicit CashSummary() = default;

//排序向量。std：：maps的所有向量都将被填充。
    std::vector<std::pair<
        AccountID, XRPAmount>> xrpChanges;

    std::vector<std::pair<
        std::tuple<AccountID, AccountID, Currency>, STAmount>> trustChanges;

    std::vector<std::pair<
        std::tuple<AccountID, AccountID, Currency>, bool>> trustDeletions;

    std::vector<std::pair<
        std::tuple<AccountID, std::uint32_t>,
        CashDiff::OfferAmounts>> offerChanges;

//请注意，offerAmount保留删除前*的金额。
    std::vector<std::pair<
        std::tuple<AccountID, std::uint32_t>,
        CashDiff::OfferAmounts>> offerDeletions;

    bool hasDiff () const
    {
        return !xrpChanges.empty()
            || !trustChanges.empty()
            || !trustDeletions.empty()
            || !offerChanges.empty()
            || !offerDeletions.empty();
    }

    void reserve (size_t newCap)
    {
        xrpChanges.reserve (newCap);
        trustChanges.reserve (newCap);
        trustDeletions.reserve (newCap);
        offerChanges.reserve (newCap);
        offerDeletions.reserve (newCap);
    }

    void shrink_to_fit()
    {
        xrpChanges.shrink_to_fit();
        trustChanges.shrink_to_fit();
        trustDeletions.shrink_to_fit();
        offerChanges.shrink_to_fit();
        offerDeletions.shrink_to_fit();
    }

    void sort()
    {
        std::sort (xrpChanges.begin(), xrpChanges.end());
        std::sort (trustChanges.begin(), trustChanges.end());
        std::sort (trustDeletions.begin(), trustDeletions.end());
        std::sort (offerChanges.begin(), offerChanges.end());
        std::sort (offerDeletions.begin(), offerDeletions.end());
    }
};

//处理零报价删除（）
//
//较旧的支付代码可能会设置报价的接受方支付，而接受方获得
//零，以后再把报盘结清。更新的版本
//可能会更主动地取消优惠。我们试图用纸
//在这个差异上。
//
//检查两个条件：
//
//o接受方付费和接受方付费均设为零的修改报价是
//添加到offerDeletions（而不是offerChanges）。
//
//o删除前为零的任何已删除报价将被忽略。它将
//在报价首次设为零时被视为已删除。
//
//返回的bool指示是否处理了传入的数据。
//这允许调用者避免进一步处理。
static bool treatZeroOfferAsDeletion (CashSummary& result, bool isDelete,
    std::shared_ptr<SLE const> const& before,
    std::shared_ptr<SLE const> const& after)
{
    using beast::zero;

    if (!before)
        return false;

    auto const& prev = *before;

    if (isDelete)
    {
        if (prev.getType() == ltOFFER &&
            prev[sfTakerPays] == zero && prev[sfTakerGets] == zero)
        {
//报价在归零时被视为已删除。
            return true;
        }
    }
    else
    {
//修改
        if (!after)
            return false;

        auto const& cur = *after;
        if (cur.getType() == ltOFFER &&
            cur[sfTakerPays] == zero && cur[sfTakerGets] == zero)
        {
//忽略或视为删除。
            auto const oldTakerPays = prev[sfTakerPays];
            auto const oldTakerGets = prev[sfTakerGets];
            if (oldTakerPays != zero && oldTakerGets != zero)
            {
                result.offerDeletions.push_back (
                    std::make_pair (
                        std::make_tuple (prev[sfAccount], prev[sfSequence]),
                    CashDiff::OfferAmounts {{oldTakerPays, oldTakerGets}}));
                return true;
            }
        }
    }
    return false;
}

static bool getBasicCashFlow (CashSummary& result, bool isDelete,
    std::shared_ptr<SLE const> const& before,
    std::shared_ptr<SLE const> const& after)
{
    if (isDelete)
    {
        if (!before)
            return false;

        auto const& prev = *before;
        switch(prev.getType())
        {
        case ltACCOUNT_ROOT:
            result.xrpChanges.push_back (
                std::make_pair (prev[sfAccount], XRPAmount {0}));
            return true;

        case ltRIPPLE_STATE:
            result.trustDeletions.push_back(
                std::make_pair (
                    std::make_tuple (
                        prev[sfLowLimit].getIssuer(),
                        prev[sfHighLimit].getIssuer(),
                        prev[sfBalance].getCurrency()), false));
            return true;

        case ltOFFER:
            result.offerDeletions.push_back (
                std::make_pair (
                    std::make_tuple (prev[sfAccount], prev[sfSequence]),
                CashDiff::OfferAmounts {{
                    prev[sfTakerPays],
                    prev[sfTakerGets]}}));
            return true;

        default:
            return false;
        }
    }
    else
    {
//插入或修改
        if (!after)
        {
            assert (after);
            return false;
        }

        auto const& cur = *after;
        switch(cur.getType())
        {
        case ltACCOUNT_ROOT:
        {
            auto const curXrp = cur[sfBalance].xrp();
            if (!before || (*before)[sfBalance].xrp() != curXrp)
                result.xrpChanges.push_back (
                    std::make_pair (cur[sfAccount], curXrp));
            return true;
        }
        case ltRIPPLE_STATE:
        {
            auto const curBalance = cur[sfBalance];
            if (!before || (*before)[sfBalance] != curBalance)
                result.trustChanges.push_back (
                    std::make_pair (
                        std::make_tuple (
                            cur[sfLowLimit].getIssuer(),
                            cur[sfHighLimit].getIssuer(),
                            curBalance.getCurrency()),
                        curBalance));
            return true;
        }
        case ltOFFER:
        {
            auto const curTakerPays = cur[sfTakerPays];
            auto const curTakerGets = cur[sfTakerGets];
            if (!before || (*before)[sfTakerGets] != curTakerGets ||
                (*before)[sfTakerPays] != curTakerPays)
            {
                result.offerChanges.push_back (
                    std::make_pair (
                        std::make_tuple (cur[sfAccount], cur[sfSequence]),
                    CashDiff::OfferAmounts {{curTakerPays, curTakerGets}}));
            }
            return true;
        }
        default:
            break;
        }
    }
    return false;
}

//从ApplyStateTable中提取最终现金状态。
static CashSummary
getCashFlow (ReadView const& view, CashFilter f, ApplyStateTable const& table)
{
    CashSummary result;
    result.reserve (table.size());

//根据传入的筛选器标志创建筛选器容器。
    using FuncType = decltype (&getBasicCashFlow);
    boost::container::static_vector<FuncType, 2> filters;

    if ((f & CashFilter::treatZeroOfferAsDeletion) != CashFilter::none)
        filters.push_back (treatZeroOfferAsDeletion);

    filters.push_back (&getBasicCashFlow);

    auto each = [&result, &filters](uint256 const& key, bool isDelete,
        std::shared_ptr<SLE const> const& before,
        std::shared_ptr<SLE const> const& after) {
        auto discarded =
            std::find_if (filters.begin(), filters.end(),
                [&result, isDelete, &before, &after] (FuncType func) {
                    return func (result, isDelete, before, after);
        });
        (void) discarded;
    };

    table.visit (view, each);
    result.sort();
    result.shrink_to_fit();
    return result;
}

} //细节

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//保存所有与cashdiff相关的数据。
class CashDiff::Impl
{
private:
//注意两个ApplyStateTables之间损坏的xrp的差异。
    struct DropsGone
    {
        XRPAmount lhs;
        XRPAmount rhs;
    };

    ReadView const& view_;

std::size_t commonKeys_ = 0;  //右舵和左舵通用的钥匙数量。
std::size_t lhsKeys_ = 0;     //左钥匙而不是右钥匙的数量。
std::size_t rhsKeys_ = 0;     //右舵驾驶但不是左舵驾驶的钥匙数。
    boost::optional<DropsGone> dropsGone_;
    detail::CashSummary lhsDiffs_;
    detail::CashSummary rhsDiffs_;

public:
//构造函数。
    Impl (ReadView const& view,
          CashFilter lhsFilter, detail::ApplyStateTable const& lhs,
          CashFilter rhsFilter, detail::ApplyStateTable const& rhs)
    : view_ (view)
    {
        findDiffs (lhsFilter, lhs, rhsFilter, rhs);
    }

    std::size_t commonCount() const
    {
        return commonKeys_;
    }

    std::size_t lhsOnlyCount() const
    {
        return lhsKeys_;
    }

    std::size_t rhsOnlyCount () const
    {
        return rhsKeys_;
    }

    bool hasDiff () const
    {
        return dropsGone_ != boost::none
            || lhsDiffs_.hasDiff()
            || rhsDiffs_.hasDiff();
    }

    int xrpRoundToZero () const;

//过滤掉足够小的浮动差异
//点噪声
    bool rmDust ();

//从给定方面消除报价删除差异
    bool rmLhsDeletedOffers();
    bool rmRhsDeletedOffers();

private:
    void findDiffs (
        CashFilter lhsFilter, detail::ApplyStateTable const& lhs,
        CashFilter rhsFilter, detail::ApplyStateTable const& rhs);
};

//用于计算单个cashdiff向量中差异类型的模板函数。
//假设这些向量已排序。
//
//返回数组：
//[0]两个向量中的键计数。
//[1]仅在左侧显示钥匙计数。
//[2]仅在RHS中存在的密钥计数。
template <typename T, typename U>
static std::array<std::size_t, 3> countKeys (
    std::vector<std::pair<T, U>> const& lhs,
    std::vector<std::pair<T, U>> const& rhs)
{
std::array<std::size_t, 3> ret {};  //零初始化；

    auto lhsItr = lhs.cbegin();
    auto rhsItr = rhs.cbegin();

    while (lhsItr != lhs.cend() || rhsItr != rhs.cend())
    {
        if (lhsItr == lhs.cend())
        {
//右侧有一个不在左侧的条目。
            ret[2] += 1;
            ++rhsItr;
        }
        else if (rhsItr == rhs.cend())
        {
//左侧有一个不在右侧的条目。
            ret[1] += 1;
            ++lhsItr;
        }
        else if (lhsItr->first < rhsItr->first)
        {
//这把钥匙只在左手边。
            ret[1] += 1;
            ++lhsItr;
        }
        else if (rhsItr->first < lhsItr->first)
        {
//此钥匙仅在右侧。
            ret[2] += 1;
            ++rhsItr;
        }
        else
        {
//两个向量中都存在等价键。
            ret[0] += 1;
            ++lhsItr;
            ++rhsItr;
        }
    }
    return ret;
}

//给定两个cashsummary实例，计算键。假设两者
//现金汇总有已排序的条目。
//
//返回数组：
//[0]两个向量中的键计数。
//[1]仅在左侧显示钥匙计数。
//[2]仅在RHS中存在的密钥计数。
static std::array<std::size_t, 3>
countKeys (detail::CashSummary const& lhs, detail::CashSummary const& rhs)
{
std::array<std::size_t, 3> ret {};  //零初始化；

//lambda以添加新结果。
    auto addIn = [&ret] (std::array<std::size_t, 3> const& a)
    {
        std::transform (a.cbegin(), a.cend(),
            ret.cbegin(), ret.begin(), std::plus<std::size_t>());
    };
    addIn (countKeys(lhs.xrpChanges,     rhs.xrpChanges));
    addIn (countKeys(lhs.trustChanges,   rhs.trustChanges));
    addIn (countKeys(lhs.trustDeletions, rhs.trustDeletions));
    addIn (countKeys(lhs.offerChanges,   rhs.offerChanges));
    addIn (countKeys(lhs.offerDeletions, rhs.offerDeletions));
    return ret;
}

int CashDiff::Impl::xrpRoundToZero () const
{
//本案有一项报价变更，同时存在于左侧和右侧。
//那个offerchange应该为takergets提供xrp。应该有一个1
//拉什迪夫和拉什迪夫之间的差。
    if (lhsDiffs_.offerChanges.size() != 1 ||
        rhsDiffs_.offerChanges.size() != 1)
            return 0;

    if (! lhsDiffs_.offerChanges[0].second.takerGets().native() ||
        ! rhsDiffs_.offerChanges[0].second.takerGets().native())
            return 0;

    bool const lhsBigger =
        lhsDiffs_.offerChanges[0].second.takerGets().mantissa() >
        rhsDiffs_.offerChanges[0].second.takerGets().mantissa();

    detail::CashSummary const& bigger = lhsBigger ? lhsDiffs_ : rhsDiffs_;
    detail::CashSummary const& smaller = lhsBigger ? rhsDiffs_ : lhsDiffs_;
    if (bigger.offerChanges[0].second.takerGets().mantissa() -
        smaller.offerChanges[0].second.takerGets().mantissa() != 1)
           return 0;

//报价变更中XRP余额较小的一方应
//两个XRP差异。另一方应该没有xrp差异。
    if (smaller.xrpChanges.size() != 2)
        return 0;
    if (! bigger.xrpChanges.empty())
        return 0;

//不应该有其他的区别。
    if (!smaller.trustChanges.empty()   || !bigger.trustChanges.empty()   ||
        !smaller.trustDeletions.empty() || !bigger.trustDeletions.empty() ||
        !smaller.offerDeletions.empty() || !bigger.offerDeletions.empty())
            return 0;

//返回出现问题的一侧。
    return lhsBigger ? -1 : 1;
}

//比较两个cashdiff:：offerAmount并返回true if的函数
//区别在于灰尘大小。
static bool diffIsDust (
    CashDiff::OfferAmounts const& lhs, CashDiff::OfferAmounts const& rhs)
{
    for (auto i = 0; i < lhs.count(); ++i)
    {
        if (!diffIsDust (lhs[i], rhs[i]))
            return false;
    }
    return true;
}

//模板函数，用于清除单个cashdiff向量中的灰尘。
template <typename T, typename U, typename L>
static bool
rmVecDust (
    std::vector<std::pair<T, U>>& lhs,
    std::vector<std::pair<T, U>>& rhs,
    L&& justDust)
{
    static_assert (
        std::is_same<bool,
        decltype (justDust (lhs[0].second, rhs[0].second))>::value,
        "Invalid lambda passed to rmVecDust");

    bool dustWasRemoved = false;
    auto lhsItr = lhs.begin();
    while (lhsItr != lhs.end())
    {
        using value_t = std::pair<T, U>;
        auto const found = std::equal_range (rhs.begin(), rhs.end(), *lhsItr,
            [] (value_t const& a, value_t const& b)
            {
                return a.first < b.first;
            });

        if (found.first != found.second)
        {
//相同的条目在左侧和右侧都发生了更改。检查是否
//这些差异很小，可以消除。
            if (justDust (lhsItr->second, found.first->second))
            {
                dustWasRemoved = true;
                rhs.erase (found.first);
//使用erase的返回值来躲避无效的迭代器。
                lhsItr = lhs.erase (lhsItr);
                continue;
            }
        }
        ++lhsItr;
    }
    return dustWasRemoved;
}

bool CashDiff::Impl::rmDust ()
{
    bool removedDust = false;

//四个容器可以小（浮点样式）
//金额差异：xrpchanges、trustchanges、offerchanges和
//报价删除。用步枪穿过那些容器
//在左侧和右侧几乎相同的条目。

//XRP.我们称之为2滴或更少的灰尘。
    removedDust |= rmVecDust (lhsDiffs_.xrpChanges, rhsDiffs_.xrpChanges,
        [](XRPAmount const& lhs, XRPAmount const& rhs)
        {
            return diffIsDust (lhs, rhs);
        });

//信任变更。
    removedDust |= rmVecDust (lhsDiffs_.trustChanges, rhsDiffs_.trustChanges,
        [](STAmount const& lhs, STAmount const& rhs)
        {
            return diffIsDust (lhs, rhs);
        });

//提供更改。
    removedDust |= rmVecDust (lhsDiffs_.offerChanges, rhsDiffs_.offerChanges,
        [](CashDiff::OfferAmounts const& lhs, CashDiff::OfferAmounts const& rhs)
        {
            return diffIsDust (lhs, rhs);
        });

//提供删除。
    removedDust |= rmVecDust (
        lhsDiffs_.offerDeletions, rhsDiffs_.offerDeletions,
        [](CashDiff::OfferAmounts const& lhs, CashDiff::OfferAmounts const& rhs)
        {
            return diffIsDust (lhs, rhs);
        });

    return removedDust;
}

bool CashDiff::Impl::rmLhsDeletedOffers()
{
    bool const ret = !lhsDiffs_.offerDeletions.empty();
    if (ret)
        lhsDiffs_.offerDeletions.clear();
    return ret;
}

bool CashDiff::Impl::rmRhsDeletedOffers()
{
    bool const ret = !rhsDiffs_.offerDeletions.empty();
    if (ret)
        rhsDiffs_.offerDeletions.clear();
    return ret;
}

//将两个排序向量之间的差异存储到目标中。
template <typename T>
static void setDiff (
    std::vector<T> const& a, std::vector<T> const& b, std::vector<T>& dest)
{
    dest.clear();
    std::set_difference (a.cbegin(), a.cend(),
        b.cbegin(), b.cend(), std::inserter (dest, dest.end()));
}

void CashDiff::Impl::findDiffs (
        CashFilter lhsFilter, detail::ApplyStateTable const& lhs,
        CashFilter rhsFilter, detail::ApplyStateTable const& rhs)
{
//如果dropsdestroyed_u不同，请注意。
    if (lhs.dropsDestroyed() != rhs.dropsDestroyed())
    {
        dropsGone_ = DropsGone{lhs.dropsDestroyed(), rhs.dropsDestroyed()};
    }

//从状态表中提取现金流更改
    auto lhsDiffs = getCashFlow (view_, lhsFilter, lhs);
    auto rhsDiffs = getCashFlow (view_, rhsFilter, rhs);

//获取有关密钥的统计信息。
    auto const counts = countKeys (lhsDiffs, rhsDiffs);
    commonKeys_ = counts[0];
    lhsKeys_    = counts[1];
    rhsKeys_    = counts[2];

//只保存结果之间的差异。
//XRP:
    setDiff (lhsDiffs.xrpChanges, rhsDiffs.xrpChanges, lhsDiffs_.xrpChanges);
    setDiff (rhsDiffs.xrpChanges, lhsDiffs.xrpChanges, rhsDiffs_.xrpChanges);

//信任改变：
    setDiff (lhsDiffs.trustChanges, rhsDiffs.trustChanges, lhsDiffs_.trustChanges);
    setDiff (rhsDiffs.trustChanges, lhsDiffs.trustChanges, rhsDiffs_.trustChanges);

//信任删除：
    setDiff (lhsDiffs.trustDeletions, rhsDiffs.trustDeletions, lhsDiffs_.trustDeletions);
    setDiff (rhsDiffs.trustDeletions, lhsDiffs.trustDeletions, rhsDiffs_.trustDeletions);

//报价变更：
    setDiff (lhsDiffs.offerChanges, rhsDiffs.offerChanges, lhsDiffs_.offerChanges);
    setDiff (rhsDiffs.offerChanges, lhsDiffs.offerChanges, rhsDiffs_.offerChanges);

//提供删除：
    setDiff (lhsDiffs.offerDeletions, rhsDiffs.offerDeletions, lhsDiffs_.offerDeletions);
    setDiff (rhsDiffs.offerDeletions, lhsDiffs.offerDeletions, rhsDiffs_.offerDeletions);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//定位两个ApplyStateTables之间的差异。
CashDiff::CashDiff (CashDiff&& other) noexcept
: impl_ (std::move (other.impl_))
{
}

CashDiff::~CashDiff() = default;

CashDiff::CashDiff (ReadView const& view,
    CashFilter lhsFilter, detail::ApplyStateTable const& lhs,
    CashFilter rhsFilter, detail::ApplyStateTable const& rhs)
: impl_ (new Impl (view, lhsFilter, lhs, rhsFilter, rhs))
{
}

std::size_t CashDiff::commonCount () const
{
    return impl_->commonCount();
}

std::size_t CashDiff::rhsOnlyCount () const
{
    return impl_->rhsOnlyCount();
}

std::size_t CashDiff::lhsOnlyCount () const
{
    return impl_->lhsOnlyCount();
}

bool CashDiff::hasDiff() const
{
    return impl_->hasDiff();
}

int CashDiff::xrpRoundToZero() const
{
    return impl_->xrpRoundToZero();
}

bool CashDiff::rmDust()
{
    return impl_->rmDust();
}

bool CashDiff::rmLhsDeletedOffers()
{
    return impl_->rmLhsDeletedOffers();
}

bool CashDiff::rmRhsDeletedOffers()
{
    return impl_->rmRhsDeletedOffers();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//比较两个stamount的函数，如果差异为
//灰尘大小。
bool diffIsDust (STAmount const& v1, STAmount const& v2, std::uint8_t e10)
{
//如果一个值为正，另一个值为负，那么
//奇怪的进展。
    if (v1 != beast::zero && v2 != beast::zero && (v1.negative() != v2.negative()))
        return false;

//v1和v2必须是同一个问题，才能使它们的区别有意义。
    if (v1.native() != v2.native())
        return false;

    if (!v1.native() && (v1.issue() != v2.issue()))
        return false;

//如果v1==v2，那么灰尘就小得无影无踪了。
    if (v1 == v2)
        return true;

    STAmount const& small = v1 < v2 ? v1 : v2;
    STAmount const& large = v1 < v2 ? v2 : v1;

//处理xrp不同于iou。
    if (v1.native())
    {
        std::uint64_t const s = small.mantissa();
        std::uint64_t const l = large.mantissa();

//总是允许几滴噪音。
        if (l - s <= 2)
            return true;

        static_assert (sizeof (1ULL) == sizeof (std::uint64_t), "");
        std::uint64_t const ratio = s / (l - s);
        static constexpr std::uint64_t e10Lookup[]
        {
                                     1ULL,
                                    10ULL,
                                   100ULL,
                                 1'000ULL,
                                10'000ULL,
                               100'000ULL,
                             1'000'000ULL,
                            10'000'000ULL,
                           100'000'000ULL,
                         1'000'000'000ULL,
                        10'000'000'000ULL,
                       100'000'000'000ULL,
                     1'000'000'000'000ULL,
                    10'000'000'000'000ULL,
                   100'000'000'000'000ULL,
                 1'000'000'000'000'000ULL,
                10'000'000'000'000'000ULL,
               100'000'000'000'000'000ULL,
             1'000'000'000'000'000'000ULL,
            10'000'000'000'000'000'000ULL,
        };
        static std::size_t constexpr maxIndex =
            sizeof (e10Lookup) / sizeof e10Lookup[0];

//确保桌子足够大。
        static_assert (
            std::numeric_limits<std::uint64_t>::max() /
            e10Lookup[maxIndex - 1] < 10, "Table too small");

        if (e10 >= maxIndex)
            return false;

        return ratio >= e10Lookup[e10];
    }

//非本地的。注意，即使大小可能不相等，
//它们的差异可能为零。一种可能发生的情况是如果两个
//值不同，但它们的差异会导致stamount
//指数小于-96。
    STAmount const diff = large - small;
    if (diff == beast::zero)
        return true;

    STAmount const ratio = divide (small, diff, v1.issue());
    STAmount const one (v1.issue(), 1);
    int const ratioExp = ratio.exponent() - one.exponent();

    return ratioExp >= e10;
};

} //涟漪
