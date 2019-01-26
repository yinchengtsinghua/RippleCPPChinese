
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

#include <ripple/basics/mulDiv.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace test {

struct mulDiv_test : beast::unit_test::suite
{
    void run() override
    {
        const auto max = std::numeric_limits<std::uint64_t>::max();
        const std::uint64_t max32 = std::numeric_limits<std::uint32_t>::max();

        auto result = mulDiv(85, 20, 5);
        BEAST_EXPECT(result.first && result.second == 340);
        result = mulDiv(20, 85, 5);
        BEAST_EXPECT(result.first && result.second == 340);

        result = mulDiv(0, max - 1, max - 3);
        BEAST_EXPECT(result.first && result.second == 0);
        result = mulDiv(max - 1, 0, max - 3);
        BEAST_EXPECT(result.first && result.second == 0);

        result = mulDiv(max, 2, max / 2);
        BEAST_EXPECT(result.first && result.second == 4);
        result = mulDiv(max, 1000, max / 1000);
        BEAST_EXPECT(result.first && result.second == 1000000);
        result = mulDiv(max, 1000, max / 1001);
        BEAST_EXPECT(result.first && result.second == 1001000);
        result = mulDiv(max32 + 1, max32 + 1, 5);
        BEAST_EXPECT(result.first && result.second == 3689348814741910323);

//溢流
        result = mulDiv(max - 1, max - 2, 5);
        BEAST_EXPECT(!result.first && result.second == max);
    }
};

BEAST_DEFINE_TESTSUITE(mulDiv, ripple_basics, ripple);

}  //命名空间测试
}  //命名空间波纹
