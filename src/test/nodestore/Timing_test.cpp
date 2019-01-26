
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

#include <test/nodestore/TestBase.h>
#include <ripple/nodestore/DummyScheduler.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/unity/rocksdb.h>
#include <ripple/beast/utility/temp_dir.h>
#include <ripple/beast/xor_shift_engine.h>
#include <ripple/beast/unit_test.h>
#include <test/unit_test/SuiteJournal.h>
#include <beast/unit_test/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <atomic>
#include <chrono>
#include <iterator>
#include <limits>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

#ifndef NODESTORE_TIMING_DO_VERIFY
#define NODESTORE_TIMING_DO_VERIFY 0
#endif

namespace ripple {
namespace NodeStore {

//用随机位填充存储器
template <class Generator>
static
void
rngcpy (void* buffer, std::size_t bytes, Generator& g)
{
    using result_type = typename Generator::result_type;
    while (bytes >= sizeof(result_type))
    {
        auto const v = g();
        memcpy(buffer, &v, sizeof(v));
        buffer = reinterpret_cast<std::uint8_t*>(buffer) + sizeof(v);
        bytes -= sizeof(v);
    }

    if (bytes > 0)
    {
        auto const v = g();
        memcpy(buffer, &v, bytes);
    }
}

//节点工厂的实例生成确定性序列
//在给定的
class Sequence
{
private:
    enum
    {
        minLedger = 1,
        maxLedger = 1000000,
        minSize = 250,
        maxSize = 1250
    };

    beast::xor_shift_engine gen_;
    std::uint8_t prefix_;
    std::discrete_distribution<std::uint32_t> d_type_;
    std::uniform_int_distribution<std::uint32_t> d_size_;

public:
    explicit
    Sequence(std::uint8_t prefix)
        : prefix_ (prefix)
//统一分布在热分类账-热交易节点
//但排除热事务=2（已删除）
        , d_type_ ({1, 1, 0, 1, 1})
        , d_size_ (minSize, maxSize)
    {
    }

//返回第n个键
    uint256
    key (std::size_t n)
    {
        gen_.seed(n+1);
        uint256 result;
        rngcpy (&*result.begin(), result.size(), gen_);
        return result;
    }

//返回第n个完整的nodeObject
    std::shared_ptr<NodeObject>
    obj (std::size_t n)
    {
        gen_.seed(n+1);
        uint256 key;
        auto const data =
            static_cast<std::uint8_t*>(&*key.begin());
        *data = prefix_;
        rngcpy (data + 1, key.size() - 1, gen_);
        Blob value(d_size_(gen_));
        rngcpy (&value[0], value.size(), gen_);
        return NodeObject::createObject (
            safe_cast<NodeObjectType>(d_type_(gen_)),
                std::move(value), key);
    }

//返回一批从n开始的nodeObjects
    void
    batch (std::size_t n, Batch& b, std::size_t size)
    {
        b.clear();
        b.reserve (size);
        while(size--)
            b.emplace_back(obj(n++));
    }
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class Timing_test : public beast::unit_test::suite
{
public:
    enum
    {
//丢失节点的提取百分比
        missingNodePercent = 20
    };

    std::size_t const default_repeat = 3;
#ifndef NDEBUG
    std::size_t const default_items = 10000;
#else
std::size_t const default_items = 100000; //释放
#endif

    using clock_type = std::chrono::steady_clock;
    using duration_type = std::chrono::milliseconds;

    struct Params
    {
        std::size_t items;
        std::size_t threads;
    };

    static
    std::string
    to_string (Section const& config)
    {
        std::string s;
        for (auto iter = config.begin(); iter != config.end(); ++iter)
            s += (iter != config.begin() ? "," : "") +
                iter->first + "=" + iter->second;
        return s;
    }

    static
    std::string
    to_string (duration_type const& d)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(3) <<
            (d.count() / 1000.) << "s";
        return ss.str();
    }

    static
    Section
    parse (std::string s)
    {
        Section section;
        std::vector <std::string> v;
        boost::split (v, s,
            boost::algorithm::is_any_of (","));
        section.append(v);
        return section;
    }

//——————————————————————————————————————————————————————————————

//lambdas中gcc参数包扩展的解决方法
//https://gcc.gnu.org/bugzilla/show_bug.cgi？ID＝47226
    template <class Body>
    class parallel_for_lambda
    {
    private:
        std::size_t const n_;
        std::atomic<std::size_t>& c_;

    public:
        parallel_for_lambda (std::size_t n,
                std::atomic<std::size_t>& c)
            : n_ (n)
            , c_ (c)
        {
        }

