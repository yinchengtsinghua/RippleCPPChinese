
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

#include <ripple/protocol/TER.h>
#include <ripple/beast/unit_test.h>

#include <tuple>
#include <type_traits>

namespace ripple {

struct TER_test : public beast::unit_test::suite
{
    void
    testTransResultInfo()
    {
        for (auto i = -400; i < 400; ++i)
        {
            TER t = TER::fromInt (i);
            auto inRange = isTelLocal(t) ||
                isTemMalformed(t) ||
                isTefFailure(t) ||
                isTerRetry(t) ||
                isTesSuccess(t) ||
                isTecClaim(t);

            std::string token, text;
            auto good = transResultInfo(t, token, text);
            BEAST_EXPECT(inRange || !good);
            BEAST_EXPECT(transToken(t) == (good ? token : "-"));
            BEAST_EXPECT(transHuman(t) == (good ? text : "-"));

            auto code = transCode(token);
            BEAST_EXPECT(good == !!code);
            BEAST_EXPECT(!code || *code == t);
        }
    }

//确保两种类型不可转换或
//如果不相同，则可分配。
//o i1一个元组索引。
//o i2其他元组索引。
//o tup应为tuple。
//它是一个函数，而不是一个函数模板，因为类模板
//不能是模板参数而不指定完整参数。
    template<std::size_t I1, std::size_t I2>
    class NotConvertible
    {
    public:
        template<typename Tup>
        void operator()(Tup const& tup, beast::unit_test::suite&) const
        {
//元组中的条目不应是可转换的或可分配的
//除非它们是相同的类型。
            using To_t = std::decay_t<decltype (std::get<I1>(tup))>;
            using From_t = std::decay_t<decltype (std::get<I2>(tup))>;
            static_assert (std::is_same<From_t, To_t>::value ==
                std::is_convertible<From_t, To_t>::value, "Convert err");
            static_assert (std::is_same<To_t, From_t>::value ==
                std::is_constructible<To_t, From_t>::value, "Construct err");
            static_assert (std::is_same <To_t, From_t>::value ==
                std::is_assignable<To_t&, From_t const&>::value, "Assign err");

//从整数到类型的赋值或转换不应该工作。
            static_assert (
                ! std::is_convertible<int, To_t>::value, "Convert err");
            static_assert (
                ! std::is_constructible<To_t, int>::value, "Construct err");
            static_assert (
                ! std::is_assignable<To_t&, int const&>::value, "Assign err");
        }
    };

//在元组上快速迭代。
    template<std::size_t I1, std::size_t I2,
        template<std::size_t, std::size_t> class Func, typename Tup>
    std::enable_if_t<I1 != 0>
    testIterate (Tup const& tup, beast::unit_test::suite& suite)
    {
        Func<I1, I2> func;
        func (tup, suite);
        testIterate<I1 - 1, I2, Func>(tup, suite);
    }

//在元组上缓慢迭代。
    template<std::size_t I1, std::size_t I2,
        template<std::size_t, std::size_t> class Func, typename Tup>
    std::enable_if_t<I1 == 0 && I2 != 0>
    testIterate (Tup const& tup, beast::unit_test::suite& suite)
    {
        Func<I1, I2> func;
        func (tup, suite);
        testIterate<std::tuple_size<Tup>::value - 1, I2 - 1, Func>(tup, suite);
    }

//完成对元组的迭代。
    template<std::size_t I1, std::size_t I2,
        template<std::size_t, std::size_t> class Func, typename Tup>
    std::enable_if_t<I1 == 0 && I2 == 0>
    testIterate (Tup const& tup, beast::unit_test::suite& suite)
    {
        Func<I1, I2> func;
        func (tup, suite);
    }

