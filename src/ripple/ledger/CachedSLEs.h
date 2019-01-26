
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

#ifndef RIPPLE_LEDGER_CACHEDSLES_H_INCLUDED
#define RIPPLE_LEDGER_CACHEDSLES_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/beast/container/aged_unordered_map.h>
#include <memory>
#include <mutex>

namespace ripple {

/*通过消化来储存SLE。*/
class CachedSLEs
{
public:
    using digest_type = uint256;

    using value_type =
        std::shared_ptr<SLE const>;

    CachedSLEs (CachedSLEs const&) = delete;
    CachedSLEs& operator= (CachedSLEs const&) = delete;

    template <class Rep, class Period>
    CachedSLEs (std::chrono::duration<
        Rep, Period> const& timeToLive,
            Stopwatch& clock)
        : timeToLive_ (timeToLive)
        , map_ (clock)
    {
    }

    /*放弃过期的条目。

        需要定期打电话。
    **/

    void
    expire();

    /*从缓存中提取项目。

        如果找不到摘要，则处理程序
        将使用此签名调用：

            std:：shared-ptr<sle const>（无效）
    **/

    template <class Handler>
    value_type
    fetch (digest_type const& digest,
        Handler const& h)
    {
        {
            std::lock_guard<
                std::mutex> lock(mutex_);
            auto iter =
                map_.find(digest);
            if (iter != map_.end())
            {
                ++hit_;
                map_.touch(iter);
                return iter->second;
            }
        }
        auto sle = h();
        if (! sle)
            return nullptr;
        std::lock_guard<
            std::mutex> lock(mutex_);
        ++miss_;
        auto const result =
            map_.emplace(
                digest, std::move(sle));
        if (! result.second)
            map_.touch(result.first);
        return  result.first->second;
    }

    /*返回缓存命中的分数。*/
    double
    rate() const;

private:
    std::size_t hit_ = 0;
    std::size_t miss_ = 0;
    std::mutex mutable mutex_;
    Stopwatch::duration timeToLive_;
    beast::aged_unordered_map <digest_type,
        value_type, Stopwatch::clock_type,
            hardened_hash<strong_hash>> map_;
};

} //涟漪

#endif
