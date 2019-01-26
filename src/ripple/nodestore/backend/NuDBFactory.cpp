
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


#include <ripple/basics/contract.h>
#include <ripple/nodestore/Factory.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/nodestore/impl/codec.h>
#include <ripple/nodestore/impl/DecodedBlob.h>
#include <ripple/nodestore/impl/EncodedBlob.h>
#include <nudb/nudb.hpp>
#include <boost/filesystem.hpp>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <exception>
#include <memory>

namespace ripple {
namespace NodeStore {

class NuDBBackend
    : public Backend
{
public:
//这需要调整为
//数据大小的分布。
    static constexpr std::size_t arena_alloc_size = megabytes(16);
    static constexpr std::size_t currentType = 1;

    beast::Journal j_;
    size_t const keyBytes_;
    std::string const name_;
    nudb::store db_;
    std::atomic <bool> deletePath_;
    Scheduler& scheduler_;

    NuDBBackend (int keyBytes, Section const& keyValues,
        Scheduler& scheduler, beast::Journal journal)
        : j_(journal)
        , keyBytes_ (keyBytes)
        , name_ (get<std::string>(keyValues, "path"))
        , deletePath_(false)
        , scheduler_ (scheduler)
    {
        if (name_.empty())
            Throw<std::runtime_error> (
                "nodestore: Missing path in NuDB backend");
    }

    ~NuDBBackend () override
    {
        close();
    }

    std::string
    getName() override
    {
        return name_;
    }

    void
    open(bool createIfMissing) override
    {
        using namespace boost::filesystem;
        if (db_.is_open())
        {
            assert(false);
            JLOG(j_.error()) <<
                "database is already open";
            return;
        }
        auto const folder = path(name_);
        auto const dp = (folder / "nudb.dat").string();
        auto const kp = (folder / "nudb.key").string();
        auto const lp = (folder / "nudb.log").string();
        nudb::error_code ec;
        if (createIfMissing)
        {
            create_directories(folder);
            nudb::create<nudb::xxhasher>(dp, kp, lp,
                currentType, nudb::make_salt(), keyBytes_,
                    nudb::block_size(kp), 0.50, ec);
            if(ec == nudb::errc::file_exists)
                ec = {};
            if(ec)
                Throw<nudb::system_error>(ec);
        }
        db_.open (dp, kp, lp, ec);
        if(ec)
            Throw<nudb::system_error>(ec);
        if (db_.appnum() != currentType)
            Throw<std::runtime_error>(
                "nodestore: unknown appnum");
    }

    void
    close() override
    {
        if (db_.is_open())
        {
            nudb::error_code ec;
            db_.close(ec);
            if(ec)
                Throw<nudb::system_error>(ec);
            if (deletePath_)
            {
                boost::filesystem::remove_all (name_);
            }
        }
    }

    Status
    fetch (void const* key, std::shared_ptr<NodeObject>* pno) override
    {
        Status status;
        pno->reset();
        nudb::error_code ec;
        db_.fetch (key,
            [key, pno, &status](void const* data, std::size_t size)
            {
                nudb::detail::buffer bf;
                auto const result =
                    nodeobject_decompress(data, size, bf);
                DecodedBlob decoded (key, result.first, result.second);
                if (! decoded.wasOk ())
                {
                    status = dataCorrupt;
                    return;
                }
                *pno = decoded.createObject();
                status = ok;
            }, ec);
        if(ec == nudb::error::key_not_found)
            return notFound;
        if(ec)
            Throw<nudb::system_error>(ec);
        return status;
    }

    bool
    canFetchBatch() override
    {
        return false;
    }

    std::vector<std::shared_ptr<NodeObject>>
    fetchBatch (std::size_t n, void const* const* keys) override
    {
        Throw<std::runtime_error> ("pure virtual called");
        return {};
    }

    void
    do_insert (std::shared_ptr <NodeObject> const& no)
    {
        EncodedBlob e;
        e.prepare (no);
        nudb::error_code ec;
        nudb::detail::buffer bf;
        auto const result = nodeobject_compress(
            e.getData(), e.getSize(), bf);
        db_.insert (e.getKey(), result.first, result.second, ec);
        if(ec && ec != nudb::error::key_exists)
            Throw<nudb::system_error>(ec);
    }

    void
    store (std::shared_ptr <NodeObject> const& no) override
    {
        BatchWriteReport report;
        report.writeCount = 1;
        auto const start =
            std::chrono::steady_clock::now();
        do_insert (no);
        report.elapsed = std::chrono::duration_cast <
            std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);
        scheduler_.onBatchWrite (report);
    }

    void
    storeBatch (Batch const& batch) override
    {
        BatchWriteReport report;
        report.writeCount = batch.size();
        auto const start =
            std::chrono::steady_clock::now();
        for (auto const& e : batch)
            do_insert (e);
        report.elapsed = std::chrono::duration_cast <
            std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);
        scheduler_.onBatchWrite (report);
    }

    void
    for_each (std::function <void(std::shared_ptr<NodeObject>)> f) override
    {
        auto const dp = db_.dat_path();
        auto const kp = db_.key_path();
        auto const lp = db_.log_path();
//auto const appum=db_.appum（）；
        nudb::error_code ec;
        db_.close(ec);
        if(ec)
            Throw<nudb::system_error>(ec);
        nudb::visit(dp,
            [&](
                void const* key, std::size_t key_bytes,
                void const* data, std::size_t size,
                nudb::error_code&)
            {
                nudb::detail::buffer bf;
                auto const result =
                    nodeobject_decompress(data, size, bf);
                DecodedBlob decoded (key, result.first, result.second);
                if (! decoded.wasOk ())
                {
                    ec = make_error_code(nudb::error::missing_value);
                    return;
                }
                f (decoded.createObject());
            }, nudb::no_progress{}, ec);
        if(ec)
            Throw<nudb::system_error>(ec);
        db_.open(dp, kp, lp, ec);
        if(ec)
            Throw<nudb::system_error>(ec);
    }

    int
    getWriteLoad () override
    {
        return 0;
    }

    void
    setDeletePath() override
    {
        deletePath_ = true;
    }

    void
    verify() override
    {
        auto const dp = db_.dat_path();
        auto const kp = db_.key_path();
        auto const lp = db_.log_path();
        nudb::error_code ec;
        db_.close(ec);
        if(ec)
            Throw<nudb::system_error>(ec);
        nudb::verify_info vi;
        nudb::verify<nudb::xxhasher>(
            vi, dp, kp, 0, nudb::no_progress{}, ec);
        if(ec)
            Throw<nudb::system_error>(ec);
        db_.open (dp, kp, lp, ec);
        if(ec)
            Throw<nudb::system_error>(ec);
    }

    /*返回后端需要的文件句柄数*/
    int
    fdlimit() const override
    {
        return 3;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class NuDBFactory : public Factory
{
public:
    NuDBFactory()
    {
        Manager::instance().insert(*this);
    }

    ~NuDBFactory() override
    {
        Manager::instance().erase(*this);
    }

    std::string
    getName() const override
    {
        return "NuDB";
    }

    std::unique_ptr <Backend>
    createInstance (
        size_t keyBytes,
        Section const& keyValues,
        Scheduler& scheduler,
        beast::Journal journal) override
    {
        return std::make_unique <NuDBBackend> (
            keyBytes, keyValues, scheduler, journal);
    }
};

static NuDBFactory nuDBFactory;

}
}
