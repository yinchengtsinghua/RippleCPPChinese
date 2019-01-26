
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

#include <ripple/protocol/Quality.h>
#include <ripple/beast/unit_test.h>
#include <type_traits>

namespace ripple {

class Quality_test : public beast::unit_test::suite
{
public:
//从尾数和指数创建一个原始的非整数金额
    STAmount
    static raw (std::uint64_t mantissa, int exponent)
    {
        return STAmount ({Currency(3), AccountID(3)}, mantissa, exponent);
    }

    template <class Integer>
    static
    STAmount
    amount (Integer integer,
        std::enable_if_t <std::is_signed <Integer>::value>* = 0)
    {
        static_assert (std::is_integral <Integer>::value, "");
        return STAmount (integer, false);
    }

    template <class Integer>
    static
    STAmount
    amount (Integer integer,
        std::enable_if_t <! std::is_signed <Integer>::value>* = 0)
    {
        static_assert (std::is_integral <Integer>::value, "");
        if (integer < 0)
            return STAmount (-integer, true);
        return STAmount (integer, false);
    }

    template <class In, class Out>
    static
    Amounts
    amounts (In in, Out out)
    {
        return Amounts (amount(in), amount(out));
    }

    template <class In1, class Out1, class Int, class In2, class Out2>
    void
    ceil_in (Quality const& q,
        In1 in, Out1 out, Int limit, In2 in_expected, Out2 out_expected)
    {
        auto expect_result (amounts (in_expected, out_expected));
        auto actual_result (q.ceil_in (
            amounts (in, out), amount (limit)));

        BEAST_EXPECT(actual_result == expect_result);
    }

    template <class In1, class Out1, class Int, class In2, class Out2>
    void
    ceil_out (Quality const& q,
        In1 in, Out1 out, Int limit, In2 in_expected, Out2 out_expected)
    {
        auto const expect_result (amounts (in_expected, out_expected));
        auto const actual_result (q.ceil_out (
            amounts (in, out), amount (limit)));

        BEAST_EXPECT(actual_result == expect_result);
    }

    void
    test_ceil_in ()
    {
        testcase ("ceil_in");

        {
//1英寸，1英寸：
            Quality q (Amounts (amount(1), amount(1)));

            ceil_in (q,
1,  1,   //1英寸，1英寸
1,       //极限值：1
1,  1);  //1英寸，1英寸

            ceil_in (q,
10, 10, //10英寸，10英寸
5,      //极限值：5
5, 5);  //5英寸，5英寸

            ceil_in (q,
5, 5,   //5英寸，5英寸
10,     //极限值：10
5, 5);  //5英寸，5英寸
        }

        {
//1英寸，2英寸：
            Quality q (Amounts (amount(1), amount(2)));

            ceil_in (q,
40, 80,   //40英寸，80英寸
40,       //极限值：40
40, 80);  //40英寸，20英寸

            ceil_in (q,
40, 80,   //40英寸，80英寸
20,       //极限值：20
20, 40);  //20英寸，40英寸

            ceil_in (q,
40, 80,   //40英寸，80英寸
60,       //极限值：60
40, 80);  //40英寸，80英寸
        }

        {
//2英寸，1英寸：
            Quality q (Amounts (amount(2), amount(1)));

            ceil_in (q,
40, 20,   //40英寸，20英寸
20,       //极限值：20
20, 10);  //20英寸，10英寸

            ceil_in (q,
40, 20,   //40英寸，20英寸
40,       //极限值：40
40, 20);  //40英寸，20英寸

            ceil_in (q,
40, 20,   //40英寸，20英寸
50,       //极限值：40
40, 20);  //40英寸，20英寸
        }
    }

    void
    test_ceil_out ()
    {
        testcase ("ceil_out");

        {
//1英寸，1英寸：
            Quality q (Amounts (amount(1),amount(1)));

            ceil_out (q,
1,  1,    //1英寸，1英寸
1,        //极限1
1,  1);   //1英寸，1英寸

            ceil_out (q,
10, 10,   //10英寸，10英寸
5,        //极限5
5, 5);    //5英寸，5英寸

            ceil_out (q,
10, 10,   //10英寸，10英寸
20,       //极限20
10, 10);  //10英寸，10英寸
        }

        {
//1英寸，2英寸：
            Quality q (Amounts (amount(1),amount(2)));

            ceil_out (q,
40, 80,    //40英寸，80英寸
40,        //极限40
20, 40);   //20英寸，40英寸

            ceil_out (q,
40, 80,    //40英寸，80英寸
80,        //极限80
40, 80);   //40英寸，80英寸

            ceil_out (q,
40, 80,    //40英寸，80英寸
100,       //极限100
40, 80);   //40英寸，80英寸
        }

        {
//2英寸，1英寸：
            Quality q (Amounts (amount(2),amount(1)));

            ceil_out (q,
40, 20,    //40英寸，20英寸
20,        //极限20
40, 20);   //40英寸，20英寸

            ceil_out (q,
40, 20,    //40英寸，20英寸
40,        //极限40
40, 20);   //40英寸，20英寸

            ceil_out (q,
40, 20,    //40英寸，20英寸
10,        //极限10
20, 10);   //20英寸，10英寸
        }
    }

