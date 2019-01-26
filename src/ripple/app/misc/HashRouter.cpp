
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

#include <ripple/app/misc/HashRouter.h>

namespace ripple {

auto
HashRouter::emplace (uint256 const& key)
    -> std::pair<Entry&, bool>
{
    auto iter = suppressionMap_.find (key);

    if (iter != suppressionMap_.end ())
    {
        suppressionMap_.touch(iter);
        return std::make_pair(
            std::ref(iter->second), false);
    }

//看看是否有任何抑制需要过期
    expire(suppressionMap_, holdTime_);

    return std::make_pair(std::ref(
        suppressionMap_.emplace (
            key, Entry ()).first->second),
                true);
}

void HashRouter::addSuppression (uint256 const& key)
{
    std::lock_guard <std::mutex> lock (mutex_);

    emplace (key);
}

bool HashRouter::addSuppressionPeer (uint256 const& key, PeerShortID peer)
{
    std::lock_guard <std::mutex> lock (mutex_);

    auto result = emplace(key);
    result.first.addPeer(peer);
    return result.second;
}

bool HashRouter::addSuppressionPeer (uint256 const& key, PeerShortID peer, int& flags)
{
    std::lock_guard <std::mutex> lock (mutex_);

    auto result = emplace(key);
    auto& s = result.first;
    s.addPeer (peer);
    flags = s.getFlags ();
    return result.second;
}

bool HashRouter::shouldProcess (uint256 const& key, PeerShortID peer,
    int& flags, std::chrono::seconds tx_interval)
{
    std::lock_guard <std::mutex> lock (mutex_);

    auto result = emplace(key);
    auto& s = result.first;
    s.addPeer (peer);
    flags = s.getFlags ();
    return s.shouldProcess (suppressionMap_.clock().now(), tx_interval);
}

int HashRouter::getFlags (uint256 const& key)
{
    std::lock_guard <std::mutex> lock (mutex_);

    return emplace(key).first.getFlags ();
}

bool HashRouter::setFlags (uint256 const& key, int flags)
{
    assert (flags != 0);

    std::lock_guard <std::mutex> lock (mutex_);

    auto& s = emplace(key).first;

    if ((s.getFlags () & flags) == flags)
        return false;

    s.setFlags (flags);
    return true;
}

auto
HashRouter::shouldRelay (uint256 const& key)
    -> boost::optional<std::set<PeerShortID>>
{
    std::lock_guard <std::mutex> lock (mutex_);

    auto& s = emplace(key).first;

    if (!s.shouldRelay(suppressionMap_.clock().now(), holdTime_))
        return boost::none;

    return s.releasePeerSet();
}

bool
HashRouter::shouldRecover(uint256 const& key)
{
    std::lock_guard <std::mutex> lock(mutex_);

    auto& s = emplace(key).first;

    return s.shouldRecover(recoverLimit_);
}

} //涟漪
