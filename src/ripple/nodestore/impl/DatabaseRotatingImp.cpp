
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

#include <ripple/nodestore/impl/DatabaseRotatingImp.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/protocol/HashPrefix.h>

namespace ripple {
namespace NodeStore {

DatabaseRotatingImp::DatabaseRotatingImp(
    std::string const& name,
    Scheduler& scheduler,
    int readThreads,
    Stoppable& parent,
    std::unique_ptr<Backend> writableBackend,
    std::unique_ptr<Backend> archiveBackend,
    Section const& config,
    beast::Journal j)
    : DatabaseRotating(name, parent, scheduler, readThreads, config, j)
    , pCache_(std::make_shared<TaggedCache<uint256, NodeObject>>(
        name, cacheTargetSize, cacheTargetAge, stopwatch(), j))
    , nCache_(std::make_shared<KeyCache<uint256>>(
        name, stopwatch(), cacheTargetSize, cacheTargetAge))
    , writableBackend_(std::move(writableBackend))
    , archiveBackend_(std::move(archiveBackend))
{
    if (writableBackend_)
        fdLimit_ += writableBackend_->fdlimit();
    if (archiveBackend_)
        fdLimit_ += archiveBackend_->fdlimit();
}

//一定要叫它已经锁定！
std::unique_ptr<Backend>
DatabaseRotatingImp::rotateBackends(
    std::unique_ptr<Backend> newBackend)
{
    auto oldBackend {std::move(archiveBackend_)};
    archiveBackend_ = std::move(writableBackend_);
    writableBackend_ = std::move(newBackend);
    return oldBackend;
}

void
DatabaseRotatingImp::store(NodeObjectType type, Blob&& data,
    uint256 const& hash, std::uint32_t seq)
{
#if RIPPLE_VERIFY_NODEOBJECT_KEYS
    assert(hash == sha512Hash(makeSlice(data)));
#endif
    auto nObj = NodeObject::createObject(type, std::move(data), hash);
    pCache_->canonicalize(hash, nObj, true);
    getWritableBackend()->store(nObj);
    nCache_->erase(hash);
    storeStats(nObj->getData().size());
}

bool
DatabaseRotatingImp::asyncFetch(uint256 const& hash,
    std::uint32_t seq, std::shared_ptr<NodeObject>& object)
{
//查看对象是否在缓存中
    object = pCache_->fetch(hash);
    if (object || nCache_->touch_if_exists(hash))
        return true;
//否则发布阅读
    Database::asyncFetch(hash, seq, pCache_, nCache_);
    return false;
}

void
DatabaseRotatingImp::tune(int size, std::chrono::seconds age)
{
    pCache_->setTargetSize(size);
    pCache_->setTargetAge(age);
    nCache_->setTargetSize(size);
    nCache_->setTargetAge(age);
}

void
DatabaseRotatingImp::sweep()
{
    pCache_->sweep();
    nCache_->sweep();
}

std::shared_ptr<NodeObject>
DatabaseRotatingImp::fetchFrom(uint256 const& hash, std::uint32_t seq)
{
    Backends b = getBackends();
    auto nObj = fetchInternal(hash, *b.writableBackend);
    if (! nObj)
    {
        nObj = fetchInternal(hash, *b.archiveBackend);
        if (nObj)
        {
            getWritableBackend()->store(nObj);
            nCache_->erase(hash);
        }
    }
    return nObj;
}

} //节点存储
} //涟漪
