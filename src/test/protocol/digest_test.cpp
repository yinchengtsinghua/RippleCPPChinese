
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

#include <ripple/protocol/digest.h>
#include <ripple/beast/utility/rngfill.h>
#include <ripple/beast/xor_shift_engine.h>
#include <ripple/beast/unit_test.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <vector>

namespace ripple {

class digest_test : public beast::unit_test::suite
{
    std::vector<uint256> dataset1;

    template <class Hasher>
    void test (char const* name)
    {
        using namespace std::chrono;

//主缓存
        for (int i = 0; i != 4; i++)
        {
            for (auto const& x : dataset1)
            {
                Hasher h;
                h (x.data (), x.size ());
                (void) static_cast<typename Hasher::result_type>(h);
            }
        }

        std::array<nanoseconds, 128> results;

        for (auto& result : results)
        {
            auto const start = high_resolution_clock::now ();

            for (auto const& x : dataset1)
            {
                Hasher h;
                h (x.data (), x.size ());
                (void) static_cast<typename Hasher::result_type>(h);
            }

            auto const d = high_resolution_clock::now () - start;

            result = d;
        }

        log << "    " << name << ":" << '\n';

        auto const sum = std::accumulate(
            results.begin(), results.end(),
            nanoseconds{0});
        {
            auto s = duration_cast<seconds>(sum);
            auto ms = duration_cast<milliseconds>(sum) - s;
            log <<
                "       Total Time = " << s.count() <<
                "." << ms.count() << " seconds" << std::endl;
        }

        auto const mean = sum / results.size();
        {
            auto s = duration_cast<seconds>(mean);
            auto ms = duration_cast<milliseconds>(mean) - s;
            log <<
                "        Mean Time = " << s.count() <<
                "." << ms.count() << " seconds" << std::endl;
        }

        std::vector<nanoseconds::rep> diff(results.size());
        std::transform(
            results.begin(), results.end(), diff.begin(),
            [&mean](nanoseconds trial)
            {
                return (trial - mean).count();
            });
        auto const sq_sum = std::inner_product(
            diff.begin(), diff.end(), diff.begin(), 0.0);
        {
            nanoseconds const stddev {
                static_cast<nanoseconds::rep>(
                    std::sqrt(sq_sum / results.size()))
            };
            auto s = duration_cast<seconds>(stddev);
            auto ms = duration_cast<milliseconds>(stddev) - s;
            log <<
                "          Std Dev = " << s.count() <<
                "." << ms.count() << " seconds" << std::endl;
        }
    }

public:
    digest_test ()
    {
        beast::xor_shift_engine g(19207813);
        std::uint8_t buf[32];

        for (int i = 0; i < 1000000; i++)
        {
            beast::rngfill (buf, sizeof(buf), g);
            dataset1.push_back (uint256::fromVoid (buf));
        }
    }

    void testSHA512 ()
    {
        testcase ("SHA512");
        test<openssl_ripemd160_hasher> ("OpenSSL");
        test<beast::ripemd160_hasher> ("Beast");
        pass ();
    }

    void testSHA256 ()
    {
        testcase ("SHA256");
        test<openssl_sha256_hasher> ("OpenSSL");
        test<beast::sha256_hasher> ("Beast");
        pass ();
    }

    void testRIPEMD160 ()
    {
        testcase ("RIPEMD160");
        test<openssl_ripemd160_hasher> ("OpenSSL");
        test<beast::ripemd160_hasher> ("Beast");
        pass ();
    }

    void run () override
    {
        testSHA512 ();
        testSHA256 ();
        testRIPEMD160 ();
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(digest,ripple_data,ripple,20);

} //涟漪
