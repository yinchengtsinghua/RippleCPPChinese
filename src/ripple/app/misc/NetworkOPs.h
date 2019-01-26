
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

#ifndef RIPPLE_APP_MISC_NETWORKOPS_H_INCLUDED
#define RIPPLE_APP_MISC_NETWORKOPS_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/consensus/RCLCxPeerPos.h>
#include <ripple/core/JobQueue.h>
#include <ripple/core/Stoppable.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/net/InfoSub.h>
#include <ripple/protocol/STValidation.h>
#include <boost/asio.hpp>
#include <memory>
#include <deque>
#include <tuple>

#include <ripple/protocol/messages.h>

namespace ripple {

//客户端可能希望对网络执行的操作
//主操作处理器、服务器序列器、网络跟踪器

class Peer;
class LedgerMaster;
class Transaction;
class ValidatorKeys;

//这是进入程序“客户机”部分的主要接口。
//希望在网络上执行正常操作的代码，例如
//创建和监控帐户、创建事务等
//应该使用这个接口。RPC代码主要是一个轻量级包装器
//通过这个代码。
//
//最后，它将检查节点的操作模式（同步、未同步、
//并遵循正确的处理方法。电流
//代码假定此节点已同步（并将继续同步，直到
//有一个功能强大的网络。
//
/*为客户端提供服务器功能。

    客户端包括后端应用程序、本地命令和连接的
    客户。此类充当代理，使用本地
    可能的话，或者询问网络并返回结果
    需要。

    后端应用程序或本地客户端可以信任的本地实例
    波纹/网络运作。但是，客户端软件连接到非本地
    需要强化涟漪的实例，以抵御敌意。
    或不可靠的服务器。
**/

class NetworkOPs
    : public InfoSub::Source
{
protected:
    explicit NetworkOPs (Stoppable& parent);

public:
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

    enum OperatingMode
    {
//我们如何处理交易或账户余额请求
omDISCONNECTED  = 0,    //尚未准备好处理请求
omCONNECTED     = 1,    //确信我们正在与网络对话
omSYNCING       = 2,    //稍微落后一点
omTRACKING      = 3,    //相信我们同意网络
omFULL          = 4     //我们有账本，甚至可以确认
    };

    enum class FailHard : unsigned char
    {
        no,
        yes
    };
    static inline FailHard doFailHard (bool noMeansDont)
    {
        return noMeansDont ? FailHard::yes : FailHard::no;
    }

public:
    ~NetworkOPs () override = default;

//——————————————————————————————————————————————————————————————
//
//网络信息
//

    virtual OperatingMode getOperatingMode () const = 0;
    virtual std::string strOperatingMode () const = 0;

//——————————————————————————————————————————————————————————————
//
//事务处理
//

//必须立即完成
    virtual void submitTransaction (std::shared_ptr<STTx const> const&) = 0;

    /*
     *处理从网络或
     *由客户提交。同步处理本地事务
     *
     *@param transaction事务对象
     *@param bunlimited是否提交了特权客户端连接。
     *@param blocal客户端提交。
     *@param failtype失败\事务提交的硬设置。
     **/

    virtual void processTransaction (std::shared_ptr<Transaction>& transaction,
        bool bUnlimited, bool bLocal, FailHard failType) = 0;

//——————————————————————————————————————————————————————————————
//
//所有者函数
//

    virtual Json::Value getOwnerInfo (std::shared_ptr<ReadView const> lpLedger,
        AccountID const& account) = 0;

//——————————————————————————————————————————————————————————————
//
//图书功能
//

    virtual void getBookPage (
        std::shared_ptr<ReadView const>& lpLedger,
        Book const& book,
        AccountID const& uTakerID,
        bool const bProof,
        unsigned int iLimit,
        Json::Value const& jvMarker,
        Json::Value& jvResult) = 0;

//——————————————————————————————————————————————————————————————

//分类帐建议/关闭功能
    virtual void processTrustedProposal (RCLCxPeerPos peerPos,
        std::shared_ptr<protocol::TMProposeSet> set) = 0;

    virtual bool recvValidation (STValidation::ref val,
        std::string const& source) = 0;

    virtual void mapComplete (std::shared_ptr<SHAMap> const& map,
        bool fromAcquire) = 0;

//网络状态机
    virtual bool beginConsensus (uint256 const& netLCL) = 0;
    virtual void endConsensus () = 0;
    virtual void setStandAlone () = 0;
    virtual void setStateTimer () = 0;

    virtual void setNeedNetworkLedger () = 0;
    virtual void clearNeedNetworkLedger () = 0;
    virtual bool isNeedNetworkLedger () = 0;
    virtual bool isFull () = 0;
    virtual bool isAmendmentBlocked () = 0;
    virtual void setAmendmentBlocked () = 0;
    virtual void consensusViewChange () = 0;

    virtual Json::Value getConsensusInfo () = 0;
    virtual Json::Value getServerInfo (
        bool human, bool admin, bool counters) = 0;
    virtual void clearLedgerFetch () = 0;
    virtual Json::Value getLedgerFetchInfo () = 0;

    /*接受当前交易树，返回新分类帐的序列

        此API仅在服务器处于独立模式和
        执行一个虚拟的共识回合，包括我们所有的交易
        提议被接受。
    **/

    virtual std::uint32_t acceptLedger (
        boost::optional<std::chrono::milliseconds> consensusDelay = boost::none) = 0;

    virtual uint256 getConsensusLCL () = 0;

    virtual void reportFeeChange () = 0;

    virtual void updateLocalTx (ReadView const& newValidLedger) = 0;
    virtual std::size_t getLocalTxCount () = 0;

//客户端信息检索功能
    using AccountTx  = std::pair<std::shared_ptr<Transaction>, TxMeta::pointer>;
    using AccountTxs = std::vector<AccountTx>;

    virtual AccountTxs getAccountTxs (
        AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger,  bool descending,
        std::uint32_t offset, int limit, bool bUnlimited) = 0;

    virtual AccountTxs getTxsAccount (
        AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger, bool forward,
        Json::Value& token, int limit, bool bUnlimited) = 0;

    using txnMetaLedgerType = std::tuple<std::string, std::string, std::uint32_t>;
    using MetaTxsList       = std::vector<txnMetaLedgerType>;

    virtual MetaTxsList getAccountTxsB (AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger,  bool descending,
            std::uint32_t offset, int limit, bool bUnlimited) = 0;

    virtual MetaTxsList getTxsAccountB (AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger,  bool forward,
        Json::Value& token, int limit, bool bUnlimited) = 0;

//——————————————————————————————————————————————————————————————
//
//监视：发布服务器端
//
    virtual void pubLedger (
        std::shared_ptr<ReadView const> const& lpAccepted) = 0;
    virtual void pubProposedTransaction (
        std::shared_ptr<ReadView const> const& lpCurrent,
        std::shared_ptr<STTx const> const& stTxn, TER terResult) = 0;
    virtual void pubValidation (STValidation::ref val) = 0;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::unique_ptr<NetworkOPs>
make_NetworkOPs (Application& app, NetworkOPs::clock_type& clock,
    bool standalone, std::size_t network_quorum, bool start_valid,
    JobQueue& job_queue, LedgerMaster& ledgerMaster, Stoppable& parent,
    ValidatorKeys const & validatorKeys, boost::asio::io_service& io_svc,
    beast::Journal journal);

} //涟漪

#endif
