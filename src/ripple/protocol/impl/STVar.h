
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

#ifndef RIPPLE_PROTOCOL_STVAR_H_INCLUDED
#define RIPPLE_PROTOCOL_STVAR_H_INCLUDED

#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/STBase.h>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <typeinfo>

namespace ripple {
namespace detail {

struct defaultObject_t
{
    explicit defaultObject_t() = default;
};

struct nonPresentObject_t
{
    explicit nonPresentObject_t() = default;
};

extern defaultObject_t defaultObject;
extern nonPresentObject_t nonPresentObject;

//可容纳任何类型的序列化对象的“variant”
//并包括一个小对象分配优化。
class STVar
{
private:
//我们能容纳的最大的“小物体”
    static std::size_t constexpr max_size = 72;

    std::aligned_storage<max_size>::type d_;
    STBase* p_ = nullptr;

public:
    ~STVar();
    STVar (STVar const& other);
    STVar (STVar&& other);
    STVar& operator= (STVar const& rhs);
    STVar& operator= (STVar&& rhs);

    STVar (STBase&& t)
    {
        p_ = t.move(max_size, &d_);
    }

    STVar (STBase const& t)
    {
        p_ = t.copy(max_size, &d_);
    }

    STVar (defaultObject_t, SField const& name);
    STVar (nonPresentObject_t, SField const& name);
    STVar (SerialIter& sit, SField const& name, int depth = 0);

    STBase& get() { return *p_; }
    STBase& operator*() { return get(); }
    STBase* operator->() { return &get(); }
    STBase const& get() const { return *p_; }
    STBase const& operator*() const { return get(); }
    STBase const* operator->() const { return &get(); }

    template <class T, class... Args>
    friend
    STVar
    make_stvar(Args&&... args);

private:
    STVar() = default;

    STVar (SerializedTypeID id, SField const& name);

    void destroy();

    template <class T, class... Args>
    void
    construct(Args&&... args)
    {
        if(sizeof(T) > max_size)
            p_ = new T(std::forward<Args>(args)...);
        else
            p_ = new(&d_) T(std::forward<Args>(args)...);
    }

    bool
    on_heap() const
    {
        return static_cast<void const*>(p_) !=
            static_cast<void const*>(&d_);
    }
};

template <class T, class... Args>
inline
STVar
make_stvar(Args&&... args)
{
    STVar st;
    st.construct<T>(std::forward<Args>(args)...);
    return st;
}

inline
bool
operator== (STVar const& lhs, STVar const& rhs)
{
    return lhs.get().isEquivalent(rhs.get());
}

inline
bool
operator!= (STVar const& lhs, STVar const& rhs)
{
    return ! (lhs == rhs);
}

} //细节
} //涟漪

#endif
