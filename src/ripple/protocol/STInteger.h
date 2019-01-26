
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

#ifndef RIPPLE_PROTOCOL_STINTEGER_H_INCLUDED
#define RIPPLE_PROTOCOL_STINTEGER_H_INCLUDED

#include <ripple/protocol/STBase.h>

namespace ripple {

template <typename Integer>
class STInteger
    : public STBase
{
public:
    using value_type = Integer;

    explicit
    STInteger (Integer v)
        : value_ (v)
    { }

    STInteger (SField const& n, Integer v = 0)
        : STBase (n), value_ (v)
    { }

    STInteger(SerialIter& sit, SField const& name);

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

    Json::Value
    getJson (int) const override;

    std::string
    getText () const override;

    void
    add (Serializer& s) const override
    {
        assert (fName->isBinary ());
        assert (fName->fieldType == getSType ());
        s.addInteger (value_);
    }

    STInteger& operator= (value_type const& v)
    {
        value_ = v;
        return *this;
    }

    value_type value() const noexcept
    {
        return value_;
    }

    void
    setValue (Integer v)
    {
        value_ = v;
    }

    operator
    Integer () const
    {
        return value_;
    }

    virtual
    bool isDefault () const override
    {
        return value_ == 0;
    }

    bool
    isEquivalent (const STBase& t) const override
    {
        const STInteger* v = dynamic_cast<const STInteger*> (&t);
        return v && (value_ == v->value_);
    }

private:
    Integer value_;
};

using STUInt8  = STInteger<unsigned char>;
using STUInt16 = STInteger<std::uint16_t>;
using STUInt32 = STInteger<std::uint32_t>;
using STUInt64 = STInteger<std::uint64_t>;

} //涟漪

#endif
