
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

#ifndef RIPPLE_LEDGER_CASHDIFF_H_INCLUDED
#define RIPPLE_LEDGER_CASHDIFF_H_INCLUDED

#include <ripple/basics/safe_cast.h>
#include <ripple/protocol/STAmount.h>
#include <memory>                       //STD:：UNIQUIGYPTR

namespace ripple {

class ReadView;

namespace detail {

class ApplyStateTable;

}

//cashdiff用于指定处理差异时应用的筛选器。
//条目是位标志，可以进行anded和ored。
enum class CashFilter : std::uint8_t
{
    none = 0x0,
    treatZeroOfferAsDeletion = 0x1
};
inline CashFilter operator| (CashFilter lhs, CashFilter rhs)
{
    using ul_t = std::underlying_type<CashFilter>::type;
    return static_cast<CashFilter>(
        safe_cast<ul_t>(lhs) | safe_cast<ul_t>(rhs));
}
inline CashFilter operator& (CashFilter lhs, CashFilter rhs)
{
    using ul_t = std::underlying_type<CashFilter>::type;
    return static_cast<CashFilter>(
        safe_cast<ul_t>(lhs) & safe_cast<ul_t>(rhs));
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//用于标识两个ApplyStateTable实例之间的差异的类
//用于调试。
class CashDiff
{
public:
    CashDiff() = delete;
    CashDiff (CashDiff const&) = delete;
    CashDiff (CashDiff&& other) noexcept;
    CashDiff& operator= (CashDiff const&) = delete;
    ~CashDiff();

    CashDiff (ReadView const& view,
        CashFilter lhsFilter, detail::ApplyStateTable const& lhs,
        CashFilter rhsFilter, detail::ApplyStateTable const& rhs);

//返回lhs和rhs具有相同条目的事例数
//（但金额不一定相同）
    std::size_t commonCount () const;

//返回在rhs中存在但在lhs中不存在的条目数。
    std::size_t rhsOnlyCount () const;

//返回lhs中存在但rhs中不存在的条目数。
    std::size_t lhsOnlyCount () const;

//返回true是否有任何要报告的差异。
    bool hasDiff() const;

//检查xrp舍入为零的情况。如果未检测到，则返回零。
//否则，如果在左侧看到-1，如果在右侧看到+1。
//
//对于小额的承购人支付借据和承购人获得xrp，案例已经
//观察到xrp舍入允许少量IOU
//在没有向优惠所有者返回xrp时从优惠中删除。
//这是因为XRP的金额被四舍五入为零下降。
//
//然而，提交微小报价的人却得不到什么。
//一无所获。交易费用明显高于
//收到的IOU的值。
//
//应在调用rmdust（）之前进行此检查。
    int xrpRoundToZero() const;

//清除灰尘尺寸差异。返回true是除去灰尘。
    bool rmDust();

//从给定的一方删除报价删除差异。返回真
//如果从差异中删除了任何已删除的报价。
    bool rmLhsDeletedOffers();
    bool rmRhsDeletedOffers();

    struct OfferAmounts
    {
        static std::size_t constexpr count_ = 2;
        static std::size_t constexpr count() { return count_; }
        STAmount amounts[count_];
        STAmount const& takerPays() const { return amounts[0]; }
        STAmount const& takerGets() const { return amounts[1]; }
        STAmount const& operator[] (std::size_t i) const
        {
            assert (i < count());
            return amounts[i];
        }
        friend bool operator< (OfferAmounts const& lhs, OfferAmounts const& rhs)
        {
            if (lhs[0] < rhs[0])
                return true;
            if (lhs[0] > rhs[0])
                return false;
            return lhs[1] < rhs[1];
        }
    };

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

//如果两个stamounts之间的差异为“small”，则返回true。
//
//如果v1和v2有不同的问题，那么它们的区别就永远不会是灰尘。
//如果v1<v2，则小度计算为v1/（v2-v1）。
//E10的论点至少说明了这个比率必须有多大。默认值为10^6。
//如果v1和v2都是xrp，考虑2滴或更少的任何差异都是灰尘。
bool diffIsDust (STAmount const& v1, STAmount const& v2, std::uint8_t e10 = 6);

} //涟漪

#endif
