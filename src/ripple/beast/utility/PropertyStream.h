
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

#ifndef BEAST_UTILITY_PROPERTYSTREAM_H_INCLUDED
#define BEAST_UTILITY_PROPERTYSTREAM_H_INCLUDED

#include <ripple/beast/core/List.h>

#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

namespace beast {

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*使用RAII容器抽象流，生成属性树。*/
class PropertyStream
{
public:
    class Map;
    class Set;
    class Source;

    PropertyStream () = default;
    virtual ~PropertyStream () = default;

protected:
    virtual void map_begin () = 0;
    virtual void map_begin (std::string const& key) = 0;
    virtual void map_end () = 0;

    virtual void add (std::string const& key, std::string const& value) = 0;

    void add (std::string const& key, char const* value)
    {
        add (key, std::string (value));
    }

    template <typename Value>
    void lexical_add (std::string const &key, Value value)
    {
        std::stringstream ss;
        ss << value;
        add (key, ss.str());
    }

    virtual void add (std::string const& key, bool value);
    virtual void add (std::string const& key, char value);
    virtual void add (std::string const& key, signed char value);
    virtual void add (std::string const& key, unsigned char value);
    virtual void add (std::string const& key, wchar_t value);
#if 0
    virtual void add (std::string const& key, char16_t value);
    virtual void add (std::string const& key, char32_t value);
#endif
    virtual void add (std::string const& key, short value);
    virtual void add (std::string const& key, unsigned short value);
    virtual void add (std::string const& key, int value);
    virtual void add (std::string const& key, unsigned int value);
    virtual void add (std::string const& key, long value);
    virtual void add (std::string const& key, unsigned long value);
    virtual void add (std::string const& key, long long value);
    virtual void add (std::string const& key, unsigned long long value);
    virtual void add (std::string const& key, float value);
    virtual void add (std::string const& key, double value);
    virtual void add (std::string const& key, long double value);

    virtual void array_begin () = 0;
    virtual void array_begin (std::string const& key) = 0;
    virtual void array_end () = 0;

    virtual void add (std::string const& value) = 0;

    void add (char const* value)
    {
        add (std::string (value));
    }

    template <typename Value>
    void lexical_add (Value value)
    {
        std::stringstream ss;
        ss << value;
        add (ss.str());
    }

