
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
    版权所有（c）2012，2015 Ripple Labs Inc.

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

#include <ripple/app/ledger/Ledger.h>
#include <test/jtx.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace test {

//测试将分类帐转换为shamapv2
//按预期工作

class SHAMapV2_test : public beast::unit_test::suite
{
    void
    testSHAMapV2()
    {
        jtx::Env env(*this);
        Config config;

        std::set<uint256> amendments;
        std::vector<uint256> amendments_v;
        amendments_v.push_back(from_hex_text<uint256>("12345"));
        amendments.insert(from_hex_text<uint256>("12345"));

        auto ledger =
            std::make_shared<Ledger>(create_genesis, config,
                amendments_v, env.app().family());
        BEAST_EXPECT(! getSHAMapV2 (ledger->info()));
        BEAST_EXPECT(ledger->stateMap().get_version() == SHAMap::version{1});
        BEAST_EXPECT(ledger->txMap().get_version() == SHAMap::version{1});
        BEAST_EXPECT(getEnabledAmendments(*ledger) == amendments);

        ledger =
            std::make_shared<Ledger>(*ledger, NetClock::time_point{});
        BEAST_EXPECT(! getSHAMapV2 (ledger->info()));
        BEAST_EXPECT(ledger->stateMap().get_version() == SHAMap::version{1});
        BEAST_EXPECT(ledger->txMap().get_version() == SHAMap::version{1});
        BEAST_EXPECT(getEnabledAmendments(*ledger) == amendments);

        ledger->make_v2();
        BEAST_EXPECT(getSHAMapV2 (ledger->info()));
        BEAST_EXPECT(ledger->stateMap().get_version() == SHAMap::version{2});
        BEAST_EXPECT(ledger->txMap().get_version() == SHAMap::version{2});
        BEAST_EXPECT(getEnabledAmendments(*ledger) == amendments);

        ledger = std::make_shared<Ledger>(*ledger, NetClock::time_point{});
        BEAST_EXPECT(getSHAMapV2 (ledger->info()));
        BEAST_EXPECT(ledger->stateMap().get_version() == SHAMap::version{2});
        BEAST_EXPECT(ledger->txMap().get_version() == SHAMap::version{2});
        BEAST_EXPECT(getEnabledAmendments(*ledger) == amendments);
    }

    void run() override
    {
        testSHAMapV2();
    }
};

BEAST_DEFINE_TESTSUITE(SHAMapV2,ledger,ripple);

}  //测试
}  //涟漪
