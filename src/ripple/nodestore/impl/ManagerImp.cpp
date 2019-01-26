
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

#include <ripple/nodestore/impl/ManagerImp.h>
#include <ripple/nodestore/impl/DatabaseNodeImp.h>

namespace ripple {
namespace NodeStore {

ManagerImp&
ManagerImp::instance()
{
    static ManagerImp _;
    return _;
}

void
ManagerImp::missing_backend()
{
    Throw<std::runtime_error> (
        "Your rippled.cfg is missing a [node_db] entry, "
        "please see the rippled-example.cfg file!"
        );
}

std::unique_ptr <Backend>
ManagerImp::make_Backend (
    Section const& parameters,
    Scheduler& scheduler,
    beast::Journal journal)
{
    std::string const type {get<std::string>(parameters, "type")};
    if (type.empty())
        missing_backend();

    auto factory {find(type)};
    if(!factory)
        missing_backend();

    return factory->createInstance(
        NodeObject::keyBytes, parameters, scheduler, journal);
}

std::unique_ptr <Database>
ManagerImp::make_Database (
    std::string const& name,
    Scheduler& scheduler,
    int readThreads,
    Stoppable& parent,
    Section const& config,
    beast::Journal journal)
{
    auto backend {make_Backend(config, scheduler, journal)};
    backend->open();
    return std::make_unique <DatabaseNodeImp>(
        name,
        scheduler,
        readThreads,
        parent,
        std::move(backend),
        config,
        journal);
}

void
ManagerImp::insert (Factory& factory)
{
    std::lock_guard<std::mutex> _(mutex_);
    list_.push_back(&factory);
}

void
ManagerImp::erase (Factory& factory)
{
    std::lock_guard<std::mutex> _(mutex_);
    auto const iter = std::find_if(list_.begin(), list_.end(),
        [&factory](Factory* other) { return other == &factory; });
    assert(iter != list_.end());
    list_.erase(iter);
}

Factory*
ManagerImp::find (std::string const& name)
{
    std::lock_guard<std::mutex> _(mutex_);
    auto const iter = std::find_if(list_.begin(), list_.end(),
        [&name](Factory* other)
        {
            return boost::beast::detail::iequals(name, other->getName());
        } );
    if (iter == list_.end())
        return nullptr;
    return *iter;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Manager&
Manager::instance()
{
    return ManagerImp::instance();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::unique_ptr <Backend>
make_Backend (Section const& config,
    Scheduler& scheduler, beast::Journal journal)
{
    return Manager::instance().make_Backend (
        config, scheduler, journal);
}

}
}
