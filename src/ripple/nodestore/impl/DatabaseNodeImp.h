
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

#ifndef RIPPLE_NODESTORE_DATABASENODEIMP_H_INCLUDED
#define RIPPLE_NODESTORE_DATABASENODEIMP_H_INCLUDED

#include <ripple/nodestore/Database.h>
#include <ripple/basics/chrono.h>

namespace ripple {
namespace NodeStore {

class DatabaseNodeImp : public Database
{
public:
    DatabaseNodeImp() = delete;
    DatabaseNodeImp(DatabaseNodeImp const&) = delete;
    DatabaseNodeImp& operator=(DatabaseNodeImp const&) = delete;

    DatabaseNodeImp(
        std::string const& name,
        Scheduler& scheduler,
        int readThreads,
        Stoppable& parent,
        std::unique_ptr<Backend> backend,
        Section const& config,
        beast::Journal j)
        : Database(name, parent, scheduler, readThreads, config, j)
        , pCache_(std::make_shared<TaggedCache<uint256, NodeObject>>(
            name, cacheTargetSize, cacheTargetAge, stopwatch(), j))
        , nCache_(std::make_shared<KeyCache<uint256>>(
            name, stopwatch(), cacheTargetSize, cacheTargetAge))
        , backend_(std::move(backend))
    {
        assert(backend_);
    }

    ~DatabaseNodeImp() override
    {
//在销毁数据成员之前停止线程。
        stopThreads();
    }

    std::string
    getName() const override
    {
        return backend_->getName();
    }

    std::int32_t
    getWriteLoad() const override
    {
        return backend_->getWriteLoad();
    }

    void
    import(Database& source) override
    {
        importInternal(*backend_.get(), source);
    }

    void
    store(NodeObjectType type, Blob&& data,
        uint256 const& hash, std::uint32_t seq) override;

    std::shared_ptr<NodeObject>
    fetch(uint256 const& hash, std::uint32_t seq) override
    {
        return doFetch(hash, seq, *pCache_, *nCache_, false);
    }

    bool
    asyncFetch(uint256 const& hash, std::uint32_t seq,
        std::shared_ptr<NodeObject>& object) override;

    bool
    copyLedger(std::shared_ptr<Ledger const> const& ledger) override
    {
        return Database::copyLedger(
            *backend_, *ledger, pCache_, nCache_, nullptr);
    }

    int
    getDesiredAsyncReadCount(std::uint32_t seq) override
    {
//我们更喜欢客户机而不是填满我们的缓存
//我们不想将数据推出缓存
//在它被取回之前
        return pCache_->getTargetSize() / asyncDivider;
    }

    float
    getCacheHitRate() override {return pCache_->getHitRate();}

    void
    tune(int size, std::chrono::seconds age) override;

    void
    sweep() override;

private:
//正缓存
    std::shared_ptr<TaggedCache<uint256, NodeObject>> pCache_;

//负高速缓存
    std::shared_ptr<KeyCache<uint256>> nCache_;

//持久密钥/值存储
    std::unique_ptr<Backend> backend_;

    std::shared_ptr<NodeObject>
    fetchFrom(uint256 const& hash, std::uint32_t seq) override
    {
        return fetchInternal(hash, *backend_);
    }

    void
    for_each(std::function<void(std::shared_ptr<NodeObject>)> f) override
    {
        backend_->for_each(f);
    }
};

}
}

#endif
