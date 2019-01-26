
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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
#ifndef RIPPLE_TEST_CSF_LEDGERS_H_INCLUDED
#define RIPPLE_TEST_CSF_LEDGERS_H_INCLUDED

#include <ripple/basics/UnorderedContainers.h>
#include <ripple/basics/chrono.h>
#include <ripple/consensus/LedgerTiming.h>
#include <ripple/basics/tagged_integer.h>
#include <ripple/consensus/LedgerTiming.h>
#include <ripple/json/json_value.h>
#include <test/csf/Tx.h>
#include <boost/bimap/bimap.hpp>
#include <boost/optional.hpp>
#include <set>

namespace ripple {
namespace test {
namespace csf {

/*分类账是一组观察到的交易和序列号。
    标识分类帐。

    在协商一致的过程中，同行们正试图就一系列交易达成一致。
    包括在分类帐中。对于模拟，每个事务都是一个
    整数和分类帐是观察到的整数集。这意味着未来
    分类帐有以前的分类帐作为子集，例如

        分类帐0：{}
        分类帐1：1,4,5_
        分类帐2：1,2,4,5,10_
        …

    分类帐是不可变的值类型。所有分类账顺序相同
    编号、交易记录、关闭时间等具有相同的分类帐ID。
    一个仿真的manges id赋值下的ledgeracle类，是
    关闭和创建新分类帐的唯一方法。因为父分类帐ID是
    这也意味着具有不同历史的分类账
    不同的ID，即使它们具有相同的事务集，序列
    数字和关闭时间。
**/

class Ledger
{
    friend class LedgerOracle;

public:
    struct SeqTag;
    using Seq = tagged_integer<std::uint32_t, SeqTag>;

    struct IdTag;
    using ID = tagged_integer<std::uint32_t, IdTag>;

    struct MakeGenesis {};
private:
//实例是常见的不可变数据，将为其分配一个唯一的
//甲骨文标识
    struct Instance
    {
        Instance() {}

//序列号
        Seq seq{0};

//为生成此分类帐而添加的交易记录
        TxSetType txs;

//用于确定关闭时间的分辨率
        NetClock::duration closeTimeResolution = ledgerDefaultTimeResolution;

//！分类帐关闭时（直到CloseTimeResolution）
        NetClock::time_point closeTime;

//！是否就关闭时间达成共识
        bool closeTimeAgree = true;

//！父分类帐ID
        ID parentID{0};

//！父分类帐关闭时间
        NetClock::time_point parentCloseTime;

//！此分类帐祖先的ID。因为每个分类帐都有唯一的
//！基于parentID的祖先，任何
//！下面的运算符。
        std::vector<Ledger::ID> ancestors;

        auto
        asTie() const
        {
            return std::tie(seq, txs, closeTimeResolution, closeTime,
                closeTimeAgree, parentID, parentCloseTime);
        }

        friend bool
        operator==(Instance const& a, Instance const& b)
        {
            return a.asTie() == b.asTie();
        }

        friend bool
        operator!=(Instance const& a, Instance const& b)
        {
            return a.asTie() != b.asTie();
        }

        friend bool
        operator<(Instance const & a, Instance const & b)
        {
            return a.asTie() < b.asTie();
        }

        template <class Hasher>
        friend void
        hash_append(Hasher& h, Ledger::Instance const& instance)
        {
            using beast::hash_append;
            hash_append(h, instance.asTie());
        }
    };


//单一共同起源实例
    static const Instance genesis;

    Ledger(ID id, Instance const* i) : id_{id}, instance_{i}
    {
    }

public:
    Ledger(MakeGenesis) : instance_(&genesis)
    {
    }

//这是目前普遍共识所要求的，应该是
//迁移到上面的MakeGenesis方法。
    Ledger() : Ledger(MakeGenesis{})
    {
    }

    ID
    id() const
    {
        return id_;
    }

    Seq
    seq() const
    {
        return instance_->seq;
    }

    NetClock::duration
    closeTimeResolution() const
    {
        return instance_->closeTimeResolution;
    }

