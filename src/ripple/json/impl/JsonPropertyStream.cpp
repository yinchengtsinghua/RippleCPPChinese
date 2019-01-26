
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

#include <ripple/json/json_value.h>
#include <ripple/json/JsonPropertyStream.h>

namespace ripple {

JsonPropertyStream::JsonPropertyStream ()
    : m_top (Json::objectValue)
{
    m_stack.reserve (64);
    m_stack.push_back (&m_top);
}

Json::Value const& JsonPropertyStream::top() const
{
    return m_top;
}

void JsonPropertyStream::map_begin ()
{
//顶部阵列
    Json::Value& top (*m_stack.back());
    Json::Value& map (top.append (Json::objectValue));
    m_stack.push_back (&map);
}

void JsonPropertyStream::map_begin (std::string const& key)
{
//顶是地图
    Json::Value& top (*m_stack.back());
    Json::Value& map (top [key] = Json::objectValue);
    m_stack.push_back (&map);
}

void JsonPropertyStream::map_end ()
{
    m_stack.pop_back ();
}

void JsonPropertyStream::add (std::string const& key, short v)
{
    (*m_stack.back())[key] = v;
}

void JsonPropertyStream::add (std::string const& key, unsigned short v)
{
    (*m_stack.back())[key] = v;
}

void JsonPropertyStream::add (std::string const& key, int v)
{
    (*m_stack.back())[key] = v;
}

void JsonPropertyStream::add (std::string const& key, unsigned int v)
{
    (*m_stack.back())[key] = v;
}

void JsonPropertyStream::add (std::string const& key, long v)
{
    (*m_stack.back())[key] = int(v);
}

void JsonPropertyStream::add (std::string const& key, float v)
{
    (*m_stack.back())[key] = v;
}

void JsonPropertyStream::add (std::string const& key, double v)
{
    (*m_stack.back())[key] = v;
}

void JsonPropertyStream::add (std::string const& key, std::string const& v)
{
    (*m_stack.back())[key] = v;
}

void JsonPropertyStream::array_begin ()
{
//顶部阵列
    Json::Value& top (*m_stack.back());
    Json::Value& vec (top.append (Json::arrayValue));
    m_stack.push_back (&vec);
}

void JsonPropertyStream::array_begin (std::string const& key)
{
//顶是地图
    Json::Value& top (*m_stack.back());
    Json::Value& vec (top [key] = Json::arrayValue);
    m_stack.push_back (&vec);
}

void JsonPropertyStream::array_end ()
{
    m_stack.pop_back ();
}

void JsonPropertyStream::add (short v)
{
    m_stack.back()->append (v);
}

void JsonPropertyStream::add (unsigned short v)
{
    m_stack.back()->append (v);
}

void JsonPropertyStream::add (int v)
{
    m_stack.back()->append (v);
}

void JsonPropertyStream::add (unsigned int v)
{
    m_stack.back()->append (v);
}

void JsonPropertyStream::add (long v)
{
    m_stack.back()->append (int (v));
}

void JsonPropertyStream::add (float v)
{
    m_stack.back()->append (v);
}

void JsonPropertyStream::add (double v)
{
    m_stack.back()->append (v);
}

void JsonPropertyStream::add (std::string const& v)
{
    m_stack.back()->append (v);
}

}

