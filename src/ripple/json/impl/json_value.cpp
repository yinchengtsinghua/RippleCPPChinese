
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

#include <ripple/basics/contract.h>
#include <ripple/json/impl/json_assert.h>
#include <ripple/json/to_string.h>
#include <ripple/json/json_writer.h>
#include <ripple/beast/core/LexicalCast.h>

namespace Json {

const Value Value::null;
const Int Value::minInt = Int ( ~ (UInt (-1) / 2) );
const Int Value::maxInt = Int ( UInt (-1) / 2 );
const UInt Value::maxUInt = UInt (-1);

class DefaultValueAllocator : public ValueAllocator
{
public:
    virtual ~DefaultValueAllocator() = default;

    char* makeMemberName ( const char* memberName ) override
    {
        return duplicateStringValue ( memberName );
    }

    void releaseMemberName ( char* memberName ) override
    {
        releaseStringValue ( memberName );
    }

    char* duplicateStringValue ( const char* value,
        unsigned int length = unknown ) override
    {
//@todo调查这个旧的优化
//如果（！）值值[0]==0）
//返回0；

        if ( length == unknown )
            length = value ? (unsigned int)strlen ( value ) : 0;

        char* newString = static_cast<char*> ( malloc ( length + 1 ) );
        if ( value )
            memcpy ( newString, value, length );
        newString[length] = 0;
        return newString;
    }