        template <class... Args>
        void
        operator()(Args&&... args)
        {
            Body body(args...);
            for(;;)
            {
                auto const i = c_++;
                if (i >= n_)
                    break;
                body (i);
            }
        }
    };

    /*执行并行for循环。

        构造'body'的'nu threads'实例'number'u
        带“args…”参数并在单个线程上运行它们
        唯一循环索引在[0，n]范围内。
    **/

    template <class Body, class... Args>
    void
    parallel_for (std::size_t const n,
        std::size_t number_of_threads, Args const&... args)
    {
        std::atomic<std::size_t> c(0);
        std::vector<beast::unit_test::thread> t;
        t.reserve(number_of_threads);
        for (std::size_t id = 0; id < number_of_threads; ++id)
            t.emplace_back(*this,
                parallel_for_lambda<Body>(n, c),
                    args...);
        for (auto& _ : t)
            _.join();
    }

    template <class Body, class... Args>
    void
    parallel_for_id (std::size_t const n,
        std::size_t number_of_threads, Args const&... args)
    {
        std::atomic<std::size_t> c(0);
        std::vector<beast::unit_test::thread> t;
        t.reserve(number_of_threads);
        for (std::size_t id = 0; id < number_of_threads; ++id)
            t.emplace_back(*this,
                parallel_for_lambda<Body>(n, c),
                    id, args...);
        for (auto& _ : t)
            _.join();
    }

//——————————————————————————————————————————————————————————————

//只插入
    void
    do_insert (Section const& config,
        Params const& params, beast::Journal journal)
    {
        DummyScheduler scheduler;
        auto backend = make_Backend (config, scheduler, journal);
        BEAST_EXPECT(backend != nullptr);
        backend->open();

        class Body
        {
        private:
            suite& suite_;
            Backend& backend_;
            Sequence seq_;

        public:
            explicit
            Body (suite& s, Backend& backend)
                : suite_ (s)
                , backend_ (backend)
                , seq_(1)
            {
            }

            void
            operator()(std::size_t i)
            {
                try
                {
                    backend_.store(seq_.obj(i));
                }
                catch(std::exception const& e)
                {
                    suite_.fail(e.what());
                }
            }
        };

        try
        {
            parallel_for<Body>(params.items,
                params.threads, std::ref(*this), std::ref(*backend));
        }
        catch (std::exception const&)
        {
        #if NODESTORE_TIMING_DO_VERIFY
            backend->verify();
        #endif
            Rethrow();
        }
        backend->close();
    }

//获取现有密钥
    void
    do_fetch (Section const& config,
        Params const& params, beast::Journal journal)
    {
        DummyScheduler scheduler;
        auto backend = make_Backend (config, scheduler, journal);
        BEAST_EXPECT(backend != nullptr);
        backend->open();

        class Body
        {
        private:
            suite& suite_;
            Backend& backend_;
            Sequence seq1_;
            beast::xor_shift_engine gen_;
            std::uniform_int_distribution<std::size_t> dist_;

        public:
            Body (std::size_t id, suite& s,
                    Params const& params, Backend& backend)
                : suite_(s)
                , backend_ (backend)
                , seq1_ (1)
                , gen_ (id + 1)
                , dist_ (0, params.items - 1)
            {
            }

            void
            operator()(std::size_t i)
            {
                try
                {
                    std::shared_ptr<NodeObject> obj;
                    std::shared_ptr<NodeObject> result;
                    obj = seq1_.obj(dist_(gen_));
                    backend_.fetch(obj->getHash().data(), &result);
                    suite_.expect(result && isSame(result, obj));
                }
                catch(std::exception const& e)
                {
                    suite_.fail(e.what());
                }
            }
        };
        try
        {
            parallel_for_id<Body>(params.items, params.threads,
                std::ref(*this), std::ref(params), std::ref(*backend));
        }
        catch (std::exception const&)
        {
        #if NODESTORE_TIMING_DO_VERIFY
            backend->verify();
        #endif
            Rethrow();
        }
        backend->close();
    }

//执行不存在密钥的查找
    void
    do_missing (Section const& config,
        Params const& params, beast::Journal journal)
    {
        DummyScheduler scheduler;
        auto backend = make_Backend (config, scheduler, journal);
        BEAST_EXPECT(backend != nullptr);
        backend->open();

        class Body
        {
        private:
            suite& suite_;
//参数常数&参数
            Backend& backend_;
            Sequence seq2_;
            beast::xor_shift_engine gen_;
            std::uniform_int_distribution<std::size_t> dist_;

        public:
            Body (std::size_t id, suite& s,
                    Params const& params, Backend& backend)
                : suite_ (s)
//，参数（params）
                , backend_ (backend)
                , seq2_ (2)
                , gen_ (id + 1)
                , dist_ (0, params.items - 1)
            {
            }

            void
            operator()(std::size_t i)
            {
                try
                {
                    auto const key = seq2_.key(i);
                    std::shared_ptr<NodeObject> result;
                    backend_.fetch(key.data(), &result);
                    suite_.expect(! result);
                }
                catch(std::exception const& e)
                {
                    suite_.fail(e.what());
                }
            }
        };

        try
        {
            parallel_for_id<Body>(params.items, params.threads,
                std::ref(*this), std::ref(params), std::ref(*backend));
        }
        catch (std::exception const&)
        {
        #if NODESTORE_TIMING_DO_VERIFY
            backend->verify();
        #endif
            Rethrow();
        }
        backend->close();
    }

//获取当前和丢失的密钥
    void
    do_mixed (Section const& config,
        Params const& params, beast::Journal journal)
    {
        DummyScheduler scheduler;
        auto backend = make_Backend (config, scheduler, journal);
        BEAST_EXPECT(backend != nullptr);
        backend->open();

        class Body
        {
        private:
            suite& suite_;
//参数常数&参数
            Backend& backend_;
            Sequence seq1_;
            Sequence seq2_;
            beast::xor_shift_engine gen_;
            std::uniform_int_distribution<std::uint32_t> rand_;
            std::uniform_int_distribution<std::size_t> dist_;

        public:
            Body (std::size_t id, suite& s,
                    Params const& params, Backend& backend)
                : suite_ (s)
//，参数（params）
                , backend_ (backend)
                , seq1_ (1)
                , seq2_ (2)
                , gen_ (id + 1)
                , rand_ (0, 99)
                , dist_ (0, params.items - 1)
            {
            }

            void
            operator()(std::size_t i)
            {
                try
                {
                    if (rand_(gen_) < missingNodePercent)
                    {
                        auto const key = seq2_.key(dist_(gen_));
                        std::shared_ptr<NodeObject> result;
                        backend_.fetch(key.data(), &result);
                        suite_.expect(! result);
                    }
                    else
                    {
                        std::shared_ptr<NodeObject> obj;
                        std::shared_ptr<NodeObject> result;
                        obj = seq1_.obj(dist_(gen_));
                        backend_.fetch(obj->getHash().data(), &result);
                        suite_.expect(result && isSame(result, obj));
                    }
                }
                catch(std::exception const& e)
                {
                    suite_.fail(e.what());
                }
            }
        };

        try
        {
            parallel_for_id<Body>(params.items, params.threads,
                std::ref(*this), std::ref(params), std::ref(*backend));
        }
        catch (std::exception const&)
        {
        #if NODESTORE_TIMING_DO_VERIFY
            backend->verify();
        #endif
            Rethrow();
        }
        backend->close();
    }

//模拟波纹工作负荷：
//每个线程随机：
//插入新键
//取出一把旧钥匙
//获取最近可能不存在的数据
    void
    do_work (Section const& config,
        Params const& params, beast::Journal journal)
    {
        DummyScheduler scheduler;
        auto backend = make_Backend (config, scheduler, journal);
        BEAST_EXPECT(backend != nullptr);
        backend->setDeletePath();
        backend->open();

        class Body
        {
        private:
            suite& suite_;
            Params const& params_;
            Backend& backend_;
            Sequence seq1_;
            beast::xor_shift_engine gen_;
            std::uniform_int_distribution<std::uint32_t> rand_;
            std::uniform_int_distribution<std::size_t> recent_;
            std::uniform_int_distribution<std::size_t> older_;

        public:
            Body (std::size_t id, suite& s,
                    Params const& params, Backend& backend)
                : suite_ (s)
                , params_ (params)
                , backend_ (backend)
                , seq1_ (1)
                , gen_ (id + 1)
                , rand_ (0, 99)
                , recent_ (params.items, params.items * 2 - 1)
                , older_ (0, params.items - 1)
            {
            }

            void
            operator()(std::size_t i)
            {
                try
                {
                    if (rand_(gen_) < 200)
                    {
//历史查找
                        std::shared_ptr<NodeObject> obj;
                        std::shared_ptr<NodeObject> result;
                        auto const j = older_(gen_);
                        obj = seq1_.obj(j);
                        std::shared_ptr<NodeObject> result1;
                        backend_.fetch(obj->getHash().data(), &result);
                        suite_.expect(result != nullptr);
                        suite_.expect(isSame(result, obj));
                    }

                    char p[2];
                    p[0] = rand_(gen_) < 50 ? 0 : 1;
                    p[1] = 1 - p[0];
                    for (int q = 0; q < 2; ++q)
                    {
                        switch (p[q])
                        {
                        case 0:
                        {
//最近拿来
                            std::shared_ptr<NodeObject> obj;
                            std::shared_ptr<NodeObject> result;
                            auto const j = recent_(gen_);
                            obj = seq1_.obj(j);
                            backend_.fetch(obj->getHash().data(), &result);
                            suite_.expect(! result ||
                                isSame(result, obj));
                            break;
                        }

                        case 1:
                        {
//插入新
                            auto const j = i + params_.items;
                            backend_.store(seq1_.obj(j));
                            break;
                        }
                        }
                    }
                }
                catch(std::exception const& e)
                {
                    suite_.fail(e.what());
                }
            }
        };

        try
        {
            parallel_for_id<Body>(params.items, params.threads,
                std::ref(*this), std::ref(params), std::ref(*backend));
        }
        catch (std::exception const&)
        {
        #if NODESTORE_TIMING_DO_VERIFY
            backend->verify();
        #endif
            Rethrow();
        }
        backend->close();
    }

//——————————————————————————————————————————————————————————————

