
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

#ifndef RIPPLE_PROTOCOL_STVECTOR256_H_INCLUDED
#define RIPPLE_PROTOCOL_STVECTOR256_H_INCLUDED

#include <ripple/protocol/STBitString.h>
#include <ripple/protocol/STInteger.h>
#include <ripple/protocol/STBase.h>

namespace ripple {

class STVector256
    : public STBase
{
public:
    using value_type = std::vector<uint256> const&;

    STVector256 () = default;

    explicit STVector256 (SField const& n)
        : STBase (n)
    { }

    explicit STVector256 (std::vector<uint256> const& vector)
        : mValue (vector)
    { }

    STVector256 (SField const& n, std::vector<uint256> const& vector)
        : STBase (n), mValue (vector)
    { }

    STVector256 (SerialIter& sit, SField const& name);

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
    getSType () const override
    {
        return STI_VECTOR256;
    }

    void
    add (Serializer& s) const override;

    Json::Value
    getJson (int) const override;

    bool
    isEquivalent (const STBase& t) const override;

    bool
    isDefault () const override
    {
        return mValue.empty ();
    }

    STVector256&
    operator= (std::vector<uint256> const& v)
    {
        mValue = v;
        return *this;
    }

    STVector256&
    operator= (std::vector<uint256>&& v)
    {
        mValue = std::move(v);
        return *this;
    }

    void
    setValue (const STVector256& v)
    {
        mValue = v.mValue;
    }

    /*检索我们包含的向量的副本*/
    explicit
    operator std::vector<uint256> () const
    {
        return mValue;
    }

    std::size_t
    size () const
    {
        return mValue.size ();
    }

    void
    resize (std::size_t n)
    {
        return mValue.resize (n);
    }

    bool
    empty () const
    {
        return mValue.empty ();
    }

    std::vector<uint256>::reference
    operator[] (std::vector<uint256>::size_type n)
    {
        return mValue[n];
    }

    std::vector<uint256>::const_reference
    operator[] (std::vector<uint256>::size_type n) const
    {
        return mValue[n];
    }

    std::vector<uint256> const&
    value() const
    {
        return mValue;
    }

    std::vector<uint256>::iterator
    insert(std::vector<uint256>::const_iterator pos, uint256 const& value)
    {
        return mValue.insert(pos, value);
    }

    std::vector<uint256>::iterator
    insert(std::vector<uint256>::const_iterator pos, uint256&& value)
    {
        return mValue.insert(pos, std::move(value));
    }

    void
    push_back (uint256 const& v)
    {
        mValue.push_back (v);
    }

    std::vector<uint256>::iterator
    begin()
    {
        return mValue.begin ();
    }

    std::vector<uint256>::const_iterator
    begin() const
    {
        return mValue.begin ();
    }

    std::vector<uint256>::iterator
    end()
    {
        return mValue.end ();
    }

    std::vector<uint256>::const_iterator
    end() const
    {
        return mValue.end ();
    }

    std::vector<uint256>::iterator
    erase (std::vector<uint256>::iterator position)
    {
        return mValue.erase (position);
    }

    void
    clear () noexcept
    {
        return mValue.clear ();
    }

private:
    std::vector<uint256> mValue;
};

} //涟漪

#endif
