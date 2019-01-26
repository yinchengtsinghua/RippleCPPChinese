
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

#ifndef RIPPLE_PROTOCOL_STOBJECT_H_INCLUDED
#define RIPPLE_PROTOCOL_STOBJECT_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/CountedObject.h>
#include <ripple/basics/Slice.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/STPathSet.h>
#include <ripple/protocol/STVector256.h>
#include <ripple/protocol/SOTemplate.h>
#include <ripple/protocol/impl/STVar.h>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/optional.hpp>
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ripple {

class STArray;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class STObject
    : public STBase
    , public CountedObject <STObject>
{
private:
//stbase派生类的代理值
    template <class T>
    class Proxy
    {
    protected:
        using value_type =
            typename T::value_type;

        STObject* st_;
        SOE_Flags style_;
        TypedField<T> const* f_;

        Proxy (Proxy const&) = default;
        Proxy (STObject* st, TypedField<T> const* f);
        value_type value() const;
        T const* find() const;

        template <class U>
        void assign (U&& u);
    };

    template <class T>
    class ValueProxy : private Proxy<T>
    {
    private:
        using value_type =
            typename T::value_type;

    public:
        ValueProxy(ValueProxy const&) = default;
        ValueProxy& operator= (ValueProxy const&) = delete;

        template <class U>
        std::enable_if_t<
            std::is_assignable<T, U>::value,
                ValueProxy&>
        operator= (U&& u);

        operator value_type() const;

    private:
        friend class STObject;

        ValueProxy (STObject* st, TypedField<T> const* f);
    };

    template <class T>
    class OptionalProxy : private Proxy<T>
    {
    private:
        using value_type =
            typename T::value_type;

        using optional_type = boost::optional<
            typename std::decay<value_type>::type>;

    public:
        OptionalProxy(OptionalProxy const&) = default;
        OptionalProxy& operator= (OptionalProxy const&) = delete;

        /*如果设置了字段，则返回“true”。

            具有SOE_默认值并设置为
            默认值将返回“true”
        **/

        explicit operator bool() const noexcept;

        /*返回包含的值

            投掷：

                Stobject:：Fielderr if！（）
        **/

        value_type operator*() const;

        operator optional_type() const;

        /*显式转换为boost：：可选*/
        optional_type
        operator~() const;

        friend bool operator==(
            OptionalProxy const& lhs,
                boost::none_t) noexcept
        {
            return ! lhs.engaged();
        }

        friend bool operator==(
            boost::none_t,
                OptionalProxy const& rhs) noexcept
        {
            return rhs == boost::none;
        }

        friend bool operator==(
            OptionalProxy const& lhs,
                optional_type const& rhs) noexcept
        {
            if (! lhs.engaged())
                return ! rhs;
            if (! rhs)
                return false;
            return *lhs == *rhs;
        }

        friend bool operator==(
            optional_type const& lhs,
                OptionalProxy const& rhs) noexcept
        {
            return rhs == lhs;
        }

        friend bool operator==(
            OptionalProxy const& lhs,
                OptionalProxy const& rhs) noexcept
        {
            if (lhs.engaged() != rhs.engaged())
                return false;
            return ! lhs.engaged() || *lhs == *rhs;
        }

        friend bool operator!=(
            OptionalProxy const& lhs,
                boost::none_t) noexcept
        {
            return ! (lhs == boost::none);
        }

        friend bool operator!=(boost::none_t,
            OptionalProxy const& rhs) noexcept
        {
            return ! (rhs == boost::none);
        }

        friend bool operator!=(
            OptionalProxy const& lhs,
                optional_type const& rhs) noexcept
        {
            return ! (lhs == rhs);
        }

        friend bool operator!=(
            optional_type const& lhs,
                OptionalProxy const& rhs) noexcept
        {
            return ! (lhs == rhs);
        }

        friend bool operator!=(
            OptionalProxy const& lhs,
                OptionalProxy const& rhs) noexcept
        {
            return ! (lhs == rhs);
        }

        OptionalProxy& operator= (boost::none_t const&);
        OptionalProxy& operator= (optional_type&& v);
        OptionalProxy& operator= (optional_type const& v);

        template <class U>
        std::enable_if_t<
            std::is_assignable<T, U>::value,
                OptionalProxy&>
        operator= (U&& u);

    private:
        friend class STObject;

        OptionalProxy (STObject* st,
            TypedField<T> const* f);

        bool engaged() const noexcept;

        void disengage();

        optional_type
        optional_value() const;
    };

    struct Transform
    {
        explicit Transform() = default;

        using argument_type = detail::STVar;
        using result_type = STBase;

        STBase const&
        operator() (detail::STVar const& e) const
        {
            return e.get();
        }
    };

    enum
    {
        reserveSize = 20
    };

    using list_type = std::vector<detail::STVar>;

    list_type v_;
    SOTemplate const* mType;

public:
    using iterator = boost::transform_iterator<
        Transform, STObject::list_type::const_iterator>;

    class FieldErr : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    static char const* getCountedObjectName () { return "STObject"; }

    STObject(STObject&&);
    STObject(STObject const&) = default;
    STObject (const SOTemplate & type, SField const& name);
    STObject (const SOTemplate& type,
        SerialIter& sit, SField const& name) noexcept (false);
    STObject (SerialIter& sit,
        SField const& name, int depth = 0) noexcept (false);
    STObject (SerialIter&& sit, SField const& name) noexcept (false)
        : STObject(sit, name)
    {
    }
    STObject& operator= (STObject const&) = default;
    STObject& operator= (STObject&& other);

    explicit STObject (SField const& name);

    virtual ~STObject();

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

    iterator begin() const
    {
        return iterator(v_.begin());
    }

    iterator end() const
    {
        return iterator(v_.end());
    }

    bool empty() const
    {
        return v_.empty();
    }

    void reserve (std::size_t n)
    {
        v_.reserve (n);
    }

    void applyTemplate (const SOTemplate & type) noexcept (false);

    void applyTemplateFromSField (SField const&) noexcept (false);

    bool isFree () const
    {
        return mType == nullptr;
    }

    void set (const SOTemplate&);
    bool set (SerialIter& u, int depth = 0);

    virtual SerializedTypeID getSType () const override
    {
        return STI_OBJECT;
    }
    virtual bool isEquivalent (const STBase & t) const override;
    virtual bool isDefault () const override
    {
        return v_.empty();
    }

    virtual void add (Serializer & s) const override
    {
add (s, withAllFields);    //只是内部元素
    }

    void addWithoutSigningFields (Serializer & s) const
    {
        add (s, omitSigningFields);
    }

//vvalco note是否返回具有
//动态缓冲区？
//vfalc todo删除此函数并修复少数调用方。
    Serializer getSerializer () const
    {
        Serializer s;
        add (s, withAllFields);
        return s;
    }

    virtual std::string getFullText () const override;
    virtual std::string getText () const override;

//TODO（汤姆）：选项应为枚举。
    virtual Json::Value getJson (int options) const override;

    template <class... Args>
    std::size_t
    emplace_back(Args&&... args)
    {
        v_.emplace_back(std::forward<Args>(args)...);
        return v_.size() - 1;
    }

    int getCount () const
    {
        return v_.size ();
    }

    bool setFlag (std::uint32_t);
    bool clearFlag (std::uint32_t);
    bool isFlag(std::uint32_t) const;
    std::uint32_t getFlags () const;

    uint256 getHash (std::uint32_t prefix) const;
    uint256 getSigningHash (std::uint32_t prefix) const;

    const STBase& peekAtIndex (int offset) const
    {
        return v_[offset].get();
    }
    STBase& getIndex(int offset)
    {
        return v_[offset].get();
    }
    const STBase* peekAtPIndex (int offset) const
    {
        return &v_[offset].get();
    }
    STBase* getPIndex (int offset)
    {
        return &v_[offset].get();
    }

    int getFieldIndex (SField const& field) const;
    SField const& getFieldSType (int index) const;

    const STBase& peekAtField (SField const& field) const;
    STBase& getField (SField const& field);
    const STBase* peekAtPField (SField const& field) const;
    STBase* getPField (SField const& field, bool createOkay = false);

//如果字段类型不匹配或返回默认值，则引发
//如果字段是可选的但不存在
    unsigned char getFieldU8 (SField const& field) const;
    std::uint16_t getFieldU16 (SField const& field) const;
    std::uint32_t getFieldU32 (SField const& field) const;
    std::uint64_t getFieldU64 (SField const& field) const;
    uint128 getFieldH128 (SField const& field) const;

    uint160 getFieldH160 (SField const& field) const;
    uint256 getFieldH256 (SField const& field) const;
    AccountID getAccountID (SField const& field) const;

    Blob getFieldVL (SField const& field) const;
    STAmount const& getFieldAmount (SField const& field) const;
    STPathSet const& getFieldPathSet (SField const& field) const;
    const STVector256& getFieldV256 (SField const& field) const;
    const STArray& getFieldArray (SField const& field) const;

    /*返回字段的值。

        投掷：

            如果字段是
            不在场。
    **/

    template<class T>
    typename T::value_type
    operator[](TypedField<T> const& f) const;

    /*将字段值作为boost：：可选返回

        @return boost：：如果字段不存在，则为无。
    **/

    template<class T>
    boost::optional<std::decay_t<typename T::value_type>>
    operator[](OptionaledField<T> const& of) const;

    /*返回可修改的字段值。

        投掷：

            如果字段是
            不在场。
    **/

    template<class T>
    ValueProxy<T>
    operator[](TypedField<T> const& f);

    /*将可修改字段值作为boost：：可选返回

        返回值等于boost：：none如果
        字段不存在。
    **/

    template<class T>
    OptionalProxy<T>
    operator[](OptionaledField<T> const& of);

    /*设置一个字段。
        如果该字段已存在，则将其替换。
    **/

    void
    set (std::unique_ptr<STBase> v);

    void setFieldU8 (SField const& field, unsigned char);
    void setFieldU16 (SField const& field, std::uint16_t);
    void setFieldU32 (SField const& field, std::uint32_t);
    void setFieldU64 (SField const& field, std::uint64_t);
    void setFieldH128 (SField const& field, uint128 const&);
    void setFieldH256 (SField const& field, uint256 const& );
    void setFieldVL (SField const& field, Blob const&);
    void setFieldVL (SField const& field, Slice const&);

    void setAccountID (SField const& field, AccountID const&);

    void setFieldAmount (SField const& field, STAmount const&);
    void setFieldPathSet (SField const& field, STPathSet const&);
    void setFieldV256 (SField const& field, STVector256 const& v);
    void setFieldArray (SField const& field, STArray const& v);

    template <class Tag>
    void setFieldH160 (SField const& field, base_uint<160, Tag> const& v)
    {
        STBase* rf = getPField (field, true);

        if (! rf)
            Throw<std::runtime_error> ("Field not found");

        if (rf->getSType () == STI_NOTPRESENT)
            rf = makeFieldPresent (field);

        using Bits = STBitString<160>;
        if (auto cf = dynamic_cast<Bits*> (rf))
            cf->setValue (v);
        else
            Throw<std::runtime_error> ("Wrong field type");
    }

    STObject& peekFieldObject (SField const& field);
    STArray& peekFieldArray (SField const& field);

    bool isFieldPresent (SField const& field) const;
    STBase* makeFieldPresent (SField const& field);
    void makeFieldAbsent (SField const& field);
    bool delField (SField const& field);
    void delField (int index);

    bool hasMatchingEntry (const STBase&);

    bool operator== (const STObject & o) const;
    bool operator!= (const STObject & o) const
    {
        return ! (*this == o);
    }

private:
    enum WhichFields : bool
    {
//如果传递这些值，则会仔细选择这些值以执行正确的操作。
//到sfield:：shouldinclude（bool）
        omitSigningFields = false,
        withAllFields = true
    };

    void add (Serializer & s, WhichFields whichFields) const;

//将Stobject中的条目按其将要执行的顺序排序
//序列化。注意：它们不按指针值顺序排序，它们
//按sfield:：fieldcode排序。
    static std::vector<STBase const*>
    getSortedFields (
        STObject const& objToSort, WhichFields whichFields);

//获取按值返回的（大多数）字段的实现。
//
//stbitstring需要remove_cv和remove_引用
//类型。它们的value（）按const-ref返回。我们返回这些类型
//按价值计算。
    template <typename T, typename V =
        typename std::remove_cv < typename std::remove_reference <
            decltype (std::declval <T> ().value ())>::type >::type >
    V getFieldByValue (SField const& field) const
    {
        const STBase* rf = peekAtPField (field);

        if (! rf)
            Throw<std::runtime_error> ("Field not found");

        SerializedTypeID id = rf->getSType ();

        if (id == STI_NOTPRESENT)
return V (); //可选字段不存在

        const T* cf = dynamic_cast<const T*> (rf);

        if (! cf)
            Throw<std::runtime_error> ("Wrong field type");

        return cf->value ();
    }

//获取常量引用返回的（大多数）字段的实现。
//
//如果反序列化缺少的可选字段，则没有任何内容
//很明显会回来。所以我们坚持让电话提供
//在这种情况下，我们返回“空”值。
    template <typename T, typename V>
    V const& getFieldByConstRef (SField const& field, V const& empty) const
    {
        const STBase* rf = peekAtPField (field);

        if (! rf)
            Throw<std::runtime_error> ("Field not found");

        SerializedTypeID id = rf->getSType ();

        if (id == STI_NOTPRESENT)
return empty; //可选字段不存在

        const T* cf = dynamic_cast<const T*> (rf);

        if (! cf)
            Throw<std::runtime_error> ("Wrong field type");

        return *cf;
    }

//使用setValue（）方法设置大多数字段的实现。
    template <typename T, typename V>
    void setFieldUsingSetValue (SField const& field, V value)
    {
        static_assert(!std::is_lvalue_reference<V>::value, "");

        STBase* rf = getPField (field, true);

        if (! rf)
            Throw<std::runtime_error> ("Field not found");

        if (rf->getSType () == STI_NOTPRESENT)
            rf = makeFieldPresent (field);

        T* cf = dynamic_cast<T*> (rf);

        if (! cf)
            Throw<std::runtime_error> ("Wrong field type");

        cf->setValue (std::move (value));
    }

//使用赋值设置字段的实现
    template <typename T>
    void setFieldUsingAssignment (SField const& field, T const& value)
    {
        STBase* rf = getPField (field, true);

        if (! rf)
            Throw<std::runtime_error> ("Field not found");

        if (rf->getSType () == STI_NOTPRESENT)
            rf = makeFieldPresent (field);

        T* cf = dynamic_cast<T*> (rf);

        if (! cf)
            Throw<std::runtime_error> ("Wrong field type");

        (*cf) = value;
    }

//窥视对象和星空的实现
    template <typename T>
    T& peekField (SField const& field)
    {
        STBase* rf = getPField (field, true);

        if (! rf)
            Throw<std::runtime_error> ("Field not found");

        if (rf->getSType () == STI_NOTPRESENT)
            rf = makeFieldPresent (field);

        T* cf = dynamic_cast<T*> (rf);

        if (! cf)
            Throw<std::runtime_error> ("Wrong field type");

        return *cf;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class T>
STObject::Proxy<T>::Proxy (STObject* st, TypedField<T> const* f)
    : st_ (st)
    , f_ (f)
{
    if (st_->mType)
    {
//Stobject已关联模板
        if (! st_->peekAtPField(*f_))
            Throw<STObject::FieldErr> (
                "Template field error '" + this->f_->getName() + "'");
        style_ = st_->mType->style(*f_);
    }
    else
    {
        style_ = SOE_INVALID;
    }
}

template <class T>
auto
STObject::Proxy<T>::value() const ->
    value_type
{
    auto const t = find();
    if (t)
        return t->value();
    if (style_ != SOE_DEFAULT)
        Throw<STObject::FieldErr> (
            "Missing field '" + this->f_->getName() + "'");
    return value_type{};
}

template <class T>
inline
T const*
STObject::Proxy<T>::find() const
{
    return dynamic_cast<T const*>(
        st_->peekAtPField(*f_));
}

template <class T>
template <class U>
void
STObject::Proxy<T>::assign(U&& u)
{
    if (style_ == SOE_DEFAULT &&
        u == value_type{})
    {
        st_->makeFieldAbsent(*f_);
        return;
    }
    T* t;
    if (style_ == SOE_INVALID)
        t = dynamic_cast<T*>(
            st_->getPField(*f_, true));
    else
        t = dynamic_cast<T*>(
            st_->makeFieldPresent(*f_));
    assert(t);
    *t = std::forward<U>(u);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class T>
template <class U>
std::enable_if_t<
    std::is_assignable<T, U>::value,
        STObject::ValueProxy<T>&>
STObject::ValueProxy<T>::operator= (U&& u)
{
    this->assign(std::forward<U>(u));
    return *this;
}

template <class T>
STObject::ValueProxy<T>::operator value_type() const
{
    return this->value();
}

template <class T>
STObject::ValueProxy<T>::ValueProxy(
        STObject* st, TypedField<T> const* f)
    : Proxy<T>(st, f)
{
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class T>
STObject::OptionalProxy<T>::operator bool() const noexcept
{
    return engaged();
}

template <class T>
auto
STObject::OptionalProxy<T>::operator*() const ->
    value_type
{
    return this->value();
}

template <class T>
STObject::OptionalProxy<T>::operator
    typename STObject::OptionalProxy<T>::optional_type() const
{
    return optional_value();
}

template <class T>
typename STObject::OptionalProxy<T>::optional_type
STObject::OptionalProxy<T>::operator~() const
{
    return optional_value();
}

template <class T>
auto
STObject::OptionalProxy<T>::operator=(boost::none_t const&) ->
    OptionalProxy&
{
    disengage();
    return *this;
}

template <class T>
auto
STObject::OptionalProxy<T>::operator=(optional_type&& v) ->
        OptionalProxy&
{
    if (v)
        this->assign(std::move(*v));
    else
        disengage();
    return *this;
}

template <class T>
auto
STObject::OptionalProxy<T>::operator=(optional_type const& v) ->
        OptionalProxy&
{
    if (v)
        this->assign(*v);
    else
        disengage();
    return *this;
}

template <class T>
template <class U>
std::enable_if_t<
    std::is_assignable<T, U>::value,
        STObject::OptionalProxy<T>&>
STObject::OptionalProxy<T>::operator=(U&& u)
{
    this->assign(std::forward<U>(u));
    return *this;
}

template <class T>
STObject::OptionalProxy<T>::OptionalProxy(
        STObject* st, TypedField<T> const* f)
    : Proxy<T>(st, f)
{
}

template <class T>
bool
STObject::OptionalProxy<T>::engaged() const noexcept
{
    return this->style_ == SOE_DEFAULT
        || this->find() != nullptr;
}

template <class T>
void
STObject::OptionalProxy<T>::disengage()
{
    if (this->style_ == SOE_REQUIRED ||
            this->style_ == SOE_DEFAULT)
        Throw<STObject::FieldErr> (
            "Template field error '" + this->f_->getName() + "'");
    if (this->style_ == SOE_INVALID)
        this->st_->delField(*this->f_);
    else
        this->st_->makeFieldAbsent(*this->f_);
}

template <class T>
auto
STObject::OptionalProxy<T>::optional_value() const ->
    optional_type
{
    if (! engaged())
        return boost::none;
    return this->value();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class T>
typename T::value_type
STObject::operator[](TypedField<T> const& f) const
{
    auto const b = peekAtPField(f);
    if (! b)
//这是一个自由对象（没有约束）
//没有模板
        Throw<STObject::FieldErr> (
            "Missing field '" + f.getName() + "'");
    auto const u =
        dynamic_cast<T const*>(b);
    if (! u)
    {
        assert(mType);
        assert(b->getSType() == STI_NOTPRESENT);
        if(mType->style(f) == SOE_OPTIONAL)
            Throw<STObject::FieldErr> (
                "Missing field '" + f.getName() + "'");
        assert(mType->style(f) == SOE_DEFAULT);
//处理值类型为
//常量引用，否则返回
//临时工的地址。
        static std::decay_t<
            typename T::value_type> const dv{};
        return dv;
    }
    return u->value();
}

template<class T>
boost::optional<std::decay_t<typename T::value_type>>
STObject::operator[](OptionaledField<T> const& of) const
{
    auto const b = peekAtPField(*of.f);
    if (! b)
        return boost::none;
    auto const u =
        dynamic_cast<T const*>(b);
    if (! u)
    {
        assert(mType);
        assert(b->getSType() == STI_NOTPRESENT);
        if(mType->style(*of.f) == SOE_OPTIONAL)
            return boost::none;
        assert(mType->style(*of.f) == SOE_DEFAULT);
        return typename T::value_type{};
    }
    return u->value();
}

template<class T>
inline
auto
STObject::operator[](TypedField<T> const& f) ->
    ValueProxy<T>
{
    return ValueProxy<T>(this, &f);
}

template<class T>
inline
auto
STObject::operator[](OptionaledField<T> const& of) ->
    OptionalProxy<T>
{
    return OptionalProxy<T>(this, of.f);
}

} //涟漪

#endif
