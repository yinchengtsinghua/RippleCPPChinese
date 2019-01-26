
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

#include <ripple/app/ledger/LedgerHistory.h>
#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/contract.h>
#include <ripple/json/to_string.h>

namespace ripple {

//vfalc todo替换宏

#ifndef CACHED_LEDGER_NUM
#define CACHED_LEDGER_NUM 96
#endif

std::chrono::seconds constexpr CachedLedgerAge = std::chrono::minutes{2};

//修正：需要在某个时刻按索引清理分类账

LedgerHistory::LedgerHistory (
    beast::insight::Collector::ptr const& collector,
        Application& app)
    : app_ (app)
    , collector_ (collector)
    , mismatch_counter_ (collector->make_counter ("ledger.history", "mismatch"))
    , m_ledgers_by_hash ("LedgerCache", CACHED_LEDGER_NUM, CachedLedgerAge,
        stopwatch(), app_.journal("TaggedCache"))
    , m_consensus_validated ("ConsensusValidated", 64, std::chrono::minutes {5},
        stopwatch(), app_.journal("TaggedCache"))
    , j_ (app.journal ("LedgerHistory"))
{
}

bool
LedgerHistory::insert(
    std::shared_ptr<Ledger const> ledger,
        bool validated)
{
    if(! ledger->isImmutable())
        LogicError("mutable Ledger in insert");

    assert (ledger->stateMap().getHash ().isNonZero ());

    LedgersByHash::ScopedLockType sl (m_ledgers_by_hash.peekMutex ());

    const bool alreadyHad = m_ledgers_by_hash.canonicalize (
        ledger->info().hash, ledger, true);
    if (validated)
        mLedgersByIndex[ledger->info().seq] = ledger->info().hash;

    return alreadyHad;
}

LedgerHash LedgerHistory::getLedgerHash (LedgerIndex index)
{
    LedgersByHash::ScopedLockType sl (m_ledgers_by_hash.peekMutex ());
    auto it = mLedgersByIndex.find (index);

    if (it != mLedgersByIndex.end ())
        return it->second;

    return uint256 ();
}

std::shared_ptr<Ledger const>
LedgerHistory::getLedgerBySeq (LedgerIndex index)
{
    {
        LedgersByHash::ScopedLockType sl (m_ledgers_by_hash.peekMutex ());
        auto it = mLedgersByIndex.find (index);

        if (it != mLedgersByIndex.end ())
        {
            uint256 hash = it->second;
            sl.unlock ();
            return getLedgerByHash (hash);
        }
    }

    std::shared_ptr<Ledger const> ret = loadByIndex (index, app_);

    if (!ret)
        return ret;

    assert (ret->info().seq == index);

    {
//按索引将此分类帐添加到本地跟踪
        LedgersByHash::ScopedLockType sl (m_ledgers_by_hash.peekMutex ());

        assert (ret->isImmutable ());
        m_ledgers_by_hash.canonicalize (ret->info().hash, ret);
        mLedgersByIndex[ret->info().seq] = ret->info().hash;
        return (ret->info().seq == index) ? ret : nullptr;
    }
}

std::shared_ptr<Ledger const>
LedgerHistory::getLedgerByHash (LedgerHash const& hash)
{
    auto ret = m_ledgers_by_hash.fetch (hash);

    if (ret)
    {
        assert (ret->isImmutable ());
        assert (ret->info().hash == hash);
        return ret;
    }

    ret = loadByHash (hash, app_);

    if (!ret)
        return ret;

    assert (ret->isImmutable ());
    assert (ret->info().hash == hash);
    m_ledgers_by_hash.canonicalize (ret->info().hash, ret);
    assert (ret->info().hash == hash);

    return ret;
}

static
void
log_one(
    ReadView const& ledger,
    uint256 const& tx,
    char const* msg,
    beast::Journal& j)
{
    auto metaData = ledger.txRead(tx).second;

    if (metaData != nullptr)
    {
        JLOG (j.debug()) << "MISMATCH on TX " << tx <<
            ": " << msg << " is missing this transaction:\n" <<
            metaData->getJson (0);
    }
    else
    {
        JLOG (j.debug()) << "MISMATCH on TX " << tx <<
            ": " << msg << " is missing this transaction.";
    }
}

static
void
log_metadata_difference(
    ReadView const& builtLedger,
    ReadView const& validLedger,
    uint256 const& tx,
    beast::Journal j)
{
    auto getMeta = [j](ReadView const& ledger,
        uint256 const& txID) -> std::shared_ptr<TxMeta>
    {
        auto meta = ledger.txRead(txID).second;
        if (!meta)
            return {};
        return std::make_shared<TxMeta> (txID, ledger.seq(), *meta, j);
    };

    auto validMetaData = getMeta (validLedger, tx);
    auto builtMetaData = getMeta (builtLedger, tx);
    assert(validMetaData != nullptr || builtMetaData != nullptr);

    if (validMetaData != nullptr && builtMetaData != nullptr)
    {
        auto const& validNodes = validMetaData->getNodes ();
        auto const& builtNodes = builtMetaData->getNodes ();

        bool const result_diff =
            validMetaData->getResultTER () != builtMetaData->getResultTER ();

        bool const index_diff =
            validMetaData->getIndex() != builtMetaData->getIndex ();

        bool const nodes_diff = validNodes != builtNodes;

        if (!result_diff && !index_diff && !nodes_diff)
        {
            JLOG (j.error()) << "MISMATCH on TX " << tx <<
                ": No apparent mismatches detected!";
            return;
        }

        if (!nodes_diff)
        {
            if (result_diff && index_diff)
            {
                JLOG (j.debug()) << "MISMATCH on TX " << tx <<
                    ": Different result and index!";
                JLOG (j.debug()) << " Built:" <<
                    " Result: " << builtMetaData->getResult () <<
                    " Index: " << builtMetaData->getIndex ();
                JLOG (j.debug()) << " Valid:" <<
                    " Result: " << validMetaData->getResult () <<
                    " Index: " << validMetaData->getIndex ();
            }
            else if (result_diff)
            {
                JLOG (j.debug()) << "MISMATCH on TX " << tx <<
                    ": Different result!";
                JLOG (j.debug()) << " Built:" <<
                    " Result: " << builtMetaData->getResult ();
                JLOG (j.debug()) << " Valid:" <<
                    " Result: " << validMetaData->getResult ();
            }
            else if (index_diff)
            {
                JLOG (j.debug()) << "MISMATCH on TX " << tx <<
                    ": Different index!";
                JLOG (j.debug()) << " Built:" <<
                    " Index: " << builtMetaData->getIndex ();
                JLOG (j.debug()) << " Valid:" <<
                    " Index: " << validMetaData->getIndex ();
            }
        }
        else
        {
            if (result_diff && index_diff)
            {
                JLOG (j.debug()) << "MISMATCH on TX " << tx <<
                    ": Different result, index and nodes!";
                JLOG (j.debug()) << " Built:\n" <<
                    builtMetaData->getJson (0);
                JLOG (j.debug()) << " Valid:\n" <<
                    validMetaData->getJson (0);
            }
            else if (result_diff)
            {
                JLOG (j.debug()) << "MISMATCH on TX " << tx <<
                    ": Different result and nodes!";
                JLOG (j.debug()) << " Built:" <<
                    " Result: " << builtMetaData->getResult () <<
                    " Nodes:\n" << builtNodes.getJson (0);
                JLOG (j.debug()) << " Valid:" <<
                    " Result: " << validMetaData->getResult () <<
                    " Nodes:\n" << validNodes.getJson (0);
            }
            else if (index_diff)
            {
                JLOG (j.debug()) << "MISMATCH on TX " << tx <<
                    ": Different index and nodes!";
                JLOG (j.debug()) << " Built:" <<
                    " Index: " << builtMetaData->getIndex () <<
                    " Nodes:\n" << builtNodes.getJson (0);
                JLOG (j.debug()) << " Valid:" <<
                    " Index: " << validMetaData->getIndex () <<
                    " Nodes:\n" << validNodes.getJson (0);
            }
else //诺德斯迪夫
            {
                JLOG (j.debug()) << "MISMATCH on TX " << tx <<
                    ": Different nodes!";
                JLOG (j.debug()) << " Built:" <<
                    " Nodes:\n" << builtNodes.getJson (0);
                JLOG (j.debug()) << " Valid:" <<
                    " Nodes:\n" << validNodes.getJson (0);
            }
        }
    }
    else if (validMetaData != nullptr)
    {
        JLOG (j.error()) << "MISMATCH on TX " << tx <<
            ": Metadata Difference (built has none)\n" <<
            validMetaData->getJson (0);
    }
else //构建元数据！= Null PTR
    {
        JLOG (j.error()) << "MISMATCH on TX " << tx <<
            ": Metadata Difference (valid has none)\n" <<
            builtMetaData->getJson (0);
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//按键排序的叶的返回列表
static
std::vector<SHAMapItem const*>
leaves (SHAMap const& sm)
{
    std::vector<SHAMapItem const*> v;
    for (auto const& item : sm)
        v.push_back(&item);
    std::sort(v.begin(), v.end(),
        [](SHAMapItem const* lhs, SHAMapItem const* rhs)
                { return lhs->key() < rhs->key(); });
    return v;
}

void
LedgerHistory::handleMismatch(
    LedgerHash const& built,
    LedgerHash const& valid,
    boost::optional<uint256> const& builtConsensusHash,
    boost::optional<uint256> const& validatedConsensusHash,
    Json::Value const& consensus)
{
    assert (built != valid);
    ++mismatch_counter_;

    auto builtLedger = getLedgerByHash (built);
    auto validLedger = getLedgerByHash (valid);

    if (!builtLedger || !validLedger)
    {
        JLOG (j_.error()) << "MISMATCH cannot be analyzed:" <<
            " builtLedger: " << to_string (built) << " -> " << builtLedger <<
            " validLedger: " << to_string (valid) << " -> " << validLedger;
        return;
    }

    assert (builtLedger->info().seq == validLedger->info().seq);

    if (auto stream = j_.debug())
    {
        stream << "Built: " << getJson (*builtLedger);
        stream << "Valid: " << getJson (*validLedger);
        stream << "Consensus: " << consensus;
    }

//确定不匹配原因，区分拜占庭
//交易处理差异失败

//对上一个分类帐的不一致表示同步问题
    if (builtLedger->info().parentHash != validLedger->info().parentHash)
    {
        JLOG (j_.error()) << "MISMATCH on prior ledger";
        return;
    }

//关闭时间上的不一致表示拜占庭式故障
    if (builtLedger->info().closeTime != validLedger->info().closeTime)
    {
        JLOG (j_.error()) << "MISMATCH on close time";
        return;
    }

    if (builtConsensusHash && validatedConsensusHash)
    {
        if (builtConsensusHash != validatedConsensusHash)
            JLOG(j_.error())
                << "MISMATCH on consensus transaction set "
                << " built: " << to_string(*builtConsensusHash)
                << " validated: " << to_string(*validatedConsensusHash);
        else
            JLOG(j_.error()) << "MISMATCH with same consensus transaction set: "
                             << to_string(*builtConsensusHash);
    }

//找出已建和有效分类账之间的差异
    auto const builtTx = leaves(builtLedger->txMap());
    auto const validTx = leaves(validLedger->txMap());

    if (builtTx == validTx)
        JLOG (j_.error()) <<
            "MISMATCH with same " << builtTx.size() <<
            " transactions";
    else
        JLOG (j_.error()) << "MISMATCH with " <<
            builtTx.size() << " built and " <<
            validTx.size() << " valid transactions.";

    JLOG (j_.error()) << "built\n" << getJson(*builtLedger);
    JLOG (j_.error()) << "valid\n" << getJson(*validLedger);

//记录已建和有效分类账之间的所有差异
    auto b = builtTx.begin();
    auto v = validTx.begin();
    while(b != builtTx.end() && v != validTx.end())
    {
        if ((*b)->key() < (*v)->key())
        {
            log_one (*builtLedger, (*b)->key(), "valid", j_);
            ++b;
        }
        else if ((*b)->key() > (*v)->key())
        {
            log_one(*validLedger, (*v)->key(), "built", j_);
            ++v;
        }
        else
        {
            if ((*b)->peekData() != (*v)->peekData())
            {
//具有不同元数据的相同事务
                log_metadata_difference(
                    *builtLedger,
                    *validLedger, (*b)->key(), j_);
            }
            ++b;
            ++v;
        }
    }
    for (; b != builtTx.end(); ++b)
        log_one (*builtLedger, (*b)->key(), "valid", j_);
    for (; v != validTx.end(); ++v)
        log_one (*validLedger, (*v)->key(), "built", j_);
}

void LedgerHistory::builtLedger (
    std::shared_ptr<Ledger const> const& ledger,
    uint256 const& consensusHash,
    Json::Value consensus)
{
    LedgerIndex index = ledger->info().seq;
    LedgerHash hash = ledger->info().hash;
    assert (!hash.isZero());

    ConsensusValidated::ScopedLockType sl (
        m_consensus_validated.peekMutex());

    auto entry = std::make_shared<cv_entry>();
    m_consensus_validated.canonicalize(index, entry, false);

    if (entry->validated && ! entry->built)
    {
        if (entry->validated.get() != hash)
        {
            JLOG (j_.error()) << "MISMATCH: seq=" << index
                << " validated:" << entry->validated.get()
                << " then:" << hash;
            handleMismatch(
                hash,
                entry->validated.get(),
                consensusHash,
                entry->validatedConsensusHash,
                consensus);
        }
        else
        {
//我们验证了一个分类账，然后在本地建立了它。
            JLOG (j_.debug()) << "MATCH: seq=" << index << " late";
        }
    }

    entry->built.emplace (hash);
    entry->builtConsensusHash.emplace(consensusHash);
    entry->consensus.emplace (std::move (consensus));
}

void LedgerHistory::validatedLedger (
    std::shared_ptr<Ledger const> const& ledger,
    boost::optional<uint256> const& consensusHash)
{
    LedgerIndex index = ledger->info().seq;
    LedgerHash hash = ledger->info().hash;
    assert (!hash.isZero());

    ConsensusValidated::ScopedLockType sl (
        m_consensus_validated.peekMutex());

    auto entry = std::make_shared<cv_entry>();
    m_consensus_validated.canonicalize(index, entry, false);

    if (entry->built && ! entry->validated)
    {
        if (entry->built.get() != hash)
        {
            JLOG (j_.error()) << "MISMATCH: seq=" << index
                << " built:" << entry->built.get()
                << " then:" << hash;
            handleMismatch(
                entry->built.get(),
                hash,
                entry->builtConsensusHash,
                consensusHash,
                entry->consensus.get());
        }
        else
        {
//我们在本地建立了一个分类账，然后验证了它。
            JLOG (j_.debug()) << "MATCH: seq=" << index;
        }
    }

    entry->validated.emplace (hash);
    entry->validatedConsensusHash = consensusHash;
}

/*确保m分类账没有针对特定索引的错误哈希
**/

bool LedgerHistory::fixIndex (
    LedgerIndex ledgerIndex, LedgerHash const& ledgerHash)
{
    LedgersByHash::ScopedLockType sl (m_ledgers_by_hash.peekMutex ());
    auto it = mLedgersByIndex.find (ledgerIndex);

    if ((it != mLedgersByIndex.end ()) && (it->second != ledgerHash) )
    {
        it->second = ledgerHash;
        return false;
    }
    return true;
}

void LedgerHistory::tune (int size, std::chrono::seconds age)
{
    m_ledgers_by_hash.setTargetSize (size);
    m_ledgers_by_hash.setTargetAge (age);
}

void LedgerHistory::clearLedgerCachePrior (LedgerIndex seq)
{
    for (LedgerHash it: m_ledgers_by_hash.getKeys())
    {
        auto const ledger = getLedgerByHash (it);
        if (!ledger || ledger->info().seq < seq)
            m_ledgers_by_hash.del (it, false);
    }
}

} //涟漪
