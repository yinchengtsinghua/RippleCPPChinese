
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

#ifndef RIPPLE_NODESTORE_DATABASE_H_INCLUDED
#define RIPPLE_NODESTORE_DATABASE_H_INCLUDED

#include <ripple/basics/TaggedCache.h>
#include <ripple/basics/KeyCache.h>
#include <ripple/core/Stoppable.h>
#include <ripple/nodestore/Backend.h>
#include <ripple/nodestore/impl/Tuning.h>
#include <ripple/nodestore/Scheduler.h>
#include <ripple/nodestore/NodeObject.h>
#include <ripple/protocol/SystemParameters.h>

#include <thread>

namespace ripple {

class Ledger;

namespace NodeStore {

/*nodeObject的持久层

    节点是由键唯一标识的分类账对象，它是
    节点主体的256位散列。有效载荷是可变长度
    序列化数据块。

    所有分类帐数据都存储为节点对象，因此需要持久化。
    在发射之间。此外，由于节点对象集将
    常规大于可用内存量，清除的节点对象
    稍后访问的内容必须从节点存储中检索。

    参见NoDeObjor
**/

class Database : public Stoppable
{
public:
    Database() = delete;

    /*构造节点存储。

        @param name这个数据库的可停止名称。
        @param parent父级父级可停止。
        @param scheduler用于执行异步任务的调度程序。
        @param read threads要创建的异步读取线程数。
        用于日志记录输出的@param journal目的地。
    **/

    Database(std::string name, Stoppable& parent, Scheduler& scheduler,
        int readThreads, Section const& config, beast::Journal j);

    /*销毁节点存储。
        所有挂起的操作都已完成，挂起的写入被刷新，
        文件在返回之前关闭。
    **/

    virtual
    ~Database();

    /*检索与此后端关联的名称。
        这用于诊断，可能不反映实际路径
        或底层后端使用的路径。
    **/

    virtual
    std::string
    getName() const = 0;

    /*从其他数据库导入对象。*/
    virtual
    void
    import(Database& source) = 0;

    /*检索估计的挂起写入操作数。
        用于诊断。
    **/

    virtual
    std::int32_t
    getWriteLoad() const = 0;

    /*存储对象。

        调用方的blob参数被覆盖。

        @param键入对象类型。
        @param data对象的有效负载。呼叫者
                    变量被覆盖。
        @param hash有效载荷数据的256位散列。
        @param seq对象所属分类帐的序列。

        @return`true`是否存储了该对象？
    **/

    virtual
    void
    store(NodeObjectType type, Blob&& data,
        uint256 const& hash, std::uint32_t seq) = 0;

    /*获取对象。
        如果已知对象不在数据库中，则在
        数据库在提取过程中，或在提取过程中未能正确加载，
        返回“nullptr”。

        @注意这可以同时调用。
        @param hash要检索的对象的键。
        @param seq存储对象的分类帐的序列。
        @返回对象，如果无法检索，则返回nullptr。
    **/

    virtual
    std::shared_ptr<NodeObject>
    fetch(uint256 const& hash, std::uint32_t seq) = 0;

    /*不等待地获取对象。
        如果需要I/O来确定对象是否存在，
        返回“false”。否则，将返回“true”并设置“object”
        引用对象，如果对象不存在，则使用“nullptr”。
        如果需要I/O，则计划I/O。

        @注意这可以同时调用。
        @param hash要检索的对象的键
        @param seq存储对象的分类帐的序列。
        @param object检索到的对象
        @返回操作是否完成
    **/

    virtual
    bool
    asyncFetch(uint256 const& hash, std::uint32_t seq,
        std::shared_ptr<NodeObject>& object) = 0;

    /*将存储在其他数据库中的分类帐复制到此分类帐。

        @param ledger要复制的分类帐。
        @如果操作成功返回true
    **/

    virtual
    bool
    copyLedger(std::shared_ptr<Ledger const> const& ledger) = 0;

    /*等待所有当前挂起的异步读取完成。
    **/

    void
    waitReads();

    /*获取节点存储所需的最大异步读取数。

        @param seq指定要查询的碎片的分类帐序列。
        @返回首选的异步读取次数。
        @注意，此序列仅用于碎片存储。
    **/

