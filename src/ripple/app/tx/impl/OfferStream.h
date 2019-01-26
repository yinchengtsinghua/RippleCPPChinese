
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

#ifndef RIPPLE_APP_BOOK_OFFERSTREAM_H_INCLUDED
#define RIPPLE_APP_BOOK_OFFERSTREAM_H_INCLUDED

#include <ripple/app/tx/impl/BookTip.h>
#include <ripple/app/tx/impl/Offer.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Quality.h>
#include <ripple/beast/utility/Journal.h>

#include <boost/container/flat_set.hpp>

namespace ripple {

template<class TIn, class TOut>
class TOfferStreamBase
{
public:
    class StepCounter
    {
    private:
        std::uint32_t const limit_;
        std::uint32_t count_;
        beast::Journal j_;

    public:
        StepCounter (std::uint32_t limit, beast::Journal j)
            : limit_ (limit)
            , count_ (0)
            , j_ (j)
        {
        }

        bool
        step ()
        {
            if (count_ >= limit_)
            {
                JLOG(j_.debug()) << "Exceeded " << limit_ << " step limit.";
                return false;
            }
            count_++;
            return true;
        }
        std::uint32_t count() const
        {
            return count_;
        }
    };

protected:
    beast::Journal j_;
    ApplyView& view_;
    ApplyView& cancelView_;
    Book book_;
    NetClock::time_point const expire_;
    BookTip tip_;
    TOffer<TIn, TOut> offer_;
    boost::optional<TOut> ownerFunds_;
    StepCounter& counter_;

    void
    erase (ApplyView& view);

    virtual
    void
    permRmOffer (uint256 const& offerIndex) = 0;

public:
    TOfferStreamBase (ApplyView& view, ApplyView& cancelView,
        Book const& book, NetClock::time_point when,
            StepCounter& counter, beast::Journal journal);

    virtual ~TOfferStreamBase() = default;

    /*返回订单顶部的报价。
        报价总是以质量下降的形式呈现。
        仅当step（）返回“true”时有效。
    **/

    TOffer<TIn, TOut>&
    tip () const
    {
        return const_cast<TOfferStreamBase*>(this)->offer_;
    }

    /*提前到下一个有效报价。
        这将自动删除：
            -缺少分类账分录的报价
            -未提供资金的报价
            -过期的报价
        @如果有有效报价，返回'true'。
    **/

    bool
    step ();

    TOut ownerFunds () const
    {
        return *ownerFunds_;
    }
};

/*在订单簿中展示和消费报价。

    两个“applyview”对象累积对分类帐的更改。“视图”
    在调用事务成功时应用。如果呼叫
    事务失败，然后应用“查看取消”。

    某些无效报价将自动删除：
    -缺少分类账分录的报价
    -已过期的优惠
    -未提供资金的报价：
    当相应余额为零时，发现报价没有资金支持。
    呼叫方未修改余额。这是完成的
    也可以在“取消”视图中查找余额。

    当报价被删除时，它将从两个视图中删除。这训练了
    订单簿，无论交易是否成功。
**/

class OfferStream : public TOfferStreamBase<STAmount, STAmount>
{
protected:
    void
    permRmOffer (uint256 const& offerIndex) override;
public:
    using TOfferStreamBase<STAmount, STAmount>::TOfferStreamBase;
};

/*在订单簿中展示和消费报价。

    “视图”和“应用视图”累积了对分类帐的更改。
    “cancelview”用于确定是否找到报价
    没有资金或变得没有资金。
    “permtoremove”集合标识应
    即使与此offerstream关联的链也被删除
    未应用。

    某些无效的优惠已添加到“permtoremove”集合中：
    -缺少分类账分录的报价
    -已过期的优惠
    -未提供资金的报价：
    当相应余额为零时，发现报价没有资金支持。
    呼叫方未修改余额。这是完成的
    也可以在“取消”视图中查找余额。
**/

template <class TIn, class TOut>
class FlowOfferStream : public TOfferStreamBase<TIn, TOut>
{
private:
    boost::container::flat_set<uint256> permToRemove_;

public:
    using TOfferStreamBase<TIn, TOut>::TOfferStreamBase;

//下面的接口允许报价穿越永久
//取消自交叉报价。动机是有点
//非直觉的参见评论中的讨论
//BookOfferCrossingStep:：LimitSelfCrossQuality（）。
    void
    permRmOffer (uint256 const& offerIndex) override;

    boost::container::flat_set<uint256> const& permToRemove () const
    {
        return permToRemove_;
    }
};
}

#endif

