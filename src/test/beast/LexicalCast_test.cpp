
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#include <ripple/beast/core/LexicalCast.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/xor_shift_engine.h>

namespace beast {

class LexicalCast_test : public unit_test::suite
{
public:
    template <class IntType>
    static IntType nextRandomInt (xor_shift_engine& r)
    {
        return static_cast <IntType> (r());
    }

    template <class IntType>
    void testInteger (IntType in)
    {
        std::string s;
        IntType out (in+1);

        expect (lexicalCastChecked (s, in));
        expect (lexicalCastChecked (out, s));
        expect (out == in);
    }

    template <class IntType>
    void testIntegers (xor_shift_engine& r)
    {
        {
            std::stringstream ss;
            ss <<
                "random " << typeid (IntType).name ();
            testcase (ss.str());

            for (int i = 0; i < 1000; ++i)
            {
                IntType const value (nextRandomInt <IntType> (r));
                testInteger (value);
            }
        }

        {
            std::stringstream ss;
            ss <<
                "numeric_limits <" << typeid (IntType).name () << ">";
            testcase (ss.str());

            testInteger (std::numeric_limits <IntType>::min ());
            testInteger (std::numeric_limits <IntType>::max ());
        }
    }

    void testPathologies()
    {
        testcase("pathologies");
        try
        {
lexicalCastThrow<int>("\xef\xbc\x91\xef\xbc\x90"); //UTF-8编码
        }
        catch(BadLexicalCast const&)
        {
            pass();
        }
    }

    template <class T>
    void tryBadConvert (std::string const& s)
    {
        T out;
        expect (!lexicalCastChecked (out, s), s);
    }

    void testConversionOverflows()
    {
        testcase ("conversion overflows");

        tryBadConvert <std::uint64_t> ("99999999999999999999");
        tryBadConvert <std::uint32_t> ("4294967300");
        tryBadConvert <std::uint16_t> ("75821");
    }

    void testConversionUnderflows ()
    {
        testcase ("conversion underflows");

        tryBadConvert <std::uint32_t> ("-1");

        tryBadConvert <std::int64_t> ("-99999999999999999999");
        tryBadConvert <std::int32_t> ("-4294967300");
        tryBadConvert <std::int16_t> ("-75821");
    }

    template <class T>
    bool tryEdgeCase (std::string const& s)
    {
        T ret;

        bool const result = lexicalCastChecked (ret, s);

        if (!result)
            return false;

        return s == std::to_string (ret);
    }

    void testEdgeCases ()
    {
        testcase ("conversion edge cases");

        expect(tryEdgeCase <std::uint64_t> ("18446744073709551614"));
        expect(tryEdgeCase <std::uint64_t> ("18446744073709551615"));
        expect(!tryEdgeCase <std::uint64_t> ("18446744073709551616"));

        expect(tryEdgeCase <std::int64_t> ("9223372036854775806"));
        expect(tryEdgeCase <std::int64_t> ("9223372036854775807"));
        expect(!tryEdgeCase <std::int64_t> ("9223372036854775808"));

        expect(tryEdgeCase <std::int64_t> ("-9223372036854775807"));
        expect(tryEdgeCase <std::int64_t> ("-9223372036854775808"));
        expect(!tryEdgeCase <std::int64_t> ("-9223372036854775809"));

        expect(tryEdgeCase <std::uint32_t> ("4294967294"));
        expect(tryEdgeCase <std::uint32_t> ("4294967295"));
        expect(!tryEdgeCase <std::uint32_t> ("4294967296"));

        expect(tryEdgeCase <std::int32_t> ("2147483646"));
        expect(tryEdgeCase <std::int32_t> ("2147483647"));
        expect(!tryEdgeCase <std::int32_t> ("2147483648"));

        expect(tryEdgeCase <std::int32_t> ("-2147483647"));
        expect(tryEdgeCase <std::int32_t> ("-2147483648"));
        expect(!tryEdgeCase <std::int32_t> ("-2147483649"));

        expect(tryEdgeCase <std::uint16_t> ("65534"));
        expect(tryEdgeCase <std::uint16_t> ("65535"));
        expect(!tryEdgeCase <std::uint16_t> ("65536"));

        expect(tryEdgeCase <std::int16_t> ("32766"));
        expect(tryEdgeCase <std::int16_t> ("32767"));
        expect(!tryEdgeCase <std::int16_t> ("32768"));

        expect(tryEdgeCase <std::int16_t> ("-32767"));
        expect(tryEdgeCase <std::int16_t> ("-32768"));
        expect(!tryEdgeCase <std::int16_t> ("-32769"));
    }

    template <class T>
    void testThrowConvert(std::string const& s, bool success)
    {
        bool result = !success;
        T out;

        try
        {
            out = lexicalCastThrow <T> (s);
            result = true;
        }
        catch(BadLexicalCast const&)
        {
            result = false;
        }

        expect (result == success, s);
    }

    void testThrowingConversions ()
    {
        testcase ("throwing conversion");

        testThrowConvert <std::uint64_t> ("99999999999999999999", false);
        testThrowConvert <std::uint64_t> ("9223372036854775806", true);

        testThrowConvert <std::uint32_t> ("4294967290", true);
        testThrowConvert <std::uint32_t> ("42949672900", false);
        testThrowConvert <std::uint32_t> ("429496729000", false);
        testThrowConvert <std::uint32_t> ("4294967290000", false);

        testThrowConvert <std::int32_t> ("5294967295", false);
        testThrowConvert <std::int32_t> ("-2147483644", true);

        testThrowConvert <std::int16_t> ("66666", false);
        testThrowConvert <std::int16_t> ("-5711", true);
    }

    void testZero ()
    {
        testcase ("zero conversion");

        {
            std::int32_t out;

            expect (lexicalCastChecked (out, "-0"), "0");
            expect (lexicalCastChecked (out, "0"), "0");
            expect (lexicalCastChecked (out, "+0"), "0");
        }

        {
            std::uint32_t out;

            expect (!lexicalCastChecked (out, "-0"), "0");
            expect (lexicalCastChecked (out, "0"), "0");
            expect (lexicalCastChecked (out, "+0"), "0");
        }
    }

    void testEntireRange ()
    {
        testcase ("entire range");

        std::int32_t i = std::numeric_limits<std::int16_t>::min();
        std::string const empty("");

        while (i <= std::numeric_limits<std::int16_t>::max())
        {
            std::int16_t j = static_cast<std::int16_t>(i);

            auto actual = std::to_string (j);

            auto result = lexicalCast (j, empty);

            expect (result == actual, actual + " (string to integer)");

            if (result == actual)
            {
                auto number = lexicalCast <std::int16_t> (result);

                if (number != j)
                    expect (false, actual + " (integer to string)");
            }

            i++;
        }
    }

    void run() override
    {
        std::int64_t const seedValue = 50;

        xor_shift_engine r (seedValue);

        testIntegers <int> (r);
        testIntegers <unsigned int> (r);
        testIntegers <short> (r);
        testIntegers <unsigned short> (r);
        testIntegers <std::int32_t> (r);
        testIntegers <std::uint32_t> (r);
        testIntegers <std::int64_t> (r);
        testIntegers <std::uint64_t> (r);

        testPathologies();
        testConversionOverflows ();
        testConversionUnderflows ();
        testThrowingConversions ();
        testZero ();
        testEdgeCases ();
        testEntireRange ();
    }
};

BEAST_DEFINE_TESTSUITE(LexicalCast,beast_core,beast);

} //野兽
