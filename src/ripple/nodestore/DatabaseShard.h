
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

#ifndef RIPPLE_NODESTORE_DATABASESHARD_H_INCLUDED
#define RIPPLE_NODESTORE_DATABASESHARD_H_INCLUDED

#include <ripple/nodestore/Database.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/RangeSet.h>
#include <ripple/nodestore/Types.h>

#include <boost/optional.hpp>

#include <memory>

namespace ripple {
namespace NodeStore {

/*历史碎片的收集
**/

class DatabaseShard : public Database
{
public:
    /*建设一个碎片商店

        @param name这个数据库的可停止名称
        @param parent父级父级可停止
        @param scheduler用于执行异步任务的调度程序
        @param read threads要创建的异步读取线程数
        @param配置数据库的配置
        用于日志记录输出的@param journal目的地
    **/

    DatabaseShard(
        std::string const& name,
        Stoppable& parent,
        Scheduler& scheduler,
        int readThreads,
        Section const& config,
        beast::Journal journal)
        : Database(name, parent, scheduler, readThreads, config, journal)
    {
    }

    /*初始化数据库

        @return`true`如果数据库初始化没有错误
    **/

    virtual
    bool
    init() = 0;

    /*准备在正在获取的碎片中存储新分类帐

        @param validledgerseq最大有效分类账的索引
        @返回如果应提取和存储分类帐，则返回分类帐
                要请求的分类帐的索引。否则返回boost：：none。
                这可能返回boost的一些原因：无：所有碎片
                已存储并已满，将超过允许的最大磁盘空间，或者
                最近请求了分类帐，但时间不够
                请求之间。
        @implmnote根据需要添加一个新的可写碎片
    **/

    virtual
    boost::optional<std::uint32_t>
    prepareLedger(std::uint32_t validLedgerSeq) = 0;

    /*准备要导入数据库的碎片索引

        @param shardindindex要准备导入的shard索引
        @return true如果shard index成功准备导入
    **/

    virtual
    bool
    prepareShard(std::uint32_t shardIndex) = 0;

    /*从准备的导入中删除碎片索引

        @param indexes要从导入中删除的shard索引
    **/

    virtual
    void
    removePreShard(std::uint32_t shardIndex) = 0;

    /*获取正在导入的碎片索引

        @返回准备导入的碎片数量
    **/

    virtual
    std::uint32_t
    getNumPreShard() = 0;

    /*将碎片导入碎片数据库

        @param shardindindex要导入的shard索引
        @param srcdir要从中导入的目录
        @param validate if true验证碎片分类帐数据
        @如果成功导入碎片，返回true
        @implmnote如果成功，srcdir将移动到数据库目录
    **/

    virtual
    bool
    importShard(std::uint32_t shardIndex,
        boost::filesystem::path const& srcDir, bool validate) = 0;

    /*从碎片商店取一本分类帐

        @param hash要检索的分类帐的键
        @param seq分类帐的序列
        @如果找到，返回分类帐，否则返回nullptr
    **/

    virtual
    std::shared_ptr<Ledger>
    fetchLedger(uint256 const& hash, std::uint32_t seq) = 0;

    /*通知数据库给定的分类帐
        完全获取和存储。

        @param ledger要标记为完成的已存储分类帐
    **/

    virtual
    void
    setStored(std::shared_ptr<Ledger const> const& ledger) = 0;

    /*查询是否存储具有给定序列的分类帐

        @param seq要检查是否存储的分类帐序列
        @如果存储了分类帐，返回'true'
    **/

    virtual
    bool
    contains(std::uint32_t seq) = 0;

    /*查询存储哪些完整碎片

        @返回完整碎片的索引
    **/

    virtual
    std::string
    getCompleteShards() = 0;

    /*验证碎片存储数据是否有效。

        @param app应用程序对象
    **/

    virtual
    void
    validate() = 0;

    /*@返回碎片中存储的最大分类帐数
    **/

    virtual
    std::uint32_t
    ledgersPerShard() const = 0;

    /*@返回最早的碎片索引
    **/

    virtual
    std::uint32_t
    earliestShardIndex() const = 0;

    /*计算给定分类帐序列的碎片索引

        @param seq分类帐序列
        @返回分类帐序列的碎片索引
    **/

    virtual
    std::uint32_t
    seqToShardIndex(std::uint32_t seq) const = 0;

    /*计算给定碎片索引的第一个分类帐序列

        @param shardindindex考虑的碎片索引
        @返回与碎片索引相关的第一个分类帐序列
    **/

    virtual
    std::uint32_t
    firstLedgerSeq(std::uint32_t shardIndex) const = 0;

    /*计算给定碎片索引的最后一个分类帐序列

        @param shardindindex考虑的碎片索引
        @返回与碎片索引相关的最后一个分类帐序列
    **/

    virtual
    std::uint32_t
    lastLedgerSeq(std::uint32_t shardIndex) const = 0;

    /*返回根数据库目录
    **/

    virtual
    boost::filesystem::path const&
    getRootDir() const = 0;

    /*碎片中的分类帐数量*/
    static constexpr std::uint32_t ledgersPerShardDefault {16384u};
};

constexpr
std::uint32_t
seqToShardIndex(std::uint32_t seq,
    std::uint32_t ledgersPerShard = DatabaseShard::ledgersPerShardDefault)
{
    return (seq - 1) / ledgersPerShard;
}

}
}

#endif
