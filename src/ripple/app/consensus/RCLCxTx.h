
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
    版权所有（c）2012-2016 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_CONSENSUS_RCLCXTX_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_RCLCXTX_H_INCLUDED

#include <ripple/app/misc/CanonicalTXSet.h>
#include <ripple/basics/chrono.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/shamap/SHAMap.h>

namespace ripple {

/*表示rclcommersion中的事务。

    rclcxtx是shamapitem上的一个薄包装，对应于
    交易。
**/

class RCLCxTx
{
public:
//！事务的唯一标识符/哈希
    using ID = uint256;

    /*构造函数

        @param txn要包装的事务
    **/

    RCLCxTx(SHAMapItem const& txn) : tx_{txn}
    {
    }

//！事务的唯一标识符/哈希
    ID const&
    id() const
    {
        return tx_.key();
    }

//！表示事务的shamapitem。
    SHAMapItem const tx_;
};

/*表示rclcommersion中的一组事务。

    rcltxset是一个覆盖在shamap上的薄包装，用于存储
    交易。
**/

class RCLTxSet
{
public:
//！事务集的唯一标识符/哈希
    using ID = uint256;
//！对应于单个事务的类型
    using Tx = RCLCxTx;

//<提供TxSet的可变视图
    class MutableTxSet
    {
        friend class RCLTxSet;
//！表示事务的shamap。
        std::shared_ptr<SHAMap> map_;

    public:
        MutableTxSet(RCLTxSet const& src) : map_{src.map_->snapShot(true)}
        {
        }

        /*在集合中插入新事务。

        @param t要插入的事务。
        @返回交易是否发生。
        **/

        bool
        insert(Tx const& t)
        {
            return map_->addItem(
                SHAMapItem{t.id(), t.tx_.peekData()}, true, false);
        }

        /*从集合中移除事务。

        @param entry要删除的事务的ID。
        @返回是否删除了事务。
        **/

        bool
        erase(Tx::ID const& entry)
        {
            return map_->delItem(entry);
        }
    };

    /*构造函数

        @param m shamap要包装
    **/

    RCLTxSet(std::shared_ptr<SHAMap> m) : map_{std::move(m)}
    {
        assert(map_);
    }

    /*以前创建的mutabletxset的构造函数

        将变为固定的@param m mutabletxset
     **/

    RCLTxSet(MutableTxSet const& m) : map_{m.map_->snapShot(false)}
    {
    }

    /*测试事务是否在集合中。

        @param entry要测试的事务的ID。
        @返回事务是否在集合中。
    **/

    bool
    exists(Tx::ID const& entry) const
    {
        return map_->hasItem(entry);
    }

    /*查找事务。

        @param entry要查找的事务的ID。
        @返回shamapitem的共享指针。

        @注意，由于查找可能不成功，这将返回
              `std：：shared鍖ptr<const shamapitem>`而不是tx，它
              无法引用缺少的事务。一般共识
              代码使用共享的\ptr语义来了解
              根据需要成功并正确地创建了一个Tx。
    **/

    std::shared_ptr<const SHAMapItem> const&
    find(Tx::ID const& entry) const
    {
        return map_->peekItem(entry);
    }

//！事务集的唯一ID/哈希
    ID
    id() const
    {
        return map_->getHash().as_uint256();
    }

    /*查找此事务和其他事务之间不相同的事务
       集合。

        @param j要与之比较的集合
        @返回此集合和'j'中事务的映射，但不能同时返回。关键
                是事务ID，值是事务的bool
                存在于此集中。
    **/

    std::map<Tx::ID, bool>
    compare(RCLTxSet const& j) const
    {
        SHAMap::Delta delta;

//把我们的工作捆绑起来以防恶意
//从可信验证器映射
        map_->compare(*(j.map_), delta, 65536);

        std::map<uint256, bool> ret;
        for (auto const& item : delta)
        {
            assert(
                (item.second.first && !item.second.second) ||
                (item.second.second && !item.second.first));

            ret[item.first] = static_cast<bool>(item.second.first);
        }
        return ret;
    }

//！表示事务的shamap。
    std::shared_ptr<SHAMap> map_;
};
}
#endif
