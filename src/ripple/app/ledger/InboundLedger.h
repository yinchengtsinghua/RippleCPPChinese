
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

#ifndef RIPPLE_APP_LEDGER_INBOUNDLEDGER_H_INCLUDED
#define RIPPLE_APP_LEDGER_INBOUNDLEDGER_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/overlay/PeerSet.h>
#include <ripple/basics/CountedObject.h>
#include <mutex>
#include <set>
#include <utility>

namespace ripple {

//我们正在试图获取的分类帐
class InboundLedger
    : public PeerSet
    , public std::enable_shared_from_this <InboundLedger>
    , public CountedObject <InboundLedger>
{
public:
    static char const* getCountedObjectName () { return "InboundLedger"; }

    using PeerDataPairType = std::pair<
        std::weak_ptr<Peer>,
        std::shared_ptr<protocol::TMLedgerData>>;

//这就是我们可能获得分类账的原因
    enum class Reason
    {
HISTORY,  //获取过去的分类帐
SHARD,    //获取碎片
GENERIC,  //一般其他原因
CONSENSUS //我们认为，协商一致需要这个分类账。
    };

    InboundLedger(Application& app, uint256 const& hash,
        std::uint32_t seq, Reason reason, clock_type&);

    ~InboundLedger ();

//对等设置计时器过期时调用
    void execute () override;

//当再次尝试获取此同一分类帐时调用
    void update (std::uint32_t seq);

    std::shared_ptr<Ledger const>
    getLedger() const
    {
        return mLedger;
    }

    std::uint32_t getSeq () const
    {
        return mSeq;
    }

    Reason
    getReason() const
    {
        return mReason;
    }

    bool checkLocal ();
    void init (ScopedLockType& collectionLock);

    bool
    gotData(std::weak_ptr<Peer>,
        std::shared_ptr<protocol::TMLedgerData> const&);

    using neededHash_t =
        std::pair <protocol::TMGetObjectByHash::ObjectType, uint256>;

    /*返回json:：objectValue。*/
    Json::Value getJson (int);

    void runData ();

    static
    LedgerInfo
    deserializeHeader(Slice data, bool hasPrefix);

private:
    enum class TriggerReason
    {
        added,
        reply,
        timeout
    };

    void filterNodes (
        std::vector<std::pair<SHAMapNodeID, uint256>>& nodes,
        TriggerReason reason);

    void trigger (std::shared_ptr<Peer> const&, TriggerReason);

    std::vector<neededHash_t> getNeededHashes ();

    void addPeers ();
    void tryDB (Family& f);

    void done ();

    void onTimer (bool progress, ScopedLockType& peerSetLock) override;

    void newPeer (std::shared_ptr<Peer> const& peer) override
    {
//对于历史节点，不要过早触发
//因为可能会有一个卖东西的包来了
        if (mReason != Reason::HISTORY)
            trigger (peer, TriggerReason::added);
    }

    std::weak_ptr <PeerSet> pmDowncast () override;

    int processData (std::shared_ptr<Peer> peer, protocol::TMLedgerData& data);

    bool takeHeader (std::string const& data);
    bool takeTxNode (const std::vector<SHAMapNodeID>& IDs,
                     const std::vector<Blob>& data,
                     SHAMapAddNode&);
    bool takeTxRootNode (Slice const& data, SHAMapAddNode&);

//vfalc todo重命名为receiveAccountStateNode
//不要使用缩写词，但如果我们至少要使用它们
//正确大写。
//
    bool takeAsNode (const std::vector<SHAMapNodeID>& IDs,
                     const std::vector<Blob>& data,
                     SHAMapAddNode&);
    bool takeAsRootNode (Slice const& data, SHAMapAddNode&);

    std::vector<uint256>
    neededTxHashes (
        int max, SHAMapSyncFilter* filter) const;

    std::vector<uint256>
    neededStateHashes (
        int max, SHAMapSyncFilter* filter) const;

    std::shared_ptr<Ledger> mLedger;
    bool mHaveHeader;
    bool mHaveState;
    bool mHaveTransactions;
    bool mSignaled;
    bool mByHash;
    std::uint32_t mSeq;
    Reason const mReason;

    std::set <uint256> mRecentNodes;

    SHAMapAddNode mStats;

//我们从同行那里收到的数据
    std::mutex mReceivedDataLock;
    std::vector <PeerDataPairType> mReceivedData;
    bool mReceiveDispatched;
};

} //涟漪

#endif
