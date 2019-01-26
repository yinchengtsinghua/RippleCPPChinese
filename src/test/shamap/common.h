
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

#ifndef RIPPLE_SHAMAP_TESTS_COMMON_H_INCLUDED
#define RIPPLE_SHAMAP_TESTS_COMMON_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/nodestore/DummyScheduler.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/shamap/Family.h>

namespace ripple {
namespace tests {

class TestFamily : public Family
{
private:
    TestStopwatch clock_;
    NodeStore::DummyScheduler scheduler_;
    TreeNodeCache treecache_;
    FullBelowCache fullbelow_;
    RootStoppable parent_;
    std::unique_ptr<NodeStore::Database> db_;
    bool shardBacked_;
    beast::Journal j_;

public:
    TestFamily (beast::Journal j)
        : treecache_ ("TreeNodeCache", 65536, std::chrono::minutes{1},
                      clock_, j)
        , fullbelow_ ("full_below", clock_)
        , parent_ ("TestRootStoppable")
        , j_ (j)
    {
        Section testSection;
        testSection.set("type", "memory");
        testSection.set("Path", "SHAMap_test");
        db_ = NodeStore::Manager::instance ().make_Database (
            "test", scheduler_, 1, parent_, testSection, j);
        shardBacked_ =
            dynamic_cast<NodeStore::DatabaseShard*>(db_.get()) != nullptr;
    }

    beast::manual_clock <std::chrono::steady_clock>
    clock()
    {
        return clock_;
    }

    beast::Journal const&
    journal() override
    {
        return j_;
    }

    FullBelowCache&
    fullbelow() override
    {
        return fullbelow_;
    }

    FullBelowCache const&
    fullbelow() const override
    {
        return fullbelow_;
    }

    TreeNodeCache&
    treecache() override
    {
        return treecache_;
    }

    TreeNodeCache const&
    treecache() const override
    {
        return treecache_;
    }

    NodeStore::Database&
    db() override
    {
        return *db_;
    }

    NodeStore::Database const&
    db() const override
    {
        return *db_;
    }

    bool
    isShardBacked() const override
    {
        return shardBacked_;
    }

    void
    missing_node (std::uint32_t refNum) override
    {
        Throw<std::runtime_error> ("missing node");
    }

    void
    missing_node (uint256 const& refHash, std::uint32_t refNum) override
    {
        Throw<std::runtime_error> ("missing node");
    }

    void
    reset() override
    {
        fullbelow_.reset();
        treecache_.reset();
    }
};

} //测验
} //涟漪

#endif
