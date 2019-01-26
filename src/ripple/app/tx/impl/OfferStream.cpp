
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

#include <ripple/app/tx/impl/OfferStream.h>
#include <ripple/basics/Log.h>

namespace ripple {

template<class TIn, class TOut>
TOfferStreamBase<TIn, TOut>::TOfferStreamBase (ApplyView& view, ApplyView& cancelView,
    Book const& book, NetClock::time_point when,
        StepCounter& counter, beast::Journal journal)
    : j_ (journal)
    , view_ (view)
    , cancelView_ (cancelView)
    , book_ (book)
    , expire_ (when)
    , tip_ (view, book_)
    , counter_ (counter)
{
}

//处理没有对应分类账分录的目录项的情况
//被发现。这不应该发生，但如果发生了，我们就把它清理干净。
template<class TIn, class TOut>
void
TOfferStreamBase<TIn, TOut>::erase (ApplyView& view)
{
//nikb注意，这应该使用applyview:：dirremove，这将
//如果目录是最后一个条目，请正确删除它。
//不幸的是，这是一个破坏协议的改变。

    auto p = view.peek (keylet::page(tip_.dir()));

    if (p == nullptr)
    {
        JLOG(j_.error()) <<
            "Missing directory " << tip_.dir() <<
            " for offer " << tip_.index();
        return;
    }

    auto v (p->getFieldV256 (sfIndexes));
    auto it (std::find (v.begin(), v.end(), tip_.index()));

    if (it == v.end())
    {
        JLOG(j_.error()) <<
            "Missing offer " << tip_.index() <<
            " for directory " << tip_.dir();
        return;
    }

    v.erase (it);
    p->setFieldV256 (sfIndexes, v);
    view.update (p);

    JLOG(j_.trace()) <<
        "Missing offer " << tip_.index() <<
        " removed from directory " << tip_.dir();
}

static
STAmount accountFundsHelper (ReadView const& view,
    AccountID const& id,
    STAmount const& saDefault,
    Issue const&,
    FreezeHandling freezeHandling,
    beast::Journal j)
{
    return accountFunds (view, id, saDefault, freezeHandling, j);
}

static
IOUAmount accountFundsHelper (ReadView const& view,
    AccountID const& id,
    IOUAmount const& amtDefault,
    Issue const& issue,
    FreezeHandling freezeHandling,
    beast::Journal j)
{
    if (issue.account == id)
//自筹资金
        return amtDefault;

    return toAmount<IOUAmount> (
        accountHolds (view, id, issue.currency, issue.account, freezeHandling, j));
}

static
XRPAmount accountFundsHelper (ReadView const& view,
    AccountID const& id,
    XRPAmount const& amtDefault,
    Issue const& issue,
    FreezeHandling freezeHandling,
    beast::Journal j)
{
    return toAmount<XRPAmount> (
        accountHolds (view, id, issue.currency, issue.account, freezeHandling, j));
}

template<class TIn, class TOut>
bool
TOfferStreamBase<TIn, TOut>::step ()
{
//修改它们的顺序或逻辑
//操作导致协议中断更改。

    for(;;)
    {
        ownerFunds_ = boost::none;
//booktip:：step从以前的视图中删除当前报价
//前进到下一个（除非缺少分类账分录）。
        if (! tip_.step(j_))
            return false;

        std::shared_ptr<SLE> entry = tip_.entry();

//如果我们超过了允许的最大步数，我们就完成了。
        if (!counter_.step ())
            return false;

//如果丢失，请删除
        if (! entry)
        {
            erase (view_);
            erase (cancelView_);
            continue;
        }

//过期时删除
        using d = NetClock::duration;
        using tp = NetClock::time_point;
        if (entry->isFieldPresent (sfExpiration) &&
            tp{d{(*entry)[sfExpiration]}} <= expire_)
        {
            JLOG(j_.trace()) <<
                "Removing expired offer " << entry->key();
                permRmOffer (entry->key());
            continue;
        }

        offer_ = TOffer<TIn, TOut> (entry, tip_.quality());

        auto const amount (offer_.amount());

//如果金额为零，则移除
        if (amount.empty())
        {
            JLOG(j_.warn()) <<
                "Removing bad offer " << entry->key();
            permRmOffer (entry->key());
            offer_ = TOffer<TIn, TOut>{};
            continue;
        }

//计算所有者资金
        ownerFunds_ = accountFundsHelper (view_, offer_.owner (), amount.out,
            offer_.issueOut (), fhZERO_IF_FROZEN, j_);

//检查无资金支持的报价
        if (*ownerFunds_ <= beast::zero)
        {
//如果所有者在原始视图中的平衡是相同的，
//我们没有修改余额，因此
//报价是“发现没有资金”还是“变得没有资金”
            auto const original_funds =
                accountFundsHelper (cancelView_, offer_.owner (), amount.out,
                    offer_.issueOut (), fhZERO_IF_FROZEN, j_);

            if (original_funds == *ownerFunds_)
            {
                permRmOffer (entry->key());
                JLOG(j_.trace()) <<
                    "Removing unfunded offer " << entry->key();
            }
            else
            {
                JLOG(j_.trace()) <<
                    "Removing became unfunded offer " << entry->key();
            }
            offer_ = TOffer<TIn, TOut>{};
            continue;
        }

        break;
    }

    return true;
}

void
OfferStream::permRmOffer (uint256 const& offerIndex)
{
    offerDelete (cancelView_,
                 cancelView_.peek(keylet::offer(offerIndex)), j_);
}

template<class TIn, class TOut>
void FlowOfferStream<TIn, TOut>::permRmOffer (uint256 const& offerIndex)
{
    permToRemove_.insert (offerIndex);
}

template class FlowOfferStream<STAmount, STAmount>;
template class FlowOfferStream<IOUAmount, IOUAmount>;
template class FlowOfferStream<XRPAmount, IOUAmount>;
template class FlowOfferStream<IOUAmount, XRPAmount>;

template class TOfferStreamBase<STAmount, STAmount>;
template class TOfferStreamBase<IOUAmount, IOUAmount>;
template class TOfferStreamBase<XRPAmount, IOUAmount>;
template class TOfferStreamBase<IOUAmount, XRPAmount>;
}
