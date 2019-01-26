
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#include <ripple/beast/utility/PropertyStream.h>
#include <algorithm>
#include <cassert>
#include <limits>
#include <iostream>

namespace beast {

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//项目
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

PropertyStream::Item::Item (Source* source)
    : m_source (source)
{
}

PropertyStream::Source& PropertyStream::Item::source() const
{
    return *m_source;
}

PropertyStream::Source* PropertyStream::Item::operator-> () const
{
    return &source();
}

PropertyStream::Source& PropertyStream::Item::operator* () const
{
    return source();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//代理
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

PropertyStream::Proxy::Proxy (
    Map const& map, std::string const& key)
    : m_map (&map)
    , m_key (key)
{
}

PropertyStream::Proxy::Proxy (Proxy const& other)
    : m_map (other.m_map)
    , m_key (other.m_key)
{
}

PropertyStream::Proxy::~Proxy ()
{
    std::string const s (m_ostream.str());
    if (! s.empty())
        m_map->add (m_key, s);
}

std::ostream& PropertyStream::Proxy::operator<< (
    std::ostream& manip (std::ostream&)) const
{
    return m_ostream << manip;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//地图
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

PropertyStream::Map::Map (PropertyStream& stream)
    : m_stream (stream)
{
}

PropertyStream::Map::Map (Set& parent)
    : m_stream (parent.stream())
{
    m_stream.map_begin ();
}

PropertyStream::Map::Map (std::string const& key, Map& map)
    : m_stream (map.stream())
{
    m_stream.map_begin (key);
}

PropertyStream::Map::Map (std::string const& key, PropertyStream& stream)
    : m_stream (stream)
{
    m_stream.map_begin (key);
}

PropertyStream::Map::~Map ()
{
    m_stream.map_end ();
}

PropertyStream& PropertyStream::Map::stream()
{
    return m_stream;
}

PropertyStream const& PropertyStream::Map::stream() const
{
    return m_stream;
}

PropertyStream::Proxy PropertyStream::Map::operator[] (std::string const& key)
{
    return Proxy (*this, key);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//集合
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

PropertyStream::Set::Set (std::string const& key, Map& map)
    : m_stream (map.stream())
{
    m_stream.array_begin (key);
}

PropertyStream::Set::Set (std::string const& key, PropertyStream& stream)
    : m_stream (stream)
{
    m_stream.array_begin (key);
}

PropertyStream::Set::~Set ()
{
    m_stream.array_end ();
}

PropertyStream& PropertyStream::Set::stream()
{
    return m_stream;
}

PropertyStream const& PropertyStream::Set::stream() const
{
    return m_stream;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//来源
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

PropertyStream::Source::Source (std::string const& name)
    : m_name (name)
    , item_ (this)
    , parent_ (nullptr)
{
}

PropertyStream::Source::~Source ()
{
    std::lock_guard<std::recursive_mutex> _(lock_);
    if (parent_ != nullptr)
        parent_->remove (*this);
    removeAll ();
}

std::string const& PropertyStream::Source::name () const
{
    return m_name;
}

void PropertyStream::Source::add (Source& source)
{
    std::lock(lock_, source.lock_);
    std::lock_guard<std::recursive_mutex> lk1(lock_, std::adopt_lock);
    std::lock_guard<std::recursive_mutex> lk2(source.lock_, std::adopt_lock);

    assert (source.parent_ == nullptr);
    children_.push_back (source.item_);
    source.parent_ = this;
}

void PropertyStream::Source::remove (Source& child)
{
    std::lock(lock_, child.lock_);
    std::lock_guard<std::recursive_mutex> lk1(lock_, std::adopt_lock);
    std::lock_guard<std::recursive_mutex> lk2(child.lock_, std::adopt_lock);

    assert (child.parent_ == this);
    children_.erase (
        children_.iterator_to (
            child.item_));
    child.parent_ = nullptr;
}

void PropertyStream::Source::removeAll ()
{
    std::lock_guard<std::recursive_mutex> _(lock_);
    for (auto iter = children_.begin(); iter != children_.end(); )
    {
        std::lock_guard<std::recursive_mutex> _cl((*iter)->lock_);
        remove (*(*iter));
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void PropertyStream::Source::write_one (PropertyStream& stream)
{
    Map map (m_name, stream);
    onWrite (map);
}

void PropertyStream::Source::write (PropertyStream& stream)
{
    Map map (m_name, stream);
    onWrite (map);

    std::lock_guard<std::recursive_mutex> _(lock_);

    for (auto& child : children_)
        child.source().write (stream);
}

void PropertyStream::Source::write (PropertyStream& stream, std::string const& path)
{
    std::pair <Source*, bool> result (find (path));

    if (result.first == nullptr)
        return;

    if (result.second)
        result.first->write (stream);
    else
        result.first->write_one (stream);
}

std::pair <PropertyStream::Source*, bool> PropertyStream::Source::find (std::string path)
{
    bool const deep (peel_trailing_slashstar (&path));
    bool const rooted (peel_leading_slash (&path));
    Source* source (this);
    if (! path.empty())
    {
        if (! rooted)
        {
            std::string const name (peel_name (&path));
            source = find_one_deep (name);
            if (source == nullptr)
                return std::make_pair (nullptr, deep);
        }
        source = source->find_path (path);
    }
    return std::make_pair (source, deep);
}

bool PropertyStream::Source::peel_leading_slash (std::string* path)
{
    if (! path->empty() && path->front() == '/')
    {
        *path = std::string (path->begin() + 1, path->end());
        return true;
    }
    return false;
}

bool PropertyStream::Source::peel_trailing_slashstar (std::string* path)
{
    bool found(false);
    if (path->empty())
        return false;
    if (path->back() == '*')
    {
        found = true;
        path->pop_back();
    }
    if(! path->empty() && path->back() == '/')
        path->pop_back();
    return found;
}

std::string PropertyStream::Source::peel_name (std::string* path)
{
    if (path->empty())
        return "";

    std::string::const_iterator first = (*path).begin();
    std::string::const_iterator last = (*path).end();
    std::string::const_iterator pos = std::find (first, last, '/');
    std::string s (first, pos);

    if (pos != last)
        *path = std::string (pos+1, last);
    else
        *path = std::string ();

    return s;
}

//在整个树中递归搜索，直到找到名称
PropertyStream::Source* PropertyStream::Source::find_one_deep (std::string const& name)
{
    Source* found = find_one (name);
    if (found != nullptr)
        return found;

    std::lock_guard<std::recursive_mutex> _(lock_);
    for (auto& s : children_)
    {
        found = s.source().find_one_deep (name);
        if (found != nullptr)
            return found;
    }
    return nullptr;
}

PropertyStream::Source* PropertyStream::Source::find_path (std::string path)
{
    if (path.empty())
        return this;
    Source* source (this);
    do
    {
        std::string const name (peel_name (&path));
        if(name.empty ())
            break;
        source = source->find_one(name);
    }
    while (source != nullptr);
    return source;
}

//此函数只查看直接子级
//如果没有立即子项匹配，则返回nullptr
PropertyStream::Source* PropertyStream::Source::find_one (std::string const& name)
{
    std::lock_guard<std::recursive_mutex> _(lock_);
    for (auto& s : children_)
    {
        if (s.source().m_name == name)
            return &s.source();
    }
    return nullptr;
}

void PropertyStream::Source::onWrite (Map&)
{
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//属性流
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void PropertyStream::add (std::string const& key, bool value)
{
    if (value)
        add (key, "true");
    else
        add (key, "false");
}

void PropertyStream::add (std::string const& key, char value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, signed char value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, unsigned char value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, wchar_t value)
{
    lexical_add (key, value);
}

#if 0
void PropertyStream::add (std::string const& key, char16_t value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, char32_t value)
{
    lexical_add (key, value);
}
#endif

void PropertyStream::add (std::string const& key, short value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, unsigned short value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, int value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, unsigned int value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, long value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, unsigned long value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, long long value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, unsigned long long value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, float value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, double value)
{
    lexical_add (key, value);
}

void PropertyStream::add (std::string const& key, long double value)
{
    lexical_add (key, value);
}

void PropertyStream::add (bool value)
{
    if (value)
        add ("true");
    else
        add ("false");
}

void PropertyStream::add (char value)
{
    lexical_add (value);
}

void PropertyStream::add (signed char value)
{
    lexical_add (value);
}

void PropertyStream::add (unsigned char value)
{
    lexical_add (value);
}

void PropertyStream::add (wchar_t value)
{
    lexical_add (value);
}

#if 0
void PropertyStream::add (char16_t value)
{
    lexical_add (value);
}

void PropertyStream::add (char32_t value)
{
    lexical_add (value);
}
#endif

void PropertyStream::add (short value)
{
    lexical_add (value);
}

void PropertyStream::add (unsigned short value)
{
    lexical_add (value);
}

void PropertyStream::add (int value)
{
    lexical_add (value);
}

void PropertyStream::add (unsigned int value)
{
    lexical_add (value);
}

void PropertyStream::add (long value)
{
    lexical_add (value);
}

void PropertyStream::add (unsigned long value)
{
    lexical_add (value);
}

void PropertyStream::add (long long value)
{
    lexical_add (value);
}

void PropertyStream::add (unsigned long long value)
{
    lexical_add (value);
}

void PropertyStream::add (float value)
{
    lexical_add (value);
}

void PropertyStream::add (double value)
{
    lexical_add (value);
}

void PropertyStream::add (long double value)
{
    lexical_add (value);
}

}

