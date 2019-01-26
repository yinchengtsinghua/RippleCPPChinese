
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

#ifndef RIPPLE_APP_BOOK_OFFER_H_INCLUDED
#define RIPPLE_APP_BOOK_OFFER_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/SField.h>
#include <ostream>
#include <stdexcept>

namespace ripple {

template <class TIn, class TOut>
class TOfferBase
{
protected:
    Issue issIn_;
    Issue issOut_;
};

template<>
class TOfferBase<STAmount, STAmount>
{
public:
    explicit TOfferBase() = default;
};


template<class TIn=STAmount, class TOut=STAmount>
class TOffer
    : private TOfferBase<TIn, TOut>
{
private:
    SLE::pointer m_entry;
    Quality m_quality;
    AccountID m_account;

    TAmounts<TIn, TOut> m_amounts;
    void setFieldAmounts ();
public:
    TOffer() = default;

    TOffer (SLE::pointer const& entry, Quality quality);

    /*返回报价的质量。
        从概念上讲，质量是输出货币与输入货币的比率。
        实现将其计算为输入与输出的比率。
        货币（所以它按升序排序）。质量是在当时计算的
        报价已发出，在报价有效期内不会发生任何变化。
        这是一个重要的业务规则，当
        报价部分填写；后续部分填写将使用
        原始质量。
    **/

    Quality const
    quality () const noexcept
    {
        return m_quality;
    }

    /*返回报价所有者的帐户ID。*/
    AccountID const&
    owner () const
    {
        return m_account;
    }

    /*返回进出金额。
        部分或全部支出可能没有资金。
    **/

    TAmounts<TIn, TOut> const&
    amount () const
    {
        return m_amounts;
    }

    /*如果没有更多资金可以通过此报价，则返回“true”。*/
    bool
    fully_consumed () const
    {
        if (m_amounts.in <= beast::zero)
            return true;
        if (m_amounts.out <= beast::zero)
            return true;
        return false;
    }

    /*调整报价以表明我们已经消费了其中的一部分（或全部）。*/
    void
    consume (ApplyView& view,
        TAmounts<TIn, TOut> const& consumed)
    {
        if (consumed.in > m_amounts.in)
            Throw<std::logic_error> ("can't consume more than is available.");

        if (consumed.out > m_amounts.out)
            Throw<std::logic_error> ("can't produce more than is available.");

        m_amounts -= consumed;
        setFieldAmounts ();
        view.update (m_entry);
    }

    std::string id () const
    {
        return to_string (m_entry->key());
    }

    uint256 key () const
    {
        return m_entry->key();
    }

    Issue issueIn () const;
    Issue issueOut () const;
};

using Offer = TOffer <>;

template<class TIn, class TOut>
TOffer<TIn, TOut>::TOffer (SLE::pointer const& entry, Quality quality)
        : m_entry (entry)
        , m_quality (quality)
        , m_account (m_entry->getAccountID (sfAccount))
{
    auto const tp = m_entry->getFieldAmount (sfTakerPays);
    auto const tg = m_entry->getFieldAmount (sfTakerGets);
    m_amounts.in = toAmount<TIn> (tp);
    m_amounts.out = toAmount<TOut> (tg);
    this->issIn_ = tp.issue ();
    this->issOut_ = tg.issue ();
}

template<>
inline
TOffer<STAmount, STAmount>::TOffer (SLE::pointer const& entry, Quality quality)
        : m_entry (entry)
        , m_quality (quality)
        , m_account (m_entry->getAccountID (sfAccount))
        , m_amounts (
            m_entry->getFieldAmount (sfTakerPays),
            m_entry->getFieldAmount (sfTakerGets))
{
}


template<class TIn, class TOut>
void TOffer<TIn, TOut>::setFieldAmounts ()
{
#ifdef _MSC_VER
	assert(0);
#else
    static_assert(sizeof(TOut) == -1, "Must be specialized");
#endif
}

template<>
inline
void TOffer<STAmount, STAmount>::setFieldAmounts ()
{
    m_entry->setFieldAmount (sfTakerPays, m_amounts.in);
    m_entry->setFieldAmount (sfTakerGets, m_amounts.out);
}

template<>
inline
void TOffer<IOUAmount, IOUAmount>::setFieldAmounts ()
{
    m_entry->setFieldAmount (sfTakerPays, toSTAmount(m_amounts.in, issIn_));
    m_entry->setFieldAmount (sfTakerGets, toSTAmount(m_amounts.out, issOut_));
}

template<>
inline
void TOffer<IOUAmount, XRPAmount>::setFieldAmounts ()
{
    m_entry->setFieldAmount (sfTakerPays, toSTAmount(m_amounts.in, issIn_));
    m_entry->setFieldAmount (sfTakerGets, toSTAmount(m_amounts.out));
}

template<>
inline
void TOffer<XRPAmount, IOUAmount>::setFieldAmounts ()
{
    m_entry->setFieldAmount (sfTakerPays, toSTAmount(m_amounts.in));
    m_entry->setFieldAmount (sfTakerGets, toSTAmount(m_amounts.out, issOut_));
}

template<class TIn, class TOut>
Issue TOffer<TIn, TOut>::issueIn () const
{
    return this->issIn_;
}

template<>
inline
Issue TOffer<STAmount, STAmount>::issueIn () const
{
    return m_amounts.in.issue ();
}

template<class TIn, class TOut>
Issue TOffer<TIn, TOut>::issueOut () const
{
    return this->issOut_;
}

template<>
inline
Issue TOffer<STAmount, STAmount>::issueOut () const
{
    return m_amounts.out.issue ();
}

template<class TIn, class TOut>
inline
std::ostream&
operator<< (std::ostream& os, TOffer<TIn, TOut> const& offer)
{
    return os << offer.id ();
}

}

#endif
