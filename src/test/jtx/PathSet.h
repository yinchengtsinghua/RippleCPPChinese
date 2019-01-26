
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
  版权所有（c）2012-2015 Ripple Labs Inc.

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

#ifndef RIPPLE_LEDGER_TESTS_PATHSET_H_INCLUDED
#define RIPPLE_LEDGER_TESTS_PATHSET_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/protocol/TxFlags.h>
#include <test/jtx.h>

namespace ripple {
namespace test {

/*要约存在
 **/

inline
bool
isOffer (jtx::Env& env,
    jtx::Account const& account,
    STAmount const& takerPays,
    STAmount const& takerGets)
{
    bool exists = false;
    forEachItem (*env.current(), account,
        [&](std::shared_ptr<SLE const> const& sle)
        {
            if (sle->getType () == ltOFFER &&
                sle->getFieldAmount (sfTakerPays) == takerPays &&
                    sle->getFieldAmount (sfTakerGets) == takerGets)
                exists = true;
        });
    return exists;
}

class Path
{
public:
    STPath path;

    Path () = default;
    Path (Path const&) = default;
    Path& operator=(Path const&) = default;
    Path (Path&&) = default;
    Path& operator=(Path&&) = default;

    template <class First, class... Rest>
    explicit Path (First&& first, Rest&&... rest)
    {
        addHelper (std::forward<First> (first), std::forward<Rest> (rest)...);
    }
    Path& push_back (Issue const& iss);
    Path& push_back (jtx::Account const& acc);
    Path& push_back (STPathElement const& pe);
    Json::Value json () const;
 private:
    Path& addHelper (){return *this;}
    template <class First, class... Rest>
    Path& addHelper (First&& first, Rest&&... rest);
};

inline Path& Path::push_back (STPathElement const& pe)
{
    path.emplace_back (pe);
    return *this;
}

inline Path& Path::push_back (Issue const& iss)
{
    path.emplace_back (STPathElement::typeCurrency | STPathElement::typeIssuer,
        beast::zero, iss.currency, iss.account);
    return *this;
}

inline
Path& Path::push_back (jtx::Account const& account)
{
    path.emplace_back (account.id (), beast::zero, beast::zero);
    return *this;
}

template <class First, class... Rest>
Path& Path::addHelper (First&& first, Rest&&... rest)
{
    push_back (std::forward<First> (first));
    return addHelper (std::forward<Rest> (rest)...);
}

inline
Json::Value Path::json () const
{
    return path.getJson (0);
}

class PathSet
{
public:
    STPathSet paths;

    PathSet () = default;
    PathSet (PathSet const&) = default;
    PathSet& operator=(PathSet const&) = default;
    PathSet (PathSet&&) = default;
    PathSet& operator=(PathSet&&) = default;

    template <class First, class... Rest>
    explicit PathSet (First&& first, Rest&&... rest)
    {
        addHelper (std::forward<First> (first), std::forward<Rest> (rest)...);
    }
    Json::Value json () const
    {
        Json::Value v;
        v["Paths"] = paths.getJson (0);
        return v;
    }
private:
    PathSet& addHelper ()
    {
        return *this;
    }
    template <class First, class... Rest>
    PathSet& addHelper (First first, Rest... rest)
    {
        paths.emplace_back (std::move (first.path));
        return addHelper (std::move (rest)...);
    }
};

} //测试
} //涟漪

#endif