    using test_func = void (Timing_test::*)(
        Section const&, Params const&, beast::Journal);
    using test_list = std::vector <std::pair<std::string, test_func>>;

    duration_type
    do_test (test_func f,
        Section const& config, Params const& params, beast::Journal journal)
    {
        auto const start = clock_type::now();
        (this->*f)(config, params, journal);
        return std::chrono::duration_cast<duration_type> (
            clock_type::now() - start);
    }

    void
    do_tests (std::size_t threads, test_list const& tests,
        std::vector<std::string> const& config_strings)
    {
        using std::setw;
        int w = 8;
        for (auto const& test : tests)
            if (w < test.first.size())
                w = test.first.size();
        log <<
            threads << " Thread" << (threads > 1 ? "s" : "") << ", " <<
            default_items << " Objects" << std::endl;
        {
            std::stringstream ss;
            ss << std::left << setw(10) << "Backend" << std::right;
            for (auto const& test : tests)
                ss << " " << setw(w) << test.first;
            log << ss.str() << std::endl;
        }

        using namespace beast::severities;
        test::SuiteJournal journal ("Timing_test", *this);

        for (auto const& config_string : config_strings)
        {
            Params params;
            params.items = default_items;
            params.threads = threads;
            for (auto i = default_repeat; i--;)
            {
                beast::temp_dir tempDir;
                Section config = parse(config_string);
                config.set ("path", tempDir.path());
                std::stringstream ss;
                ss << std::left << setw(10) <<
                    get(config, "type", std::string()) << std::right;
                for (auto const& test : tests)
                    ss << " " << setw(w) << to_string(
                        do_test (test.second, config, params, journal));
                ss << "   " << to_string(config);
                log << ss.str() << std::endl;
            }
        }
    }

