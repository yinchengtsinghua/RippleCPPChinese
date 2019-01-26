
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
    版权所有（c）2012，2017 Ripple Labs Inc.

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

#ifndef RIPPLE_NODESTORE_DATABASESHARDIMP_H_INCLUDED
#define RIPPLE_NODESTORE_DATABASESHARDIMP_H_INCLUDED

#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/nodestore/impl/Shard.h>

namespace ripple {
namespace NodeStore {

class DatabaseShardImp : public DatabaseShard
{
public:
    DatabaseShardImp() = delete;
    DatabaseShardImp(DatabaseShardImp const&) = delete;
    DatabaseShardImp& operator=(DatabaseShardImp const&) = delete;

    DatabaseShardImp(Application& app, std::string const& name,
        Stoppable& parent, Scheduler& scheduler, int readThreads,
            Section const& config, beast::Journal j);

    ~DatabaseShardImp() override;

    bool
    init() override;

    boost::optional<std::uint32_t>
    prepareLedger(std::uint32_t validLedgerSeq) override;

    bool
    prepareShard(std::uint32_t shardIndex) override;

    void
    removePreShard(std::uint32_t shardIndex) override;

    std::uint32_t
    getNumPreShard() override;

    bool
    importShard(std::uint32_t shardIndex,
        boost::filesystem::path const& srcDir, bool validate) override;

    std::shared_ptr<Ledger>
    fetchLedger(uint256 const& hash, std::uint32_t seq) override;

    void
    setStored(std::shared_ptr<Ledger const> const& ledger) override;

    bool
    contains(std::uint32_t seq) override;

    std::string
    getCompleteShards() override;

    void
    validate() override;

    std::uint32_t
    ledgersPerShard() const override
    {
        return ledgersPerShard_;
    }

    std::uint32_t
    earliestShardIndex() const override
    {
        return earliestShardIndex_;
    }

    std::uint32_t
    seqToShardIndex(std::uint32_t seq) const override
    {
        assert(seq >= earliestSeq());
        return NodeStore::seqToShardIndex(seq, ledgersPerShard_);
    }

    std::uint32_t
    firstLedgerSeq(std::uint32_t shardIndex) const override
    {
        assert(shardIndex >= earliestShardIndex_);
        if (shardIndex <= earliestShardIndex_)
            return earliestSeq();
        return 1 + (shardIndex * ledgersPerShard_);
    }

    std::uint32_t
    lastLedgerSeq(std::uint32_t shardIndex) const override
    {
        assert(shardIndex >= earliestShardIndex_);
        return (shardIndex + 1) * ledgersPerShard_;
    }

    boost::filesystem::path const&
    getRootDir() const override
    {
        return dir_;
    }

    std::string
    getName() const override
    {
        return backendName_;
    }

    /*导入应用程序本地节点存储

        @param source应用程序节点存储。
    **/

    void
    import(Database& source) override;

    std::int32_t
    getWriteLoad() const override;

    void
    store(NodeObjectType type, Blob&& data,
        uint256 const& hash, std::uint32_t seq) override;

    std::shared_ptr<NodeObject>
    fetch(uint256 const& hash, std::uint32_t seq) override;

    bool
    asyncFetch(uint256 const& hash, std::uint32_t seq,
        std::shared_ptr<NodeObject>& object) override;

    bool
    copyLedger(std::shared_ptr<Ledger const> const& ledger) override;

    int
    getDesiredAsyncReadCount(std::uint32_t seq) override;

    float
    getCacheHitRate() override;

    void
    tune(int size, std::chrono::seconds age) override;

    void
    sweep() override;

private:
    Application& app_;
    mutable std::mutex m_;
    bool init_ {false};

//完整碎片
    std::map<std::uint32_t, std::unique_ptr<Shard>> complete_;

//从对等网络获取的碎片
    std::unique_ptr<Shard> incomplete_;

//准备进口的碎片
    std::map<std::uint32_t, Shard*> preShards_;

    Section const config_;
    boost::filesystem::path const dir_;

//如果可以存储新碎片
    bool canAdd_ {true};

//完整的碎片索引
    std::string status_;

//如果后端类型使用永久存储
    bool backed_;

//与用于碎片存储的后端关联的名称
    std::string const backendName_;

//数据库可以使用的最大磁盘空间（字节）
    std::uint64_t const maxDiskSpace_;

//用于存储碎片的磁盘空间（字节）
    std::uint64_t usedDiskSpace_ {0};

//每个碎片存储16384个分类帐。最早的碎片可能会存储
//如果最早的分类帐序列截断其开始，则减去。
//只应为单元测试更改该值。
    std::uint32_t const ledgersPerShard_;

//最早的碎片指数
    std::uint32_t const earliestShardIndex_;

//碎片所需的平均磁盘空间（字节）
    std::uint64_t avgShardSz_;

//碎片缓存调整
    int cacheSz_ {shardCacheSz};
    std::chrono::seconds cacheAge_ {shardCacheAge};

//用于标记从节点存储区导入的碎片的文件名
    static constexpr auto importMarker_ = "import";

    std::shared_ptr<NodeObject>
    fetchFrom(uint256 const& hash, std::uint32_t seq) override;

    void
    for_each(std::function <void(std::shared_ptr<NodeObject>)> f) override
    {
        Throw<std::runtime_error>("Shard store import not supported");
    }

//查找未存储的随机碎片索引
//必须保持锁定
    boost::optional<std::uint32_t>
    findShardIndexToAdd(std::uint32_t validLedgerSeq,
        std::lock_guard<std::mutex>&);

//更新统计数据
//必须保持锁定
    void
    updateStats(std::lock_guard<std::mutex>&);

    std::pair<std::shared_ptr<PCache>, std::shared_ptr<NCache>>
    selectCache(std::uint32_t seq);

//返回调整缓存大小除以碎片数
//必须保持锁定
    int
    calcTargetCacheSz(std::lock_guard<std::mutex>&) const
    {
        return std::max(shardCacheSz, cacheSz_ / std::max(
            1, static_cast<int>(complete_.size() + (incomplete_ ? 1 : 0))));
    }

//返回可用存储空间
    std::uint64_t
    available() const;
};

} //节点存储
} //涟漪

#endif
