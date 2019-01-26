
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

#include <ripple/nodestore/Database.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/chrono.h>
#include <ripple/beast/core/CurrentThreadName.h>
#include <ripple/protocol/HashPrefix.h>

namespace ripple {
namespace NodeStore {

Database::Database(
    std::string name,
    Stoppable& parent,
    Scheduler& scheduler,
    int readThreads,
    Section const& config,
    beast::Journal journal)
    : Stoppable(name, parent)
    , j_(journal)
    , scheduler_(scheduler)
{
    std::uint32_t seq;
    if (get_if_exists<std::uint32_t>(config, "earliest_seq", seq))
    {
        if (seq < 1)
            Throw<std::runtime_error>("Invalid earliest_seq");
        earliestSeq_ = seq;
    }

    while (readThreads-- > 0)
        readThreads_.emplace_back(&Database::threadEntry, this);
}

Database::~Database()
{
//注意！
//任何派生类都应在其
//析构函数。否则，有时派生类可能
//当其成员被
//这些线程在派生类被销毁之后但在
//这个基类被销毁。
    stopThreads();
}

void
Database::waitReads()
{
    std::unique_lock<std::mutex> l(readLock_);
//在两代人中醒来。
//每一代人都是对空间的全面超越。
//如果我们在N代，而你发出请求，
//该请求仅在第n代期间完成
//如果刚好在通行证当前所在位置之后着陆。
//但是，如果不是的话，这肯定会在一代人中完成。
//n+1，因为请求在通过之前就在表中
//甚至开始了。所以当你达到n+2代时，
//你知道请求已经完成了。
    std::uint64_t const wakeGen = readGen_ + 2;
    while (! readShut_ && ! read_.empty() && (readGen_ < wakeGen))
        readGenCondVar_.wait(l);
}

void
Database::onStop()
{
//停止时间过后，我们不能再使用作业队列作为背景
//阅读。加入后台读取线程。
    stopThreads();
    stopped();
}

void
Database::stopThreads()
{
    {
        std::lock_guard <std::mutex> l(readLock_);
if (readShut_) //只停止线程一次。
            return;

        readShut_ = true;
        readCondVar_.notify_all();
        readGenCondVar_.notify_all();
    }

    for (auto& e : readThreads_)
        e.join();
}

void
Database::asyncFetch(uint256 const& hash, std::uint32_t seq,
    std::shared_ptr<TaggedCache<uint256, NodeObject>> const& pCache,
        std::shared_ptr<KeyCache<uint256>> const& nCache)
{
//读一读
    std::lock_guard <std::mutex> l(readLock_);
    if (read_.emplace(hash, std::make_tuple(seq, pCache, nCache)).second)
        readCondVar_.notify_one();
}

std::shared_ptr<NodeObject>
Database::fetchInternal(uint256 const& hash, Backend& srcBackend)
{
    std::shared_ptr<NodeObject> nObj;
    Status status;
    try
    {
        status = srcBackend.fetch(hash.begin(), &nObj);
    }
    catch (std::exception const& e)
    {
        JLOG(j_.fatal()) <<
            "Exception, " << e.what();
        Rethrow();
    }

    switch(status)
    {
    case ok:
        ++fetchHitCount_;
        if (nObj)
            fetchSz_ += nObj->getData().size();
        break;
    case notFound:
        break;
    case dataCorrupt:
//vfalc todo处理遇到的损坏数据！
        JLOG(j_.fatal()) <<
            "Corrupt NodeObject #" << hash;
        break;
    default:
        JLOG(j_.warn()) <<
            "Unknown status=" << status;
        break;
    }
    return nObj;
}

void
Database::importInternal(Backend& dstBackend, Database& srcDB)
{
    Batch b;
    b.reserve(batchWritePreallocationSize);
    srcDB.for_each(
        [&](std::shared_ptr<NodeObject> nObj)
        {
            assert(nObj);
if (! nObj) //这不应该发生
                return;

            ++storeCount_;
            storeSz_ += nObj->getData().size();

            b.push_back(nObj);
            if (b.size() >= batchWritePreallocationSize)
            {
                dstBackend.storeBatch(b);
                b.clear();
                b.reserve(batchWritePreallocationSize);
            }
        });
    if (! b.empty())
        dstBackend.storeBatch(b);
}

//执行提取并报告所用时间
std::shared_ptr<NodeObject>
Database::doFetch(uint256 const& hash, std::uint32_t seq,
    TaggedCache<uint256, NodeObject>& pCache,
        KeyCache<uint256>& nCache, bool isAsync)
{
    FetchReport report;
    report.isAsync = isAsync;
    report.wentToDisk = false;

    using namespace std::chrono;
    auto const before = steady_clock::now();

//查看对象是否已存在于缓存中
    auto nObj = pCache.fetch(hash);
    if (! nObj && ! nCache.touch_if_exists(hash))
    {
//尝试数据库
        report.wentToDisk = true;
        nObj = fetchFrom(hash, seq);
        ++fetchTotalCount_;
        if (! nObj)
        {
//以防发生写操作
            nObj = pCache.fetch(hash);
            if (! nObj)
//我们放弃
                nCache.insert(hash);
        }
        else
        {
//确保所有线程都获得相同的对象
            pCache.canonicalize(hash, nObj);

//因为这是一个“困难”的获取，我们将记录它。
            JLOG(j_.trace()) <<
                "HOS: " << hash << " fetch: in db";
        }
    }
    report.wasFound = static_cast<bool>(nObj);
    report.elapsed = duration_cast<milliseconds>(
        steady_clock::now() - before);
    scheduler_.onFetch(report);
    return nObj;
}

bool
Database::copyLedger(Backend& dstBackend, Ledger const& srcLedger,
    std::shared_ptr<TaggedCache<uint256, NodeObject>> const& pCache,
        std::shared_ptr<KeyCache<uint256>> const& nCache,
            std::shared_ptr<Ledger const> const& srcNext)
{
    assert(static_cast<bool>(pCache) == static_cast<bool>(nCache));
    if (srcLedger.info().hash.isZero() ||
        srcLedger.info().accountHash.isZero())
    {
        assert(false);
        JLOG(j_.error()) <<
            "source ledger seq " << srcLedger.info().seq <<
            " is invalid";
        return false;
    }
    auto& srcDB = const_cast<Database&>(
        srcLedger.stateMap().family().db());
    if (&srcDB == this)
    {
        assert(false);
        JLOG(j_.error()) <<
            "source and destination databases are the same";
        return false;
    }

    Batch batch;
    batch.reserve(batchWritePreallocationSize);
    auto storeBatch = [&]() {
#if RIPPLE_VERIFY_NODEOBJECT_KEYS
        for (auto& nObj : batch)
        {
            assert(nObj->getHash() ==
                sha512Hash(makeSlice(nObj->getData())));
            if (pCache && nCache)
            {
                pCache->canonicalize(nObj->getHash(), nObj, true);
                nCache->erase(nObj->getHash());
                storeStats(nObj->getData().size());
            }
        }
#else
        if (pCache && nCache)
            for (auto& nObj : batch)
            {
                pCache->canonicalize(nObj->getHash(), nObj, true);
                nCache->erase(nObj->getHash());
                storeStats(nObj->getData().size());
            }
#endif
        dstBackend.storeBatch(batch);
        batch.clear();
        batch.reserve(batchWritePreallocationSize);
    };
    bool error = false;
    auto f = [&](SHAMapAbstractNode& node) {
        if (auto nObj = srcDB.fetch(
            node.getNodeHash().as_uint256(), srcLedger.info().seq))
        {
            batch.emplace_back(std::move(nObj));
            if (batch.size() >= batchWritePreallocationSize)
                storeBatch();
        }
        else
            error = true;
        return !error;
    };

//商店分类帐标题
    {
        Serializer s(1024);
        s.add32(HashPrefix::ledgerMaster);
        addRaw(srcLedger.info(), s);
        auto nObj = NodeObject::createObject(hotLEDGER,
            std::move(s.modData()), srcLedger.info().hash);
        batch.emplace_back(std::move(nObj));
    }

//存储状态图
    if (srcLedger.stateMap().getHash().isNonZero())
    {
        if (!srcLedger.stateMap().isValid())
        {
            JLOG(j_.error()) <<
                "source ledger seq " << srcLedger.info().seq <<
                " state map invalid";
            return false;
        }
        if (srcNext && srcNext->info().parentHash == srcLedger.info().hash)
        {
            auto have = srcNext->stateMap().snapShot(false);
            srcLedger.stateMap().snapShot(
                false)->visitDifferences(&(*have), f);
        }
        else
            srcLedger.stateMap().snapShot(false)->visitNodes(f);
        if (error)
            return false;
    }

//存储事务映射
    if (srcLedger.info().txHash.isNonZero())
    {
        if (!srcLedger.txMap().isValid())
        {
            JLOG(j_.error()) <<
                "source ledger seq " << srcLedger.info().seq <<
                " transaction map invalid";
            return false;
        }
        srcLedger.txMap().snapShot(false)->visitNodes(f);
        if (error)
            return false;
    }

    if (!batch.empty())
        storeBatch();
    return true;
}

//异步读取线程的入口点
void
Database::threadEntry()
{
    beast::setCurrentThreadName("prefetch");
    while (true)
    {
        uint256 lastHash;
        std::uint32_t lastSeq;
        std::shared_ptr<TaggedCache<uint256, NodeObject>> lastPcache;
        std::shared_ptr<KeyCache<uint256>> lastNcache;
        {
            std::unique_lock<std::mutex> l(readLock_);
            while (! readShut_ && read_.empty())
            {
//所有工作都完成了
                readGenCondVar_.notify_all();
                readCondVar_.wait(l);
            }
            if (readShut_)
                break;

//读取密钥以提高后端效率
            auto it = read_.lower_bound(readLastHash_);
            if (it == read_.end())
            {
                it = read_.begin();
//一代人已经完成
                ++readGen_;
                readGenCondVar_.notify_all();
            }
            lastHash = it->first;
            lastSeq = std::get<0>(it->second);
            lastPcache = std::get<1>(it->second).lock();
            lastNcache = std::get<2>(it->second).lock();
            read_.erase(it);
            readLastHash_ = lastHash;
        }

//执行读取
        if (lastPcache && lastPcache)
            doFetch(lastHash, lastSeq, *lastPcache, *lastNcache, true);
    }
}

} //节点存储
} //涟漪
