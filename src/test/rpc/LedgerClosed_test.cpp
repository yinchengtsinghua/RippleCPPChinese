
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
    版权所有（c）2016 Ripple Labs Inc.

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

#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/Feature.h>
#include <test/jtx.h>

namespace ripple {

class LedgerClosed_test : public beast::unit_test::suite
{
public:

    void testMonitorRoot()
    {
        using namespace test::jtx;
        Env env {*this, FeatureBitset{}};
        Account const alice {"alice"};
        env.fund(XRP(10000), alice);

        auto lc_result = env.rpc("ledger_closed") [jss::result];
        BEAST_EXPECT(lc_result[jss::ledger_hash]  == "8AEDBB96643962F1D40F01E25632ABB3C56C9F04B0231EE4B18248B90173D189");
        BEAST_EXPECT(lc_result[jss::ledger_index] == 2);

        env.close();
        auto const ar_master = env.le(env.master);
        BEAST_EXPECT(ar_master->getAccountID(sfAccount) == env.master.id());
        BEAST_EXPECT((*ar_master)[sfBalance] == drops( 99999989999999980 ));

        auto const ar_alice = env.le(alice);
        BEAST_EXPECT(ar_alice->getAccountID(sfAccount) == alice.id());
        BEAST_EXPECT((*ar_alice)[sfBalance] == XRP( 10000 ));

        lc_result = env.rpc("ledger_closed") [jss::result];
        BEAST_EXPECT(lc_result[jss::ledger_hash]  == "7C3EEDB3124D92E49E75D81A8826A2E65A75FD71FC3FD6F36FEB803C5F1D812D");
        BEAST_EXPECT(lc_result[jss::ledger_index] == 3);
    }

    void run() override
    {
        testMonitorRoot();
    }
};

BEAST_DEFINE_TESTSUITE(LedgerClosed,app,ripple);

}