    void testConversion()
    {
//验证有效转换是否有效和无效转换
//无效。

//各种枚举的示例。
        static auto const terEnums = std::make_tuple (telLOCAL_ERROR,
            temMALFORMED, tefFAILURE, terRETRY, tesSUCCESS, tecCLAIM);
        static const int hiIndex {
            std::tuple_size<decltype (terEnums)>::value - 1};

//请验证枚举不能转换为其他枚举类型。
        testIterate<hiIndex, hiIndex, NotConvertible> (terEnums, *this);

//验证可分配性和可转换性的lambda。
        auto isConvertable = [] (auto from, auto to)
        {
            using From_t = std::decay_t<decltype (from)>;
            using To_t = std::decay_t<decltype (to)>;
            static_assert (
                std::is_convertible<From_t, To_t>::value, "Convert err");
            static_assert (
                std::is_constructible<To_t, From_t>::value, "Construct err");
            static_assert (
                std::is_assignable<To_t&, From_t const&>::value, "Assign err");
        };

//验证是否将正确的类型转换为nottec。
        NotTEC const notTec;
        isConvertable (telLOCAL_ERROR, notTec);
        isConvertable (temMALFORMED,   notTec);
        isConvertable (tefFAILURE,     notTec);
        isConvertable (terRETRY,       notTec);
        isConvertable (tesSUCCESS,     notTec);
        isConvertable (notTec,         notTec);

//验证类型且不可赋值或可转换的lambda。
        auto notConvertible = [] (auto from, auto to)
        {
            using To_t = std::decay_t<decltype (to)>;
            using From_t = std::decay_t<decltype (from)>;
            static_assert (
                !std::is_convertible<From_t, To_t>::value, "Convert err");
            static_assert (
                !std::is_constructible<To_t, From_t>::value, "Construct err");
            static_assert (
                !std::is_assignable<To_t&, From_t const&>::value, "Assign err");
        };

//验证不应转换为NotTec的类型。
        TER const ter;
        notConvertible (tecCLAIM, notTec);
        notConvertible (ter,      notTec);
        notConvertible (4,        notTec);

//验证正确的类型是否转换为ter。
        isConvertable (telLOCAL_ERROR, ter);
        isConvertable (temMALFORMED,   ter);
        isConvertable (tefFAILURE,     ter);
        isConvertable (terRETRY,       ter);
        isConvertable (tesSUCCESS,     ter);
        isConvertable (tecCLAIM,       ter);
        isConvertable (notTec,         ter);
        isConvertable (ter,            ter);

//验证您不能从int转换为ter。
        notConvertible (4, ter);
    }

//帮助器模板，确保两种类型具有可比性。阿尔索
//验证其中一个类型是否与int进行比较。
//o i1一个元组索引。
//o i2其他元组索引。
//o tup应为tuple。
//它是一个函数，而不是一个函数模板，因为类模板
//不能是模板参数而不指定完整参数。
    template<std::size_t I1, std::size_t I2>
    class CheckComparable
    {
    public:
        template<typename Tup>
        void operator()(Tup const& tup, beast::unit_test::suite& suite) const
        {
//元组中的所有条目都应该可以相互比较。
            auto const lhs = std::get<I1>(tup);
            auto const rhs = std::get<I2>(tup);

            static_assert (std::is_same<
                decltype (operator== (lhs, rhs)), bool>::value, "== err");

            static_assert (std::is_same<
                decltype (operator!= (lhs, rhs)), bool>::value, "!= err");

            static_assert (std::is_same<
                decltype (operator<  (lhs, rhs)), bool>::value, "< err");

            static_assert (std::is_same<
                decltype (operator<= (lhs, rhs)), bool>::value, "<= err");

            static_assert (std::is_same<
                decltype (operator>  (lhs, rhs)), bool>::value, "> err");

            static_assert (std::is_same<
                decltype (operator>= (lhs, rhs)), bool>::value, ">= err");

//确保对ter类型的抽样显示出预期的行为。
//用于所有比较运算符。
            suite.expect ((lhs == rhs) == (TERtoInt (lhs) == TERtoInt (rhs)));
            suite.expect ((lhs != rhs) == (TERtoInt (lhs) != TERtoInt (rhs)));
            suite.expect ((lhs <  rhs) == (TERtoInt (lhs) <  TERtoInt (rhs)));
            suite.expect ((lhs <= rhs) == (TERtoInt (lhs) <= TERtoInt (rhs)));
            suite.expect ((lhs >  rhs) == (TERtoInt (lhs) >  TERtoInt (rhs)));
            suite.expect ((lhs >= rhs) == (TERtoInt (lhs) >= TERtoInt (rhs)));
        }
    };

    void testComparison()
    {
//所有与TER相关的类型都应具有可比性。

//我们希望成功比较的所有类型的示例。
        static auto const ters = std::make_tuple (telLOCAL_ERROR, temMALFORMED,
            tefFAILURE, terRETRY, tesSUCCESS, tecCLAIM,
            NotTEC {telLOCAL_ERROR}, TER {tecCLAIM});
        static const int hiIndex {
            std::tuple_size<decltype (ters)>::value - 1};

//验证ters元组中的所有类型都可以与
//其他类型。
        testIterate<hiIndex, hiIndex, CheckComparable> (ters, *this);
    }

    void
    run() override
    {
        testTransResultInfo();
        testConversion();
        testComparison();
    }
};

BEAST_DEFINE_TESTSUITE(TER,protocol,ripple);

} //命名空间波纹
