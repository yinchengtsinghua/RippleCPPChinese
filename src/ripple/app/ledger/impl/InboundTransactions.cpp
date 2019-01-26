
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

#include <ripple/app/ledger/InboundTransactions.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/impl/TransactionAcquire.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/basics/Log.h>
#include <ripple/core/JobQueue.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/resource/Fees.h>
#include <memory>
#include <mutex>

namespace ripple {

enum
{
//理想的对等数
    startPeers = 2,

//一套要多少回合
    setKeepRounds = 3,
};

class InboundTransactionSet
{
//我们生成、获取或正在获取的事务集
public:
    std::uint32_t               mSeq;
    TransactionAcquire::pointer mAcquire;
    std::shared_ptr <SHAMap>    mSet;

    InboundTransactionSet (
        std::uint32_t seq,
        std::shared_ptr <SHAMap> const& set) :
            mSeq (seq), mSet (set)
    { ; }
    InboundTransactionSet () : mSeq (0)
    { ; }
};

class InboundTransactionsImp
    : public InboundTransactions
    , public Stoppable
{
public:
    Application& app_;

    InboundTransactionsImp (
            Application& app,
            clock_type& clock,
            Stoppable& parent,
            beast::insight::Collector::ptr const& collector,
            std::function <void (std::shared_ptr <SHAMap> const&,
                bool)> gotSet)
        : Stoppable ("InboundTransactions", parent)
        , app_ (app)
        , m_clock (clock)
        , m_seq (0)
        , m_zeroSet (m_map[uint256()])
        , m_gotSet (std::move (gotSet))
    {
        m_zeroSet.mSet = std::make_shared<SHAMap> (
            SHAMapType::TRANSACTION, uint256(),
            app_.family(), SHAMap::version{1});
        m_zeroSet.mSet->setUnbacked();
    }

    TransactionAcquire::pointer getAcquire (uint256 const& hash)
    {
        {
            ScopedLockType sl (mLock);

            auto it = m_map.find (hash);

            if (it != m_map.end ())
                return it->second.mAcquire;
        }
        return {};
    }

    std::shared_ptr <SHAMap> getSet (
       uint256 const& hash,
       bool acquire) override
    {
        TransactionAcquire::pointer ta;

        {
            ScopedLockType sl (mLock);

            auto it = m_map.find (hash);

            if (it != m_map.end ())
            {
                if (acquire)
                {
                    it->second.mSeq = m_seq;
                    if (it->second.mAcquire)
                    {
                        it->second.mAcquire->stillNeed ();
                    }
                }
                return it->second.mSet;
            }

            if (!acquire || isStopping ())
                return std::shared_ptr <SHAMap> ();

            ta = std::make_shared <TransactionAcquire> (app_, hash, m_clock);

            auto &obj = m_map[hash];
            obj.mAcquire = ta;
            obj.mSeq = m_seq;
        }


        ta->init (startPeers);

        return {};
    }

    /*我们收到一位同行的TMledgerData。
    **/

    void gotData (LedgerHash const& hash,
            std::shared_ptr<Peer> peer,
            std::shared_ptr<protocol::TMLedgerData> packet_ptr) override
    {
        protocol::TMLedgerData& packet = *packet_ptr;

        JLOG (app_.journal("InboundLedger").trace()) <<
            "Got data (" << packet.nodes ().size () << ") "
            "for acquiring ledger: " << hash;

        TransactionAcquire::pointer ta = getAcquire (hash);

        if (ta == nullptr)
        {
            peer->charge (Resource::feeUnwantedData);
            return;
        }

        std::list<SHAMapNodeID> nodeIDs;
        std::list< Blob > nodeData;
        for (auto const &node : packet.nodes())
        {
            if (!node.has_nodeid () || !node.has_nodedata () || (
                node.nodeid ().size () != 33))
            {
                peer->charge (Resource::feeInvalidRequest);
                return;
            }

            nodeIDs.emplace_back (node.nodeid ().data (),
                               static_cast<int>(node.nodeid ().size ()));
            nodeData.emplace_back (node.nodedata ().begin (),
                node.nodedata ().end ());
        }

        if (! ta->takeNodes (nodeIDs, nodeData, peer).isUseful ())
            peer->charge (Resource::feeUnwantedData);
    }

    void giveSet (uint256 const& hash,
        std::shared_ptr <SHAMap> const& set,
        bool fromAcquire) override
    {
        bool isNew = true;

        {
            ScopedLockType sl (mLock);

            auto& inboundSet = m_map [hash];

            if (inboundSet.mSeq < m_seq)
                inboundSet.mSeq = m_seq;

            if (inboundSet.mSet)
                isNew = false;
            else
                inboundSet.mSet = set;

             inboundSet.mAcquire.reset ();

        }

        if (isNew)
            m_gotSet (set, fromAcquire);
    }

    Json::Value getInfo() override
    {
        Json::Value ret (Json::objectValue);

        Json::Value& sets = (ret["sets"] = Json::arrayValue);

        {
            ScopedLockType sl (mLock);

            ret["seq"] = m_seq;

            for (auto const& it : m_map)
            {
                Json::Value& set = sets [to_string (it.first)];
                set["seq"] = it.second.mSeq;
                if (it.second.mSet)
                    set["state"] = "complete";
                else if (it.second.mAcquire)
                    set["state"] = "acquiring";
                else
                    set["state"] = "dead";
            }

        }

        return ret;
    }

    void newRound (std::uint32_t seq) override
    {
        ScopedLockType lock (mLock);

//保护零集不过期
        m_zeroSet.mSeq = seq;

        if (m_seq != seq)
        {

            m_seq = seq;

            auto it = m_map.begin ();

            std::uint32_t const minSeq =
                (seq < setKeepRounds) ? 0 : (seq - setKeepRounds);
            std::uint32_t maxSeq = seq + setKeepRounds;

            while (it != m_map.end ())
            {
                if (it->second.mSeq < minSeq || it->second.mSeq > maxSeq)
                    it = m_map.erase (it);
                else
                    ++it;
            }
        }
    }

    void onStop () override
    {
        ScopedLockType lock (mLock);

        m_map.clear ();

        stopped();
    }

private:
    clock_type& m_clock;

    using MapType = hash_map <uint256, InboundTransactionSet>;

    using ScopedLockType = std::lock_guard <std::recursive_mutex>;
    std::recursive_mutex mLock;

    MapType m_map;
    std::uint32_t m_seq;

//哈希为零的空事务集
    InboundTransactionSet& m_zeroSet;

    std::function <void (std::shared_ptr <SHAMap> const&, bool)> m_gotSet;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

InboundTransactions::~InboundTransactions() = default;

std::unique_ptr <InboundTransactions>
make_InboundTransactions (
    Application& app,
    InboundLedgers::clock_type& clock,
    Stoppable& parent,
    beast::insight::Collector::ptr const& collector,
    std::function <void (std::shared_ptr <SHAMap> const&,
        bool)> gotSet)
{
    return std::make_unique <InboundTransactionsImp>
        (app, clock, parent, collector, std::move (gotSet));
}

} //涟漪
