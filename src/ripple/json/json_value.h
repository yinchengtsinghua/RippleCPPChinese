
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

#ifndef RIPPLE_JSON_JSON_VALUE_H_INCLUDED
#define RIPPLE_JSON_JSON_VALUE_H_INCLUDED

#include <ripple/json/json_forwards.h>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>

/*\brief json（javascript对象表示法）。
 **/

namespace Json
{

/*\值对象持有的值的简短类型。
 **/

enum ValueType
{
nullValue = 0, ///<空值
intValue,      ///<有符号整数值
uintValue,     ///<无符号整数值
realValue,     ///<双值
stringValue,   ///<utf-8字符串值
booleanValue,  ///BoOL值
arrayValue,    ///<数组值（有序列表）
objectValue    ///<对象值（名称/值对的集合）。
};

enum CommentPlacement
{
commentBefore = 0,        ///<放在值前面的行上的注释
commentAfterOnSameLine,   ///<同一行上值后的注释
commentAfter,             ///<值后的行上的注释（仅对根值有意义）
    numberOfCommentPlacement
};

/*\brief轻量级包装以标记静态字符串。
 *
 *值构造函数和ObjectValue成员分配利用
 *静态字符串，并避免在存储
 *字符串或成员名称。
 *
 *使用示例：
 ＊代码
 *json:：value avalue（staticString（“some text”））；
 *json:：value对象；
 *static const staticstring代码（“代码”）；
 *对象[代码]=1234；
 ＊端码
 **/

class StaticString
{
public:
    constexpr explicit StaticString ( const char* czstring )
        : str_ ( czstring )
    {
    }

    constexpr operator const char* () const
    {
        return str_;
    }

    constexpr const char* c_str () const
    {
        return str_;
    }

private:
    const char* str_;
};

inline bool operator== (StaticString x, StaticString y)
{
//托多（汤姆）：我能用X吗？=Y这里是因为静态字符串应该
//独特吗？
    return strcmp (x.c_str(), y.c_str()) == 0;
}

inline bool operator!= (StaticString x, StaticString y)
{
    return ! (x == y);
}

inline bool operator== (std::string const& x, StaticString y)
{
    return strcmp(x.c_str(), y.c_str()) == 0;
}

inline bool operator!= (std::string const& x, StaticString y)
{
    return ! (x == y);
}

inline bool operator== (StaticString x, std::string const& y)
{
    return y == x;
}

inline bool operator!= (StaticString x, std::string const& y)
{
    return ! (y == x);
}

/*\brief表示a<a href=“http://www.json.org”>json<a>value。
 *
 *这个类是一个可区分的联合包装，它可以表示：
 *-有符号整数[范围：value:：minint-value:：maxint]
 *-无符号整数（范围：0-值：：maxuint）
 *双
 *-utf-8字符串
 *-布尔
 *-“空”
 *—有序的值列表
 *-名称/值对集合（javascript对象）
 *
 *持有价值的类型由价值类型和
 *可以使用type（）获取。
 *
 *可以使用运算符[]（）方法访问objectValue或arrayValue的值。
 *非常量方法将自动创建a nullValue元素
 *如果它不存在。
 *数组值的序列将自动调整大小并初始化。
 *带空值。resize（）可用于放大或截断数组值。
 *
 *get（）方法可用于在需要元素的情况下获取默认值。
 *不存在。
 *
 *可以使用
 *getMemberNames（）方法。
 **/

class Value
{
    friend class ValueIteratorBase;

public:
    using Members = std::vector<std::string>;
    using iterator = ValueIterator;
    using const_iterator = ValueConstIterator;
    using UInt = Json::UInt;
    using Int = Json::Int;
    using ArrayIndex = UInt;

    static const Value null;
    static const Int minInt;
    static const Int maxInt;
    static const UInt maxUInt;

private:
    class CZString
    {
    public:
        enum DuplicationPolicy
        {
            noDuplication = 0,
            duplicate,
            duplicateOnCopy
        };
        CZString ( int index );
        CZString ( const char* cstr, DuplicationPolicy allocate );
        CZString ( const CZString& other );
        ~CZString ();
        CZString& operator = ( const CZString& other );
        bool operator< ( const CZString& other ) const;
        bool operator== ( const CZString& other ) const;
        int index () const;
        const char* c_str () const;
        bool isStaticString () const;
    private:
        void swap ( CZString& other ) noexcept;
        const char* cstr_;
        int index_;
    };

public:
    using ObjectValues = std::map<CZString, Value>;

public:
    /*\brief创建给定类型的默认值。

      这是一个非常有用的构造函数。
      若要创建空数组，请传递arrayValue。
      若要创建空对象，请传递objectValue。
      然后可以通过赋值将另一个值设置为此值。
    这很有用，因为clear（）和resize（）不会改变类型。

           实例：
    代码
    json:：value null_value；//null
    json:：value arr_value（json:：arrayvalue）；/[]
    json:：value obj_value（json:：objectvalue）；/
    终止码
         **/

