
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

#include <ripple/protocol/IOUAmount.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

class IOUAmount_test : public beast::unit_test::suite
{
public:
    void testZero ()
    {
        testcase ("zero");

        IOUAmount const z (0, 0);

        BEAST_EXPECT(z.mantissa () == 0);
        BEAST_EXPECT(z.exponent () == -100);
        BEAST_EXPECT(!z);
        BEAST_EXPECT(z.signum () == 0);
        BEAST_EXPECT(z == beast::zero);

        BEAST_EXPECT((z + z) == z);
        BEAST_EXPECT((z - z) == z);
        BEAST_EXPECT(z == -z);

        IOUAmount const zz (beast::zero);
        BEAST_EXPECT(z == zz);
    }

    void testSigNum ()
    {
        testcase ("signum");

        IOUAmount const neg (-1, 0);
        BEAST_EXPECT(neg.signum () < 0);

        IOUAmount const zer (0, 0);
        BEAST_EXPECT(zer.signum () == 0);

        IOUAmount const pos (1, 0);
        BEAST_EXPECT(pos.signum () > 0);
    }

    void testBeastZero ()
    {
        testcase ("beast::Zero Comparisons");

        using beast::zero;

        {
            IOUAmount z (zero);
            BEAST_EXPECT(z == zero);
            BEAST_EXPECT(z >= zero);
            BEAST_EXPECT(z <= zero);
            unexpected (z != zero);
            unexpected (z > zero);
            unexpected (z < zero);
        }

        {
            IOUAmount const neg (-2, 0);
            BEAST_EXPECT(neg < zero);
            BEAST_EXPECT(neg <= zero);
            BEAST_EXPECT(neg != zero);
            unexpected (neg == zero);
        }

        {
            IOUAmount const pos (2, 0);
            BEAST_EXPECT(pos > zero);
            BEAST_EXPECT(pos >= zero);
            BEAST_EXPECT(pos != zero);
            unexpected (pos == zero);
        }
    }

    void testComparisons ()
    {
        testcase ("IOU Comparisons");

        IOUAmount const n (-2, 0);
        IOUAmount const z (0, 0);
        IOUAmount const p (2, 0);

        BEAST_EXPECT(z == z);
        BEAST_EXPECT(z >= z);
        BEAST_EXPECT(z <= z);
        BEAST_EXPECT(z == -z);
        unexpected (z > z);
        unexpected (z < z);
        unexpected (z != z);
        unexpected (z != -z);

        BEAST_EXPECT(n < z);
        BEAST_EXPECT(n <= z);
        BEAST_EXPECT(n != z);
        unexpected (n > z);
        unexpected (n >= z);
        unexpected (n == z);

        BEAST_EXPECT(p > z);
        BEAST_EXPECT(p >= z);
        BEAST_EXPECT(p != z);
        unexpected (p < z);
        unexpected (p <= z);
        unexpected (p == z);

        BEAST_EXPECT(n < p);
        BEAST_EXPECT(n <= p);
        BEAST_EXPECT(n != p);
        unexpected (n > p);
        unexpected (n >= p);
        unexpected (n == p);

        BEAST_EXPECT(p > n);
        BEAST_EXPECT(p >= n);
        BEAST_EXPECT(p != n);
        unexpected (p < n);
        unexpected (p <= n);
        unexpected (p == n);

        BEAST_EXPECT(p > -p);
        BEAST_EXPECT(p >= -p);
        BEAST_EXPECT(p != -p);

        BEAST_EXPECT(n < -n);
        BEAST_EXPECT(n <= -n);
        BEAST_EXPECT(n != -n);
    }

    void testToString()
    {
        testcase("IOU strings");

        BEAST_EXPECT(to_string(IOUAmount (-2, 0)) == "-2");
        BEAST_EXPECT(to_string(IOUAmount (0, 0)) == "0");
        BEAST_EXPECT(to_string(IOUAmount (2, 0)) == "2");
        BEAST_EXPECT(to_string(IOUAmount (25, -3)) == "0.025");
        BEAST_EXPECT(to_string(IOUAmount (-25, -3)) == "-0.025");
        BEAST_EXPECT(to_string(IOUAmount (25, 1)) == "250");
        BEAST_EXPECT(to_string(IOUAmount (-25, 1)) == "-250");
        BEAST_EXPECT(to_string(IOUAmount (2, 20)) == "2000000000000000e5");
        BEAST_EXPECT(to_string(IOUAmount (-2, -20)) == "-2000000000000000e-35");
    }

