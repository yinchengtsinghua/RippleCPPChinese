
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

#include <ripple/app/main/Application.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/core/Config.h>
#include <ripple/core/TimeKeeper.h>
#include <ripple/overlay/Cluster.h>
#include <ripple/overlay/ClusterNode.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/tokens.h>
#include <boost/regex.hpp>
#include <memory.h>

namespace ripple {

Cluster::Cluster (beast::Journal j)
    : j_ (j)
{
}

boost::optional<std::string>
Cluster::member (PublicKey const& identity) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto iter = nodes_.find (identity);
    if (iter == nodes_.end ())
        return boost::none;
    return iter->name ();
}

std::size_t
Cluster::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    return nodes_.size();
}

bool
Cluster::update (
    PublicKey const& identity,
    std::string name,
    std::uint32_t loadFee,
    NetClock::time_point reportTime)
{
    std::lock_guard<std::mutex> lock(mutex_);

//由于libstdc++问题，我们还不能在这里使用auto
//在https://gcc.gnu.org/bugzilla/show_bug.cgi中描述？ID＝68190
    std::set<ClusterNode, Comparator>::iterator iter =
        nodes_.find (identity);

    if (iter != nodes_.end ())
    {
        if (reportTime <= iter->getReportTime())
            return false;

        if (name.empty())
            name = iter->name();

        iter = nodes_.erase (iter);
    }

    nodes_.emplace_hint (iter, identity, name, loadFee, reportTime);
    return true;
}

void
Cluster::for_each (
    std::function<void(ClusterNode const&)> func) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto const& ni : nodes_)
        func (ni);
}

bool
Cluster::load (Section const& nodes)
{
    static boost::regex const re (
"[[:space:]]*"            //跳过前导空格
"([[:alnum:]]+)"          //节点恒等式
"(?:"                     //开始可选注释块
"[[:space:]]+"            //（跳过所有前导空格）
"(?:"                     //开始可选注释
"(.*[^[:space:]]+)"       //评论
"[[:space:]]*"            //（跳过所有尾随空格）
")?"                      //结束可选注释
")?"                      //结束可选注释块
    );

    for (auto const& n : nodes.values())
    {
        boost::smatch match;

        if (!boost::regex_match (n, match, re))
        {
            JLOG (j_.error()) <<
                "Malformed entry: '" << n << "'";
            return false;
        }

        auto const id = parseBase58<PublicKey>(
            TokenType::NodePublic, match[1]);

        if (!id)
        {
            JLOG (j_.error()) <<
                "Invalid node identity: " << match[1];
            return false;
        }

        if (member (*id))
        {
            JLOG (j_.warn()) <<
                "Duplicate node identity: " << match[1];
            continue;
        }

        update(*id, trim_whitespace(match[2]));
    }

    return true;
}

} //涟漪
