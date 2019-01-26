
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

#include <ripple/resource/Consumer.h>
#include <ripple/resource/impl/Entry.h>
#include <ripple/resource/impl/Logic.h>
#include <cassert>

namespace ripple {
namespace Resource {

Consumer::Consumer (Logic& logic, Entry& entry)
    : m_logic (&logic)
    , m_entry (&entry)
{
}

Consumer::Consumer ()
    : m_logic (nullptr)
    , m_entry (nullptr)
{
}

Consumer::Consumer (Consumer const& other)
    : m_logic (other.m_logic)
    , m_entry (nullptr)
{
    if (m_logic && other.m_entry)
    {
        m_entry = other.m_entry;
        m_logic->acquire (*m_entry);
    }
}

Consumer::~Consumer()
{
    if (m_logic && m_entry)
        m_logic->release (*m_entry);
}

Consumer& Consumer::operator= (Consumer const& other)
{
//去掉旧裁判
    if (m_logic && m_entry)
        m_logic->release (*m_entry);

    m_logic = other.m_logic;
    m_entry = other.m_entry;

//添加新参考文献
    if (m_logic && m_entry)
        m_logic->acquire (*m_entry);

    return *this;
}

std::string Consumer::to_string () const
{
    if (m_logic == nullptr)
        return "(none)";

    return m_entry->to_string();
}

bool Consumer::isUnlimited () const
{
    if (m_entry)
        return m_entry->isUnlimited();

    return false;
}

Disposition Consumer::disposition() const
{
    Disposition d = ok;
    if (m_logic && m_entry)
        d =  m_logic->charge(*m_entry, Charge(0));

    return d;
}

Disposition Consumer::charge (Charge const& what)
{
    Disposition d = ok;

    if (m_logic && m_entry)
        d = m_logic->charge (*m_entry, what);

    return d;
}

bool Consumer::warn ()
{
    assert (m_entry != nullptr);
    return m_logic->warn (*m_entry);
}

bool Consumer::disconnect ()
{
    assert (m_entry != nullptr);
    return m_logic->disconnect (*m_entry);
}

int Consumer::balance()
{
    assert (m_entry != nullptr);
    return m_logic->balance (*m_entry);
}

Entry& Consumer::entry()
{
    assert (m_entry != nullptr);
    return *m_entry;
}

std::ostream& operator<< (std::ostream& os, Consumer const& v)
{
    os << v.to_string();
    return os;
}

}
}