    void releaseStringValue ( char* value ) override
    {
        if ( value )
            free ( value );
    }
};

static ValueAllocator*& valueAllocator ()
{
    static ValueAllocator* valueAllocator = new DefaultValueAllocator;
    return valueAllocator;
}

static struct DummyValueAllocatorInitializer
{
    DummyValueAllocatorInitializer ()
    {
valueAllocator ();     //确保在main（）之前初始化valueallocator（）静态。
    }
} dummyValueAllocatorInitializer;

//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////
//类值：：czString
//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////

//注：index_u表示字符串是否在
//存储一个字符串。

Value::CZString::CZString ( int index )
    : cstr_ ( 0 )
    , index_ ( index )
{
}

Value::CZString::CZString ( const char* cstr, DuplicationPolicy allocate )
    : cstr_ ( allocate == duplicate ? valueAllocator ()->makeMemberName (cstr)
              : cstr )
    , index_ ( allocate )
{
}

Value::CZString::CZString ( const CZString& other )
    : cstr_ ( other.index_ != noDuplication&&   other.cstr_ != 0
              ?  valueAllocator ()->makeMemberName ( other.cstr_ )
              : other.cstr_ )
    , index_ ( other.cstr_ ? (other.index_ == noDuplication ? noDuplication : duplicate)
               : other.index_ )
{
}

Value::CZString::~CZString ()
{
    if ( cstr_  &&  index_ == duplicate )
        valueAllocator ()->releaseMemberName ( const_cast<char*> ( cstr_ ) );
}

void
Value::CZString::swap ( CZString& other ) noexcept
{
    std::swap ( cstr_, other.cstr_ );
    std::swap ( index_, other.index_ );
}

Value::CZString&
Value::CZString::operator = ( const CZString& other )
{
    CZString temp ( other );
    swap ( temp );
    return *this;
}

bool
Value::CZString::operator< ( const CZString& other ) const
{
    if ( cstr_ )
        return strcmp ( cstr_, other.cstr_ ) < 0;

    return index_ < other.index_;
}

bool
Value::CZString::operator== ( const CZString& other ) const
{
    if ( cstr_ )
        return strcmp ( cstr_, other.cstr_ ) == 0;

    return index_ == other.index_;
}


int
Value::CZString::index () const
{
    return index_;
}


const char*
Value::CZString::c_str () const
{
    return cstr_;
}

bool
Value::CZString::isStaticString () const
{
    return index_ == noDuplication;
}

//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////
//类值：：值
//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////

/*\内部默认构造函数初始化必须等效于：
 *memset（this，0，sizeof（value））。
 *此优化用于ValueInternalMap快速分配器。
 **/

Value::Value ( ValueType type )
    : type_ ( type )
    , allocated_ ( 0 )
{
    switch ( type )
    {
    case nullValue:
        break;

    case intValue:
    case uintValue:
        value_.int_ = 0;
        break;

    case realValue:
        value_.real_ = 0.0;
        break;

    case stringValue:
        value_.string_ = 0;
        break;

    case arrayValue:
    case objectValue:
        value_.map_ = new ObjectValues ();
        break;

    case booleanValue:
        value_.bool_ = false;
        break;

    default:
        JSON_ASSERT_UNREACHABLE;
    }
}


Value::Value ( Int value )
    : type_ ( intValue )
{
    value_.int_ = value;
}


Value::Value ( UInt value )
    : type_ ( uintValue )
{
    value_.uint_ = value;
}

Value::Value ( double value )
    : type_ ( realValue )
{
    value_.real_ = value;
}

Value::Value ( const char* value )
    : type_ ( stringValue )
    , allocated_ ( true )
{
    value_.string_ = valueAllocator ()->duplicateStringValue ( value );
}


Value::Value ( const char* beginValue,
               const char* endValue )
    : type_ ( stringValue )
    , allocated_ ( true )
{
    value_.string_ = valueAllocator ()->duplicateStringValue ( beginValue,
                     UInt (endValue - beginValue) );
}


Value::Value ( std::string const& value )
    : type_ ( stringValue )
    , allocated_ ( true )
{
    value_.string_ = valueAllocator ()->duplicateStringValue ( value.c_str (),
                     (unsigned int)value.length () );

}

Value::Value ( const StaticString& value )
    : type_ ( stringValue )
    , allocated_ ( false )
{
    value_.string_ = const_cast<char*> ( value.c_str () );
}

Value::Value ( bool value )
    : type_ ( booleanValue )
{
    value_.bool_ = value;
}


Value::Value ( const Value& other )
    : type_ ( other.type_ )
{
    switch ( type_ )
    {
    case nullValue:
    case intValue:
    case uintValue:
    case realValue:
    case booleanValue:
        value_ = other.value_;
        break;

    case stringValue:
        if ( other.value_.string_ )
        {
            value_.string_ = valueAllocator ()->duplicateStringValue ( other.value_.string_ );
            allocated_ = true;
        }
        else
            value_.string_ = 0;

        break;

    case arrayValue:
    case objectValue:
        value_.map_ = new ObjectValues ( *other.value_.map_ );
        break;

    default:
        JSON_ASSERT_UNREACHABLE;
    }
}


Value::~Value ()
{
    switch ( type_ )
    {
    case nullValue:
    case intValue:
    case uintValue:
    case realValue:
    case booleanValue:
        break;

    case stringValue:
        if ( allocated_ )
            valueAllocator ()->releaseStringValue ( value_.string_ );

        break;

    case arrayValue:
    case objectValue:
        if (value_.map_)
            delete value_.map_;
        break;

    default:
        JSON_ASSERT_UNREACHABLE;
    }
}

Value&
Value::operator=(Value const& other)
{
    Value tmp(other);
    swap(tmp);
    return *this;
}

Value::Value ( Value&& other ) noexcept
    : value_ ( other.value_ )
    , type_ ( other.type_ )
    , allocated_ ( other.allocated_ )
{
    other.type_ = nullValue;
    other.allocated_ = 0;
}

Value&
Value::operator=(Value&& other)
{
    Value tmp(std::move(other));
    swap(tmp);
    return *this;
}

void
Value::swap ( Value& other ) noexcept
{
    std::swap ( value_, other.value_ );

    ValueType temp = type_;
    type_ = other.type_;
    other.type_ = temp;

    int temp2 = allocated_;
    allocated_ = other.allocated_;
    other.allocated_ = temp2;
}

ValueType
Value::type () const
{
    return type_;
}

static
int integerCmp (Int i, UInt ui)
{
//所有负数都小于所有无符号数。
    if (i < 0)
        return -1;

//现在我们可以安全地进行比较了。
    return (i < ui) ? -1 : (i == ui) ? 0 : 1;
}

bool operator < (const Value& x, const Value& y)
{
    if (auto signum = x.type_ - y.type_)
    {
        if (x.type_ == intValue && y.type_ == uintValue)
            signum = integerCmp (x.value_.int_, y.value_.uint_);
        else if (x.type_ == uintValue && y.type_ == intValue)
            signum = - integerCmp (y.value_.int_, x.value_.uint_);
        return signum < 0;
    }

    switch (x.type_)
    {
    case nullValue:
        return false;

    case intValue:
        return x.value_.int_ < y.value_.int_;

    case uintValue:
        return x.value_.uint_ < y.value_.uint_;

    case realValue:
        return x.value_.real_ < y.value_.real_;

    case booleanValue:
        return x.value_.bool_ < y.value_.bool_;

    case stringValue:
        return (x.value_.string_ == 0  &&  y.value_.string_)
               || (y.value_.string_ && x.value_.string_ &&
                   strcmp (x.value_.string_, y.value_.string_) < 0);

    case arrayValue:
    case objectValue:
    {
        if (int signum = int (x.value_.map_->size ()) - y.value_.map_->size ())
            return signum < 0;

        return *x.value_.map_ < *y.value_.map_;
    }

    default:
        JSON_ASSERT_UNREACHABLE;
    }

return 0;  //不可达
}

bool operator== (const Value& x, const Value& y)
{
    if (x.type_ != y.type_)
    {
        if (x.type_ == intValue && y.type_ == uintValue)
            return ! integerCmp (x.value_.int_, y.value_.uint_);
        if (x.type_ == uintValue && y.type_ == intValue)
            return ! integerCmp (y.value_.int_, x.value_.uint_);
        return false;
    }

    switch (x.type_)
    {
    case nullValue:
        return true;

    case intValue:
        return x.value_.int_ == y.value_.int_;

    case uintValue:
        return x.value_.uint_ == y.value_.uint_;

    case realValue:
        return x.value_.real_ == y.value_.real_;

    case booleanValue:
        return x.value_.bool_ == y.value_.bool_;

    case stringValue:
        return x.value_.string_ == y.value_.string_
               || (y.value_.string_ && x.value_.string_ &&
                    ! strcmp (x.value_.string_, y.value_.string_));

    case arrayValue:
    case objectValue:
        return x.value_.map_->size () == y.value_.map_->size ()
               && *x.value_.map_ == *y.value_.map_;

    default:
        JSON_ASSERT_UNREACHABLE;
    }

return 0;  //不可达
}

const char*
Value::asCString () const
{
    JSON_ASSERT ( type_ == stringValue );
    return value_.string_;
}


std::string
Value::asString () const
{
    switch ( type_ )
    {
    case nullValue:
        return "";

    case stringValue:
        return value_.string_ ? value_.string_ : "";

    case booleanValue:
        return value_.bool_ ? "true" : "false";

    case intValue:
        return beast::lexicalCastThrow <std::string> (value_.int_);

    case uintValue:
        return beast::lexicalCastThrow <std::string> (value_.uint_);

    case realValue:
        return beast::lexicalCastThrow <std::string> (value_.real_);

    case arrayValue:
    case objectValue:
        JSON_ASSERT_MESSAGE ( false, "Type is not convertible to string" );

    default:
        JSON_ASSERT_UNREACHABLE;
    }

return ""; //不可达
}

Value::Int
Value::asInt () const
{
    switch ( type_ )
    {
    case nullValue:
        return 0;

    case intValue:
        return value_.int_;

    case uintValue:
        JSON_ASSERT_MESSAGE ( value_.uint_ < (unsigned)maxInt, "integer out of signed integer range" );
        return value_.uint_;

    case realValue:
        JSON_ASSERT_MESSAGE ( value_.real_ >= minInt  &&  value_.real_ <= maxInt, "Real out of signed integer range" );
        return Int ( value_.real_ );

    case booleanValue:
        return value_.bool_ ? 1 : 0;

    case stringValue:
        return beast::lexicalCastThrow <int> (value_.string_);

    case arrayValue:
    case objectValue:
        JSON_ASSERT_MESSAGE ( false, "Type is not convertible to int" );

    default:
        JSON_ASSERT_UNREACHABLE;
    }

return 0; //遥不可及；
}

Value::UInt
Value::asUInt () const
{
    switch ( type_ )
    {
    case nullValue:
        return 0;

    case intValue:
        JSON_ASSERT_MESSAGE ( value_.int_ >= 0, "Negative integer can not be converted to unsigned integer" );
        return value_.int_;

    case uintValue:
        return value_.uint_;

    case realValue:
        JSON_ASSERT_MESSAGE ( value_.real_ >= 0  &&  value_.real_ <= maxUInt,  "Real out of unsigned integer range" );
        return UInt ( value_.real_ );

    case booleanValue:
        return value_.bool_ ? 1 : 0;

    case stringValue:
        return beast::lexicalCastThrow <unsigned int> (value_.string_);

    case arrayValue:
    case objectValue:
        JSON_ASSERT_MESSAGE ( false, "Type is not convertible to uint" );

    default:
        JSON_ASSERT_UNREACHABLE;
    }

return 0; //遥不可及；
}

double
Value::asDouble () const
{
    switch ( type_ )
    {
    case nullValue:
        return 0.0;

    case intValue:
        return value_.int_;

    case uintValue:
        return value_.uint_;

    case realValue:
        return value_.real_;

    case booleanValue:
        return value_.bool_ ? 1.0 : 0.0;

    case stringValue:
    case arrayValue:
    case objectValue:
        JSON_ASSERT_MESSAGE ( false, "Type is not convertible to double" );

    default:
        JSON_ASSERT_UNREACHABLE;
    }

return 0; //遥不可及；
}

bool
Value::asBool () const
{
    switch ( type_ )
    {
    case nullValue:
        return false;

    case intValue:
    case uintValue:
        return value_.int_ != 0;

    case realValue:
        return value_.real_ != 0.0;

    case booleanValue:
        return value_.bool_;

    case stringValue:
        return value_.string_  &&  value_.string_[0] != 0;

    case arrayValue:
    case objectValue:
        return value_.map_->size () != 0;

    default:
        JSON_ASSERT_UNREACHABLE;
    }

return false; //遥不可及；
}


bool
Value::isConvertibleTo ( ValueType other ) const
{
    switch ( type_ )
    {
    case nullValue:
        return true;

    case intValue:
        return ( other == nullValue  &&  value_.int_ == 0 )
               || other == intValue
               || ( other == uintValue  && value_.int_ >= 0 )
               || other == realValue
               || other == stringValue
               || other == booleanValue;

    case uintValue:
        return ( other == nullValue  &&  value_.uint_ == 0 )
               || ( other == intValue  && value_.uint_ <= (unsigned)maxInt )
               || other == uintValue
               || other == realValue
               || other == stringValue
               || other == booleanValue;

    case realValue:
        return ( other == nullValue  &&  value_.real_ == 0.0 )
               || ( other == intValue  &&  value_.real_ >= minInt  &&  value_.real_ <= maxInt )
               || ( other == uintValue  &&  value_.real_ >= 0  &&  value_.real_ <= maxUInt )
               || other == realValue
               || other == stringValue
               || other == booleanValue;

    case booleanValue:
        return ( other == nullValue  &&  value_.bool_ == false )
               || other == intValue
               || other == uintValue
               || other == realValue
               || other == stringValue
               || other == booleanValue;

    case stringValue:
        return other == stringValue
               || ( other == nullValue  &&  (!value_.string_  ||  value_.string_[0] == 0) );

    case arrayValue:
        return other == arrayValue
               ||  ( other == nullValue  &&  value_.map_->size () == 0 );

    case objectValue:
        return other == objectValue
               ||  ( other == nullValue  &&  value_.map_->size () == 0 );

    default:
        JSON_ASSERT_UNREACHABLE;
    }

return false; //遥不可及；
}


///数组或对象中的值数
Value::UInt
Value::size () const
{
    switch ( type_ )
    {
    case nullValue:
    case intValue:
    case uintValue:
    case realValue:
    case booleanValue:
    case stringValue:
        return 0;

case arrayValue:  //数组的大小是最高索引+1
        if ( !value_.map_->empty () )
        {
            ObjectValues::const_iterator itLast = value_.map_->end ();
            --itLast;
            return (*itLast).first.index () + 1;
        }

        return 0;

    case objectValue:
        return Int ( value_.map_->size () );

    default:
        JSON_ASSERT_UNREACHABLE;
    }

return 0; //遥不可及；
}


Value::operator bool () const
{
    if (isNull ())
        return false;

    if (isString ())
    {
        auto s = asCString();
        return s && s[0];
    }

    return ! (isArray() || isObject()) || size ();
}

void
Value::clear ()
{
    JSON_ASSERT ( type_ == nullValue  ||  type_ == arrayValue  || type_ == objectValue );

    switch ( type_ )
    {
    case arrayValue:
    case objectValue:
        value_.map_->clear ();
        break;

    default:
        break;
    }
}

void
Value::resize ( UInt newSize )
{
    JSON_ASSERT ( type_ == nullValue  ||  type_ == arrayValue );

    if ( type_ == nullValue )
        *this = Value ( arrayValue );

    UInt oldSize = size ();

    if ( newSize == 0 )
        clear ();
    else if ( newSize > oldSize )
        (*this)[ newSize - 1 ];
    else
    {
        for ( UInt index = newSize; index < oldSize; ++index )
            value_.map_->erase ( index );

        assert ( size () == newSize );
    }
}


Value&
Value::operator[] ( UInt index )
{
    JSON_ASSERT ( type_ == nullValue  ||  type_ == arrayValue );

    if ( type_ == nullValue )
        *this = Value ( arrayValue );

    CZString key ( index );
    ObjectValues::iterator it = value_.map_->lower_bound ( key );

    if ( it != value_.map_->end ()  &&  (*it).first == key )
        return (*it).second;

    ObjectValues::value_type defaultValue ( key, null );
    it = value_.map_->insert ( it, defaultValue );
    return (*it).second;
}


const Value&
Value::operator[] ( UInt index ) const
{
    JSON_ASSERT ( type_ == nullValue  ||  type_ == arrayValue );

    if ( type_ == nullValue )
        return null;

    CZString key ( index );
    ObjectValues::const_iterator it = value_.map_->find ( key );

    if ( it == value_.map_->end () )
        return null;

    return (*it).second;
}


Value&
Value::operator[] ( const char* key )
{
    return resolveReference ( key, false );
}


Value&
Value::resolveReference ( const char* key,
                          bool isStatic )
{
    JSON_ASSERT ( type_ == nullValue  ||  type_ == objectValue );

    if ( type_ == nullValue )
        *this = Value ( objectValue );

    CZString actualKey ( key, isStatic ? CZString::noDuplication
                         : CZString::duplicateOnCopy );
    ObjectValues::iterator it = value_.map_->lower_bound ( actualKey );

    if ( it != value_.map_->end ()  &&  (*it).first == actualKey )
        return (*it).second;

    ObjectValues::value_type defaultValue ( actualKey, null );
    it = value_.map_->insert ( it, defaultValue );
    Value& value = (*it).second;
    return value;
}


Value
Value::get ( UInt index,
             const Value& defaultValue ) const
{
    const Value* value = & ((*this)[index]);
    return value == &null ? defaultValue : *value;
}


bool
Value::isValidIndex ( UInt index ) const
{
    return index < size ();
}



const Value&
Value::operator[] ( const char* key ) const
{
    JSON_ASSERT ( type_ == nullValue  ||  type_ == objectValue );

    if ( type_ == nullValue )
        return null;

    CZString actualKey ( key, CZString::noDuplication );
    ObjectValues::const_iterator it = value_.map_->find ( actualKey );

    if ( it == value_.map_->end () )
        return null;

    return (*it).second;
}


Value&
Value::operator[] ( std::string const& key )
{
    return (*this)[ key.c_str () ];
}


const Value&
Value::operator[] ( std::string const& key ) const
{
    return (*this)[ key.c_str () ];
}

Value&
Value::operator[] ( const StaticString& key )
{
    return resolveReference ( key, true );
}

Value&
Value::append ( const Value& value )
{
    return (*this)[size ()] = value;
}


Value
Value::get ( const char* key,
             const Value& defaultValue ) const
{
    const Value* value = & ((*this)[key]);
    return value == &null ? defaultValue : *value;
}


Value
Value::get ( std::string const& key,
             const Value& defaultValue ) const
{
    return get ( key.c_str (), defaultValue );
}

Value
Value::removeMember ( const char* key )
{
    JSON_ASSERT ( type_ == nullValue  ||  type_ == objectValue );

    if ( type_ == nullValue )
        return null;

    CZString actualKey ( key, CZString::noDuplication );
    ObjectValues::iterator it = value_.map_->find ( actualKey );

    if ( it == value_.map_->end () )
        return null;

    Value old (it->second);
    value_.map_->erase (it);
    return old;
}

Value
Value::removeMember ( std::string const& key )
{
    return removeMember ( key.c_str () );
}

bool
Value::isMember ( const char* key ) const
{
    if (type_ != objectValue)
        return false;

    const Value* value = & ((*this)[key]);
    return value != &null;
}


bool
Value::isMember ( std::string const& key ) const
{
    return isMember ( key.c_str () );
}


Value::Members
Value::getMemberNames () const
{
    JSON_ASSERT ( type_ == nullValue  ||  type_ == objectValue );

    if ( type_ == nullValue )
        return Value::Members ();

    Members members;
    members.reserve ( value_.map_->size () );
    ObjectValues::const_iterator it = value_.map_->begin ();
    ObjectValues::const_iterator itEnd = value_.map_->end ();

    for ( ; it != itEnd; ++it )
        members.push_back ( std::string ( (*it).first.c_str () ) );

    return members;
}

bool
Value::isNull () const
{
    return type_ == nullValue;
}


bool
Value::isBool () const
{
    return type_ == booleanValue;
}


bool
Value::isInt () const
{
    return type_ == intValue;
}


bool
Value::isUInt () const
{
    return type_ == uintValue;
}


bool
Value::isIntegral () const
{
    return type_ == intValue
           ||  type_ == uintValue
           ||  type_ == booleanValue;
}


bool
Value::isDouble () const
{
    return type_ == realValue;
}


bool
Value::isNumeric () const
{
    return isIntegral () || isDouble ();
}


bool
Value::isString () const
{
    return type_ == stringValue;
}


bool
Value::isArray() const
{
    return type_ == arrayValue;
}

bool
Value::isArrayOrNull () const
{
    return type_ == nullValue  ||  type_ == arrayValue;
}


bool
Value::isObject() const
{
    return type_ == objectValue;
}

bool
Value::isObjectOrNull () const
{
    return type_ == nullValue  ||  type_ == objectValue;
}

std::string
Value::toStyledString () const
{
    StyledWriter writer;
    return writer.write ( *this );
}


Value::const_iterator
Value::begin () const
{
    switch ( type_ )
    {
    case arrayValue:
    case objectValue:
        if ( value_.map_ )
            return const_iterator ( value_.map_->begin () );

        break;
    default:
        break;
    }

    return const_iterator ();
}

Value::const_iterator
Value::end () const
{
    switch ( type_ )
    {
    case arrayValue:
    case objectValue:
        if ( value_.map_ )
            return const_iterator ( value_.map_->end () );

        break;
    default:
        break;
    }

    return const_iterator ();
}


Value::iterator
Value::begin ()
{
    switch ( type_ )
    {
    case arrayValue:
    case objectValue:
        if ( value_.map_ )
            return iterator ( value_.map_->begin () );
        break;
    default:
        break;
    }

    return iterator ();
}

Value::iterator
Value::end ()
{
    switch ( type_ )
    {
    case arrayValue:
    case objectValue:
        if ( value_.map_ )
            return iterator ( value_.map_->end () );
        break;
    default:
        break;
    }

    return iterator ();
}

} //命名空间JSON
