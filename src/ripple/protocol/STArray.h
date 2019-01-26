
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

#ifndef RIPPLE_PROTOCOL_STARRAY_H_INCLUDED
#define RIPPLE_PROTOCOL_STARRAY_H_INCLUDED

#include <ripple/basics/CountedObject.h>
#include <ripple/protocol/STObject.h>

namespace ripple {

class STArray final
    : public STBase
    , public CountedObject <STArray>
{
private:
    using list_type = std::vector<STObject>;

    enum
    {
        reserveSize = 8
    };

    list_type v_;

public:
//只读迭代
    class Items;

    static char const* getCountedObjectName () { return "STArray"; }

    using size_type = list_type::size_type;
    using iterator = list_type::iterator;
    using const_iterator = list_type::const_iterator;

    STArray();
    STArray (STArray&&);
    STArray (STArray const&) = default;
    STArray (SField const& f, int n);
    STArray (SerialIter& sit, SField const& f, int depth = 0);
    explicit STArray (int n);
    explicit STArray (SField const& f);
    STArray& operator= (STArray const&) = default;
    STArray& operator= (STArray&&);

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

    STObject& operator[] (std::size_t j)
    {
        return v_[j];
    }

    STObject const& operator[] (std::size_t j) const
    {
        return v_[j];
    }

    STObject& back() { return v_.back(); }

    STObject const& back() const { return v_.back(); }

    template <class... Args>
    void
    emplace_back(Args&&... args)
    {
        v_.emplace_back(std::forward<Args>(args)...);
    }

    void push_back (STObject const& object)
    {
        v_.push_back(object);
    }

    void push_back (STObject&& object)
    {
        v_.push_back(std::move(object));
    }

    iterator begin ()
    {
        return v_.begin ();
    }

    iterator end ()
    {
        return v_.end ();
    }

    const_iterator begin () const
    {
        return v_.begin ();
    }

    const_iterator end () const
    {
        return v_.end ();
    }

    size_type size () const
    {
        return v_.size ();
    }

    bool empty () const
    {
        return v_.empty ();
    }
    void clear ()
    {
        v_.clear ();
    }
    void reserve (std::size_t n)
    {
        v_.reserve (n);
    }
    void swap (STArray & a) noexcept
    {
        v_.swap (a.v_);
    }

    virtual std::string getFullText () const override;
    virtual std::string getText () const override;

    virtual Json::Value getJson (int index) const override;
    virtual void add (Serializer & s) const override;

    void sort (bool (*compare) (const STObject & o1, const STObject & o2));

    bool operator== (const STArray & s) const
    {
        return v_ == s.v_;
    }
    bool operator!= (const STArray & s) const
    {
        return v_ != s.v_;
    }

    virtual SerializedTypeID getSType () const override
    {
        return STI_ARRAY;
    }
    virtual bool isEquivalent (const STBase & t) const override;
    virtual bool isDefault () const override
    {
        return v_.empty ();
    }
};

} //涟漪

#endif
