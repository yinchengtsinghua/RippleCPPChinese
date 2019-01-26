
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

#include <ripple/basics/BasicConfig.h>
#include <boost/regex.hpp>
#include <algorithm>

namespace ripple {

Section::Section (std::string const& name)
    : name_(name)
{
}

void
Section::set (std::string const& key, std::string const& value)
{
    auto const result = cont().emplace (key, value);
    if (! result.second)
        result.first->second = value;
}

void
Section::append (std::vector <std::string> const& lines)
{
//<key>'='<value>
    static boost::regex const re1 (
"^"                         //起点线
"(?:\\s*)"                  //空白（视觉）
"([a-zA-Z][_a-zA-Z0-9]*)"   //<密钥>
"(?:\\s*)"                  //空白（可选）
"(?:=)"                     //“=”
"(?:\\s*)"                  //空白（可选）
"(.*\\S+)"                  //<值>
"(?:\\s*)"                  //空白（可选）
        , boost::regex_constants::optimize
    );

    lines_.reserve (lines_.size() + lines.size());
    for (auto const& line : lines)
    {
        boost::smatch match;
        lines_.push_back (line);
        if (boost::regex_match (line, match, re1))
            set (match[1], match[2]);
        else
            values_.push_back (line);
    }
}

bool
Section::exists (std::string const& name) const
{
    return cont().find (name) != cont().end();
}

std::pair <std::string, bool>
Section::find (std::string const& name) const
{
    auto const iter = cont().find (name);
    if (iter == cont().end())
        return {{}, false};
    return {iter->second, true};
}

std::ostream&
operator<< (std::ostream& os, Section const& section)
{
    for (auto const& kv : section.cont())
        os << kv.first << "=" << kv.second << "\n";
    return os;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

bool
BasicConfig::exists (std::string const& name) const
{
    return map_.find(name) != map_.end();
}

Section&
BasicConfig::section (std::string const& name)
{
    return map_[name];
}

Section const&
BasicConfig::section (std::string const& name) const
{
    static Section none("");
    auto const iter = map_.find (name);
    if (iter == map_.end())
        return none;
    return iter->second;
}

void
BasicConfig::overwrite (std::string const& section, std::string const& key,
    std::string const& value)
{
    auto const result = map_.emplace (std::piecewise_construct,
        std::make_tuple(section), std::make_tuple(section));
    result.first->second.set (key, value);
}

void
BasicConfig::deprecatedClearSection (std::string const& section)
{
    auto i = map_.find(section);
    if (i != map_.end())
        i->second = Section(section);
}

void
BasicConfig::legacy(std::string const& section, std::string value)
{
    map_[section].legacy(std::move(value));
}

std::string
BasicConfig::legacy(std::string const& sectionName) const
{
    return section (sectionName).legacy ();
}

void
BasicConfig::build (IniFileSections const& ifs)
{
    for (auto const& entry : ifs)
    {
        auto const result = map_.emplace (std::piecewise_construct,
            std::make_tuple(entry.first), std::make_tuple(entry.first));
        result.first->second.append (entry.second);
    }
}

std::ostream&
operator<< (std::ostream& ss, BasicConfig const& c)
{
    for (auto const& s : c.map_)
        ss << "[" << s.first << "]\n" << s.second;
    return ss;
}

} //涟漪