    void testMulRatio()
    {
        testcase ("mulRatio");

        /*归一化后尾数的范围*/
        constexpr std::int64_t minMantissa = 1000000000000000ull;
        constexpr std::int64_t maxMantissa = 9999999999999999ull;
//对数（2，最大尾数）~53.15
        /*归一化后的指数范围*/
        constexpr int minExponent = -96;
        constexpr int maxExponent = 80;
        constexpr auto maxUInt = std::numeric_limits<std::uint32_t>::max ();

        {
//乘以一个会溢出尾数的数字，然后
//除以同一个数字，检查我们没有丢失任何值
            IOUAmount bigMan (maxMantissa, 0);
            BEAST_EXPECT(bigMan == mulRatio (bigMan, maxUInt, maxUInt, true));
//舍入模式不重要，因为结果是准确的
            BEAST_EXPECT(bigMan == mulRatio (bigMan, maxUInt, maxUInt, false));
        }
        {
//与上述试验类似，但为负值
            IOUAmount bigMan (-maxMantissa, 0);
            BEAST_EXPECT(bigMan == mulRatio (bigMan, maxUInt, maxUInt, true));
//舍入模式不重要，因为结果是准确的
            BEAST_EXPECT(bigMan == mulRatio (bigMan, maxUInt, maxUInt, false));
        }

        {
//少量
            IOUAmount tiny (minMantissa, minExponent);
//四舍五入应给出允许的最小数目
            BEAST_EXPECT(tiny == mulRatio (tiny, 1, maxUInt, true));
            BEAST_EXPECT(tiny == mulRatio (tiny, maxUInt - 1, maxUInt, true));
//四舍五入应为零
            BEAST_EXPECT(beast::zero == mulRatio (tiny, 1, maxUInt, false));
            BEAST_EXPECT(beast::zero == mulRatio (tiny, maxUInt - 1, maxUInt, false));

//极小的负数
            IOUAmount tinyNeg (-minMantissa, minExponent);
//四舍五入等于零
            BEAST_EXPECT(beast::zero == mulRatio (tinyNeg, 1, maxUInt, true));
            BEAST_EXPECT(beast::zero == mulRatio (tinyNeg, maxUInt - 1, maxUInt, true));
//四舍五入应该很小
            BEAST_EXPECT(tinyNeg == mulRatio (tinyNeg, 1, maxUInt, false));
            BEAST_EXPECT(tinyNeg == mulRatio (tinyNeg, maxUInt - 1, maxUInt, false));
        }

        {
//舍入
            {
                IOUAmount one (1, 0);
                auto const rup = mulRatio (one, maxUInt - 1, maxUInt, true);
                auto const rdown = mulRatio (one, maxUInt - 1, maxUInt, false);
                BEAST_EXPECT(rup.mantissa () - rdown.mantissa () == 1);
            }
            {
                IOUAmount big (maxMantissa, maxExponent);
                auto const rup = mulRatio (big, maxUInt - 1, maxUInt, true);
                auto const rdown = mulRatio (big, maxUInt - 1, maxUInt, false);
                BEAST_EXPECT(rup.mantissa () - rdown.mantissa () == 1);
            }

            {
                IOUAmount negOne (-1, 0);
                auto const rup = mulRatio (negOne, maxUInt - 1, maxUInt, true);
                auto const rdown = mulRatio (negOne, maxUInt - 1, maxUInt, false);
                BEAST_EXPECT(rup.mantissa () - rdown.mantissa () == 1);
            }
        }

        {
//除以零
            IOUAmount one (1, 0);
            except ([&] {mulRatio (one, 1, 0, true);});
        }

        {
//溢流
            IOUAmount big (maxMantissa, maxExponent);
            except ([&] {mulRatio (big, 2, 0, true);});
        }
    }

//——————————————————————————————————————————————————————————————

    void run () override
    {
        testZero ();
        testSigNum ();
        testBeastZero ();
        testComparisons ();
        testToString ();
        testMulRatio ();
    }
};

BEAST_DEFINE_TESTSUITE(IOUAmount,protocol,ripple);

} //涟漪