    virtual
    int
    getDesiredAsyncReadCount(std::uint32_t seq) = 0;

    /*获取缓存命中与总尝试次数之比。*/
    virtual
    float
    getCacheHitRate() = 0;

    /*设置两个缓存的最大条目数和最大缓存期限。

        @param size缓存条目数（0=忽略）
        @param age缓存最长使用时间（秒）
    **/

    virtual
    void
    tune(int size, std::chrono::seconds age) = 0;

    /*从正缓存和负缓存中删除过期项。*/
    virtual
    void
    sweep() = 0;

    /*收集与读写活动有关的统计信息。

        @返回读写字节总数。
     **/

    std::uint32_t
    getStoreCount() const { return storeCount_; }

    std::uint32_t
    getFetchTotalCount() const { return fetchTotalCount_; }

    std::uint32_t
    getFetchHitCount() const { return fetchHitCount_; }

    std::uint32_t
    getStoreSize() const { return storeSz_; }

    std::uint32_t
    getFetchSize() const { return fetchSz_; }

    /*返回后端所需的文件数*/
    int
    fdlimit() const { return fdLimit_; }

    void
    onStop() override;

    /*@返回允许的最早分类帐序列
    **/

    std::uint32_t
    earliestSeq() const
    {
        return earliestSeq_;
    }

protected:
    beast::Journal j_;
    Scheduler& scheduler_;
    int fdLimit_ {0};

    void
    stopThreads();

    void
    storeStats(size_t sz)
    {
        ++storeCount_;
        storeSz_ += sz;
    }

    void
    asyncFetch(uint256 const& hash, std::uint32_t seq,
        std::shared_ptr<TaggedCache<uint256, NodeObject>> const& pCache,
            std::shared_ptr<KeyCache<uint256>> const& nCache);

    std::shared_ptr<NodeObject>
    fetchInternal(uint256 const& hash, Backend& srcBackend);

    void
    importInternal(Backend& dstBackend, Database& srcDB);

    std::shared_ptr<NodeObject>
    doFetch(uint256 const& hash, std::uint32_t seq,
        TaggedCache<uint256, NodeObject>& pCache,
            KeyCache<uint256>& nCache, bool isAsync);

    bool
    copyLedger(Backend& dstBackend, Ledger const& srcLedger,
        std::shared_ptr<TaggedCache<uint256, NodeObject>> const& pCache,
            std::shared_ptr<KeyCache<uint256>> const& nCache,
                std::shared_ptr<Ledger const> const& srcNext);

private:
    std::atomic<std::uint32_t> storeCount_ {0};
    std::atomic<std::uint32_t> fetchTotalCount_ {0};
    std::atomic<std::uint32_t> fetchHitCount_ {0};
    std::atomic<std::uint32_t> storeSz_ {0};
    std::atomic<std::uint32_t> fetchSz_ {0};

    std::mutex readLock_;
    std::condition_variable readCondVar_;
    std::condition_variable readGenCondVar_;

//读做
    std::map<uint256, std::tuple<std::uint32_t,
        std::weak_ptr<TaggedCache<uint256, NodeObject>>,
            std::weak_ptr<KeyCache<uint256>>>> read_;

//最后阅读
    uint256 readLastHash_;

    std::vector<std::thread> readThreads_;
    bool readShut_ {false};

//当前读取生成
    uint64_t readGen_ {0};

//默认值为32570，以匹配最早的xrp分类帐网络
//允许的序列。备用网络可以设置此值。
    std::uint32_t earliestSeq_ {XRP_LEDGER_EARLIEST_SEQ};

    virtual
    std::shared_ptr<NodeObject>
    fetchFrom(uint256 const& hash, std::uint32_t seq) = 0;

    /*访问数据库中的每个对象
        这通常在导入期间调用。

        @注意，此例程不会与自身同时调用
                或其他方法。
        参见进口
    **/

    virtual
    void
    for_each(std::function <void(std::shared_ptr<NodeObject>)> f) = 0;

    void
    threadEntry();
};

}
}

#endif