    void
    run() override
    {
        testcase ("Timing", beast::unit_test::abort_on_fail);

        /*参数：

            重复多次以重复每次测试
            要在数据库中创建的对象数

        **/

        std::string default_args =
            "type=nudb"
        #if RIPPLE_ROCKSDB_AVAILABLE
            ";type=rocksdb,open_files=2000,filter_bits=12,cache_mb=256,"
                "file_size_mb=8,file_size_mult=2"
        #endif
        #if 0
            ";type=memory|path=NodeStore"
        #endif
            ;

        test_list const tests =
            {
                 { "Insert",    &Timing_test::do_insert }
                ,{ "Fetch",     &Timing_test::do_fetch }
                ,{ "Missing",   &Timing_test::do_missing }
                ,{ "Mixed",     &Timing_test::do_mixed }
                ,{ "Work",      &Timing_test::do_work }
            };

        auto args = arg().empty() ? default_args : arg();
        std::vector <std::string> config_strings;
        boost::split (config_strings, args,
            boost::algorithm::is_any_of (";"));
        for (auto iter = config_strings.begin();
                iter != config_strings.end();)
            if (iter->empty())
                iter = config_strings.erase (iter);
            else
                ++iter;

        do_tests ( 1, tests, config_strings);
        do_tests ( 4, tests, config_strings);
        do_tests ( 8, tests, config_strings);
//做测试（16，测试，配置字符串）；
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(Timing,NodeStore,ripple,1);

}
}

