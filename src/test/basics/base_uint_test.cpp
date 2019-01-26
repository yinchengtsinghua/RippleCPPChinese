
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

#include <ripple/basics/base_uint.h>
#include <ripple/basics/hardened_hash.h>
#include <ripple/beast/unit_test.h>
#include <boost/algorithm/string.hpp>

#include <type_traits>

namespace ripple {
namespace test {

//只复制字节的非哈希哈希散列程序。
//用于测试基单元中的哈希附加
template <std::size_t Bits>
struct nonhash
{
    static constexpr std::size_t WIDTH = Bits / 8;
    std::array<std::uint8_t, WIDTH> data_;
    static beast::endian const endian = beast::endian::big;

    nonhash() = default;

    void
    operator() (void const* key, std::size_t len) noexcept
    {
        assert(len == WIDTH);
        memcpy(data_.data(), key, len);
    }

    explicit
    operator std::size_t() noexcept { return WIDTH; }
};

struct base_uint_test : beast::unit_test::suite
{
    using test96 = base_uint<96>;
    static_assert(std::is_copy_constructible<test96>::value, "");
    static_assert(std::is_copy_assignable<test96>::value, "");

    void run() override
    {
//用于验证集合插入（需要哈希）
        std::unordered_set<test96, hardened_hash<>> uset;

        Blob raw { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        BEAST_EXPECT(test96::bytes == raw.size());

        test96 u { raw };
        uset.insert(u);
        BEAST_EXPECT(raw.size() == u.size());
        BEAST_EXPECT(to_string(u) == "0102030405060708090A0B0C");
        BEAST_EXPECT(*u.data() == 1);
        BEAST_EXPECT(u.signum() == 1);
        BEAST_EXPECT(!!u);
        BEAST_EXPECT(!u.isZero());
        BEAST_EXPECT(u.isNonZero());
        unsigned char t = 0;
        for (auto& d : u)
        {
            BEAST_EXPECT(d == ++t);
        }

//通过“hashing”（无操作散列器）测试散列附加（h）
//然后提取哈希期间写入的字节
//返回到另一个基值（w）与原始值进行比较
        nonhash<96> h;
        hash_append(h, u);
        test96 w {std::vector<std::uint8_t>(h.data_.begin(), h.data_.end())};
        BEAST_EXPECT(w == u);

        test96 v { ~u };
        uset.insert(v);
        BEAST_EXPECT(to_string(v) == "FEFDFCFBFAF9F8F7F6F5F4F3");
        BEAST_EXPECT(*v.data() == 0xfe);
        BEAST_EXPECT(v.signum() == 1);
        BEAST_EXPECT(!!v);
        BEAST_EXPECT(!v.isZero());
        BEAST_EXPECT(v.isNonZero());
        t = 0xff;
        for (auto& d : v)
        {
            BEAST_EXPECT(d == --t);
        }

        BEAST_EXPECT(compare(u, v) < 0);
        BEAST_EXPECT(compare(v, u) > 0);

        v = u;
        BEAST_EXPECT(v == u);

        test96 z { beast::zero };
        uset.insert(z);
        BEAST_EXPECT(to_string(z) == "000000000000000000000000");
        BEAST_EXPECT(*z.data() == 0);
        BEAST_EXPECT(*z.begin() == 0);
        BEAST_EXPECT(*std::prev(z.end(), 1) == 0);
        BEAST_EXPECT(z.signum() == 0);
        BEAST_EXPECT(!z);
        BEAST_EXPECT(z.isZero());
        BEAST_EXPECT(!z.isNonZero());
        for (auto& d : z)
        {
            BEAST_EXPECT(d == 0);
        }

        test96 n { z };
        n++;
        BEAST_EXPECT(n == test96(1));
        n--;
        BEAST_EXPECT(n == beast::zero);
        BEAST_EXPECT(n == z);
        n--;
        BEAST_EXPECT(to_string(n) == "FFFFFFFFFFFFFFFFFFFFFFFF");
        n = beast::zero;
        BEAST_EXPECT(n == z);

        test96 zp1 { z };
        zp1++;
        test96 zm1 { z };
        zm1--;
        test96 x { zm1 ^ zp1 };
        uset.insert(x);
        BEAST_EXPECTS(to_string(x) == "FFFFFFFFFFFFFFFFFFFFFFFE", to_string(x));

        BEAST_EXPECT(uset.size() == 4);

//SETEX测试…
        test96 fromHex;
        BEAST_EXPECT(fromHex.SetHexExact(to_string(u)));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

//使用额外字符失败
        BEAST_EXPECT(! fromHex.SetHexExact("A" + to_string(u)));
        fromHex = z;

//失败，结尾有多余的字符
        BEAST_EXPECT(! fromHex.SetHexExact(to_string(u) + "A"));
//注意：实际上正确解析了十六进制的值
//在本例中，但这是一个实现细节，
//不保证，因此我们不检查这里的值。
        fromHex = z;

        BEAST_EXPECT(fromHex.SetHex(to_string(u)));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

//如果不严格，则允许前导空格/0x
        BEAST_EXPECT(fromHex.SetHex("  0x" + to_string(u)));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

//还允许使用其他前导字符（忽略）
        BEAST_EXPECT(fromHex.SetHex("FEFEFE" + to_string(u)));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

//无效的十六进制字符应失败（0替换为Z）
        BEAST_EXPECT(! fromHex.SetHex(
            boost::algorithm::replace_all_copy(to_string(u), "0", "Z")));
        fromHex = z;

        BEAST_EXPECT(fromHex.SetHex(to_string(u), true));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

//严格模式失败，带前导字符
        BEAST_EXPECT(! fromHex.SetHex("  0x" + to_string(u), true));
        fromHex = z;

//sethex忽略多余的前导十六进制，因此解析的值
//对于以下情况仍然正确（严格或非严格）
        BEAST_EXPECT(fromHex.SetHex("DEAD" + to_string(u), true ));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

        BEAST_EXPECT(fromHex.SetHex("DEAD" + to_string(u), false ));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;
    }
};

BEAST_DEFINE_TESTSUITE(base_uint, ripple_basics, ripple);

}  //命名空间测试
}  //命名空间波纹