    void
    test_raw()
    {
        testcase ("raw");

        {
Quality q (0x5d048191fb9130daull);      //126836389.7680090
            Amounts const value (
amount(349469768),                  //349.469768 XRP
raw (2755280000000000ull, -15));    //二点七五五二八
            STAmount const limit (
raw (4131113916555555, -16));       //4131113916555555号
            Amounts const result (
                q.ceil_out (value, limit));
            BEAST_EXPECT(result.in != beast::zero);
        }
    }

    void
    test_round()
    {
        testcase ("round");

Quality q (0x59148191fb913522ull);      //57719.63525051682
        BEAST_EXPECT(q.round(3).rate().getText() == "57800");
        BEAST_EXPECT(q.round(4).rate().getText() == "57720");
        BEAST_EXPECT(q.round(5).rate().getText() == "57720");
        BEAST_EXPECT(q.round(6).rate().getText() == "57719.7");
        BEAST_EXPECT(q.round(7).rate().getText() == "57719.64");
        BEAST_EXPECT(q.round(8).rate().getText() == "57719.636");
        BEAST_EXPECT(q.round(9).rate().getText() == "57719.6353");
        BEAST_EXPECT(q.round(10).rate().getText() == "57719.63526");
        BEAST_EXPECT(q.round(11).rate().getText() == "57719.635251");
        BEAST_EXPECT(q.round(12).rate().getText() == "57719.6352506");
        BEAST_EXPECT(q.round(13).rate().getText() == "57719.63525052");
        BEAST_EXPECT(q.round(14).rate().getText() == "57719.635250517");
        BEAST_EXPECT(q.round(15).rate().getText() == "57719.6352505169");
        BEAST_EXPECT(q.round(16).rate().getText() == "57719.63525051682");
    }

    void
    test_comparisons()
    {
        testcase ("comparisons");

        STAmount const amount1 (noIssue(), 231);
        STAmount const amount2 (noIssue(), 462);
        STAmount const amount3 (noIssue(), 924);

        Quality const q11 (Amounts (amount1, amount1));
        Quality const q12 (Amounts (amount1, amount2));
        Quality const q13 (Amounts (amount1, amount3));
        Quality const q21 (Amounts (amount2, amount1));
        Quality const q31 (Amounts (amount3, amount1));

        BEAST_EXPECT(q11 == q11);
        BEAST_EXPECT(q11 < q12);
        BEAST_EXPECT(q12 < q13);
        BEAST_EXPECT(q31 < q21);
        BEAST_EXPECT(q21 < q11);
        BEAST_EXPECT(q11 >= q11);
        BEAST_EXPECT(q12 >= q11);
        BEAST_EXPECT(q13 >= q12);
        BEAST_EXPECT(q21 >= q31);
        BEAST_EXPECT(q11 >= q21);
        BEAST_EXPECT(q12 > q11);
        BEAST_EXPECT(q13 > q12);
        BEAST_EXPECT(q21 > q31);
        BEAST_EXPECT(q11 > q21);
        BEAST_EXPECT(q11 <= q11);
        BEAST_EXPECT(q11 <= q12);
        BEAST_EXPECT(q12 <= q13);
        BEAST_EXPECT(q31 <= q21);
        BEAST_EXPECT(q21 <= q11);
        BEAST_EXPECT(q31 != q21);
    }

    void
    test_composition ()
    {
        testcase ("composition");

        STAmount const amount1 (noIssue(), 231);
        STAmount const amount2 (noIssue(), 462);
        STAmount const amount3 (noIssue(), 924);

        Quality const q11 (Amounts (amount1, amount1));
        Quality const q12 (Amounts (amount1, amount2));
        Quality const q13 (Amounts (amount1, amount3));
        Quality const q21 (Amounts (amount2, amount1));
        Quality const q31 (Amounts (amount3, amount1));

        BEAST_EXPECT(
            composed_quality (q12, q21) == q11);

        Quality const q13_31 (
            composed_quality (q13, q31));
        Quality const q31_13 (
            composed_quality (q31, q13));

        BEAST_EXPECT(q13_31 == q31_13);
        BEAST_EXPECT(q13_31 == q11);
    }

    void
    test_operations ()
    {
        testcase ("operations");

        Quality const q11 (Amounts (
            STAmount (noIssue(), 731),
            STAmount (noIssue(), 731)));

        Quality qa (q11);
        Quality qb (q11);

        BEAST_EXPECT(qa == qb);
        BEAST_EXPECT(++qa != q11);
        BEAST_EXPECT(qa != qb);
        BEAST_EXPECT(--qb != q11);
        BEAST_EXPECT(qa != qb);
        BEAST_EXPECT(qb < qa);
        BEAST_EXPECT(qb++ < qa);
        BEAST_EXPECT(qb++ < qa);
        BEAST_EXPECT(qb++ == qa);
        BEAST_EXPECT(qa < qb);
    }
    void
    run() override
    {
        test_comparisons ();
        test_composition ();
        test_operations ();
        test_ceil_in ();
        test_ceil_out ();
        test_raw ();
        test_round ();
    }
};

BEAST_DEFINE_TESTSUITE(Quality,protocol,ripple);

}
