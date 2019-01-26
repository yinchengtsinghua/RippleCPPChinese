
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

#include <ripple/protocol/XRPAmount.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

class XRPAmount_test : public beast::unit_test::suite
{
public:
    void testSigNum ()
    {
        testcase ("signum");

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x(i);

            if (i < 0)
                BEAST_EXPECT(x.signum () < 0);
            else if (i > 0)
                BEAST_EXPECT(x.signum () > 0);
            else
                BEAST_EXPECT(x.signum () == 0);
        }
    }

    void testBeastZero ()
    {
        testcase ("beast::Zero Comparisons");

        using beast::zero;

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x (i);

            BEAST_EXPECT((i == 0) == (x == zero));
            BEAST_EXPECT((i != 0) == (x != zero));
            BEAST_EXPECT((i < 0) == (x < zero));
            BEAST_EXPECT((i > 0) == (x > zero));
            BEAST_EXPECT((i <= 0) == (x <= zero));
            BEAST_EXPECT((i >= 0) == (x >= zero));

            BEAST_EXPECT((0 == i) == (zero == x));
            BEAST_EXPECT((0 != i) == (zero != x));
            BEAST_EXPECT((0 < i) == (zero < x));
            BEAST_EXPECT((0 > i) == (zero > x));
            BEAST_EXPECT((0 <= i) == (zero <= x));
            BEAST_EXPECT((0 >= i) == (zero >= x));
        }
    }

    void testComparisons ()
    {
        testcase ("XRP Comparisons");

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x (i);

            for (auto j : { -1, 0, 1})
            {
                XRPAmount const y (j);

                BEAST_EXPECT((i == j) == (x == y));
                BEAST_EXPECT((i != j) == (x != y));
                BEAST_EXPECT((i < j) == (x < y));
                BEAST_EXPECT((i > j) == (x > y));
                BEAST_EXPECT((i <= j) == (x <= y));
                BEAST_EXPECT((i >= j) == (x >= y));
            }
        }
    }

    void testAddSub ()
    {
        testcase ("Addition & Subtraction");

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x (i);

            for (auto j : { -1, 0, 1})
            {
                XRPAmount const y (j);

                BEAST_EXPECT(XRPAmount(i + j) == (x + y));
                BEAST_EXPECT(XRPAmount(i - j) == (x - y));

BEAST_EXPECT((x + y) == (y + x));   //加法是交换的
            }
        }
    }

    void testMulRatio()
    {
        testcase ("mulRatio");

        constexpr auto maxUInt32 = std::numeric_limits<std::uint32_t>::max ();
        constexpr auto maxUInt64 = std::numeric_limits<std::uint64_t>::max ();

        {
//乘以一个会溢出的数字，然后除以相同的数字
//数字，检查我们没有损失任何价值
            XRPAmount big (maxUInt64);
            BEAST_EXPECT(big == mulRatio (big, maxUInt32, maxUInt32, true));
//舍入模式不重要，因为结果是准确的
            BEAST_EXPECT(big == mulRatio (big, maxUInt32, maxUInt32, false));
        }

        {
//与上述类似的测试，但对于整齐的值
            XRPAmount big (maxUInt64);
            BEAST_EXPECT(big == mulRatio (big, maxUInt32, maxUInt32, true));
//舍入模式不重要，因为结果是准确的
            BEAST_EXPECT(big == mulRatio (big, maxUInt32, maxUInt32, false));
        }

        {
//少量
            XRPAmount tiny (1);
//四舍五入应给出允许的最小数目
            BEAST_EXPECT(tiny == mulRatio (tiny, 1, maxUInt32, true));
//四舍五入应为零
            BEAST_EXPECT(beast::zero == mulRatio (tiny, 1, maxUInt32, false));
            BEAST_EXPECT(beast::zero ==
                mulRatio (tiny, maxUInt32 - 1, maxUInt32, false));

//极小的负数
            XRPAmount tinyNeg (-1);
//四舍五入等于零
            BEAST_EXPECT(beast::zero == mulRatio (tinyNeg, 1, maxUInt32, true));
            BEAST_EXPECT(beast::zero == mulRatio (tinyNeg, maxUInt32 - 1, maxUInt32, true));
//四舍五入应该很小
            BEAST_EXPECT(tinyNeg == mulRatio (tinyNeg, maxUInt32 - 1, maxUInt32, false));
        }

        {
//舍入
            {
                XRPAmount one (1);
                auto const rup = mulRatio (one, maxUInt32 - 1, maxUInt32, true);
                auto const rdown = mulRatio (one, maxUInt32 - 1, maxUInt32, false);
                BEAST_EXPECT(rup.drops () - rdown.drops () == 1);
            }

            {
                XRPAmount big (maxUInt64);
                auto const rup = mulRatio (big, maxUInt32 - 1, maxUInt32, true);
                auto const rdown = mulRatio (big, maxUInt32 - 1, maxUInt32, false);
                BEAST_EXPECT(rup.drops () - rdown.drops () == 1);
            }

            {
                XRPAmount negOne (-1);
                auto const rup = mulRatio (negOne, maxUInt32 - 1, maxUInt32, true);
                auto const rdown = mulRatio (negOne, maxUInt32 - 1, maxUInt32, false);
                BEAST_EXPECT(rup.drops () - rdown.drops () == 1);
            }
        }

        {
//除以零
            XRPAmount one (1);
            except ([&] {mulRatio (one, 1, 0, true);});
        }

        {
//溢流
            XRPAmount big (maxUInt64);
            except ([&] {mulRatio (big, 2, 0, true);});
        }
    }

//——————————————————————————————————————————————————————————————

    void run () override
    {
        testSigNum ();
        testBeastZero ();
        testComparisons ();
        testAddSub ();
        testMulRatio ();
    }
};

BEAST_DEFINE_TESTSUITE(XRPAmount,protocol,ripple);

} //涟漪