    Value ( ValueType type = nullValue );
    Value ( Int value );
    Value ( UInt value );
    Value ( double value );
    Value ( const char* value );
    Value ( const char* beginValue, const char* endValue );
    /*\brief从静态字符串构造值。

     *与其他值字符串构造函数类似，但不要复制的字符串
     *内部存储。调用后，给定的字符串必须保持活动
     *构造函数。
     *使用示例：
     ＊代码
     *json:：value avalue（staticString（“some text”））；
     ＊端码
     **/

    Value ( const StaticString& value );
    Value ( std::string const& value );
    Value ( bool value );
    Value ( const Value& other );
    ~Value ();

    Value& operator= ( Value const& other );
    Value& operator= ( Value&& other );

    Value ( Value&& other ) noexcept;

//交换值。
///\注意：目前，评论是故意不交换的，因为
///逻辑和效率。
    void swap ( Value& other ) noexcept;

    ValueType type () const;

    const char* asCString () const;
    /*返回未带引号的字符串值。*/
    std::string asString () const;
    Int asInt () const;
    UInt asUInt () const;
    double asDouble () const;
    bool asBool () const;

//TODO:这个docstring提到的“empty（）”方法是什么？
    /*isNull（）测试该字段是否为空。不要使用此方法
        空性测试：使用空（）。*/

    bool isNull () const;
    bool isBool () const;
    bool isInt () const;
    bool isUInt () const;
    bool isIntegral () const;
    bool isDouble () const;
    bool isNumeric () const;
    bool isString () const;
    bool isArray() const;
    bool isArrayOrNull () const;
    bool isObject() const;
    bool isObjectOrNull () const;

    bool isConvertibleTo ( ValueType other ) const;

///数组或对象中的值数
    UInt size () const;

    /*如果这是空数组、空对象、空字符串，
        或为空。*/

    explicit
    operator bool() const;

///删除所有对象成员和数组元素。
///\pre-type（）是arrayValue、objectValue或nullValue
///\post type（）未更改
    void clear ();

///将数组大小调整为元素大小。
///new元素初始化为空。
//只能对NullValue或ArrayValue调用/。
///\pre-type（）是arrayvalue或nullvalue
///\post type（）是arrayValue
    void resize ( UInt size );

///访问数组元素（从零开始的索引）。
///如果数组包含小于index元素，则插入空值。
///在数组中，使其大小为index+1。
///（您可能需要说“value[0u]”才能让编译器区分
///this来自接受字符串的运算符[]。
    Value& operator[] ( UInt index );
///访问数组元素（从零开始的索引）
///（您可能需要说“value[0u]”才能让编译器区分
///this来自接受字符串的运算符[]。
    const Value& operator[] ( UInt index ) const;
///如果数组至少包含index+1个元素，则返回元素值，
///否则返回默认值。
    Value get ( UInt index,
                const Value& defaultValue ) const;
///return true if index<size（）。
    bool isValidIndex ( UInt index ) const;
///\brief在末尾向数组追加值。
///
///equivalent to jsonvalue[jsonvalue.size（）]=值；
    Value& append ( const Value& value );

///按名称访问对象值，如果对象值不存在，则创建一个空成员。
    Value& operator[] ( const char* key );
///access按名称访问对象值，如果没有具有该名称的成员，则返回空值。
    const Value& operator[] ( const char* key ) const;
///按名称访问对象值，如果对象值不存在，则创建一个空成员。
    Value& operator[] ( std::string const& key );
///access按名称访问对象值，如果没有具有该名称的成员，则返回空值。
    const Value& operator[] ( std::string const& key ) const;
    /*\brief按名称访问对象值，如果对象值不存在，则创建一个空成员。

     *如果对象没有该名称的条目，则用于存储的成员名称
     *新条目不重复。
     *使用示例：
     ＊代码
     *json:：value对象；
     *static const staticstring代码（“代码”）；
     *对象[代码]=1234；
     ＊端码
     **/

    Value& operator[] ( const StaticString& key );

///返回名为key的成员（如果存在），否则返回defaultvalue。
    Value get ( const char* key,
                const Value& defaultValue ) const;
///返回名为key的成员（如果存在），否则返回defaultvalue。
    Value get ( std::string const& key,
                const Value& defaultValue ) const;

///\brief删除并返回命名成员。
///
///如果不存在，则不执行任何操作。
///\返回删除的值，或为空。
///\pre-type（）是objectValue或nullValue
///\post type（）未更改
    Value removeMember ( const char* key );
///与removemember相同（const char*）
    Value removeMember ( std::string const& key );

///如果对象具有名为key的成员，则返回true。
    bool isMember ( const char* key ) const;
///如果对象具有名为key的成员，则返回true。
    bool isMember ( std::string const& key ) const;

///\brief返回成员名称列表。
///
///如果为空，则返回空列表。
///\pre-type（）是objectValue或nullValue
///\post如果type（）为nullvalue，则仍为nullvalue
    Members getMemberNames () const;

    bool hasComment ( CommentPlacement placement ) const;
///包括分隔符和嵌入换行符。
    std::string getComment ( CommentPlacement placement ) const;

    std::string toStyledString () const;

