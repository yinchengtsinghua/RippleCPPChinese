
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

#ifndef RIPPLE_PROTOCOL_STACCOUNT_H_INCLUDED
#define RIPPLE_PROTOCOL_STACCOUNT_H_INCLUDED

#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/STBase.h>
#include <string>

namespace ripple {

class STAccount final
    : public STBase
{
private:
//statcount的原始实现将值保存在stblob中。
//但是stataccount总是160位的，所以我们可以用更少的
//纹波中的开销：：uint160。但是，因此
//statcount保持不变，我们像stblob一样进行序列化和反序列化。
    uint160 value_;
    bool default_;

public:
    using value_type = AccountID;

    STAccount ();
    STAccount (SField const& n);
    STAccount (SField const& n, Buffer&& v);
    STAccount (SerialIter& sit, SField const& name);
    STAccount (SField const& n, AccountID const& v);

    STBase*
    copy (std::size_t n, void* buf) const override
    {
        return emplace (n, buf, *this);
    }

    STBase*
    move (std::size_t n, void* buf) override
    {
        return emplace (n, buf, std::move(*this));
    }

    SerializedTypeID getSType () const override
    {
        return STI_ACCOUNT;
    }

    std::string getText () const override;

    void
    add (Serializer& s) const override
    {
        assert (fName->isBinary ());
        assert (fName->fieldType == STI_ACCOUNT);

//保留stblob的序列化行为：
//o如果我们是默认的（全部为零），则序列化为空blob。
//o否则序列化160位。
        int const size = isDefault() ? 0 : uint160::bytes;
        s.addVL (value_.data(), size);
    }

    bool
    isEquivalent (const STBase& t) const override
    {
        auto const* const tPtr = dynamic_cast<STAccount const*>(&t);
        return tPtr && (default_ == tPtr->default_) && (value_ == tPtr->value_);
    }

    bool
    isDefault () const override
    {
        return default_;
    }

    STAccount&
    operator= (AccountID const& value)
    {
        setValue (value);
        return *this;
    }

    AccountID
    value() const noexcept
    {
        AccountID result;
        result.copyFrom (value_);
        return result;
    }

    void setValue (AccountID const& v)
    {
        value_.copyFrom (v);
        default_ = false;
    }
};

} //涟漪

#endif
