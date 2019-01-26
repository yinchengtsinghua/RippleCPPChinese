
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

#ifndef RIPPLE_BASICS_BASICCONFIG_H_INCLUDED
#define RIPPLE_BASICS_BASICCONFIG_H_INCLUDED

#include <ripple/basics/contract.h>
#include <beast/unit_test/detail/const_container.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace ripple {

using IniFileSections = std::map<std::string, std::vector<std::string>>;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*保存配置值的集合。
    配置文件包含零个或多个节。
**/

class Section
    : public beast::unit_test::detail::const_container <
        std::map <std::string, std::string, boost::beast::iless>>
{
private:
    std::string name_;
    std::vector <std::string> lines_;
    std::vector <std::string> values_;

public:
    /*创建空节。*/
    explicit
    Section (std::string const& name = "");

    /*返回此节的名称。*/
    std::string const&
    name() const
    {
        return name_;
    }

    /*返回节中的所有行。
        这包括一切。
    **/

    std::vector <std::string> const&
    lines() const
    {
        return lines_;
    }

    /*返回节中的所有值。
        值是非空行，不是键/值对。
    **/

    std::vector <std::string> const&
    values() const
    {
        return values_;
    }

    /*
     *设置此部分的旧值。
     **/

    void
    legacy (std::string value)
    {
        if (lines_.empty ())
            lines_.emplace_back (std::move (value));
        else
            lines_[0] = std::move (value);
    }

    /*
     *获取此部分的旧值。
     *
     *@返回检索到的值。返回具有空旧值的节
               空字符串。
     **/

    std::string
    legacy () const
    {
        if (lines_.empty ())
            return "";
        if (lines_.size () > 1)
            Throw<std::runtime_error> (
                "A legacy value must have exactly one line. Section: " + name_);
        return lines_[0];
    }

    /*设置键/值对。
        前一个值被丢弃。
    **/

    void
    set (std::string const& key, std::string const& value);

    /*在此节中追加一组行。
        将包含键/值对的行添加到映射中，
        否则，它们将添加到值列表中。一切都是
        添加到行列表中。
    **/

    void
    append (std::vector <std::string> const& lines);

    /*在此节中追加一行。*/
    void
    append (std::string const& line)
    {
        append (std::vector<std::string>{ line });
    }

    /*如果存在具有给定名称的键，则返回“true”。*/
    bool
    exists (std::string const& name) const;

    /*检索键/值对。
        @如果找到字符串，返回一对bool为'true'。
    **/

    std::pair <std::string, bool>
    find (std::string const& name) const;

    template <class T>
    boost::optional<T>
    get (std::string const& name) const
    {
        auto const iter = cont().find(name);
        if (iter == cont().end())
            return boost::none;
        return boost::lexical_cast<T>(iter->second);
    }

///返回一个值（如果存在），否则返回另一个值。
    template<class T>
    T
    value_or(std::string const& name, T const& other) const
    {
        auto const iter = cont().find(name);
        if (iter == cont().end())
            return other;
        return boost::lexical_cast<T>(iter->second);
    }

    friend
    std::ostream&
    operator<< (std::ostream&, Section const& section);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*保存未分析的配置信息。
    原始数据部分使用特定于中间分析器的
    到每个模块，而不是在一个中心位置全部解析。
**/

class BasicConfig
{
private:
    std::map <std::string, Section, boost::beast::iless> map_;

public:
    /*如果存在具有给定名称的节，则返回“true”。*/
    bool
    exists (std::string const& name) const;

    /*返回具有给定名称的节。
        如果该节不存在，则返回空节。
    **/

    /*@ {*/
    Section&
    section (std::string const& name);

    Section const&
    section (std::string const& name) const;

    Section const&
    operator[] (std::string const& name) const
    {
        return section(name);
    }

    Section&
    operator[] (std::string const& name)
    {
        return section(name);
    }
    /*@ }*/

    /*用命令行参数覆盖键/值对
        如果该节不存在，则创建该节。
        上一个值（如果有）将被覆盖。
    **/

    void
    overwrite (std::string const& section, std::string const& key,
        std::string const& value);

    /*从部分中删除所有键/值对。
     **/

    void
    deprecatedClearSection (std::string const& section);

    /*
     *设置不是键/值对的值。
     *
     *该值存储为节的第一个值，可以检索到该值。
     *通过节：：Legacy。
     *
     *@param要修改的节的节名称。
     *@param value旧值的内容。
     **/

    void
    legacy(std::string const& section, std::string value);

    /*
     *获取节的旧值。带有
     *单行值可以作为旧值检索。
     *
     *@param sectionname检索此节的内容
     *传统价值。
     *@返回旧值的内容。
     **/

    std::string
    legacy(std::string const& sectionName) const;

    friend
    std::ostream&
    operator<< (std::ostream& ss, BasicConfig const& c);

protected:
    void
    build (IniFileSections const& ifs);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*从配置节设置值
    如果未找到命名值，则变量不变。
    @return`true`如果设置了值。
**/

template <class T>
bool
set (T& target, std::string const& name, Section const& section)
{
    auto const result = section.find (name);
    if (! result.second)
        return false;
    try
    {
        target = boost::lexical_cast <T> (result.first);
        return true;
    }
    catch (boost::bad_lexical_cast&)
    {
    }
    return false;
}

/*从配置节设置值
    如果找不到命名值，将为变量分配默认值。
    @return`true`如果在节中找到命名值。
**/

template <class T>
bool
set (T& target, T const& defaultValue,
    std::string const& name, Section const& section)
{
    auto const result = section.find (name);
    if (! result.second)
        return false;
    try
    {
//vfalc todo使用try_lexical_convert（boost 1.56.0）
        target = boost::lexical_cast <T> (result.first);
        return true;
    }
    catch (boost::bad_lexical_cast&)
    {
        target = defaultValue;
    }
    return false;
}

/*从节中检索键/值对。
    @返回转换为t的值字符串（如果存在）
            并且可以被解析，或者默认值。
**/

//注意这个程序可能比前两个更笨拙
template <class T>
T
get (Section const& section,
    std::string const& name, T const& defaultValue = T{})
{
    auto const result = section.find (name);
    if (! result.second)
        return defaultValue;
    try
    {
        return boost::lexical_cast <T> (result.first);
    }
    catch (boost::bad_lexical_cast&)
    {
    }
    return defaultValue;
}

inline
std::string
get (Section const& section,
    std::string const& name, const char* defaultValue)
{
    auto const result = section.find (name);
    if (! result.second)
        return defaultValue;
    try
    {
        return boost::lexical_cast <std::string> (result.first);
    }
    catch(std::exception const&)
    {
    }
    return defaultValue;
}

template <class T>
bool
get_if_exists (Section const& section,
    std::string const& name, T& v)
{
    auto const result = section.find (name);
    if (! result.second)
        return false;
    try
    {
        v = boost::lexical_cast <T> (result.first);
        return true;
    }
    catch (boost::bad_lexical_cast&)
    {
    }
    return false;
}

template <>
inline
bool
get_if_exists<bool> (Section const& section,
    std::string const& name, bool& v)
{
    int intVal = 0;
    if (get_if_exists (section, name, intVal))
    {
        v = bool (intVal);
        return true;
    }
    return false;
}

} //涟漪

#endif