    const_iterator begin () const;
    const_iterator end () const;

    iterator begin ();
    iterator end ();

    friend bool operator== (const Value&, const Value&);
    friend bool operator< (const Value&, const Value&);

private:
    Value& resolveReference ( const char* key,
                              bool isStatic );

private:
    union ValueHolder
    {
        Int int_;
        UInt uint_;
        double real_;
        bool bool_;
        char* string_;
        ObjectValues* map_ {nullptr};
    } value_;
    ValueType type_ : 8;
int allocated_ : 1;     //注意：如果声明为bool，则bitfield无效。
};

bool operator== (const Value&, const Value&);

inline
bool operator!= (const Value& x, const Value& y)
{
    return ! (x == y);
}

bool operator< (const Value&, const Value&);

inline
bool operator<= (const Value& x, const Value& y)
{
    return ! (y < x);
}

inline
bool operator> (const Value& x, const Value& y)
{
    return y < x;
}

inline
bool operator>= (const Value& x, const Value& y)
{
    return ! (x < y);
}

/*\brief experimental不要使用：分配器自定义按值执行的成员名和字符串值内存管理。
 *
 *-调用makemembername（）和releasemembername（）分别复制和
 *释放json:：objectValue成员名称。
 *-DuplicateStringValue（）和ReleaseStringValue（）的调用方式与
 *复制并释放json:：StringValue值。
 **/

class ValueAllocator
{
public:
    enum { unknown = (unsigned) - 1 };

    virtual ~ValueAllocator () = default;

    virtual char* makeMemberName ( const char* memberName ) = 0;
    virtual void releaseMemberName ( char* memberName ) = 0;
    virtual char* duplicateStringValue ( const char* value,
                                         unsigned int length = unknown ) = 0;
    virtual void releaseStringValue ( char* value ) = 0;
};

/*\brief值迭代器的基类。
 *
 **/

class ValueIteratorBase
{
public:
    using size_t = unsigned int;
    using difference_type = int;
    using SelfType = ValueIteratorBase;

    ValueIteratorBase ();

    explicit ValueIteratorBase ( const Value::ObjectValues::iterator& current );

    bool operator == ( const SelfType& other ) const
    {
        return isEqual ( other );
    }

    bool operator != ( const SelfType& other ) const
    {
        return !isEqual ( other );
    }

    difference_type operator - ( const SelfType& other ) const
    {
        return computeDistance ( other );
    }

///将引用值的索引或成员名称作为值返回。
    Value key () const;

///返回引用值的索引。-1，如果它不是arrayValue。
    UInt index () const;

///返回引用值的成员名称。“”如果不是objectValue。
    const char* memberName () const;

protected:
    Value& deref () const;

    void increment ();

    void decrement ();

    difference_type computeDistance ( const SelfType& other ) const;

    bool isEqual ( const SelfType& other ) const;

    void copy ( const SelfType& other );

private:
    Value::ObjectValues::iterator current_;
//指示迭代器为空值。
    bool isNull_;
};

/*\brief对象和数组值的const迭代器。
 *
 **/

class ValueConstIterator : public ValueIteratorBase
{
    friend class Value;
public:
    using size_t = unsigned int;
    using difference_type = int;
    using reference = const Value&;
    using pointer = const Value*;
    using SelfType = ValueConstIterator;

    ValueConstIterator () = default;
private:
    /*\按值内部使用以创建迭代器。
     **/

    explicit ValueConstIterator ( const Value::ObjectValues::iterator& current );
public:
    SelfType& operator = ( const ValueIteratorBase& other );

    SelfType operator++ ( int )
    {
        SelfType temp ( *this );
        ++*this;
        return temp;
    }

    SelfType operator-- ( int )
    {
        SelfType temp ( *this );
        --*this;
        return temp;
    }

    SelfType& operator-- ()
    {
        decrement ();
        return *this;
    }

    SelfType& operator++ ()
    {
        increment ();
        return *this;
    }

    reference operator * () const
    {
        return deref ();
    }
};


/*\brief对象和数组值的迭代器。
 **/

class ValueIterator : public ValueIteratorBase
{
    friend class Value;
public:
    using size_t = unsigned int;
    using difference_type = int;
    using reference = Value&;
    using pointer = Value*;
    using SelfType = ValueIterator;

    ValueIterator () = default;
    ValueIterator ( const ValueConstIterator& other );
    ValueIterator ( const ValueIterator& other );
private:
    /*\按值内部使用以创建迭代器。
     **/

    explicit ValueIterator ( const Value::ObjectValues::iterator& current );
public:

    SelfType& operator = ( const SelfType& other );

    SelfType operator++ ( int )
    {
        SelfType temp ( *this );
        ++*this;
        return temp;
    }

    SelfType operator-- ( int )
    {
        SelfType temp ( *this );
        --*this;
        return temp;
    }

    SelfType& operator-- ()
    {
        decrement ();
        return *this;
    }

    SelfType& operator++ ()
    {
        increment ();
        return *this;
    }

    reference operator * () const
    {
        return deref ();
    }
};

} //命名空间JSON


#endif //包含CPptl_json_h_
