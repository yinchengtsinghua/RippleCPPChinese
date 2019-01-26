
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

#include <ripple/basics/RangeSet.h>
#include <ripple/beast/unit_test.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace ripple
{
class RangeSet_test : public beast::unit_test::suite
{
public:
    void
    testPrevMissing()
    {
        testcase("prevMissing");

//套装包括：
//〔0, 5〕
//[10，15]
//[20，25]
//等。。。

        RangeSet<std::uint32_t> set;
        for (std::uint32_t i = 0; i < 10; ++i)
            set.insert(range(10 * i, 10 * i + 5));

        for (std::uint32_t i = 1; i < 100; ++i)
        {
            boost::optional<std::uint32_t> expected;
//对于i<=6，域中没有缺少prev
            if (i > 6)
            {
                std::uint32_t const oneBelowRange = (10 * (i / 10)) - 1;

                expected = ((i % 10) > 6) ? (i - 1) : oneBelowRange;
            }
            BEAST_EXPECT(prevMissing(set, i) == expected);
        }
    }

    void
    testToString()
    {
        testcase("toString");

        RangeSet<std::uint32_t> set;
        BEAST_EXPECT(to_string(set) == "empty");

        set.insert(1);
        BEAST_EXPECT(to_string(set) == "1");

        set.insert(range(4u, 6u));
        BEAST_EXPECT(to_string(set) == "1,4-6");

        set.insert(2);
        BEAST_EXPECT(to_string(set) == "1-2,4-6");

        set.erase(range(4u, 5u));
        BEAST_EXPECT(to_string(set) == "1-2,6");
    }

    void
    testSerialization()
    {

        auto works = [](RangeSet<std::uint32_t> const & orig)
        {
            std::stringstream ss;
            boost::archive::binary_oarchive oa(ss);
            oa << orig;

            boost::archive::binary_iarchive ia(ss);
            RangeSet<std::uint32_t> deser;
            ia >> deser;

            return orig == deser;
        };

        RangeSet<std::uint32_t> rs;

        BEAST_EXPECT(works(rs));

        rs.insert(3);
        BEAST_EXPECT(works(rs));

        rs.insert(range(7u, 10u));
        BEAST_EXPECT(works(rs));

    }
    void
    run() override
    {
        testPrevMissing();
        testToString();
        testSerialization();
    }
};

BEAST_DEFINE_TESTSUITE(RangeSet, ripple_basics, ripple);

}  //命名空间波纹
