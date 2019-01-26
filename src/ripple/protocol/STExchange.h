
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

#ifndef RIPPLE_PROTOCOL_STEXCHANGE_H_INCLUDED
#define RIPPLE_PROTOCOL_STEXCHANGE_H_INCLUDED

#include <ripple/basics/Buffer.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Slice.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/STBlob.h>
#include <ripple/protocol/STInteger.h>
#include <ripple/protocol/STObject.h>
#include <ripple/basics/Blob.h>
#include <boost/optional.hpp>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ripple {

/*在序列化类型U和C++类型T之间转换。*/
template <class U, class T>
struct STExchange;


template <class U, class T>
struct STExchange<STInteger<U>, T>
{
    explicit STExchange() = default;

    using value_type = U;

    static
    void
    get (boost::optional<T>& t,
        STInteger<U> const& u)
    {
        t = u.value();
    }

    static
    std::unique_ptr<STInteger<U>>
    set (SField const& f, T const& t)
    {
        return std::make_unique<
            STInteger<U>>(f, t);
    }
};

template <>
struct STExchange<STBlob, Slice>
{
    explicit STExchange() = default;

    using value_type = Slice;

    static
    void
    get (boost::optional<value_type>& t,
        STBlob const& u)
    {
        t.emplace (u.data(), u.size());
    }

    static
    std::unique_ptr<STBlob>
    set (TypedField<STBlob> const& f,
        Slice const& t)
    {
        return std::make_unique<STBlob>(
            f, t.data(), t.size());
    }
};

template <>
struct STExchange<STBlob, Buffer>
{
    explicit STExchange() = default;

    using value_type = Buffer;

    static
    void
    get (boost::optional<Buffer>& t,
        STBlob const& u)
    {
        t.emplace (
            u.data(), u.size());
    }

    static
    std::unique_ptr<STBlob>
    set (TypedField<STBlob> const& f,
        Buffer const& t)
    {
        return std::make_unique<STBlob>(
            f, t.data(), t.size());
    }

    static
    std::unique_ptr<STBlob>
    set (TypedField<STBlob> const& f,
        Buffer&& t)
    {
        return std::make_unique<STBlob>(
            f, std::move(t));
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*以给定类型返回Stobject中字段的值。*/
/*@ {*/
template <class T, class U>
boost::optional<T>
get (STObject const& st,
    TypedField<U> const& f)
{
    boost::optional<T> t;
    STBase const* const b =
        st.peekAtPField(f);
    if (! b)
        return t;
    auto const id = b->getSType();
    if (id == STI_NOTPRESENT)
        return t;
    auto const u =
        dynamic_cast<U const*>(b);
//这不应该发生
    if (! u)
        Throw<std::runtime_error> (
            "Wrong field type");
    STExchange<U, T>::get(t, *u);
    return t;
}

template <class U>
boost::optional<typename STExchange<
    U, typename U::value_type>::value_type>
get (STObject const& st,
    TypedField<U> const& f)
{
    return get<typename U::value_type>(st, f);
}
/*@ }*/

/*在Stobject中设置字段值。*/
template <class U, class T>
void
set (STObject& st,
    TypedField<U> const& f, T&& t)
{
    st.set(STExchange<U,
        typename std::decay<T>::type>::set(
            f, std::forward<T>(t)));
}

/*使用init函数设置blob字段。*/
template <class Init>
void
set (STObject& st,
    TypedField<STBlob> const& f,
        std::size_t size, Init&& init)
{
    st.set(std::make_unique<STBlob>(
        f, size, init));
}

/*从数据设置一个blob字段。*/
template <class = void>
void
set (STObject& st,
    TypedField<STBlob> const& f,
        void const* data, std::size_t size)
{
    st.set(std::make_unique<STBlob>(
        f, data, size));
}

/*删除Stobject中的字段。*/
template <class U>
void
erase (STObject& st,
    TypedField<U> const& f)
{
    st.makeFieldAbsent(f);
}

} //涟漪

#endif
