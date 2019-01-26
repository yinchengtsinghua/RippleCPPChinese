
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
    版权所有2014，nikolaos d.bogalis<nikb@bogalis.net>


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

#include <ripple/basics/tagged_integer.h>
#include <ripple/beast/unit_test.h>
#include <type_traits>

namespace ripple {
namespace test {

class tagged_integer_test
    : public beast::unit_test::suite
{
private:
    struct Tag1 { };
    struct Tag2 { };

//静态检查类型是否不可互操作

    using TagUInt1 = tagged_integer <std::uint32_t, Tag1>;
    using TagUInt2 = tagged_integer <std::uint32_t, Tag2>;
    using TagUInt3 = tagged_integer <std::uint64_t, Tag1>;

//检查标记的_整数的构造
    static_assert (std::is_constructible<TagUInt1, std::uint32_t>::value,
        "TagUInt1 should be constructible using a std::uint32_t");

    static_assert (!std::is_constructible<TagUInt1, std::uint64_t>::value,
        "TagUInt1 should not be constructible using a std::uint64_t");

    static_assert (std::is_constructible<TagUInt3, std::uint32_t>::value,
        "TagUInt3 should be constructible using a std::uint32_t");

    static_assert (std::is_constructible<TagUInt3, std::uint64_t>::value,
        "TagUInt3 should be constructible using a std::uint64_t");

//检查标记的_整数的赋值
    static_assert (!std::is_assignable<TagUInt1, std::uint32_t>::value,
        "TagUInt1 should not be assignable with a std::uint32_t");

    static_assert (!std::is_assignable<TagUInt1, std::uint64_t>::value,
        "TagUInt1 should not be assignable with a std::uint64_t");

    static_assert (!std::is_assignable<TagUInt3, std::uint32_t>::value,
        "TagUInt3 should not be assignable with a std::uint32_t");

    static_assert (!std::is_assignable<TagUInt3, std::uint64_t>::value,
        "TagUInt3 should not be assignable with a std::uint64_t");

    static_assert (std::is_assignable<TagUInt1, TagUInt1>::value,
        "TagUInt1 should be assignable with a TagUInt1");

    static_assert (!std::is_assignable<TagUInt1, TagUInt2>::value,
        "TagUInt1 should not be assignable with a TagUInt2");

    static_assert (std::is_assignable<TagUInt3, TagUInt3>::value,
        "TagUInt3 should be assignable with a TagUInt1");

    static_assert (!std::is_assignable<TagUInt1, TagUInt3>::value,
        "TagUInt1 should not be assignable with a TagUInt3");

    static_assert (!std::is_assignable<TagUInt3, TagUInt1>::value,
        "TagUInt3 should not be assignable with a TagUInt1");

//检查标记的_整数的可转换性
    static_assert (!std::is_convertible<std::uint32_t, TagUInt1>::value,
        "std::uint32_t should not be convertible to a TagUInt1");

    static_assert (!std::is_convertible<std::uint32_t, TagUInt3>::value,
        "std::uint32_t should not be convertible to a TagUInt3");

    static_assert (!std::is_convertible<std::uint64_t, TagUInt3>::value,
        "std::uint64_t should not be convertible to a TagUInt3");

    static_assert (!std::is_convertible<std::uint64_t, TagUInt2>::value,
        "std::uint64_t should not be convertible to a TagUInt2");

    static_assert (!std::is_convertible<TagUInt1, TagUInt2>::value,
        "TagUInt1 should not be convertible to TagUInt2");

    static_assert (!std::is_convertible<TagUInt1, TagUInt3>::value,
        "TagUInt1 should not be convertible to TagUInt3");

    static_assert (!std::is_convertible<TagUInt2, TagUInt3>::value,
        "TagUInt2 should not be convertible to a TagUInt3");


public:
    void run () override
    {
        using TagInt = tagged_integer<std::int32_t, Tag1>;

        {
            testcase ("Comparison Operators");

            TagInt const zero(0);
            TagInt const one(1);

            BEAST_EXPECT(one == one);
            BEAST_EXPECT(!(one == zero));

            BEAST_EXPECT(one != zero);
            BEAST_EXPECT(!(one != one));

            BEAST_EXPECT(zero < one);
            BEAST_EXPECT(!(one < zero));

            BEAST_EXPECT(one > zero);
            BEAST_EXPECT(!(zero > one));

            BEAST_EXPECT(one >= one);
            BEAST_EXPECT(one >= zero);
            BEAST_EXPECT(!(zero >= one));

            BEAST_EXPECT(zero <= one);
            BEAST_EXPECT(zero <= zero);
            BEAST_EXPECT(!(one <= zero));
        }

        {
            testcase ("Increment/Decrement Operators");
            TagInt const zero(0);
            TagInt const one(1);
            TagInt a{0};
            ++a;
            BEAST_EXPECT(a == one);
            --a;
            BEAST_EXPECT(a == zero);
            a++;
            BEAST_EXPECT(a == one);
            a--;
            BEAST_EXPECT(a == zero);
        }


        {
            testcase ("Arithmetic Operators");
            TagInt a{-2};
            BEAST_EXPECT(+a == TagInt{-2});
            BEAST_EXPECT(-a == TagInt{2});
            BEAST_EXPECT(TagInt{-3} + TagInt{4} == TagInt{1});
            BEAST_EXPECT(TagInt{-3} - TagInt{4} == TagInt{-7});
            BEAST_EXPECT(TagInt{-3} * TagInt{4} == TagInt{-12});
            BEAST_EXPECT(TagInt{8}/TagInt{4} == TagInt{2});
            BEAST_EXPECT(TagInt{7} %TagInt{4} == TagInt{3});

            BEAST_EXPECT(~TagInt{8}  == TagInt{~TagInt::value_type{8}});
            BEAST_EXPECT((TagInt{6} & TagInt{3}) == TagInt{2});
            BEAST_EXPECT((TagInt{6} | TagInt{3}) == TagInt{7});
            BEAST_EXPECT((TagInt{6} ^ TagInt{3}) == TagInt{5});

            BEAST_EXPECT((TagInt{4} << TagInt{2}) == TagInt{16});
            BEAST_EXPECT((TagInt{16} >> TagInt{2}) == TagInt{4});
        }
        {
            testcase ("Assignment Operators");
            TagInt a{-2};
            TagInt b{0};
            b = a;
            BEAST_EXPECT(b == TagInt{-2});

//- 3＋4＝＝1
            a = TagInt{-3};
            a += TagInt{4};
            BEAST_EXPECT(a == TagInt{1});

//- 3 - 4＝＝7
            a = TagInt{-3};
            a -= TagInt{4};
            BEAST_EXPECT(a == TagInt{-7});

//- 3×4＝＝12
            a = TagInt{-3};
            a *= TagInt{4};
            BEAST_EXPECT(a == TagInt{-12});

//8/4＝2
            a = TagInt{8};
            a /= TagInt{4};
            BEAST_EXPECT(a == TagInt{2});

//7% 4＝3
            a = TagInt{7};
            a %= TagInt{4};
            BEAST_EXPECT(a == TagInt{3});

//6和3＝2
            a = TagInt{6};
            a /= TagInt{3};
            BEAST_EXPECT(a == TagInt{2});

//6乘3＝7
            a = TagInt{6};
            a |= TagInt{3};
            BEAST_EXPECT(a == TagInt{7});

//6 ^ 3＝5
            a = TagInt{6};
            a ^= TagInt{3};
            BEAST_EXPECT(a == TagInt{5});

//4＜2＝16
            a = TagInt{4};
            a <<= TagInt{2};
            BEAST_EXPECT(a == TagInt{16});

//16＞2＝4
            a = TagInt{16};
            a >>= TagInt{2};
            BEAST_EXPECT(a == TagInt{4});
        }



    }
};

BEAST_DEFINE_TESTSUITE(tagged_integer,ripple_basics,ripple);

} //测试
} //涟漪
