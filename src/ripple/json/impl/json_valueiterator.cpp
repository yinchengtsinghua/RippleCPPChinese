
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

//包含在json_value.cpp中

#include <ripple/json/json_value.h>

namespace Json {

//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////
//类值迭代器数据库
//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////

ValueIteratorBase::ValueIteratorBase ()
    : current_ ()
    , isNull_ ( true )
{
}

ValueIteratorBase::ValueIteratorBase ( const Value::ObjectValues::iterator& current )
    : current_ ( current )
    , isNull_ ( false )
{
}

Value&
ValueIteratorBase::deref () const
{
    return current_->second;
}


void
ValueIteratorBase::increment ()
{
    ++current_;
}


void
ValueIteratorBase::decrement ()
{
    --current_;
}


ValueIteratorBase::difference_type
ValueIteratorBase::computeDistance ( const SelfType& other ) const
{
//使用默认值初始化空值的迭代器
//构造器，将当前值初始化为默认值
//标准：：映射：：迭代器。因为begin（）和end（）是两个实例
//对于默认的std:：map:：iterator，不能比较它们。
//为了实现这一点，我们专门处理这种比较。
    if ( isNull_  &&  other.isNull_ )
    {
        return 0;
    }


//std:：Distance的用法不可移植（不使用Sun Studio 12 roguewave stl编译，
//默认使用）。
//将便携式手工版本用于非随机迭代器：
//返回差_类型（std：：距离（current_uuu，other.current_uu））；
    difference_type myDistance = 0;

    for ( Value::ObjectValues::iterator it = current_; it != other.current_; ++it )
    {
        ++myDistance;
    }

    return myDistance;
}


bool
ValueIteratorBase::isEqual ( const SelfType& other ) const
{
    if ( isNull_ )
    {
        return other.isNull_;
    }

    return current_ == other.current_;
}


void
ValueIteratorBase::copy ( const SelfType& other )
{
    current_ = other.current_;
}


Value
ValueIteratorBase::key () const
{
    const Value::CZString czstring = (*current_).first;

    if ( czstring.c_str () )
    {
        if ( czstring.isStaticString () )
            return Value ( StaticString ( czstring.c_str () ) );

        return Value ( czstring.c_str () );
    }

    return Value ( czstring.index () );
}


UInt
ValueIteratorBase::index () const
{
    const Value::CZString czstring = (*current_).first;

    if ( !czstring.c_str () )
        return czstring.index ();

    return Value::UInt ( -1 );
}


const char*
ValueIteratorBase::memberName () const
{
    const char* name = (*current_).first.c_str ();
    return name ? name : "";
}


//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////
//类ValueConst迭代器
//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////


ValueConstIterator::ValueConstIterator ( const Value::ObjectValues::iterator& current )
    : ValueIteratorBase ( current )
{
}

ValueConstIterator&
ValueConstIterator::operator = ( const ValueIteratorBase& other )
{
    copy ( other );
    return *this;
}


//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////
//类值迭代器
//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////


ValueIterator::ValueIterator ( const Value::ObjectValues::iterator& current )
    : ValueIteratorBase ( current )
{
}

ValueIterator::ValueIterator ( const ValueConstIterator& other )
    : ValueIteratorBase ( other )
{
}

ValueIterator::ValueIterator ( const ValueIterator& other )
    : ValueIteratorBase ( other )
{
}

ValueIterator&
ValueIterator::operator = ( const SelfType& other )
{
    copy ( other );
    return *this;
}

} //杰森
