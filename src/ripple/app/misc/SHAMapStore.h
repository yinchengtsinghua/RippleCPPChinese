
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

#ifndef RIPPLE_APP_MISC_SHAMAPSTORE_H_INCLUDED
#define RIPPLE_APP_MISC_SHAMAPSTORE_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/core/Stoppable.h>

namespace ripple {

class TransactionMaster;

/*
 *类创建数据库，启动联机删除线程，以及
 *相关的sqlite数据库
 **/

class SHAMapStore
    : public Stoppable
{
public:
    struct Setup
    {
        explicit Setup() = default;

        bool standalone = false;
        std::uint32_t deleteInterval = 0;
        bool advisoryDelete = false;
        std::uint32_t ledgerHistory = 0;
        Section nodeDatabase;
        std::string databasePath;
        std::uint32_t deleteBatch = 100;
        std::uint32_t backOff = 100;
        std::int32_t ageThreshold = 60;
        Section shardDatabase;
    };

    SHAMapStore (Stoppable& parent) : Stoppable ("SHAMapStore", parent) {}

    /*每次分类帐验证时由LedgerMaster调用。*/
    virtual void onLedgerClosed(std::shared_ptr<Ledger const> const& ledger) = 0;

    virtual void rendezvous() const = 0;

    virtual std::uint32_t clampFetchDepth (std::uint32_t fetch_depth) const = 0;

    virtual std::unique_ptr <NodeStore::Database> makeDatabase (
            std::string const& name,
            std::int32_t readThreads, Stoppable& parent) = 0;

    virtual std::unique_ptr <NodeStore::DatabaseShard> makeDatabaseShard(
        std::string const& name, std::int32_t readThreads,
            Stoppable& parent) = 0;

    /*可以删除的最高分类帐。*/
    virtual LedgerIndex setCanDelete (LedgerIndex canDelete) = 0;

    /*是否启用建议删除。*/
    virtual bool advisoryDelete() const = 0;

    /*最后一个在后端轮换期间复制的分类帐。*/
    virtual LedgerIndex getLastRotated() = 0;

    /*可以删除的最高分类帐。*/
    virtual LedgerIndex getCanDelete() = 0;

    /*需要的文件数。*/
    virtual int fdlimit() const = 0;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

SHAMapStore::Setup
setup_SHAMapStore(Config const& c);

std::unique_ptr<SHAMapStore>
make_SHAMapStore(
    Application& app,
    SHAMapStore::Setup const& s,
    Stoppable& parent,
    NodeStore::Scheduler& scheduler,
    beast::Journal journal,
    beast::Journal nodeStoreJournal,
    TransactionMaster& transactionMaster,
    BasicConfig const& conf);
}

#endif