    bool
    closeAgree() const
    {
        return instance_->closeTimeAgree;
    }

    NetClock::time_point
    closeTime() const
    {
        return instance_->closeTime;
    }

    NetClock::time_point
    parentCloseTime() const
    {
        return instance_->parentCloseTime;
    }

    ID
    parentID() const
    {
        return instance_->parentID;
    }

    TxSetType const&
    txs() const
    {
        return instance_->txs;
    }

    /*确定上级是否确实是此分类帐的上级*/
    bool
    isAncestor(Ledger const& ancestor) const;

    /*返回具有给定seq的祖先ID（如果存在/已知）
     **/

    ID
    operator[](Seq seq) const;

    /*返回第一个不匹配祖先的序列号
    **/

    friend
    Ledger::Seq
    mismatch(Ledger const & a, Ledger const & o);

    Json::Value getJson() const;

    friend bool
    operator<(Ledger const & a, Ledger const & b)
    {
        return a.id() < b.id();
    }

private:
    ID id_{0};
    Instance const* instance_;
};


/*Oracle为模拟维护唯一的分类帐。
**/

class LedgerOracle
{
    using InstanceMap = boost::bimaps::bimap<Ledger::Instance, Ledger::ID>;
    using InstanceEntry = InstanceMap::value_type;

//所有已知分类账的集合；注意，此项从不修剪
    InstanceMap instances_;

//下一个唯一分类帐的ID
    Ledger::ID
    nextID() const;

public:

    LedgerOracle();

    /*查找具有给定ID的分类帐*/
    boost::optional<Ledger>
    lookup(Ledger::ID const & id) const;

    /*接受给定的TXS并生成新的分类帐

        @param curr当前分类帐
        @param txs要应用于当前分类帐的交易
        @param close time resolution用于确定关闭时间的分辨率
        @param consensus close time协商一致结束时间，无效时间
                                  如果0
    **/

    Ledger
    accept(Ledger const & curr, TxSetType const& txs,
        NetClock::duration closeTimeResolution,
        NetClock::time_point const& consensusCloseTime);

    Ledger
    accept(Ledger const& curr, Tx tx)
    {
        using namespace std::chrono_literals;
        return accept(
            curr,
            TxSetType{tx},
            curr.closeTimeResolution(),
            curr.closeTime() + 1s);
    }

    /*确定一组分类帐的不同分支数。

        如果A，分类账A和B在不同的分支上！=b，a不是祖先
        b和b不是a的祖先，例如

          /-->
        o
          > -B
    **/

    std::size_t
    branches(std::set<Ledger> const & ledgers) const;

};

/*帮助编写带有受控分类帐历史记录的单元测试。

    此类允许客户端将不同的分类帐作为字符串引用，其中
    字符串中的每个字符都表示一个唯一的分类帐。它强化了
    运行时的唯一性，但这简化了备用分类账的创建
    历史，例如

     历史助手hh；
     H[ [ A ] ]
     H[ [ ab ] ]
     H[〔AC〕]
     H[ [ ABD ] ]

   创建一个类似
           B-D
         /
       A—C

**/

struct LedgerHistoryHelper
{
    LedgerOracle oracle;
    Tx::ID nextTx{0};
    std::unordered_map<std::string, Ledger> ledgers;
    std::set<char> seen;

    LedgerHistoryHelper()
    {
        ledgers[""] = Ledger{Ledger::MakeGenesis{}};
    }

    /*获取或创建具有给定字符串历史记录的分类帐。

        创建任何必要的中间分类帐，但如果
        重新使用字母（例如“abc”然后“adc”将断言）
    **/

    Ledger const& operator[](std::string const& s)
    {
        auto it = ledgers.find(s);
        if (it != ledgers.end())
            return it->second;

//强制新后缀从未出现
        assert(seen.emplace(s.back()).second);

        Ledger const& parent = (*this)[s.substr(0, s.size() - 1)];
        return ledgers.emplace(s, oracle.accept(parent, ++nextTx))
            .first->second;
    }
};

}  //脑脊液
}  //测试
}  //涟漪

#endif
