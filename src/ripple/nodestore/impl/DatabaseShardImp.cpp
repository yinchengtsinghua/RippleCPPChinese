
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
    版权所有（c）2012，2017 Ripple Labs Inc.

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


#include <ripple/nodestore/impl/DatabaseShardImp.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/random.h>
#include <ripple/nodestore/DummyScheduler.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/overlay/predicates.h>
#include <ripple/protocol/HashPrefix.h>

namespace ripple {
namespace NodeStore {

constexpr std::uint32_t DatabaseShard::ledgersPerShardDefault;

DatabaseShardImp::DatabaseShardImp(Application& app,
    std::string const& name, Stoppable& parent, Scheduler& scheduler,
    int readThreads, Section const& config, beast::Journal j)
    : DatabaseShard(name, parent, scheduler, readThreads, config, j)
    , app_(app)
    , config_(config)
    , dir_(get<std::string>(config, "path"))
    , backendName_(Manager::instance().find(
        get<std::string>(config_, "type"))->getName())
    , maxDiskSpace_(get<std::uint64_t>(config, "max_size_gb") << 30)
    , ledgersPerShard_(get<std::uint32_t>(
        config, "ledgers_per_shard", ledgersPerShardDefault))
    , earliestShardIndex_(seqToShardIndex(earliestSeq()))
    , avgShardSz_(ledgersPerShard_ * (192 * 1024))
{
    if (ledgersPerShard_ == 0 || ledgersPerShard_ % 256 != 0)
        Throw<std::runtime_error>(
            "ledgers_per_shard must be a multiple of 256");
}

DatabaseShardImp::~DatabaseShardImp()
{
//在销毁数据成员之前停止线程
    stopThreads();
}

bool
DatabaseShardImp::init()
{
    using namespace boost::filesystem;
    std::lock_guard<std::mutex> l(m_);
    if (init_)
    {
        assert(false);
        JLOG(j_.error()) <<
            "Already initialized";
        return false;
    }

//查找后端类型和文件句柄要求
    try
    {
        fdLimit_ = Manager::instance().make_Backend(
            config_, scheduler_, j_)->fdlimit();
    }
    catch (std::exception const&)
    {
        JLOG(j_.error()) <<
            "Invalid or missing shard store "
            "type specified in [shard_db]";
        return false;
    }

    backed_ = static_cast<bool>(fdLimit_);
    if (!backed_)
    {
        init_ = true;
        return true;
    }

    try
    {
//寻找碎片
        for (auto const& d : directory_iterator(dir_))
        {
            if (!is_directory(d))
                continue;

//验证碎片目录名是否为数字
            auto dirName = d.path().stem().string();
            if (!std::all_of(
                dirName.begin(),
                dirName.end(),
                [](auto c) {
                return ::isdigit(static_cast<unsigned char>(c));
            }))
            {
                continue;
            }

            auto const shardIndex {std::stoul(dirName)};
            if (shardIndex < earliestShardIndex())
            {
                JLOG(j_.fatal()) <<
                    "Invalid shard index " << shardIndex <<
                    ". Earliest shard index " << earliestShardIndex();
                return false;
            }

//检查以前的导入是否失败
            if (is_regular_file(
                dir_ / std::to_string(shardIndex) / importMarker_))
            {
                JLOG(j_.warn()) <<
                    "shard " << shardIndex <<
                    " previously failed import, removing";
                remove_all(dir_ / std::to_string(shardIndex));
                continue;
            }

            auto shard {std::make_unique<Shard>(
                *this, shardIndex, cacheSz_, cacheAge_, j_)};
            if (!shard->open(config_, scheduler_))
                return false;

            usedDiskSpace_ += shard->fileSize();
            if (shard->complete())
                complete_.emplace(shard->index(), std::move(shard));
            else
            {
                if (incomplete_)
                {
                    JLOG(j_.fatal()) <<
                        "More than one control file found";
                    return false;
                }
                incomplete_ = std::move(shard);
            }
        }
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "exception: " << e.what();
        return false;
    }

    if (!incomplete_ && complete_.empty())
    {
//新的碎片存储，计算文件描述符要求
        if (maxDiskSpace_ > available())
        {
            JLOG(j_.error()) <<
                "Insufficient disk space";
        }
        fdLimit_ = 1 + (fdLimit_ *
            std::max<std::uint64_t>(1, maxDiskSpace_ / avgShardSz_));
    }
    else
        updateStats(l);
    init_ = true;
    return true;
}

boost::optional<std::uint32_t>
DatabaseShardImp::prepareLedger(std::uint32_t validLedgerSeq)
{
    std::lock_guard<std::mutex> l(m_);
    assert(init_);
    if (incomplete_)
        return incomplete_->prepare();
    if (!canAdd_)
        return boost::none;
    if (backed_)
    {
//检查可用磁盘空间
        if (usedDiskSpace_ + avgShardSz_ > maxDiskSpace_)
        {
            JLOG(j_.debug()) <<
                "Maximum size reached";
            canAdd_ = false;
            return boost::none;
        }
        if (avgShardSz_ > available())
        {
            JLOG(j_.error()) <<
                "Insufficient disk space";
            canAdd_ = false;
            return boost::none;
        }
    }

    auto const shardIndex {findShardIndexToAdd(validLedgerSeq, l)};
    if (!shardIndex)
    {
        JLOG(j_.debug()) <<
            "No new shards to add";
        canAdd_ = false;
        return boost::none;
    }
//用每一块新的碎片，清除家庭缓存
    app_.shardFamily()->reset();
    int const sz {std::max(shardCacheSz, cacheSz_ / std::max(
        1, static_cast<int>(complete_.size() + 1)))};
    incomplete_ = std::make_unique<Shard>(
        *this, *shardIndex, sz, cacheAge_, j_);
    if (!incomplete_->open(config_, scheduler_))
    {
        incomplete_.reset();
        return boost::none;
    }

    return incomplete_->prepare();
}

bool
DatabaseShardImp::prepareShard(std::uint32_t shardIndex)
{
    std::lock_guard<std::mutex> l(m_);
    assert(init_);
    if (!canAdd_)
    {
        JLOG(j_.error()) <<
            "Unable to add more shards to the database";
        return false;
    }

    if (shardIndex < earliestShardIndex())
    {
        JLOG(j_.error()) <<
            "Invalid shard index " << shardIndex;
        return false;
    }

//如果我们同步到网络，请检查碎片索引
//大于或等于当前碎片。
    auto seqCheck = [&](std::uint32_t seq)
    {
//如果有效，seq将大于零
        if (seq > earliestSeq() && shardIndex >= seqToShardIndex(seq))
        {
            JLOG(j_.error()) <<
                "Invalid shard index " << shardIndex;
            return false;
        }
        return true;
    };
    if (!seqCheck(app_.getLedgerMaster().getValidLedgerIndex()) ||
        !seqCheck(app_.getLedgerMaster().getCurrentLedgerIndex()))
    {
        return false;
    }

    if (complete_.find(shardIndex) != complete_.end())
    {
        JLOG(j_.debug()) <<
            "Shard index " << shardIndex <<
            " stored";
        return false;
    }
    if (incomplete_ && incomplete_->index() == shardIndex)
    {
        JLOG(j_.debug()) <<
            "Shard index " << shardIndex <<
            " is being acquired";
        return false;
    }
    if (preShards_.find(shardIndex) != preShards_.end())
    {
        JLOG(j_.debug()) <<
            "Shard index " << shardIndex <<
            " is prepared for import";
        return false;
    }

//检查极限和空间要求
    if (backed_)
    {
        std::uint64_t const sz {
            (preShards_.size() + 1 + (incomplete_ ? 1 : 0)) * avgShardSz_};
        if (usedDiskSpace_ + sz > maxDiskSpace_)
        {
            JLOG(j_.debug()) <<
                "Exceeds maximum size";
            return false;
        }
        if (sz > available())
        {
            JLOG(j_.error()) <<
                "Insufficient disk space";
            return false;
        }
    }

//添加到准备好的碎片中
    preShards_.emplace(shardIndex, nullptr);
    return true;
}

void
DatabaseShardImp::removePreShard(std::uint32_t shardIndex)
{
    std::lock_guard<std::mutex> l(m_);
    assert(init_);
    preShards_.erase(shardIndex);
}

std::uint32_t
DatabaseShardImp::getNumPreShard()
{
    std::lock_guard<std::mutex> l(m_);
    assert(init_);
    return preShards_.size();
}

bool
DatabaseShardImp::importShard(std::uint32_t shardIndex,
    boost::filesystem::path const& srcDir, bool validate)
{
    using namespace boost::filesystem;
    try
    {
        if (!is_directory(srcDir) || is_empty(srcDir))
        {
            JLOG(j_.error()) <<
                "Invalid source directory " << srcDir.string();
            return false;
        }
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "exception: " << e.what();
        return false;
    }

    auto move = [&](path const& src, path const& dst)
    {
        try
        {
            rename(src, dst);
        }
        catch (std::exception const& e)
        {
            JLOG(j_.error()) <<
                "exception: " << e.what();
            return false;
        }
        return true;
    };

    std::unique_lock<std::mutex> l(m_);
    assert(init_);

//检查碎片是否准备好
    auto it {preShards_.find(shardIndex)};
    if(it == preShards_.end())
    {
        JLOG(j_.error()) <<
            "Invalid shard index " << std::to_string(shardIndex);
        return false;
    }

//将源目录移动到shard数据库目录
    auto const dstDir {dir_ / std::to_string(shardIndex)};
    if (!move(srcDir, dstDir))
        return false;

//创建新碎片
    auto shard {std::make_unique<Shard>(
        *this, shardIndex, cacheSz_, cacheAge_, j_)};
    auto fail = [&](std::string msg)
    {
        JLOG(j_.error()) << msg;
        shard.release();
        move(dstDir, srcDir);
        return false;
    };

    if (!shard->open(config_, scheduler_))
        return fail("Failure");
    if (!shard->complete())
        return fail("Incomplete shard");

    try
    {
//验证数据库完整性
        shard->getBackend()->verify();
    }
    catch (std::exception const& e)
    {
        return fail(std::string("exception: ") + e.what());
    }

//验证碎片分类帐
    if (validate)
    {
//碎片验证需要释放锁
//这样数据库就可以从中获取数据
        it->second = shard.get();
        l.unlock();
        auto const valid {shard->validate(app_)};
        l.lock();
        if (!valid)
        {
            it = preShards_.find(shardIndex);
            if(it != preShards_.end())
                it->second = nullptr;
            return fail("failed validation");
        }
    }

//添加碎片
    usedDiskSpace_ += shard->fileSize();
    complete_.emplace(shardIndex, std::move(shard));
    preShards_.erase(shardIndex);
    return true;
}

std::shared_ptr<Ledger>
DatabaseShardImp::fetchLedger(uint256 const& hash, std::uint32_t seq)
{
    if (!contains(seq))
        return {};
    auto nObj = fetch(hash, seq);
    if (!nObj)
        return {};

    auto ledger = std::make_shared<Ledger>(
        InboundLedger::deserializeHeader(makeSlice(nObj->getData()), true),
            app_.config(), *app_.shardFamily());
    if (ledger->info().hash != hash || ledger->info().seq != seq)
    {
        JLOG(j_.error()) <<
            "shard " << seqToShardIndex(seq) <<
            " ledger seq " << seq <<
            " hash " << hash <<
            " has corrupt data";
        return {};
    }
    ledger->setFull();
    if (!ledger->stateMap().fetchRoot(
        SHAMapHash {ledger->info().accountHash}, nullptr))
    {
        JLOG(j_.error()) <<
            "shard " << seqToShardIndex(seq) <<
            " ledger seq " << seq <<
            " missing Account State root";
        return {};
    }
    if (ledger->info().txHash.isNonZero())
    {
        if (!ledger->txMap().fetchRoot(
            SHAMapHash {ledger->info().txHash}, nullptr))
        {
            JLOG(j_.error()) <<
                "shard " << seqToShardIndex(seq) <<
                " ledger seq " << seq <<
                " missing TX root";
            return {};
        }
    }
    return ledger;
}

void
DatabaseShardImp::setStored(std::shared_ptr<Ledger const> const& ledger)
{
    if (ledger->info().hash.isZero() ||
        ledger->info().accountHash.isZero())
    {
        assert(false);
        JLOG(j_.error()) <<
            "Invalid ledger";
        return;
    }
    auto const shardIndex {seqToShardIndex(ledger->info().seq)};
    std::lock_guard<std::mutex> l(m_);
    assert(init_);
    if (!incomplete_ || shardIndex != incomplete_->index())
    {
        JLOG(j_.warn()) <<
            "ledger seq " << ledger->info().seq <<
            " is not being acquired";
        return;
    }

    auto const before {incomplete_->fileSize()};
    if (!incomplete_->setStored(ledger))
        return;
    auto const after {incomplete_->fileSize()};
     if(after > before)
         usedDiskSpace_ += (after - before);
     else if(after < before)
         usedDiskSpace_ -= std::min(before - after, usedDiskSpace_);

    if (incomplete_->complete())
    {
        complete_.emplace(incomplete_->index(), std::move(incomplete_));
        incomplete_.reset();
        updateStats(l);

//使用新的碎片索引更新对等点
        protocol::TMShardInfo message;
        PublicKey const& publicKey {app_.nodeIdentity().first};
        message.set_nodepubkey(publicKey.data(), publicKey.size());
        message.set_shardindexes(std::to_string(shardIndex));
        app_.overlay().foreach(send_always(
            std::make_shared<Message>(message, protocol::mtSHARD_INFO)));
    }
}

bool
DatabaseShardImp::contains(std::uint32_t seq)
{
    auto const shardIndex {seqToShardIndex(seq)};
    std::lock_guard<std::mutex> l(m_);
    assert(init_);
    if (complete_.find(shardIndex) != complete_.end())
        return true;
    if (incomplete_ && incomplete_->index() == shardIndex)
        return incomplete_->contains(seq);
    return false;
}

std::string
DatabaseShardImp::getCompleteShards()
{
    std::lock_guard<std::mutex> l(m_);
    assert(init_);
    return status_;
}

void
DatabaseShardImp::validate()
{
    {
        std::lock_guard<std::mutex> l(m_);
        assert(init_);
        if (complete_.empty() && !incomplete_)
        {
            JLOG(j_.error()) <<
                "No shards to validate";
            return;
        }

        std::string s {"Found shards "};
        for (auto& e : complete_)
            s += std::to_string(e.second->index()) + ",";
        if (incomplete_)
            s += std::to_string(incomplete_->index());
        else
            s.pop_back();
        JLOG(j_.debug()) << s;
    }

    for (auto& e : complete_)
    {
        app_.shardFamily()->reset();
        e.second->validate(app_);
    }
    if (incomplete_)
    {
        app_.shardFamily()->reset();
        incomplete_->validate(app_);
    }
    app_.shardFamily()->reset();
}

void
DatabaseShardImp::import(Database& source)
{
    std::unique_lock<std::mutex> l(m_);
    assert(init_);

//只能导入应用程序本地节点存储
    if (&source != &app_.getNodeStore())
    {
        assert(false);
        JLOG(j_.error()) <<
            "Invalid source database";
        return;
    }

    std::uint32_t earliestIndex;
    std::uint32_t latestIndex;
    {
        auto loadLedger = [&](bool ascendSort = true) ->
            boost::optional<std::uint32_t>
        {
            std::shared_ptr<Ledger> ledger;
            std::uint32_t seq;
            std::tie(ledger, seq, std::ignore) = loadLedgerHelper(
                "WHERE LedgerSeq >= " + std::to_string(earliestSeq()) +
                " order by LedgerSeq " + (ascendSort ? "asc" : "desc") +
                " limit 1", app_, false);
            if (!ledger || seq == 0)
            {
                JLOG(j_.error()) <<
                    "No suitable ledgers were found in" <<
                    " the sqlite database to import";
                return boost::none;
            }
            return seq;
        };

//查找存储的最早分类帐序列
        auto seq {loadLedger()};
        if (!seq)
            return;
        earliestIndex = seqToShardIndex(*seq);

//只考虑完整的碎片
        if (seq != firstLedgerSeq(earliestIndex))
            ++earliestIndex;

//查找上次存储的分类帐序列
        seq = loadLedger(false);
        if (!seq)
            return;
        latestIndex = seqToShardIndex(*seq);

//只考虑完整的碎片
        if (seq != lastLedgerSeq(latestIndex))
            --latestIndex;

        if (latestIndex < earliestIndex)
        {
            JLOG(j_.error()) <<
                "No suitable ledgers were found in" <<
                " the sqlite database to import";
            return;
        }
    }

//导入碎片
    for (std::uint32_t shardIndex = earliestIndex;
        shardIndex <= latestIndex; ++shardIndex)
    {
        if (usedDiskSpace_ + avgShardSz_ > maxDiskSpace_)
        {
            JLOG(j_.error()) <<
                "Maximum size reached";
            canAdd_ = false;
            break;
        }
        if (avgShardSz_ > available())
        {
            JLOG(j_.error()) <<
                "Insufficient disk space";
            canAdd_ = false;
            break;
        }

//如果已存储，则跳过
        if (complete_.find(shardIndex) != complete_.end() ||
            (incomplete_ && incomplete_->index() == shardIndex))
        {
            JLOG(j_.debug()) <<
                "shard " << shardIndex <<
                " already exists";
            continue;
        }

//验证sqlite分类帐是否在节点存储区中
        {
            auto const firstSeq {firstLedgerSeq(shardIndex)};
            auto const lastSeq {std::max(firstSeq, lastLedgerSeq(shardIndex))};
            auto const numLedgers {shardIndex == earliestShardIndex()
                ? lastSeq - firstSeq + 1 : ledgersPerShard_};
            auto ledgerHashes{getHashesByIndex(firstSeq, lastSeq, app_)};
            if (ledgerHashes.size() != numLedgers)
                continue;

            bool valid {true};
            for (std::uint32_t n = firstSeq; n <= lastSeq; n += 256)
            {
                if (!source.fetch(ledgerHashes[n].first, n))
                {
                    JLOG(j_.warn()) <<
                        "SQL DB ledger sequence " << n <<
                        " mismatches node store";
                    valid = false;
                    break;
                }
            }
            if (!valid)
                continue;
        }

//创建新碎片
        app_.shardFamily()->reset();
        auto const shardDir {dir_ / std::to_string(shardIndex)};
        auto shard = std::make_unique<Shard>(
            *this, shardIndex, shardCacheSz, cacheAge_, j_);
        if (!shard->open(config_, scheduler_))
        {
            shard.reset();
            continue;
        }

//创建标记文件以表示正在导入
        auto const markerFile {shardDir / importMarker_};
        std::ofstream ofs {markerFile.string()};
        if (!ofs.is_open())
        {
            JLOG(j_.error()) <<
                "shard " << shardIndex <<
                " unable to create temp marker file";
            shard.reset();
            removeAll(shardDir, j_);
            continue;
        }
        ofs.close();

//从节点存储复制分类帐
        while (auto seq = shard->prepare())
        {
            auto ledger = loadByIndex(*seq, app_, false);
            if (!ledger || ledger->info().seq != seq ||
                !Database::copyLedger(*shard->getBackend(), *ledger,
                    nullptr, nullptr, shard->lastStored()))
                break;

            auto const before {shard->fileSize()};
            if (!shard->setStored(ledger))
                break;
            auto const after {shard->fileSize()};
            if (after > before)
                usedDiskSpace_ += (after - before);
            else if(after < before)
                usedDiskSpace_ -= std::min(before - after, usedDiskSpace_);

            if (shard->complete())
            {
                JLOG(j_.debug()) <<
                    "shard " << shardIndex <<
                    " successfully imported";
                removeAll(markerFile, j_);
                break;
            }
        }

        if (!shard->complete())
        {
            JLOG(j_.error()) <<
                "shard " << shardIndex <<
                " failed to import";
            shard.reset();
            removeAll(shardDir, j_);
        }
    }

//重新初始化碎片存储
    init_ = false;
    complete_.clear();
    incomplete_.reset();
    usedDiskSpace_ = 0;
    l.unlock();

    if (!init())
        Throw<std::runtime_error>("Failed to initialize");
}

std::int32_t
DatabaseShardImp::getWriteLoad() const
{
    std::int32_t wl {0};
    {
        std::lock_guard<std::mutex> l(m_);
        assert(init_);
        for (auto const& c : complete_)
            wl += c.second->getBackend()->getWriteLoad();
        if (incomplete_)
            wl += incomplete_->getBackend()->getWriteLoad();
    }
    return wl;
}

void
DatabaseShardImp::store(NodeObjectType type,
    Blob&& data, uint256 const& hash, std::uint32_t seq)
{
#if RIPPLE_VERIFY_NODEOBJECT_KEYS
    assert(hash == sha512Hash(makeSlice(data)));
#endif
    std::shared_ptr<NodeObject> nObj;
    auto const shardIndex {seqToShardIndex(seq)};
    {
        std::lock_guard<std::mutex> l(m_);
        assert(init_);
        if (!incomplete_ || shardIndex != incomplete_->index())
        {
            JLOG(j_.warn()) <<
               "ledger seq " << seq <<
                " is not being acquired";
            return;
        }
        nObj = NodeObject::createObject(
            type, std::move(data), hash);
        incomplete_->pCache()->canonicalize(hash, nObj, true);
        incomplete_->getBackend()->store(nObj);
        incomplete_->nCache()->erase(hash);
    }
    storeStats(nObj->getData().size());
}

std::shared_ptr<NodeObject>
DatabaseShardImp::fetch(uint256 const& hash, std::uint32_t seq)
{
    auto cache {selectCache(seq)};
    if (cache.first)
        return doFetch(hash, seq, *cache.first, *cache.second, false);
    return {};
}

bool
DatabaseShardImp::asyncFetch(uint256 const& hash,
    std::uint32_t seq, std::shared_ptr<NodeObject>& object)
{
    auto cache {selectCache(seq)};
    if (cache.first)
    {
//查看对象是否在缓存中
        object = cache.first->fetch(hash);
        if (object || cache.second->touch_if_exists(hash))
            return true;
//否则发布阅读
        Database::asyncFetch(hash, seq, cache.first, cache.second);
    }
    return false;
}

bool
DatabaseShardImp::copyLedger(std::shared_ptr<Ledger const> const& ledger)
{
    auto const shardIndex {seqToShardIndex(ledger->info().seq)};
    std::lock_guard<std::mutex> l(m_);
    assert(init_);
    if (!incomplete_ || shardIndex != incomplete_->index())
    {
        JLOG(j_.warn()) <<
            "source ledger seq " << ledger->info().seq <<
            " is not being acquired";
        return false;
    }

    if (!Database::copyLedger(*incomplete_->getBackend(), *ledger,
        incomplete_->pCache(), incomplete_->nCache(),
        incomplete_->lastStored()))
    {
        return false;
    }

    auto const before {incomplete_->fileSize()};
    if (!incomplete_->setStored(ledger))
        return false;
    auto const after {incomplete_->fileSize()};
     if(after > before)
         usedDiskSpace_ += (after - before);
     else if(after < before)
         usedDiskSpace_ -= std::min(before - after, usedDiskSpace_);

    if (incomplete_->complete())
    {
        complete_.emplace(incomplete_->index(), std::move(incomplete_));
        incomplete_.reset();
        updateStats(l);
    }
    return true;
}

int
DatabaseShardImp::getDesiredAsyncReadCount(std::uint32_t seq)
{
    auto const shardIndex {seqToShardIndex(seq)};
    {
        std::lock_guard<std::mutex> l(m_);
        assert(init_);
        auto it = complete_.find(shardIndex);
        if (it != complete_.end())
            return it->second->pCache()->getTargetSize() / asyncDivider;
        if (incomplete_ && incomplete_->index() == shardIndex)
            return incomplete_->pCache()->getTargetSize() / asyncDivider;
    }
    return cacheTargetSize / asyncDivider;
}

float
DatabaseShardImp::getCacheHitRate()
{
    float sz, f {0};
    {
        std::lock_guard<std::mutex> l(m_);
        assert(init_);
        sz = complete_.size();
        for (auto const& c : complete_)
            f += c.second->pCache()->getHitRate();
        if (incomplete_)
        {
            f += incomplete_->pCache()->getHitRate();
            ++sz;
        }
    }
    return f / std::max(1.0f, sz);
}

void
DatabaseShardImp::tune(int size, std::chrono::seconds age)
{
    std::lock_guard<std::mutex> l(m_);
    assert(init_);
    cacheSz_ = size;
    cacheAge_ = age;
    int const sz {calcTargetCacheSz(l)};
    for (auto const& c : complete_)
    {
        c.second->pCache()->setTargetSize(sz);
        c.second->pCache()->setTargetAge(cacheAge_);
        c.second->nCache()->setTargetSize(sz);
        c.second->nCache()->setTargetAge(cacheAge_);
    }
    if (incomplete_)
    {
        incomplete_->pCache()->setTargetSize(sz);
        incomplete_->pCache()->setTargetAge(cacheAge_);
        incomplete_->nCache()->setTargetSize(sz);
        incomplete_->nCache()->setTargetAge(cacheAge_);
    }
}

void
DatabaseShardImp::sweep()
{
    std::lock_guard<std::mutex> l(m_);
    assert(init_);
    int const sz {calcTargetCacheSz(l)};
    for (auto const& c : complete_)
    {
        c.second->pCache()->sweep();
        c.second->nCache()->sweep();
        if (c.second->pCache()->getTargetSize() > sz)
            c.second->pCache()->setTargetSize(sz);
    }
    if (incomplete_)
    {
        incomplete_->pCache()->sweep();
        incomplete_->nCache()->sweep();
        if (incomplete_->pCache()->getTargetSize() > sz)
            incomplete_->pCache()->setTargetSize(sz);
    }
}

std::shared_ptr<NodeObject>
DatabaseShardImp::fetchFrom(uint256 const& hash, std::uint32_t seq)
{
    auto const shardIndex {seqToShardIndex(seq)};
    std::unique_lock<std::mutex> l(m_);
    assert(init_);
    {
        auto it = complete_.find(shardIndex);
        if (it != complete_.end())
        {
            l.unlock();
            return fetchInternal(hash, *it->second->getBackend());
        }
    }
    if (incomplete_ && incomplete_->index() == shardIndex)
    {
        l.unlock();
        return fetchInternal(hash, *incomplete_->getBackend());
    }

//用于验证导入碎片
    auto it = preShards_.find(shardIndex);
    if (it != preShards_.end() && it->second)
    {
        l.unlock();
        return fetchInternal(hash, *it->second->getBackend());
    }
    return {};
}

boost::optional<std::uint32_t>
DatabaseShardImp::findShardIndexToAdd(
    std::uint32_t validLedgerSeq, std::lock_guard<std::mutex>&)
{
    auto maxShardIndex {seqToShardIndex(validLedgerSeq)};
    if (validLedgerSeq != lastLedgerSeq(maxShardIndex))
        --maxShardIndex;

    auto const numShards {complete_.size() + (incomplete_ ? 1 : 0)};
//如果相等，所有碎片
    if (numShards >= maxShardIndex + 1)
        return boost::none;

    if (maxShardIndex < 1024 || float(numShards) / maxShardIndex > 0.5f)
    {
//要采样的索引空间很小或大部分是满索引空间
//找到可用的索引并随机选择一个
        std::vector<std::uint32_t> available;
        available.reserve(maxShardIndex - numShards + 1);
        for (std::uint32_t i = earliestShardIndex(); i <= maxShardIndex; ++i)
        {
            if (complete_.find(i) == complete_.end() &&
                (!incomplete_ || incomplete_->index() != i) &&
                preShards_.find(i) == preShards_.end())
                    available.push_back(i);
        }
        if (!available.empty())
            return available[rand_int(0u,
                static_cast<std::uint32_t>(available.size() - 1))];
    }

//要采样的大型稀疏索引空间
//一直随机选择索引，直到找到可用的索引为止
//跑30次以上的机会不足十亿分之一
    for (int i = 0; i < 40; ++i)
    {
        auto const r {rand_int(earliestShardIndex(), maxShardIndex)};
        if (complete_.find(r) == complete_.end() &&
            (!incomplete_ || incomplete_->index() != r) &&
            preShards_.find(r) == preShards_.end())
                return r;
    }
    assert(0);
    return boost::none;
}

void
DatabaseShardImp::updateStats(std::lock_guard<std::mutex>&)
{
//计算碎片文件大小和更新状态字符串
    std::uint32_t filesPerShard {0};
    if (!complete_.empty())
    {
        status_.clear();
        filesPerShard = complete_.begin()->second->fdlimit();
        std::uint64_t avgShardSz {0};
        for (auto it = complete_.begin(); it != complete_.end(); ++it)
        {
            if (it == complete_.begin())
                status_ = std::to_string(it->first);
            else
            {
                if (it->first - std::prev(it)->first > 1)
                {
                    if (status_.back() == '-')
                        status_ += std::to_string(std::prev(it)->first);
                    status_ += "," + std::to_string(it->first);
                }
                else
                {
                    if (status_.back() != '-')
                        status_ += "-";
                    if (std::next(it) == complete_.end())
                        status_ += std::to_string(it->first);
                }
            }
            avgShardSz += it->second->fileSize();
        }
        if (backed_)
            avgShardSz_ = avgShardSz / complete_.size();
    }
    else if(incomplete_)
        filesPerShard = incomplete_->fdlimit();
    if (!backed_)
        return;

    fdLimit_ = 1 + (filesPerShard *
        (complete_.size() + (incomplete_ ? 1 : 0)));

    if (usedDiskSpace_ >= maxDiskSpace_)
    {
        JLOG(j_.warn()) <<
            "Maximum size reached";
        canAdd_ = false;
    }
    else
    {
        auto const sz = maxDiskSpace_ - usedDiskSpace_;
        if (sz > available())
        {
            JLOG(j_.warn()) <<
                "Max Shard Store size exceeds "
                "remaining free disk space";
        }
        fdLimit_ += (filesPerShard * (sz / avgShardSz_));
    }
}

std::pair<std::shared_ptr<PCache>, std::shared_ptr<NCache>>
DatabaseShardImp::selectCache(std::uint32_t seq)
{
    auto const shardIndex {seqToShardIndex(seq)};
    std::lock_guard<std::mutex> l(m_);
    assert(init_);
    {
        auto it = complete_.find(shardIndex);
        if (it != complete_.end())
        {
            return std::make_pair(it->second->pCache(),
                it->second->nCache());
        }
    }
    if (incomplete_ && incomplete_->index() == shardIndex)
    {
        return std::make_pair(incomplete_->pCache(),
            incomplete_->nCache());
    }

//用于验证导入碎片
    auto it = preShards_.find(shardIndex);
    if (it != preShards_.end() && it->second)
    {
        return std::make_pair(it->second->pCache(),
            it->second->nCache());
    }
    return {};
}

std::uint64_t
DatabaseShardImp::available() const
{
    try
    {
        return boost::filesystem::space(dir_).available;
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "exception: " << e.what();
        return 0;
    }
}

} //节点存储
} //涟漪
