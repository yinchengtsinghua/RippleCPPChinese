
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

#include <ripple/nodestore/impl/Shard.h>
#include <ripple/app/ledger/InboundLedger.h>
#include <ripple/nodestore/impl/DatabaseShardImp.h>
#include <ripple/nodestore/Manager.h>

#include <fstream>

namespace ripple {
namespace NodeStore {

Shard::Shard(DatabaseShard const& db, std::uint32_t index,
    int cacheSz, std::chrono::seconds cacheAge, beast::Journal& j)
    : index_(index)
    , firstSeq_(db.firstLedgerSeq(index))
    , lastSeq_(std::max(firstSeq_, db.lastLedgerSeq(index)))
    , maxLedgers_(index == db.earliestShardIndex() ?
        lastSeq_ - firstSeq_ + 1 : db.ledgersPerShard())
    , pCache_(std::make_shared<PCache>(
        "shard " + std::to_string(index_),
        cacheSz, cacheAge, stopwatch(), j))
    , nCache_(std::make_shared<NCache>(
        "shard " + std::to_string(index_),
        stopwatch(), cacheSz, cacheAge))
    , dir_(db.getRootDir() / std::to_string(index_))
    , control_(dir_ / controlFileName)
    , j_(j)
{
    if (index_ < db.earliestShardIndex())
        Throw<std::runtime_error>("Shard: Invalid index");
}

bool
Shard::open(Section config, Scheduler& scheduler)
{
    assert(!backend_);
    using namespace boost::filesystem;

    bool dirPreexist;
    bool dirEmpty;
    try
    {
        if (!exists(dir_))
        {
            dirPreexist = false;
            dirEmpty = true;
        }
        else if (is_directory(dir_))
        {
            dirPreexist = true;
            dirEmpty = is_empty(dir_);
        }
        else
        {
             JLOG(j_.error()) <<
                "path exists as file: " << dir_.string();
            return false;
        }
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "shard " + std::to_string(index_) + " exception: " + e.what();
        return false;
    }

    auto fail = [&](std::string msg)
    {
        JLOG(j_.error()) <<
            "shard " << std::to_string(index_) << " error: " << msg;

        if (!dirPreexist)
            removeAll(dir_, j_);
        else if (dirEmpty)
        {
            for (auto const& p : recursive_directory_iterator(dir_))
                removeAll(p.path(), j_);
        }
        return false;
    };

    config.set("path", dir_.string());
    try
    {
        backend_ = Manager::instance().make_Backend(
            config, scheduler, j_);
        backend_->open(!dirPreexist || dirEmpty);

        if (backend_->fdlimit() == 0)
            return true;

        if (!dirPreexist || dirEmpty)
        {
//新建碎片，创建控制文件
            if (!saveControl())
                return fail("failure");
        }
        else if (is_regular_file(control_))
        {
//碎片不完整，检查控制文件
            std::ifstream ifs(control_.string());
            if (!ifs.is_open())
            {
                return fail("shard " + std::to_string(index_) +
                    ", unable to open control file");
            }

            boost::archive::text_iarchive ar(ifs);
            ar & storedSeqs_;
            if (!storedSeqs_.empty())
            {
                if (boost::icl::first(storedSeqs_) < firstSeq_ ||
                    boost::icl::last(storedSeqs_) > lastSeq_)
                {
                    return fail("shard " + std::to_string(index_) +
                        ": Invalid control file");
                }

                if (boost::icl::length(storedSeqs_) >= maxLedgers_)
                {
                    JLOG(j_.error()) <<
                        "shard " << index_ <<
                        " found control file for complete shard";
                    storedSeqs_.clear();
                    complete_ = true;
                    remove_all(control_);
                }
            }
        }
        else
            complete_ = true;

//计算后端文件的文件底纹
        for (auto const& p : recursive_directory_iterator(dir_))
            if (!is_directory(p))
                fileSize_ += file_size(p);
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "shard " << std::to_string(index_) << " error: " << e.what();
        return false;
    }

    return true;
}

bool
Shard::setStored(std::shared_ptr<Ledger const> const& l)
{
    assert(backend_&& !complete_);
    if (boost::icl::contains(storedSeqs_, l->info().seq))
    {
        JLOG(j_.debug()) <<
            "shard " << index_ <<
            " ledger seq " << l->info().seq <<
            " already stored";
        return false;
    }
    if (boost::icl::length(storedSeqs_) >= maxLedgers_ - 1)
    {
        if (backend_->fdlimit() != 0)
        {
            if (!removeAll(control_, j_))
                return false;

//更新后端文件的文件底纹
            using namespace boost::filesystem;
            std::uint64_t sz {0};
            try
            {
                for (auto const& p : recursive_directory_iterator(dir_))
                    if (!is_directory(p))
                        sz += file_size(p);
            }
            catch (const filesystem_error& e)
            {
                JLOG(j_.error()) <<
                    "exception: " << e.what();
                fileSize_ = std::max(fileSize_, sz);
                return false;
            }
            fileSize_ = sz;
        }
        complete_ = true;
        storedSeqs_.clear();

        JLOG(j_.debug()) <<
            "shard " << index_ <<
            " ledger seq " << l->info().seq <<
            " stored. Shard complete";
    }
    else
    {
        storedSeqs_.insert(l->info().seq);
        lastStored_ = l;
        if (backend_->fdlimit() != 0 && !saveControl())
            return false;

        JLOG(j_.debug()) <<
            "shard " << index_ <<
            " ledger seq " << l->info().seq <<
            " stored";
    }

    return true;
}

boost::optional<std::uint32_t>
Shard::prepare()
{
    if (storedSeqs_.empty())
         return lastSeq_;
    return prevMissing(storedSeqs_, 1 + lastSeq_, firstSeq_);
}

bool
Shard::contains(std::uint32_t seq) const
{
    if (seq < firstSeq_ || seq > lastSeq_)
        return false;
    if (complete_)
        return true;
    return boost::icl::contains(storedSeqs_, seq);
}

bool
Shard::validate(Application& app)
{
    uint256 hash;
    std::uint32_t seq;
    std::shared_ptr<Ledger> l;
//查找此碎片中最后一个分类帐的哈希
    {
        std::tie(l, seq, hash) = loadLedgerHelper(
            "WHERE LedgerSeq >= " + std::to_string(lastSeq_) +
            " order by LedgerSeq desc limit 1", app, false);
        if (!l)
        {
            JLOG(j_.error()) <<
                "shard " << index_ <<
                " unable to validate. No lookup data";
            return false;
        }
        if (seq != lastSeq_)
        {
            l->setImmutable(app.config());
            boost::optional<uint256> h;
            try
            {
                h = hashOfSeq(*l, lastSeq_, j_);
            }
            catch (std::exception const& e)
            {
                JLOG(j_.error()) <<
                    "exception: " << e.what();
                return false;
            }
            if (!h)
            {
                JLOG(j_.error()) <<
                    "shard " << index_ <<
                    " No hash for last ledger seq " << lastSeq_;
                return false;
            }
            hash = *h;
            seq = lastSeq_;
        }
    }

    JLOG(j_.debug()) <<
        "Validating shard " << index_ <<
        " ledgers " << firstSeq_ <<
        "-" << lastSeq_;

//使用短时间保持低内存消耗
    auto const savedAge {pCache_->getTargetAge()};
    using namespace std::chrono_literals;
    pCache_->setTargetAge(1s);

//验证此碎片中存储的每个分类帐
    std::shared_ptr<Ledger const> next;
    while (seq >= firstSeq_)
    {
        auto nObj = valFetch(hash);
        if (!nObj)
            break;
        l = std::make_shared<Ledger>(
            InboundLedger::deserializeHeader(makeSlice(nObj->getData()),
                true), app.config(), *app.shardFamily());
        if (l->info().hash != hash || l->info().seq != seq)
        {
            JLOG(j_.error()) <<
                "ledger seq " << seq <<
                " hash " << hash <<
                " cannot be a ledger";
            break;
        }
        l->stateMap().setLedgerSeq(seq);
        l->txMap().setLedgerSeq(seq);
        l->setImmutable(app.config());
        if (!l->stateMap().fetchRoot(
            SHAMapHash {l->info().accountHash}, nullptr))
        {
            JLOG(j_.error()) <<
                "ledger seq " << seq <<
                " missing Account State root";
            break;
        }
        if (l->info().txHash.isNonZero())
        {
            if (!l->txMap().fetchRoot(
                SHAMapHash {l->info().txHash}, nullptr))
            {
                JLOG(j_.error()) <<
                    "ledger seq " << seq <<
                    " missing TX root";
                break;
            }
        }
        if (!valLedger(l, next))
            break;
        hash = l->info().parentHash;
        --seq;
        next = l;
        if (seq % 128 == 0)
            pCache_->sweep();
    }

    pCache_->reset();
    nCache_->reset();
    pCache_->setTargetAge(savedAge);

    if (seq >= firstSeq_)
    {
        JLOG(j_.error()) <<
            "shard " << index_ <<
            (complete_ ? " is invalid, failed" : " is incomplete, stopped") <<
            " at seq " << seq <<
            " hash " << hash;
        return false;
    }

    JLOG(j_.debug()) <<
        "shard " << index_ <<
        " is complete.";
    return true;
}

bool
Shard::valLedger(std::shared_ptr<Ledger const> const& l,
    std::shared_ptr<Ledger const> const& next)
{
    if (l->info().hash.isZero() || l->info().accountHash.isZero())
    {
        JLOG(j_.error()) <<
            "invalid ledger";
        return false;
    }
    bool error {false};
    auto f = [&, this](SHAMapAbstractNode& node) {
        if (!valFetch(node.getNodeHash().as_uint256()))
            error = true;
        return !error;
    };
//验证状态映射
    if (l->stateMap().getHash().isNonZero())
    {
        if (!l->stateMap().isValid())
        {
            JLOG(j_.error()) <<
                "invalid state map";
            return false;
        }
        try
        {
            if (next && next->info().parentHash == l->info().hash)
                l->stateMap().visitDifferences(&next->stateMap(), f);
            else
                l->stateMap().visitNodes(f);
        }
        catch (std::exception const& e)
        {
            JLOG(j_.error()) <<
                "exception: " << e.what();
            return false;
        }
        if (error)
            return false;
    }
//验证Tx映射
    if (l->info().txHash.isNonZero())
    {
        if (!l->txMap().isValid())
        {
            JLOG(j_.error()) <<
                "invalid transaction map";
            return false;
        }
        try
        {
            l->txMap().visitNodes(f);
        }
        catch (std::exception const& e)
        {
            JLOG(j_.error()) <<
                "exception: " << e.what();
            return false;
        }
        if (error)
            return false;
    }
    return true;
};

std::shared_ptr<NodeObject>
Shard::valFetch(uint256 const& hash)
{
    assert(backend_);
    std::shared_ptr<NodeObject> nObj;
    try
    {
        switch (backend_->fetch(hash.begin(), &nObj))
        {
        case ok:
            break;
        case notFound:
        {
            JLOG(j_.error()) <<
                "NodeObject not found. hash " << hash;
            break;
        }
        case dataCorrupt:
        {
            JLOG(j_.error()) <<
                "NodeObject is corrupt. hash " << hash;
            break;
        }
        default:
        {
            JLOG(j_.error()) <<
                "unknown error. hash " << hash;
        }
        }
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "exception: " << e.what();
    }
    return nObj;
}

bool
Shard::saveControl()
{
    std::ofstream ofs {control_.string(), std::ios::trunc};
    if (!ofs.is_open())
    {
        JLOG(j_.fatal()) <<
            "shard " << index_ <<
            " unable to save control file";
        return false;
    }
    boost::archive::text_oarchive ar(ofs);
    ar & storedSeqs_;
    return true;
}

} //节点存储
} //涟漪
