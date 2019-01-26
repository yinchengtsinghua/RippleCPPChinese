
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

#ifndef RIPPLE_APP_MAIN_APPLICATION_H_INCLUDED
#define RIPPLE_APP_MAIN_APPLICATION_H_INCLUDED

#include <ripple/shamap/FullBelowCache.h>
#include <ripple/shamap/TreeNodeCache.h>
#include <ripple/basics/TaggedCache.h>
#include <ripple/core/Config.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <boost/asio.hpp>
#include <memory>
#include <mutex>

namespace ripple {

namespace unl { class Manager; }
namespace Resource { class Manager; }
namespace NodeStore { class Database; class DatabaseShard; }
namespace perf { class PerfLog; }

//vfalc todo fix forward声明了头依赖循环所必需的
class AmendmentTable;
class CachedSLEs;
class CollectorManager;
class Family;
class HashRouter;
class Logs;
class LoadFeeTrack;
class JobQueue;
class InboundLedgers;
class InboundTransactions;
class AcceptedLedger;
class LedgerMaster;
class LoadManager;
class ManifestCache;
class NetworkOPs;
class OpenLedger;
class OrderBookDB;
class Overlay;
class PathRequests;
class PendingSaves;
class PublicKey;
class SecretKey;
class AccountIDCache;
class STLedgerEntry;
class TimeKeeper;
class TransactionMaster;
class TxQ;

class ValidatorList;
class ValidatorSite;
class Cluster;

class DatabaseCon;
class SHAMapStore;

using NodeCache     = TaggedCache <SHAMapHash, Blob>;

template <class Adaptor>
class Validations;
class RCLValidationsAdaptor;
using RCLValidations = Validations<RCLValidationsAdaptor>;

class Application : public beast::PropertyStream::Source
{
public:
    /*弗法科纸币

        主互斥保护：

        -未结分类帐
        -服务器全局状态
            *最后一个关闭的分类帐是什么？
            *共识引擎的状态

        其他事情
    **/

    using MutexType = std::recursive_mutex;
    virtual MutexType& getMasterMutex () = 0;

public:
    Application ();

    virtual ~Application () = default;

    virtual bool setup() = 0;
    virtual void doStart(bool withTimers) = 0;
    virtual void run() = 0;
    virtual bool isShutdown () = 0;
    virtual void signalStop () = 0;
    virtual bool checkSigs() const = 0;
    virtual void checkSigs(bool) = 0;

//
//---
//

    virtual Logs&                   logs() = 0;
    virtual Config&                 config() = 0;

    virtual
    boost::asio::io_service&
    getIOService () = 0;

    virtual CollectorManager&           getCollectorManager () = 0;
    virtual Family&                     family() = 0;
    virtual Family*                     shardFamily() = 0;
    virtual TimeKeeper&                 timeKeeper() = 0;
    virtual JobQueue&                   getJobQueue () = 0;
    virtual NodeCache&                  getTempNodeCache () = 0;
    virtual CachedSLEs&                 cachedSLEs() = 0;
    virtual AmendmentTable&             getAmendmentTable() = 0;
    virtual HashRouter&                 getHashRouter () = 0;
    virtual LoadFeeTrack&               getFeeTrack () = 0;
    virtual LoadManager&                getLoadManager () = 0;
    virtual Overlay&                    overlay () = 0;
    virtual TxQ&                        getTxQ() = 0;
    virtual ValidatorList&              validators () = 0;
    virtual ValidatorSite&              validatorSites () = 0;
    virtual ManifestCache&              validatorManifests () = 0;
    virtual ManifestCache&              publisherManifests () = 0;
    virtual Cluster&                    cluster () = 0;
    virtual RCLValidations&             getValidations () = 0;
    virtual NodeStore::Database&        getNodeStore () = 0;
    virtual NodeStore::DatabaseShard*   getShardStore() = 0;
    virtual InboundLedgers&             getInboundLedgers () = 0;
    virtual InboundTransactions&        getInboundTransactions () = 0;

    virtual
    TaggedCache <uint256, AcceptedLedger>&
    getAcceptedLedgerCache () = 0;

    virtual LedgerMaster&           getLedgerMaster () = 0;
    virtual NetworkOPs&             getOPs () = 0;
    virtual OrderBookDB&            getOrderBookDB () = 0;
    virtual TransactionMaster&      getMasterTransaction () = 0;
    virtual perf::PerfLog&          getPerfLog () = 0;

    virtual
    std::pair<PublicKey, SecretKey> const&
    nodeIdentity () = 0;

    virtual
    PublicKey const &
    getValidationPublicKey() const  = 0;

    virtual Resource::Manager&      getResourceManager () = 0;
    virtual PathRequests&           getPathRequests () = 0;
    virtual SHAMapStore&            getSHAMapStore () = 0;
    virtual PendingSaves&           pendingSaves() = 0;
    virtual AccountIDCache const&   accountIDCache() const = 0;
    virtual OpenLedger&             openLedger() = 0;
    virtual OpenLedger const&       openLedger() const = 0;
    virtual DatabaseCon&            getTxnDB () = 0;
    virtual DatabaseCon&            getLedgerDB () = 0;

    virtual
    std::chrono::milliseconds
    getIOLatency () = 0;

    virtual bool serverOkay (std::string& reason) = 0;

    virtual beast::Journal journal (std::string const& name) = 0;

    /*返回应用程序所需的文件描述符数*/
    virtual int fdlimit () const = 0;

    /*检索“钱包数据库”*/
    virtual DatabaseCon& getWalletDB () = 0;

    /*确保新启动的验证程序不签署旧的建议
     *比它保存的上一个分类帐还要多。*/

    virtual LedgerIndex getMaxDisallowedLedger() = 0;
};

std::unique_ptr <Application>
make_Application(
    std::unique_ptr<Config> config,
    std::unique_ptr<Logs> logs,
    std::unique_ptr<TimeKeeper> timeKeeper);

}

#endif
