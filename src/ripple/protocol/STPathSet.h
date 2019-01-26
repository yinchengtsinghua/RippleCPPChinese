
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

#ifndef RIPPLE_PROTOCOL_STPATHSET_H_INCLUDED
#define RIPPLE_PROTOCOL_STPATHSET_H_INCLUDED

#include <ripple/json/json_value.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/STBase.h>
#include <ripple/protocol/UintTypes.h>
#include <boost/optional.hpp>
#include <cassert>
#include <cstddef>

namespace ripple {

class STPathElement
{
public:
    enum Type
    {
        typeNone        = 0x00,
typeAccount     = 0x01, //在一个帐户中翻来覆去（而不是接受一个报价）。
typeCurrency    = 0x10, //货币紧随其后。
typeIssuer      = 0x20, //发行人如下。
typeBoundary    = 0xFF, //交替路径之间的边界。
        typeAll = typeAccount | typeCurrency | typeIssuer,
//所有类型的组合。
    };

private:
    static
    std::size_t
    get_hash (STPathElement const& element);

public:
    STPathElement(
        boost::optional<AccountID> const& account,
        boost::optional<Currency> const& currency,
        boost::optional<AccountID> const& issuer)
        : mType (typeNone)
    {
        if (! account)
        {
            is_offer_ = true;
        }
        else
        {
            is_offer_ = false;
            mAccountID = *account;
            mType |= typeAccount;
            assert(mAccountID != noAccount());
        }

        if (currency)
        {
            mCurrencyID = *currency;
            mType |= typeCurrency;
        }

        if (issuer)
        {
            mIssuerID = *issuer;
            mType |= typeIssuer;
            assert(mIssuerID != noAccount());
        }

        hash_value_ = get_hash (*this);
    }

    STPathElement (
        AccountID const& account, Currency const& currency,
        AccountID const& issuer, bool forceCurrency = false)
        : mType (typeNone), mAccountID (account), mCurrencyID (currency)
        , mIssuerID (issuer), is_offer_ (isXRP(mAccountID))
    {
        if (!is_offer_)
            mType |= typeAccount;

        if (forceCurrency || !isXRP(currency))
            mType |= typeCurrency;

        if (!isXRP(issuer))
            mType |= typeIssuer;

        hash_value_ = get_hash (*this);
    }

    STPathElement (
        unsigned int uType, AccountID const& account, Currency const& currency,
        AccountID const& issuer)
        : mType (uType), mAccountID (account), mCurrencyID (currency)
        , mIssuerID (issuer), is_offer_ (isXRP(mAccountID))
    {
        hash_value_ = get_hash (*this);
    }

    STPathElement ()
        : mType (typeNone), is_offer_ (true)
    {
        hash_value_ = get_hash (*this);
    }

    STPathElement(STPathElement const&) = default;
    STPathElement& operator=(STPathElement const&) = default;

    auto
    getNodeType () const
    {
        return mType;
    }

    bool
    isOffer () const
    {
        return is_offer_;
    }

    bool
    isAccount () const
    {
        return !isOffer ();
    }

    bool
    hasIssuer () const
    {
        return getNodeType () & STPathElement::typeIssuer;
    }

    bool
    hasCurrency () const
    {
        return getNodeType () & STPathElement::typeCurrency;
    }

    bool
    isNone () const
    {
        return getNodeType () == STPathElement::typeNone;
    }

//节点可以是帐户ID或提供前缀。提供前缀表示
//报价类别。
    AccountID const&
    getAccountID () const
    {
        return mAccountID;
    }

    Currency const&
    getCurrency () const
    {
        return mCurrencyID;
    }

    AccountID const&
    getIssuerID () const
    {
        return mIssuerID;
    }

    bool
    operator== (const STPathElement& t) const
    {
        return (mType & typeAccount) == (t.mType & typeAccount) &&
            hash_value_ == t.hash_value_ &&
            mAccountID == t.mAccountID &&
            mCurrencyID == t.mCurrencyID &&
            mIssuerID == t.mIssuerID;
    }

    bool
    operator!= (const STPathElement& t) const
    {
        return !operator==(t);
    }

private:
    unsigned int mType;
    AccountID mAccountID;
    Currency mCurrencyID;
    AccountID mIssuerID;

    bool is_offer_;
    std::size_t hash_value_;
};

class STPath
{
public:
    STPath () = default;

    STPath (std::vector<STPathElement> p)
        : mPath (std::move(p))
    { }

    std::vector<STPathElement>::size_type
    size () const
    {
        return mPath.size ();
    }

    bool empty() const
    {
        return mPath.empty ();
    }

    void
    push_back (STPathElement const& e)
    {
        mPath.push_back (e);
    }

    template <typename ...Args>
    void
    emplace_back (Args&&... args)
    {
        mPath.emplace_back (std::forward<Args> (args)...);
    }

    bool
    hasSeen (
        AccountID const& account, Currency const& currency,
        AccountID const& issuer) const;

    Json::Value
    getJson (int) const;

    std::vector<STPathElement>::const_iterator
    begin () const
    {
        return mPath.begin ();
    }

    std::vector<STPathElement>::const_iterator
    end () const
    {
        return mPath.end ();
    }

    bool
    operator== (STPath const& t) const
    {
        return mPath == t.mPath;
    }

    std::vector<STPathElement>::const_reference
    back () const
    {
        return mPath.back ();
    }

    std::vector<STPathElement>::const_reference
    front () const
    {
        return mPath.front ();
    }

    STPathElement& operator[](int i)
    {
        return mPath[i];
    }

    const STPathElement& operator[](int i) const
    {
        return mPath[i];
    }

    void reserve(size_t s)
    {
        mPath.reserve(s);
    }
private:
    std::vector<STPathElement> mPath;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//一组零个或多个付款路径
class STPathSet final
    : public STBase
{
public:
    STPathSet () = default;

    STPathSet (SField const& n)
        : STBase (n)
    { }

    STPathSet (SerialIter& sit, SField const& name);

    STBase*
    copy (std::size_t n, void* buf) const override
    {
        return emplace(n, buf, *this);
    }

    STBase*
    move (std::size_t n, void* buf) override
    {
        return emplace(n, buf, std::move(*this));
    }

    void
    add (Serializer& s) const override;

    Json::Value
    getJson (int) const override;

    SerializedTypeID
    getSType () const override
    {
        return STI_PATHSET;
    }

    bool
    assembleAdd(STPath const& base, STPathElement const& tail);

    bool
    isEquivalent (const STBase& t) const override;

    bool
    isDefault () const override
    {
        return value.empty ();
    }

//std：：矢量式接口：
    std::vector<STPath>::const_reference
    operator[] (std::vector<STPath>::size_type n) const
    {
        return value[n];
    }

    std::vector<STPath>::reference
    operator[] (std::vector<STPath>::size_type n)
    {
        return value[n];
    }

    std::vector<STPath>::const_iterator
    begin () const
    {
        return value.begin ();
    }

    std::vector<STPath>::const_iterator
    end () const
    {
        return value.end ();
    }

    std::vector<STPath>::size_type
    size () const
    {
        return value.size ();
    }

    bool
    empty () const
    {
        return value.empty ();
    }

    void
    push_back (STPath const& e)
    {
        value.push_back (e);
    }

    template <typename... Args>
    void emplace_back (Args&&... args)
    {
        value.emplace_back (std::forward<Args> (args)...);
    }

private:
    std::vector<STPath> value;
};

} //涟漪

#endif
