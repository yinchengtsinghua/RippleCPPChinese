
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

#ifndef RIPPLE_NODESTORE_DATABASEROTATINGIMP_H_INCLUDED
#define RIPPLE_NODESTORE_DATABASEROTATINGIMP_H_INCLUDED

#include <ripple/nodestore/DatabaseRotating.h>

namespace ripple {
namespace NodeStore {

class DatabaseRotatingImp : public DatabaseRotating
{
public:
    DatabaseRotatingImp() = delete;
    DatabaseRotatingImp(DatabaseRotatingImp const&) = delete;
    DatabaseRotatingImp& operator=(DatabaseRotatingImp const&) = delete;

    DatabaseRotatingImp(
        std::string const& name,
        Scheduler& scheduler,
        int readThreads,
        Stoppable& parent,
        std::unique_ptr<Backend> writableBackend,
        std::unique_ptr<Backend> archiveBackend,
        Section const& config,
        beast::Journal j);

    ~DatabaseRotatingImp() override
    {
//在销毁数据成员之前停止线程。
        stopThreads();
    }

    std::unique_ptr<Backend> const&
    getWritableBackend() const override
    {
        std::lock_guard <std::mutex> lock (rotateMutex_);
        return writableBackend_;
    }

    std::unique_ptr<Backend>
    rotateBackends(std::unique_ptr<Backend> newBackend) override;

    std::mutex& peekMutex() const override
    {
        return rotateMutex_;
    }

    std::string getName() const override
    {
        return getWritableBackend()->getName();
    }

    std::int32_t getWriteLoad() const override
    {
        return getWritableBackend()->getWriteLoad();
    }

    void import (Database& source) override
    {
        importInternal (*getWritableBackend(), source);
    }

    void store(NodeObjectType type, Blob&& data,
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
            *getWritableBackend(), *ledger, pCache_, nCache_, nullptr);
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

    TaggedCache<uint256, NodeObject> const&
    getPositiveCache() override {return *pCache_;}

private:
//正缓存
    std::shared_ptr<TaggedCache<uint256, NodeObject>> pCache_;

//负高速缓存
    std::shared_ptr<KeyCache<uint256>> nCache_;

    std::unique_ptr<Backend> writableBackend_;
    std::unique_ptr<Backend> archiveBackend_;
    mutable std::mutex rotateMutex_;

    struct Backends {
        std::unique_ptr<Backend> const& writableBackend;
        std::unique_ptr<Backend> const& archiveBackend;
    };

    Backends getBackends() const
    {
        std::lock_guard <std::mutex> lock (rotateMutex_);
        return Backends {writableBackend_, archiveBackend_};
    }

    std::shared_ptr<NodeObject> fetchFrom(
        uint256 const& hash, std::uint32_t seq) override;

    void
    for_each(std::function <void(std::shared_ptr<NodeObject>)> f) override
    {
        Backends b = getBackends();
        b.archiveBackend->for_each(f);
        b.writableBackend->for_each(f);
    }
};

}
}

#endif
