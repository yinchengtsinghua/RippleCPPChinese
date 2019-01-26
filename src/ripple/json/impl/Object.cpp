
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
#include <ripple/json/Object.h>
#include <cassert>

namespace Json {

Collection::Collection (Collection* parent, Writer* writer)
        : parent_ (parent), writer_ (writer), enabled_ (true)
{
    checkWritable ("Collection::Collection()");
    if (parent_)
    {
        check (parent_->enabled_, "Parent not enabled in constructor");
        parent_->enabled_ = false;
    }
}

Collection::~Collection ()
{
    if (writer_)
        writer_->finish ();
    if (parent_)
        parent_->enabled_ = true;
}

Collection& Collection::operator= (Collection&& that) noexcept
{
    parent_ = that.parent_;
    writer_ = that.writer_;
    enabled_ = that.enabled_;

    that.parent_ = nullptr;
    that.writer_ = nullptr;
    that.enabled_ = false;

    return *this;
}

Collection::Collection (Collection&& that) noexcept
{
    *this = std::move (that);
}

void Collection::checkWritable (std::string const& label)
{
    if (! enabled_)
        ripple::Throw<std::logic_error> (label + ": not enabled");
    if (! writer_)
        ripple::Throw<std::logic_error> (label + ": not writable");
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Object::Root::Root (Writer& w) : Object (nullptr, &w)
{
    writer_->startRoot (Writer::object);
}

Object Object::setObject (std::string const& key)
{
    checkWritable ("Object::setObject");
    if (writer_)
        writer_->startSet (Writer::object, key);
    return Object (this, writer_);
}

Array Object::setArray (std::string const& key) {
    checkWritable ("Object::setArray");
    if (writer_)
        writer_->startSet (Writer::array, key);
    return Array (this, writer_);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Object Array::appendObject ()
{
    checkWritable ("Array::appendObject");
    if (writer_)
        writer_->startAppend (Writer::object);
    return Object (this, writer_);
}

Array Array::appendArray ()
{
    checkWritable ("Array::makeArray");
    if (writer_)
        writer_->startAppend (Writer::array);
    return Array (this, writer_);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Object::Proxy::Proxy (Object& object, std::string const& key)
    : object_ (object)
    , key_ (key)
{
}

Object::Proxy Object::operator[] (std::string const& key)
{
    return Proxy (*this, key);
}

Object::Proxy Object::operator[] (Json::StaticString const& key)
{
    return Proxy (*this, std::string (key));
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void Array::append (Json::Value const& v)
{
    auto t = v.type();
    switch (t)
    {
    case Json::nullValue:    return append (nullptr);
    case Json::intValue:     return append (v.asInt());
    case Json::uintValue:    return append (v.asUInt());
    case Json::realValue:    return append (v.asDouble());
    case Json::stringValue:  return append (v.asString());
    case Json::booleanValue: return append (v.asBool());

    case Json::objectValue:
    {
        auto object = appendObject ();
        copyFrom (object, v);
        return;
    }

    case Json::arrayValue:
    {
        auto array = appendArray ();
        for (auto& item: v)
            array.append (item);
        return;
    }
    }
assert (false);  //无法到达这里。
}

void Object::set (std::string const& k, Json::Value const& v)
{
    auto t = v.type();
    switch (t)
    {
    case Json::nullValue:    return set (k, nullptr);
    case Json::intValue:     return set (k, v.asInt());
    case Json::uintValue:    return set (k, v.asUInt());
    case Json::realValue:    return set (k, v.asDouble());
    case Json::stringValue:  return set (k, v.asString());
    case Json::booleanValue: return set (k, v.asBool());

    case Json::objectValue:
    {
        auto object = setObject (k);
        copyFrom (object, v);
        return;
    }

    case Json::arrayValue:
    {
        auto array = setArray (k);
        for (auto& item: v)
            array.append (item);
        return;
    }
    }
assert (false);  //无法到达这里。
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace {

template <class Object>
void doCopyFrom (Object& to, Json::Value const& from)
{
    assert (from.isObjectOrNull());
    auto members = from.getMemberNames();
    for (auto& m: members)
        to[m] = from[m];
}

}

void copyFrom (Json::Value& to, Json::Value const& from)
{
if (!to)  //短路这很常见的情况。
        to = from;
    else
        doCopyFrom (to, from);
}

void copyFrom (Object& to, Json::Value const& from)
{
    doCopyFrom (to, from);
}

WriterObject stringWriterObject (std::string& s)
{
    return WriterObject (stringOutput (s));
}

} //杰森
