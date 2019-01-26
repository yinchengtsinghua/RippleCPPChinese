
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


#include <ripple/unity/rocksdb.h>

#if RIPPLE_ROCKSDB_AVAILABLE

#include <ripple/basics/contract.h>
#include <ripple/basics/ByteUtilities.h>
#include <ripple/core/Config.h> //vfalc不良依赖
#include <ripple/nodestore/Factory.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/nodestore/impl/BatchWriter.h>
#include <ripple/nodestore/impl/DecodedBlob.h>
#include <ripple/nodestore/impl/EncodedBlob.h>
#include <ripple/beast/core/CurrentThreadName.h>
#include <atomic>
#include <memory>

namespace ripple {
namespace NodeStore {

class RocksDBEnv : public rocksdb::EnvWrapper
{
public:
    RocksDBEnv ()
        : EnvWrapper (rocksdb::Env::Default())
    {
    }

    struct ThreadParams
    {
        ThreadParams (void (*f_)(void*), void* a_)
            : f (f_)
            , a (a_)
        {
        }

        void (*f)(void*);
        void* a;
    };

    static
    void
    thread_entry (void* ptr)
    {
        ThreadParams* const p (reinterpret_cast <ThreadParams*> (ptr));
        void (*f)(void*) = p->f;
        void* a (p->a);
        delete p;

        static std::atomic <std::size_t> n;
        std::size_t const id (++n);
        std::stringstream ss;
        ss << "rocksdb #" << id;
        beast::setCurrentThreadName (ss.str());

        (*f)(a);
    }