    virtual void add (bool value);
    virtual void add (char value);
    virtual void add (signed char value);
    virtual void add (unsigned char value);
    virtual void add (wchar_t value);
#if 0
    virtual void add (char16_t value);
    virtual void add (char32_t value);
#endif
    virtual void add (short value);
    virtual void add (unsigned short value);
    virtual void add (int value);
    virtual void add (unsigned int value);
    virtual void add (long value);
    virtual void add (unsigned long value);
    virtual void add (long long value);
    virtual void add (unsigned long long value);
    virtual void add (float value);
    virtual void add (double value);
    virtual void add (long double value);

private:
    class Item;
    class Proxy;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//项目
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class PropertyStream::Item : public List <Item>::Node
{
public:
    explicit Item (Source* source);
    Source& source() const;
    Source* operator-> () const;
    Source& operator* () const;
private:
    Source* m_source;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//代理
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class PropertyStream::Proxy
{
private:
    Map const* m_map;
    std::string m_key;
    std::ostringstream mutable m_ostream;

public:
    Proxy (Map const& map, std::string const& key);
    Proxy (Proxy const& other);
    ~Proxy ();

    template <typename Value>
    Proxy& operator= (Value value);

    std::ostream& operator<< (std::ostream& manip (std::ostream&)) const;

    template <typename T>
    std::ostream& operator<< (T const& t) const
    {
        return m_ostream << t;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//地图
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class PropertyStream::Map
{
private:
    PropertyStream& m_stream;

public:
    explicit Map (PropertyStream& stream);
    explicit Map (Set& parent);
    Map (std::string const& key, Map& parent);
    Map (std::string const& key, PropertyStream& stream);
    ~Map ();

    Map(Map const&) = delete;
    Map& operator= (Map const&) = delete;

    PropertyStream& stream();
    PropertyStream const& stream() const;

    template <typename Value>
    void add (std::string const& key, Value value) const
    {
        m_stream.add (key, value);
    }

    template <typename Key, typename Value>
    void add (Key key, Value value) const
    {
        std::stringstream ss;
        ss << key;
        add (ss.str(), value);
    }

    Proxy operator[] (std::string const& key);

    Proxy operator[] (char const* key)
        { return Proxy (*this, key); }

    template <typename Key>
    Proxy operator[] (Key key) const
    {
        std::stringstream ss;
        ss << key;
        return Proxy (*this, ss.str());
    }
};

//——————————————————————————————————————————————————————————————

template <typename Value>
PropertyStream::Proxy& PropertyStream::Proxy::operator= (Value value)
{
    m_map->add (m_key, value);
    return *this;
}

//——————————————————————————————————————————————————————————————
//
//集合
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class PropertyStream::Set
{
private:
    PropertyStream& m_stream;

public:
    Set (std::string const& key, Map& map);
    Set (std::string const& key, PropertyStream& stream);
    ~Set ();

    Set (Set const&) = delete;
    Set& operator= (Set const&) = delete;

    PropertyStream& stream();
    PropertyStream const& stream() const;

    template <typename Value>
    void add (Value value) const
        { m_stream.add (value); }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//来源
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*可以调用子类来写入流并有子类。*/
class PropertyStream::Source
{
private:
    std::string const m_name;
    std::recursive_mutex lock_;
    Item item_;
    Source* parent_;
    List <Item> children_;

public:
    explicit Source (std::string const& name);
    virtual ~Source ();

    Source (Source const&) = delete;
    Source& operator= (Source const&) = delete;

    /*返回此源的名称。*/
    std::string const& name() const;

    /*添加子源。*/
    void add (Source& source);

    /*通过指针添加子源。
        返回了源指针，因此可以在ctor初始值设定项中使用它。
    **/

    template <class Derived>
    Derived* add (Derived* child)
    {
        add (*static_cast <Source*>(child));
        return child;
    }

    /*从此源中删除子源。*/
    void remove (Source& child);

    /*从此源中删除所有子源。*/
    void removeAll ();

    /*只将此源写入流。*/
    void write_one  (PropertyStream& stream);

    /*将此源及其所有子级递归写入流。*/
    void write      (PropertyStream& stream);

    /*解析路径并写入相应的源和可选子级。
        如果找到源，则写入。如果通配符“*”
        作为路径中的最后一个字符存在，则所有子级都是
        以递归方式写入。
    **/

    void write      (PropertyStream& stream, std::string const& path);

    /*分析点分隔的源路径并返回结果。
        第一个值将是指向对应的源对象的指针
        到给定的路径。如果不存在源对象，则第一个值
        将为nullptr，第二个值将未定义。
        第二个值是一个布尔值，指示路径字符串
        指定通配符“*”作为最后一个字符。

        打印语句示例
        “parent.child”打印子级及其所有子级
        “parent.child.”从父级开始，打印到子级
        “parent.grandchild”不打印任何内容-孙子不直接丢弃
        “parent.grandchild.”从父级开始，打印到孙级
        “parent.grandchild.*”从父级开始，通过孙级打印
    **/

    std::pair <Source*, bool> find (std::string path);

    Source* find_one_deep (std::string const& name);
    PropertyStream::Source* find_path(std::string path);
    PropertyStream::Source* find_one(std::string const& name);

    static bool peel_leading_slash (std::string* path);
    static bool peel_trailing_slashstar (std::string* path);
    static std::string peel_name(std::string* path);


//——————————————————————————————————————————————————————————————

    /*子类重写。
        默认版本不起作用。
    **/

    virtual void onWrite (Map&);
};

}

#endif
