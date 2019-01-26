
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
    版权所有（c）2012-2016 Ripple Labs Inc.

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
#include <ripple/beast/unit_test.h>
#include <ripple/consensus/LedgerTiming.h>

namespace ripple {
namespace test {

class LedgerTiming_test : public beast::unit_test::suite
{
    void testGetNextLedgerTimeResolution()
    {
//迭代调用GetNextLeadgerTimeResolution的帮助程序
        struct test_res
        {
            std::uint32_t decrease = 0;
            std::uint32_t equal = 0;
            std::uint32_t increase = 0;

            static test_res run(bool previousAgree, std::uint32_t rounds)
            {
                test_res res;
                auto closeResolution = ledgerDefaultTimeResolution;
                auto nextCloseResolution = closeResolution;
                std::uint32_t round = 0;
                do
                {
                   nextCloseResolution = getNextLedgerTimeResolution(
                       closeResolution, previousAgree, ++round);
                   if (nextCloseResolution < closeResolution)
                       ++res.decrease;
                   else if (nextCloseResolution > closeResolution)
                       ++res.increase;
                   else
                       ++res.equal;
                   std::swap(nextCloseResolution, closeResolution);
                } while (round < rounds);
                return res;
            }
        };


//如果我们不同意关闭时间，只能提高分辨率
//直到达到最大值
        auto decreases = test_res::run(false, 10);
        BEAST_EXPECT(decreases.increase == 3);
        BEAST_EXPECT(decreases.decrease == 0);
        BEAST_EXPECT(decreases.equal    == 7);

//如果我们总是同意关闭时间，只能降低分辨率
//直到达到最小值
        auto increases = test_res::run(false, 100);
        BEAST_EXPECT(increases.increase == 3);
        BEAST_EXPECT(increases.decrease == 0);
        BEAST_EXPECT(increases.equal    == 97);

    }

    void testRoundCloseTime()
    {
        using namespace std::chrono_literals;
//不修改等于epoch的closetime
        using tp = NetClock::time_point;
        tp def;
        BEAST_EXPECT(def == roundCloseTime(def, 30s));

//否则，关闭时间四舍五入到最近的时间
//领带的四舍五入
        BEAST_EXPECT(tp{ 0s } == roundCloseTime(tp{ 29s }, 60s));
        BEAST_EXPECT(tp{ 30s } == roundCloseTime(tp{ 30s }, 1s));
        BEAST_EXPECT(tp{ 60s } == roundCloseTime(tp{ 31s }, 60s));
        BEAST_EXPECT(tp{ 60s } == roundCloseTime(tp{ 30s }, 60s));
        BEAST_EXPECT(tp{ 60s } == roundCloseTime(tp{ 59s }, 60s));
        BEAST_EXPECT(tp{ 60s } == roundCloseTime(tp{ 60s }, 60s));
        BEAST_EXPECT(tp{ 60s } == roundCloseTime(tp{ 61s }, 60s));

    }

    void testEffCloseTime()
    {
        using namespace std::chrono_literals;
        using tp = NetClock::time_point;
        tp close = effCloseTime(tp{10s}, 30s, tp{0s});
        BEAST_EXPECT(close == tp{1s});

        close = effCloseTime(tp{16s}, 30s, tp{0s});
        BEAST_EXPECT(close == tp{30s});

        close = effCloseTime(tp{16s}, 30s, tp{30s});
        BEAST_EXPECT(close == tp{31s});

        close = effCloseTime(tp{16s}, 30s, tp{60s});
        BEAST_EXPECT(close == tp{61s});

        close = effCloseTime(tp{31s}, 30s, tp{0s});
        BEAST_EXPECT(close == tp{30s});
    }

    void
    run() override
    {
        testGetNextLedgerTimeResolution();
        testRoundCloseTime();
        testEffCloseTime();
    }

};

BEAST_DEFINE_TESTSUITE(LedgerTiming, consensus, ripple);
} //测试
} //涟漪
