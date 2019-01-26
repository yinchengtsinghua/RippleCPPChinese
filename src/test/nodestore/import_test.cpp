
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
#include <ripple/nodestore/impl/codec.h>
#include <ripple/beast/clock/basic_seconds_clock.h>
#include <ripple/beast/rfc2616.h>
#include <ripple/beast/core/LexicalCast.h>
#include <ripple/beast/unit_test.h>
#include <nudb/create.hpp>
#include <nudb/detail/format.hpp>
#include <nudb/xxhasher.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/regex.hpp>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <map>
#include <sstream>

#include <ripple/unity/rocksdb.h>

/*

数学：

1000 GB数据文件
170 GB密钥文件
容量113把钥匙/桶

正常的：
读取1000GB数据文件
19210GB密钥文件读取（113*170）
19210GB密钥文件写入

多（32GB）：
6道（170/32）
6000GB数据文件读取
170GB密钥文件写入


**/


namespace ripple {
namespace NodeStore {

namespace detail {

class save_stream_state
{
    std::ostream& os_;
    std::streamsize precision_;
    std::ios::fmtflags flags_;
    std::ios::char_type fill_;
public:
    ~save_stream_state()
    {
        os_.precision(precision_);
        os_.flags(flags_);
        os_.fill(fill_);
    }
    save_stream_state(save_stream_state const&) = delete;
    save_stream_state& operator=(save_stream_state const&) = delete;
    explicit save_stream_state(std::ostream& os)
        : os_(os)
        , precision_(os.precision())
        , flags_(os.flags())
        , fill_(os.fill())
    {
    }
};

template <class Rep, class Period>
std::ostream&
pretty_time(std::ostream& os, std::chrono::duration<Rep, Period> d)
{
    save_stream_state _(os);
    using namespace std::chrono;
    if (d < microseconds{1})
    {
//使用纳秒
        if (d < nanoseconds{100})
        {
//使用浮动
            using ns = duration<float, std::nano>;
            os << std::fixed << std::setprecision(1) << ns(d).count();
        }
        else
        {
//使用积分
            os << date::round<nanoseconds>(d).count();
        }
        os << "ns";
    }
    else if (d < milliseconds{1})
    {
//使用微秒
        if (d < microseconds{100})
        {
//使用浮动
            using ms = duration<float, std::micro>;
            os << std::fixed << std::setprecision(1) << ms(d).count();
        }
        else
        {
//使用积分
            os << date::round<microseconds>(d).count();
        }
        os << "us";
    }
    else if (d < seconds{1})
    {
//使用毫秒
        if (d < milliseconds{100})
        {
//使用浮动
            using ms = duration<float, std::milli>;
            os << std::fixed << std::setprecision(1) << ms(d).count();
        }
        else
        {
//使用积分
            os << date::round<milliseconds>(d).count();
        }
        os << "ms";
    }
    else if (d < minutes{1})
    {
//使用秒
        if (d < seconds{100})
        {
//使用浮动
            using s = duration<float>;
            os << std::fixed << std::setprecision(1) << s(d).count();
        }
        else
        {
//使用积分
            os << date::round<seconds>(d).count();
        }
        os << "s";
    }
    else
    {
//使用分钟
        if (d < minutes{100})
        {
//使用浮动
            using m = duration<float, std::ratio<60>>;
            os << std::fixed << std::setprecision(1) << m(d).count();
        }
        else
        {
//使用积分
            os << date::round<minutes>(d).count();
        }
        os << "min";
    }
    return os;
}

template <class Period, class Rep>
inline
std::string
fmtdur(std::chrono::duration<Period, Rep> const& d)
{
    std::stringstream ss;
    pretty_time(ss, d);
    return ss.str();
}

} //细节

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class progress
{
private:
    using clock_type =
        beast::basic_seconds_clock<
            std::chrono::steady_clock>;

    std::size_t const work_;
    clock_type::time_point start_ = clock_type::now();
    clock_type::time_point now_ = clock_type::now();
    clock_type::time_point report_ = clock_type::now();
    std::size_t prev_ = 0;
    bool estimate_ = false;

public:
    explicit
    progress(std::size_t work)
        : work_(work)
    {
    }

    template <class Log>
    void
    operator()(Log& log, std::size_t work)
    {
        using namespace std::chrono;
        auto const now = clock_type::now();
        if (now == now_)
            return;
        now_ = now;
        auto const elapsed = now - start_;
        if (! estimate_)
        {
            if (elapsed < seconds(15))
                return;
            estimate_ = true;
        }
        else if (now - report_ <
            std::chrono::seconds(60))
        {
            return;
        }
        auto const rate =
            elapsed.count() / double(work);
        clock_type::duration const remain(
            static_cast<clock_type::duration::rep>(
                (work_ - work) * rate));
        log <<
            "Remaining: " << detail::fmtdur(remain) <<
                " (" << work << " of " << work_ <<
                    " in " << detail::fmtdur(elapsed) <<
                ", " << (work - prev_) <<
                    " in " << detail::fmtdur(now - report_) <<
                ")";
        report_ = now;
        prev_ = work;
    }

    template <class Log>
    void
    finish(Log& log)
    {
        log <<
            "Total time: " << detail::fmtdur(
                clock_type::now() - start_);
    }
};

std::map <std::string, std::string, boost::beast::iless>
parse_args(std::string const& s)
{
//<key>'='<value>
    static boost::regex const re1 (
"^"                         //起点线
"(?:\\s*)"                  //空白（视觉）
"([a-zA-Z][_a-zA-Z0-9]*)"   //<密钥>
"(?:\\s*)"                  //空白（可选）
"(?:=)"                     //“=”
"(?:\\s*)"                  //空白（可选）
"(.*\\S+)"                  //<值>
"(?:\\s*)"                  //空白（可选）
        , boost::regex_constants::optimize
    );
    std::map <std::string,
        std::string, boost::beast::iless> map;
    auto const v = beast::rfc2616::split(
        s.begin(), s.end(), ',');
    for (auto const& kv : v)
    {
        boost::smatch m;
        if (! boost::regex_match (kv, m, re1))
            Throw<std::runtime_error> (
                "invalid parameter " + kv);
        auto const result =
            map.emplace(m[1], m[2]);
        if (! result.second)
            Throw<std::runtime_error> (
                "duplicate parameter " + m[1]);
    }
    return map;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if RIPPLE_ROCKSDB_AVAILABLE

class import_test : public beast::unit_test::suite
{
public:
    void
    run() override
    {
        testcase(beast::unit_test::abort_on_fail) << arg();

        using namespace nudb;
        using namespace nudb::detail;

        pass();
        auto const args = parse_args(arg());
        bool usage = args.empty();

        if (! usage &&
            args.find("from") == args.end())
        {
            log <<
                "Missing parameter: from";
            usage = true;
        }
        if (! usage &&
            args.find("to") == args.end())
        {
            log <<
                "Missing parameter: to";
            usage = true;
        }
        if (! usage &&
            args.find("buffer") == args.end())
        {
            log <<
                "Missing parameter: buffer";
            usage = true;
        }

        if (usage)
        {
            log <<
                "Usage:\n" <<
                "--unittest-arg=from=<from>,to=<to>,buffer=<buffer>\n" <<
                "from:   RocksDB database to import from\n" <<
                "to:     NuDB database to import to\n" <<
                "buffer: Buffer size (bigger is faster)\n" <<
                "NuDB database must not already exist.";
            return;
        }

//这将控制桶缓冲区的大小。
//对于1TB数据文件，建议使用32GB的bucket缓冲区。
//缓冲区越大，导入速度越快。
//
        std::size_t const buffer_size =
            std::stoull(args.at("buffer"));
        auto const from_path = args.at("from");
        auto const to_path = args.at("to");

        using hash_type = nudb::xxhasher;
        auto const bulk_size = 64 * 1024 * 1024;
        float const load_factor = 0.5;

        auto const dp = to_path + ".dat";
        auto const kp = to_path + ".key";

        auto const start =
            std::chrono::steady_clock::now();

        log <<
            "from:    " << from_path << "\n"
            "to:      " << to_path << "\n"
            "buffer:  " << buffer_size;

        std::unique_ptr<rocksdb::DB> db;
        {
            rocksdb::Options options;
            options.create_if_missing = false;
options.max_open_files = 2000; //5000？
            rocksdb::DB* pdb = nullptr;
            rocksdb::Status status =
                rocksdb::DB::OpenForReadOnly(
                    options, from_path, &pdb);
            if (! status.ok () || ! pdb)
                Throw<std::runtime_error> (
                    "Can't open '" + from_path + "': " +
                        status.ToString());
            db.reset(pdb);
        }
//使用值创建数据文件
        std::size_t nitems = 0;
        std::size_t nbytes = 0;
        dat_file_header dh;
        dh.version = currentVersion;
        dh.uid = make_uid();
        dh.appnum = 1;
        dh.key_size = 32;

        native_file df;
        error_code ec;
        df.create(file_mode::append, dp, ec);
        if (ec)
            Throw<nudb::system_error>(ec);
        bulk_writer<native_file> dw(
            df, 0, bulk_size);
        {
            {
                auto os = dw.prepare(dat_file_header::size, ec);
                if (ec)
                    Throw<nudb::system_error>(ec);
                write(os, dh);
            }
            rocksdb::ReadOptions options;
            options.verify_checksums = false;
            options.fill_cache = false;
            std::unique_ptr<rocksdb::Iterator> it(
                db->NewIterator(options));

            buffer buf;
            for (it->SeekToFirst (); it->Valid (); it->Next())
            {
                if (it->key().size() != 32)
                    Throw<std::runtime_error> (
                        "Unexpected key size " +
                            std::to_string(it->key().size()));
                void const* const key = it->key().data();
                void const* const data = it->value().data();
                auto const size = it->value().size();
                std::unique_ptr<char[]> clean(
                    new char[size]);
                std::memcpy(clean.get(), data, size);
                filter_inner(clean.get(), size);
                auto const out = nodeobject_compress(
                    clean.get(), size, buf);
//验证编解码器的正确性
                {
                    buffer buf2;
                    auto const check = nodeobject_decompress(
                        out.first, out.second, buf2);
                    BEAST_EXPECT(check.second == size);
                    BEAST_EXPECT(std::memcmp(
                        check.first, clean.get(), size) == 0);
                }
//数据记录
                auto os = dw.prepare(
field<uint48_t>::size + //尺寸
32 +                    //钥匙
                    out.second, ec);
                if (ec)
                    Throw<nudb::system_error>(ec);
                write<uint48_t>(os, out.second);
                std::memcpy(os.data(32), key, 32);
                std::memcpy(os.data(out.second),
                    out.first, out.second);
                ++nitems;
                nbytes += size;
            }
            dw.flush(ec);
            if (ec)
                Throw<nudb::system_error>(ec);
        }
        db.reset();
        log <<
            "Import data: " << detail::fmtdur(
                std::chrono::steady_clock::now() - start);
        auto const df_size = df.size(ec);
        if (ec)
            Throw<nudb::system_error>(ec);
//创建密钥文件
        key_file_header kh;
        kh.version = currentVersion;
        kh.uid = dh.uid;
        kh.appnum = dh.appnum;
        kh.key_size = 32;
        kh.salt = make_salt();
        kh.pepper = pepper<hash_type>(kh.salt);
        kh.block_size = block_size(kp);
        kh.load_factor = std::min<std::size_t>(
            65536.0 * load_factor, 65535);
        kh.buckets = std::ceil(nitems / (bucket_capacity(
            kh.block_size) * load_factor));
        kh.modulus = ceil_pow2(kh.buckets);
        native_file kf;
        kf.create(file_mode::append, kp, ec);
        if (ec)
            Throw<nudb::system_error>(ec);
        buffer buf(kh.block_size);
        {
            std::memset(buf.get(), 0, kh.block_size);
            ostream os(buf.get(), kh.block_size);
            write(os, kh);
            kf.write(0, buf.get(), kh.block_size, ec);
            if (ec)
                Throw<nudb::system_error>(ec);
        }
//构建
//使用多次传递数据的密钥文件。
//
        auto const buckets = std::max<std::size_t>(1,
            buffer_size / kh.block_size);
        buf.reserve(buckets * kh.block_size);
        auto const passes =
            (kh.buckets + buckets - 1) / buckets;
        log <<
            "items:   " << nitems << "\n"
            "buckets: " << kh.buckets << "\n"
            "data:    " << df_size << "\n"
            "passes:  " << passes;
        progress p(df_size * passes);
        std::size_t npass = 0;
        for (std::size_t b0 = 0; b0 < kh.buckets;
                b0 += buckets)
        {
            auto const b1 = std::min(
                b0 + buckets, kh.buckets);
//缓冲范围为[b0，b1）
            auto const bn = b1 - b0;
//创建空存储桶
            for (std::size_t i = 0; i < bn; ++i)
            {
                bucket b(kh.block_size,
                    buf.get() + i * kh.block_size,
                        empty);
            }
//将所有键插入存储桶
//迭代数据文件
            bulk_reader<native_file> r(
                df, dat_file_header::size,
                    df_size, bulk_size);
            while (! r.eof())
            {
                auto const offset = r.offset();
//数据记录或泄漏记录
                std::size_t size;
                auto is = r.prepare(
field<uint48_t>::size, ec); //尺寸
                if (ec)
                    Throw<nudb::system_error>(ec);
                read<uint48_t>(is, size);
                if (size > 0)
                {
//数据记录
                    is = r.prepare(
dh.key_size +           //钥匙
size, ec);                  //数据
                    if (ec)
                        Throw<nudb::system_error>(ec);
                    std::uint8_t const* const key =
                        is.data(dh.key_size);
                    auto const h = hash<hash_type>(
                        key, kh.key_size, kh.salt);
                    auto const n = bucket_index(
                        h, kh.buckets, kh.modulus);
                    p(log,
                        npass * df_size + r.offset());
                    if (n < b0 || n >= b1)
                        continue;
                    bucket b(kh.block_size, buf.get() +
                        (n - b0) * kh.block_size);
                    maybe_spill(b, dw, ec);
                    if (ec)
                        Throw<nudb::system_error>(ec);
                    b.insert(offset, size, h);
                }
                else
                {
//vfalc不应该来这里
//溢出记录
                    is = r.prepare(
                        field<std::uint16_t>::size, ec);
                    if (ec)
                        Throw<nudb::system_error>(ec);
read<std::uint16_t>(is, size);  //尺寸
r.prepare(size, ec); //跳过
                    if (ec)
                        Throw<nudb::system_error>(ec);
                }
            }
            kf.write((b0 + 1) * kh.block_size,
                buf.get(), bn * kh.block_size, ec);
            if (ec)
                Throw<nudb::system_error>(ec);
            ++npass;
        }
        dw.flush(ec);
        if (ec)
            Throw<nudb::system_error>(ec);
        p.finish(log);
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL(import,NodeStore,ripple);

#endif

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

} //节点存储
} //涟漪

