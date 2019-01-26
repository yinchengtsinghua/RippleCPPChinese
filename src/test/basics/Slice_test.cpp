
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Rippled的一部分：https://github0.com/ripple/rippled
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

#include <ripple/basics/Slice.h>
#include <ripple/beast/unit_test.h>
#include <array>
#include <cstdint>

namespace ripple {
namespace test {

struct Slice_test : beast::unit_test::suite
{
    void run() override
    {
        std::uint8_t const data[] =
        {
            0xa8, 0xa1, 0x38, 0x45, 0x23, 0xec, 0xe4, 0x23,
            0x71, 0x6d, 0x2a, 0x18, 0xb4, 0x70, 0xcb, 0xf5,
            0xac, 0x2d, 0x89, 0x4d, 0x19, 0x9c, 0xf0, 0x2c,
            0x15, 0xd1, 0xf9, 0x9b, 0x66, 0xd2, 0x30, 0xd3
        };

        {
            testcase ("Equality & Inequality");

            Slice const s0{};

            BEAST_EXPECT (s0.size() == 0);
            BEAST_EXPECT (s0.data() == nullptr);
            BEAST_EXPECT (s0 == s0);

//指向相同数据的大小相等或不相等的测试片：
            for (std::size_t i = 0; i != sizeof(data); ++i)
            {
                Slice const s1 { data, i };

                BEAST_EXPECT (s1.size() == i);
                BEAST_EXPECT (s1.data() != nullptr);

                if (i == 0)
                    BEAST_EXPECT (s1 == s0);
                else
                    BEAST_EXPECT (s1 != s0);

                for (std::size_t j = 0; j != sizeof(data); ++j)
                {
                    Slice const s2 { data, j };

                    if (i == j)
                        BEAST_EXPECT (s1 == s2);
                    else
                        BEAST_EXPECT (s1 != s2);
                }
            }

//相同大小但指向不同数据的测试片：
            std::array<std::uint8_t, sizeof(data)> a;
            std::array<std::uint8_t, sizeof(data)> b;

            for (std::size_t i = 0; i != sizeof(data); ++i)
                a[i] = b[i] = data[i];

            BEAST_EXPECT (makeSlice(a) == makeSlice(b));
            b[7]++;
            BEAST_EXPECT (makeSlice(a) != makeSlice(b));
            a[7]++;
            BEAST_EXPECT (makeSlice(a) == makeSlice(b));
        }

        {
            testcase ("Indexing");

            Slice const s  { data, sizeof(data) };

            for (std::size_t i = 0; i != sizeof(data); ++i)
                BEAST_EXPECT (s[i] == data[i]);
        }

        {
            testcase ("Advancing");

            for (std::size_t i = 0; i < sizeof(data); ++i)
            {
                for (std::size_t j = 0; i + j < sizeof(data); ++j)
                {
                    Slice s (data + i, sizeof(data) - i);
                    s += j;

                    BEAST_EXPECT (s.data() == data + i + j);
                    BEAST_EXPECT (s.size() == sizeof(data) - i - j);
                }
            }
        }
    }
};

BEAST_DEFINE_TESTSUITE(Slice, ripple_basics, ripple);

}  //命名空间测试
}  //命名空间波纹