    void
    StartThread (void (*f)(void*), void* a) override
    {
        ThreadParams* const p (new ThreadParams (f, a));
        EnvWrapper::StartThread (&RocksDBEnv::thread_entry, p);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class RocksDBBackend
    : public Backend
    , public BatchWriter::Callback
{
private:
    std::atomic <bool> m_deletePath;

public:
    beast::Journal m_journal;
    size_t const m_keyBytes;
    Scheduler& m_scheduler;
    BatchWriter m_batch;
    std::string m_name;
    std::unique_ptr <rocksdb::DB> m_db;
    int fdlimit_ = 2048;
    rocksdb::Options m_options;

    RocksDBBackend (int keyBytes, Section const& keyValues,
        Scheduler& scheduler, beast::Journal journal, RocksDBEnv* env)
        : m_deletePath (false)
        , m_journal (journal)
        , m_keyBytes (keyBytes)
        , m_scheduler (scheduler)
        , m_batch (*this, scheduler)
    {
        if (! get_if_exists(keyValues, "path", m_name))
            Throw<std::runtime_error> ("Missing path in RocksDBFactory backend");

        rocksdb::BlockBasedTableOptions table_options;
        m_options.env = env;

        if (keyValues.exists ("cache_mb"))
            table_options.block_cache = rocksdb::NewLRUCache (
                get<int>(keyValues, "cache_mb") * megabytes(1));

        if (auto const v = get<int>(keyValues, "filter_bits"))
        {
            bool const filter_blocks = !keyValues.exists ("filter_full") ||
                (get<int>(keyValues, "filter_full") == 0);
            table_options.filter_policy.reset (rocksdb::NewBloomFilterPolicy (v, filter_blocks));
        }

        if (get_if_exists (keyValues, "open_files", m_options.max_open_files))
            fdlimit_ = m_options.max_open_files;

        if (keyValues.exists ("file_size_mb"))
        {
            m_options.target_file_size_base = megabytes(1) * get<int>(keyValues,"file_size_mb");
            m_options.max_bytes_for_level_base = 5 * m_options.target_file_size_base;
            m_options.write_buffer_size = 2 * m_options.target_file_size_base;
        }

        get_if_exists (keyValues, "file_size_mult", m_options.target_file_size_multiplier);

        if (keyValues.exists ("bg_threads"))
        {
            m_options.env->SetBackgroundThreads
                (get<int>(keyValues, "bg_threads"), rocksdb::Env::LOW);
        }

        if (keyValues.exists ("high_threads"))
        {
            auto const highThreads = get<int>(keyValues, "high_threads");
            m_options.env->SetBackgroundThreads (highThreads, rocksdb::Env::HIGH);

//如果我们有高优先级的线程，我们可能希望
//使用它们进行背景刷新
            if (highThreads > 0)
                m_options.max_background_flushes = highThreads;
        }

        if (keyValues.exists ("compression") &&
            (get<int>(keyValues, "compression") == 0))
        {
            m_options.compression = rocksdb::kNoCompression;
        }

        get_if_exists (keyValues, "block_size", table_options.block_size);

        if (keyValues.exists ("universal_compaction") &&
            (get<int>(keyValues, "universal_compaction") != 0))
        {
            m_options.compaction_style = rocksdb::kCompactionStyleUniversal;
            m_options.min_write_buffer_number_to_merge = 2;
            m_options.max_write_buffer_number = 6;
            m_options.write_buffer_size = 6 * m_options.target_file_size_base;
        }

        if (keyValues.exists("bbt_options"))
        {
            auto const s = rocksdb::GetBlockBasedTableOptionsFromString(
                table_options,
                get<std::string>(keyValues, "bbt_options"),
                &table_options);
            if (! s.ok())
                Throw<std::runtime_error> (
                    std::string("Unable to set RocksDB bbt_options: ") + s.ToString());
        }

        m_options.table_factory.reset(NewBlockBasedTableFactory(table_options));

        if (keyValues.exists("options"))
        {
            auto const s = rocksdb::GetOptionsFromString(
                m_options, get<std::string>(keyValues, "options"), &m_options);
            if (! s.ok())
                Throw<std::runtime_error> (
                    std::string("Unable to set RocksDB options: ") + s.ToString());
        }

        std::string s1, s2;
        rocksdb::GetStringFromDBOptions(&s1, m_options, "; ");
        rocksdb::GetStringFromColumnFamilyOptions(&s2, m_options, "; ");
        JLOG(m_journal.debug()) << "RocksDB DBOptions: " << s1;
        JLOG(m_journal.debug()) << "RocksDB CFOptions: " << s2;
    }

    ~RocksDBBackend () override
    {
        close();
    }

    void
    open(bool createIfMissing) override
    {
        if (m_db)
        {
            assert(false);
            JLOG(m_journal.error()) <<
                "database is already open";
            return;
        }
        rocksdb::DB* db = nullptr;
        m_options.create_if_missing = createIfMissing;
        rocksdb::Status status = rocksdb::DB::Open(m_options, m_name, &db);
        if (!status.ok() || !db)
            Throw<std::runtime_error>(
                std::string("Unable to open/create RocksDB: ") +
                status.ToString());
        m_db.reset(db);
    }

    void
    close() override
    {
        if (m_db)
        {
            m_db.reset();
            if (m_deletePath)
            {
                boost::filesystem::path dir = m_name;
                boost::filesystem::remove_all (dir);
            }
        }
    }

    std::string
    getName() override
    {
        return m_name;
    }

//——————————————————————————————————————————————————————————————

    Status
    fetch (void const* key, std::shared_ptr<NodeObject>* pObject) override
    {
        assert(m_db);
        pObject->reset ();

        Status status (ok);

        rocksdb::ReadOptions const options;
        rocksdb::Slice const slice (static_cast <char const*> (key), m_keyBytes);

        std::string string;

        rocksdb::Status getStatus = m_db->Get (options, slice, &string);

        if (getStatus.ok ())
        {
            DecodedBlob decoded (key, string.data (), string.size ());

            if (decoded.wasOk ())
            {
                *pObject = decoded.createObject ();
            }
            else
            {
//解码失败，可能已损坏！
//
                status = dataCorrupt;
            }
        }
        else
        {
            if (getStatus.IsCorruption ())
            {
                status = dataCorrupt;
            }
            else if (getStatus.IsNotFound ())
            {
                status = notFound;
            }
            else
            {
                status = Status (customCode + getStatus.code());

                JLOG(m_journal.error()) << getStatus.ToString ();
            }
        }

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
    store (std::shared_ptr<NodeObject> const& object) override
    {
        m_batch.store (object);
    }

    void
    storeBatch (Batch const& batch) override
    {
        assert(m_db);
        rocksdb::WriteBatch wb;

        EncodedBlob encoded;

        for (auto const& e : batch)
        {
            encoded.prepare (e);

            wb.Put (
                rocksdb::Slice (reinterpret_cast <char const*> (
                    encoded.getKey ()), m_keyBytes),
                rocksdb::Slice (reinterpret_cast <char const*> (
                    encoded.getData ()), encoded.getSize ()));
        }

        rocksdb::WriteOptions const options;

        auto ret = m_db->Write (options, &wb);

        if (! ret.ok ())
            Throw<std::runtime_error> ("storeBatch failed: " + ret.ToString());
    }

    void
    for_each (std::function <void(std::shared_ptr<NodeObject>)> f) override
    {
        assert(m_db);
        rocksdb::ReadOptions const options;

        std::unique_ptr <rocksdb::Iterator> it (m_db->NewIterator (options));

        for (it->SeekToFirst (); it->Valid (); it->Next ())
        {
            if (it->key ().size () == m_keyBytes)
            {
                DecodedBlob decoded (it->key ().data (),
                                                it->value ().data (),
                                                it->value ().size ());

                if (decoded.wasOk ())
                {
                    f (decoded.createObject ());
                }
                else
                {
//噢，数据被破坏了！
                    JLOG(m_journal.fatal()) <<
                        "Corrupt NodeObject #" <<
                        from_hex_text<uint256>(it->key ().data ());
                }
            }
            else
            {
//vvalco注意到找到一个
//钥匙尺寸不正确？腐败？
                JLOG(m_journal.fatal()) <<
                    "Bad key size = " << it->key ().size ();
            }
        }
    }

    int
    getWriteLoad () override
    {
        return m_batch.getWriteLoad ();
    }

    void
    setDeletePath() override
    {
        m_deletePath = true;
    }

//——————————————————————————————————————————————————————————————

    void
    writeBatch (Batch const& batch) override
    {
        storeBatch (batch);
    }

    void
    verify() override
    {
    }

    /*返回后端需要的文件句柄数*/
    int
    fdlimit() const override
    {
        return fdlimit_;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class RocksDBFactory : public Factory
{
public:
    RocksDBEnv m_env;

    RocksDBFactory ()
    {
        Manager::instance().insert(*this);
    }

    ~RocksDBFactory () override
    {
        Manager::instance().erase(*this);
    }

    std::string
    getName () const override
    {
        return "RocksDB";
    }

    std::unique_ptr <Backend>
    createInstance (
        size_t keyBytes,
        Section const& keyValues,
        Scheduler& scheduler,
        beast::Journal journal) override
    {
        return std::make_unique <RocksDBBackend> (
            keyBytes, keyValues, scheduler, journal, &m_env);
    }
};

static RocksDBFactory rocksDBFactory;

}
}

#endif
