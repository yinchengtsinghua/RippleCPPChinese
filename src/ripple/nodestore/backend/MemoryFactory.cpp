
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
#include <boost/beast/core/string.hpp>
#include <boost/core/ignore_unused.hpp>
#include <map>
#include <memory>
#include <mutex>

namespace ripple {
namespace NodeStore {

struct MemoryDB
{
    explicit MemoryDB() = default;

    std::mutex mutex;
    bool open = false;
    std::map <uint256 const, std::shared_ptr<NodeObject>> table;
};

class MemoryFactory : public Factory
{
private:
    std::mutex mutex_;
    std::map <std::string, MemoryDB, boost::beast::iless> map_;

public:
    MemoryFactory();
    ~MemoryFactory() override;

    std::string
    getName() const override;

    std::unique_ptr <Backend>
    createInstance (
        size_t keyBytes,
        Section const& keyValues,
        Scheduler& scheduler,
        beast::Journal journal) override;

    MemoryDB&
    open (std::string const& path)
    {
        std::lock_guard<std::mutex> _(mutex_);
        auto const result = map_.emplace (std::piecewise_construct,
            std::make_tuple(path), std::make_tuple());
        MemoryDB& db = result.first->second;
        if (db.open)
            Throw<std::runtime_error> ("already open");
        return db;
    }
};

static MemoryFactory memoryFactory;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class MemoryBackend : public Backend
{
private:
    using Map = std::map <uint256 const, std::shared_ptr<NodeObject>>;

    std::string name_;
    beast::Journal journal_;
    MemoryDB* db_ {nullptr};

public:
    MemoryBackend (size_t keyBytes, Section const& keyValues,
        Scheduler& scheduler, beast::Journal journal)
        : name_ (get<std::string>(keyValues, "path"))
        , journal_ (journal)
    {
boost::ignore_unused (journal_); //保留未使用的日志，以防万一。
        if (name_.empty())
            Throw<std::runtime_error> ("Missing path in Memory backend");
    }

    ~MemoryBackend () override
    {
        close();
    }

    std::string
    getName () override
    {
        return name_;
    }

    void
    open(bool createIfMissing) override
    {
        db_ = &memoryFactory.open(name_);
    }

    void
    close() override
    {
        db_ = nullptr;
    }

//——————————————————————————————————————————————————————————————

    Status
    fetch (void const* key, std::shared_ptr<NodeObject>* pObject) override
    {
        assert(db_);
        uint256 const hash (uint256::fromVoid (key));

        std::lock_guard<std::mutex> _(db_->mutex);

        Map::iterator iter = db_->table.find (hash);
        if (iter == db_->table.end())
        {
            pObject->reset();
            return notFound;
        }
        *pObject = iter->second;
        return ok;
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
    store (std::shared_ptr<NodeObject> const& object) override
    {
        assert(db_);
        std::lock_guard<std::mutex> _(db_->mutex);
        db_->table.emplace (object->getHash(), object);
    }

    void
    storeBatch (Batch const& batch) override
    {
        for (auto const& e : batch)
            store (e);
    }

    void
    for_each (std::function <void(std::shared_ptr<NodeObject>)> f) override
    {
        assert(db_);
        for (auto const& e : db_->table)
            f (e.second);
    }

    int
    getWriteLoad() override
    {
        return 0;
    }

    void
    setDeletePath() override
    {
    }

    void
    verify() override
    {
    }

    int
    fdlimit() const override
    {
        return 0;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

MemoryFactory::MemoryFactory()
{
    Manager::instance().insert(*this);
}

MemoryFactory::~MemoryFactory()
{
    Manager::instance().erase(*this);
}

std::string
MemoryFactory::getName() const
{
    return "Memory";
}

std::unique_ptr <Backend>
MemoryFactory::createInstance (
    size_t keyBytes,
    Section const& keyValues,
    Scheduler& scheduler,
    beast::Journal journal)
{
    return std::make_unique <MemoryBackend> (
        keyBytes, keyValues, scheduler, journal);
}

}
}
