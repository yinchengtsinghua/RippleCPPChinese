
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

#include <ripple/beast/insight/Group.h>
#include <ripple/beast/insight/Groups.h>
#include <ripple/beast/hash/uhash.h>
#include <unordered_map>
#include <memory>

namespace beast {
namespace insight {

namespace detail {

class GroupImp
    : public std::enable_shared_from_this <GroupImp>
    , public Group
{
public:
    using Items = std::vector <std::shared_ptr <BaseImpl>>;

    std::string const m_name;
    Collector::ptr m_collector;

    GroupImp (std::string const& name_,
        Collector::ptr const& collector)
        : m_name (name_)
        , m_collector (collector)
    {
    }

    ~GroupImp() = default;

    std::string const& name () const override
    {
        return m_name;
    }

    std::string make_name (std::string const& name)
    {
        return m_name + "." + name;
    }

    Hook make_hook (HookImpl::HandlerType const& handler) override
    {
        return m_collector->make_hook (handler);
    }

    Counter make_counter (std::string const& name) override
    {
        return m_collector->make_counter (make_name (name));
    }

    Event make_event (std::string const& name) override
    {
        return m_collector->make_event (make_name (name));
    }

    Gauge make_gauge (std::string const& name) override
    {
        return m_collector->make_gauge (make_name (name));
    }

    Meter make_meter (std::string const& name) override
    {
        return m_collector->make_meter (make_name (name));
    }

private:
    GroupImp& operator= (GroupImp const&);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class GroupsImp : public Groups
{
public:
    using Items = std::unordered_map <std::string, std::shared_ptr <Group>, uhash <>>;

    Collector::ptr m_collector;
    Items m_items;

    explicit GroupsImp (Collector::ptr const& collector)
        : m_collector (collector)
    {
    }

    ~GroupsImp() = default;

    Group::ptr const& get (std::string const& name) override
    {
        std::pair <Items::iterator, bool> result (
            m_items.emplace (name, Group::ptr ()));
        Group::ptr& group (result.first->second);
        if (result.second)
            group = std::make_shared <GroupImp> (name, m_collector);
        return group;
    }
};

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Groups::~Groups() = default;

std::unique_ptr <Groups> make_Groups (Collector::ptr const& collector)
{
    return std::make_unique <detail::GroupsImp> (collector);
}

}
}
