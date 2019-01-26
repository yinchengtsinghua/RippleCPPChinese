
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

#ifndef RIPPLE_APP_LEDGER_LEDGER_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGER_H_INCLUDED

#include <ripple/ledger/TxMeta.h>
#include <ripple/ledger/View.h>
#include <ripple/ledger/CachedView.h>
#include <ripple/basics/CountedObject.h>
#include <ripple/core/TimeKeeper.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/Book.h>
#include <ripple/shamap/SHAMap.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>
#include <mutex>

namespace ripple {

class Application;
class Job;
class TransactionMaster;

class SqliteStatement;

struct create_genesis_t
{
    explicit create_genesis_t() = default;
};
extern create_genesis_t const create_genesis;

/*持有分类帐

    分类账由两个假账户组成。国家地图包含所有
    会计分录，如科目根和订单簿。Tx地图保持不变
    所有的事务和相关的元数据
    特定分类帐。分类帐上的大部分操作都是有关的
    用国家地图。

    这可以只保存头、部分数据集或整个数据集
    数据。这完全取决于相应的shamap条目中的内容。
    提供了各种函数来填充或取消填充
    对象保存对的引用。

    分类帐被构造为可变的或不可变的。

    1）如果你是可变账本的唯一所有者，你可以做任何你想做的事。
    不需要锁就想要。

    2）如果你有一个不变的分类账，你就不能更改它，所以不需要
    锁。

    3）不能共享可变分类账。

    @注释以readview的形式呈现给客户
    @note在构造函数中调用virtuals，因此标记为final
**/

class Ledger final
    : public std::enable_shared_from_this <Ledger>
    , public DigestAwareReadView
    , public TxsRawView
    , public CountedObject <Ledger>
{
public:
    static char const* getCountedObjectName () { return "Ledger"; }

    Ledger (Ledger const&) = delete;
    Ledger& operator= (Ledger const&) = delete;

    /*创建Genesis分类帐。

        Genesis分类账包含一个账户，
        通过使用种子的生成器生成accountID
        根据字符串“masterpassphrase”和序数计算
        零。

        该账户的xrp余额等于总金额
        系统中的xrp。不超过
        此帐户中的起始值可以存在，金额为
        用来支付被销毁的费用。

        指定的修改在Genesis分类账中启用。
    **/

    Ledger (
        create_genesis_t,
        Config const& config,
        std::vector<uint256> const& amendments,
        Family& family);

    Ledger (
        LedgerInfo const& info,
        Config const& config,
        Family& family);

    /*用于从JSON文件加载的分类帐

        @param acquire if true，如果未在本地找到，则获取分类帐
    **/

    Ledger (
        LedgerInfo const& info,
        bool& loaded,
        bool acquire,
        Config const& config,
        Family& family,
        beast::Journal j);

    /*在上一个分类帐之后创建新分类帐

        分类帐将具有序列号
        遵循前面的，并且
        parentCloseTime==previous.closeTime。
    **/

    Ledger (Ledger const& previous,
        NetClock::time_point closeTime);

//用于数据库分类帐
    Ledger (std::uint32_t ledgerSeq,
        NetClock::time_point closeTime, Config const& config,
            Family& family);

    ~Ledger() = default;

//
//读取视图
//

    bool
    open() const override
    {
        return false;
    }

    LedgerInfo const&
    info() const override
    {
        return info_;
    }

    Fees const&
    fees() const override
    {
        return fees_;
    }

    Rules const&
    rules() const override
    {
        return rules_;
    }

    bool
    exists (Keylet const& k) const override;

    boost::optional<uint256>
    succ (uint256 const& key, boost::optional<
        uint256> const& last = boost::none) const override;

    std::shared_ptr<SLE const>
    read (Keylet const& k) const override;

    std::unique_ptr<sles_type::iter_base>
    slesBegin() const override;

    std::unique_ptr<sles_type::iter_base>
    slesEnd() const override;

    std::unique_ptr<sles_type::iter_base>
    slesUpperBound(uint256 const& key) const override;

    std::unique_ptr<txs_type::iter_base>
    txsBegin() const override;

    std::unique_ptr<txs_type::iter_base>
    txsEnd() const override;

    bool
    txExists (uint256 const& key) const override;

    tx_type
    txRead (key_type const& key) const override;

//
//摘要警告视图
//

    boost::optional<digest_type>
    digest (key_type const& key) const override;

//
//罗维尤
//

    void
    rawErase (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawInsert (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawReplace (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawDestroyXRP (XRPAmount const& fee) override
    {
        info_.drops -= fee;
    }

//
//TXSRAWVIEW
//

    void
    rawTxInsert (uint256 const& key,
        std::shared_ptr<Serializer const
            > const& txn, std::shared_ptr<
                Serializer const> const& metaData) override;

//——————————————————————————————————————————————————————————————

    void setValidated() const
    {
        info_.validated = true;
    }

    void setAccepted (NetClock::time_point closeTime,
        NetClock::duration closeResolution, bool correctCloseTime,
            Config const& config);

    void setImmutable (Config const& config);

    bool isImmutable () const
    {
        return mImmutable;
    }

    /*将此分类帐标记为“应已满”。

        “full”是分类帐的元数据属性，它表示
        本地服务器需要所有相应的节点
        存放在耐用的地方。

        这被标记为“const”，因为它反映元数据
        而不是与
        网络。
    **/

    void
    setFull() const
    {
        txMap_->setFull();
        stateMap_->setFull();
        txMap_->setLedgerSeq(info_.seq);
        stateMap_->setLedgerSeq(info_.seq);
    }

    void setTotalDrops (std::uint64_t totDrops)
    {
        info_.drops = totDrops;
    }

    SHAMap const&
    stateMap() const
    {
        return *stateMap_;
    }

    SHAMap&
    stateMap()
    {
        return *stateMap_;
    }

    SHAMap const&
    txMap() const
    {
        return *txMap_;
    }

    SHAMap&
    txMap()
    {
        return *txMap_;
    }

//出错时返回false
    bool addSLE (SLE const& sle);

//——————————————————————————————————————————————————————————————

    void updateSkipList ();

    bool walkLedger (beast::Journal j) const;

    bool assertSane (beast::Journal ledgerJ) const;

    void make_v2();
    void invariants() const;
    void unshare() const;
private:
    class sles_iter_impl;
    class txs_iter_impl;

    bool
    setup (Config const& config);

    std::shared_ptr<SLE>
    peek (Keylet const& k) const;

    bool mImmutable;

    std::shared_ptr<SHAMap> txMap_;
    std::shared_ptr<SHAMap> stateMap_;

//保护费用变量
    std::mutex mutable mutex_;

    Fees fees_;
    Rules rules_;
    LedgerInfo info_;
};

/*包装在cachedView中的分类帐。*/
using CachedLedger = CachedView<Ledger>;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//美国石油学会
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

extern
bool
pendSaveValidated(
    Application& app,
    std::shared_ptr<Ledger const> const& ledger,
    bool isSynchronous,
    bool isCurrent);

extern
std::shared_ptr<Ledger>
loadByIndex (std::uint32_t ledgerIndex,
    Application& app, bool acquire = true);

extern
std::tuple<std::shared_ptr<Ledger>, std::uint32_t, uint256>
loadLedgerHelper(std::string const& sqlSuffix,
    Application& app, bool acquire = true);

extern
std::shared_ptr<Ledger>
loadByHash (uint256 const& ledgerHash,
    Application& app, bool acquire = true);

extern
uint256
getHashByIndex(std::uint32_t index, Application& app);

extern
bool
getHashesByIndex(std::uint32_t index,
    uint256 &ledgerHash, uint256& parentHash,
        Application& app);

extern
std::map< std::uint32_t, std::pair<uint256, uint256>>
getHashesByIndex (std::uint32_t minSeq, std::uint32_t maxSeq,
    Application& app);

/*反序列化包含单个sttx的shamapitem

    投掷：

        可能引发反序列化错误
**/

std::shared_ptr<STTx const>
deserializeTx (SHAMapItem const& item);

/*反序列化包含sttx+stobject元数据的shamapitem

    shamap必须包含两个可变长度
    序列化对象。

    投掷：

        可能引发反序列化错误
**/

std::pair<std::shared_ptr<
    STTx const>, std::shared_ptr<
        STObject const>>
deserializeTxPlusMeta (SHAMapItem const& item);

} //涟漪

#endif
