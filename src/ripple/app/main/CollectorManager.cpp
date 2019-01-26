
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

#include <ripple/app/main/CollectorManager.h>
#include <memory>

namespace ripple {

class CollectorManagerImp
    : public CollectorManager
{
public:
    beast::Journal m_journal;
    beast::insight::Collector::ptr m_collector;
    std::unique_ptr <beast::insight::Groups> m_groups;

    CollectorManagerImp (Section const& params,
        beast::Journal journal)
        : m_journal (journal)
    {
        std::string const& server  = get<std::string> (params, "server");

        if (server == "statsd")
        {
            beast::IP::Endpoint const address (beast::IP::Endpoint::from_string (
                get<std::string> (params, "address")));
            std::string const& prefix (get<std::string> (params, "prefix"));

            m_collector = beast::insight::StatsDCollector::New (address, prefix, journal);
        }
        else
        {
            m_collector = beast::insight::NullCollector::New ();
        }

        m_groups = beast::insight::make_Groups (m_collector);
    }

    ~CollectorManagerImp() = default;

    beast::insight::Collector::ptr const& collector () override
    {
        return m_collector;
    }

    beast::insight::Group::ptr const& group (std::string const& name) override
    {
        return m_groups->get (name);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

CollectorManager::~CollectorManager() = default;

std::unique_ptr<CollectorManager> CollectorManager::New(Section const& params,
    beast::Journal journal)
{
    return std::make_unique<CollectorManagerImp>(params, journal);
}

}
