
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

#include <test/jtx.h>
#include <ripple/beast/unit_test.h>
#include <algorithm>

namespace ripple {
namespace test {

//确保“丰满”的订购书没有问题
class PlumpBook_test : public beast::unit_test::suite
{
public:
    void
    createOffers (jtx::Env& env,
        jtx::IOU const& iou, std::size_t n)
    {
        using namespace jtx;
        for (std::size_t i = 1; i <= n; ++i)
        {
            env(offer("alice", XRP(i), iou(1)));
            env.close();
        }
    }

    void
    test (std::size_t n)
    {
        using namespace jtx;
        auto const billion = 1000000000ul;
        Env env(*this);
        env.disable_sigs();
        auto const gw = Account("gateway");
        auto const USD = gw["USD"];
        env.fund(XRP(billion), gw, "alice");
        env.trust(USD(billion), "alice");
        env(pay(gw, "alice", USD(billion)));
        createOffers(env, USD, n);
    }

    void
    run() override
    {
        test(10000);
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(PlumpBook,tx,ripple,5);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//确保未签名事务在自动测试运行期间成功。
class ThinBook_test : public PlumpBook_test
{
public:
    void
        run() override
    {
        test(1);
    }
};

BEAST_DEFINE_TESTSUITE(ThinBook, tx, ripple);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class OversizeMeta_test : public beast::unit_test::suite
{
public:
    void
    createOffers (jtx::Env& env, jtx::IOU const& iou,
        std::size_t n)
    {
        using namespace jtx;
        for (std::size_t i = 1; i <= n; ++i)
        {
            env(offer("alice", XRP(1), iou(1)));
            env.close();
        }
    }

    void
    test()
    {
        std::size_t const n = 9000;
        using namespace jtx;
        auto const billion = 1000000000ul;
        Env env(*this);
        env.disable_sigs();
        auto const gw = Account("gateway");
        auto const USD = gw["USD"];
        env.fund(XRP(billion), gw, "alice");
        env.trust(USD(billion), "alice");
        env(pay(gw, "alice", USD(billion)));
        createOffers(env, USD, n);
        env(pay("alice", gw, USD(billion)));
        env(offer("alice", USD(1), XRP(1)));
    }

    void
    run() override
    {
        test();
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(OversizeMeta,tx,ripple,3);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class FindOversizeCross_test : public beast::unit_test::suite
{
public:
//返回[Lo，Hi]中的最低x，其中f（x）=true
    template <class Function>
    static
    std::size_t
    bfind(std::size_t lo, std::size_t hi, Function&& f)
    {
        auto len = hi - lo;
        while (len != 0)
        {
            auto l2 = len / 2;
            auto m = lo + l2;
            if (! f(m))
            {
                lo = ++m;
                len -= l2 + 1;
            }
            else
                len = l2;
        }
        return lo;
    }

    void
    createOffers (jtx::Env& env, jtx::IOU const& iou,
        std::size_t n)
    {
        using namespace jtx;
        for (std::size_t i = 1; i <= n; ++i)
        {
            env(offer("alice", XRP(i), iou(1)));
            env.close();
        }
    }

    bool
    oversize(std::size_t n)
    {
        using namespace jtx;
        auto const billion = 1000000000ul;
        Env env(*this);
        env.disable_sigs();
        auto const gw = Account("gateway");
        auto const USD = gw["USD"];
        env.fund(XRP(billion), gw, "alice");
        env.trust(USD(billion), "alice");
        env(pay(gw, "alice", USD(billion)));
        createOffers(env, USD, n);
        env(pay("alice", gw, USD(billion)));
        env(offer("alice", USD(1), XRP(1)), ter(std::ignore));
        return env.ter() == tecOVERSIZE;
    }

    void
    run() override
    {
        auto const result = bfind(100, 9000,
            [&](std::size_t n) { return oversize(n); });
        log << "Min oversize offers = " << result << '\n';
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(FindOversizeCross,tx,ripple,50);

} //测试
} //涟漪

