
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

#ifndef RIPPLE_NODESTORE_SHARD_H_INCLUDED
#define RIPPLE_NODESTORE_SHARD_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/basics/RangeSet.h>
#include <ripple/nodestore/NodeObject.h>
#include <ripple/nodestore/Scheduler.h>

#include <boost/filesystem.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace ripple {
namespace NodeStore {

//全部删除路径
inline static
bool
removeAll(boost::filesystem::path const& path, beast::Journal& j)
{
    try
    {
        boost::filesystem::remove_all(path);
    }
    catch (std::exception const& e)
    {
        JLOG(j.error()) <<
            "exception: " << e.what();
        return false;
    }
    return true;
}

using PCache = TaggedCache<uint256, NodeObject>;
using NCache = KeyCache<uint256>;
class DatabaseShard;

/*由节点存储支持的一系列历史分类帐。
   碎片被编入索引并存储“ledgerspershard”。
   shard`i`存储从序列开始的分类帐：`1+（i*ledgerspershard）`
   以序列结尾：`（I+1）*Ledgerspershard`。
   一旦一个碎片拥有了所有的分类账，它就不会再被写入。
**/

class Shard
{
public:
    Shard(DatabaseShard const& db, std::uint32_t index, int cacheSz,
        std::chrono::seconds cacheAge, beast::Journal& j);

    bool
    open(Section config, Scheduler& scheduler);

    bool
    setStored(std::shared_ptr<Ledger const> const& l);

    boost::optional<std::uint32_t>
    prepare();

    bool
    contains(std::uint32_t seq) const;

    bool
    validate(Application& app);

    std::uint32_t
    index() const {return index_;}

    bool
    complete() const {return complete_;}

    std::shared_ptr<PCache>&
    pCache() {return pCache_;}

    std::shared_ptr<NCache>&
    nCache() {return nCache_;}

    std::uint64_t
    fileSize() const {return fileSize_;}

    std::shared_ptr<Backend> const&
    getBackend() const
    {
        assert(backend_);
        return backend_;
    }

    std::uint32_t
    fdlimit() const
    {
        assert(backend_);
        return backend_->fdlimit();
    }

    std::shared_ptr<Ledger const>
    lastStored() {return lastStored_;}

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & storedSeqs_;
    }

    static constexpr auto controlFileName = "control.txt";

//碎片指数
    std::uint32_t const index_;

//此碎片中的第一个分类帐序列
    std::uint32_t const firstSeq_;

//此碎片中的最后一个分类帐序列
    std::uint32_t const lastSeq_;

//此碎片可以存储的最大分类帐数
//最早的碎片可能比
//后续碎片
    std::uint32_t const maxLedgers_;

//数据库正缓存
    std::shared_ptr<PCache> pCache_;

//数据库负缓存
    std::shared_ptr<NCache> nCache_;

//数据库文件的路径
    boost::filesystem::path const dir_;

//控制文件的路径
    boost::filesystem::path const control_;

    std::uint64_t fileSize_ {0};
    std::shared_ptr<Backend> backend_;
    beast::Journal j_;

//如果shard已存储其整个分类帐范围，则为true
    bool complete_ {false};

//用不完整碎片存储的分类账序列
    RangeSet<std::uint32_t> storedSeqs_;

//用作视觉效果的优化
    std::shared_ptr<Ledger const> lastStored_;

//通过浏览此分类帐的shamaps来验证此分类帐
//验证每棵梅克尔树
    bool
    valLedger(std::shared_ptr<Ledger const> const& l,
        std::shared_ptr<Ledger const> const& next);

//从后端提取并记录
//基于状态代码的错误
    std::shared_ptr<NodeObject>
    valFetch(uint256 const& hash);

//计算后端文件的文件底纹
    void
    updateFileSize();

//为不完整的碎片保存控制文件
    bool
    saveControl();
};

} //节点存储
} //涟漪

#endif
