
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

#ifndef RIPPLE_PROTOCOL_STBITSTRING_H_INCLUDED
#define RIPPLE_PROTOCOL_STBITSTRING_H_INCLUDED

#include <ripple/protocol/STBase.h>
#include <ripple/beast/utility/Zero.h>

namespace ripple {

template <std::size_t Bits>
class STBitString final
    : public STBase
{
public:
    using value_type = base_uint<Bits>;

    STBitString () = default;

    STBitString (SField const& n)
        : STBase (n)
    { }

    STBitString (const value_type& v)
        : value_ (v)
    { }

    STBitString (SField const& n, const value_type& v)
        : STBase (n), value_ (v)
    { }

    STBitString (SField const& n, const char* v)
        : STBase (n)
    {
        value_.SetHex (v);
    }

    STBitString (SField const& n, std::string const& v)
        : STBase (n)
    {
        value_.SetHex (v);
    }

    STBitString (SerialIter& sit, SField const& name)
        : STBitString(name, sit.getBitString<Bits>())
    {
    }

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

    SerializedTypeID
    getSType () const override;

    std::string
    getText () const override
    {
        return to_string (value_);
    }

    bool
    isEquivalent (const STBase& t) const override
    {
        const STBitString* v = dynamic_cast<const STBitString*> (&t);
        return v && (value_ == v->value_);
    }

    void
    add (Serializer& s) const override
    {
        assert (fName->isBinary ());
        assert (fName->fieldType == getSType());
        s.addBitString<Bits> (value_);
    }

    template <typename Tag>
    void setValue (base_uint<Bits, Tag> const& v)
    {
        value_.copyFrom(v);
    }

    value_type const&
    value() const
    {
        return value_;
    }

    operator value_type () const
    {
        return value_;
    }

    bool
    isDefault () const override
    {
        return value_ == beast::zero;
    }

private:
    value_type value_;
};

using STHash128 = STBitString<128>;
using STHash160 = STBitString<160>;
using STHash256 = STBitString<256>;

template <>
inline
SerializedTypeID
STHash128::getSType () const
{
    return STI_HASH128;
}

template <>
inline
SerializedTypeID
STHash160::getSType () const
{
    return STI_HASH160;
}

template <>
inline
SerializedTypeID
STHash256::getSType () const
{
    return STI_HASH256;
}

} //涟漪

#endif
