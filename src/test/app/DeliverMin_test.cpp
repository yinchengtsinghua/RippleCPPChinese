
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
  版权所有（c）2012-2015 Ripple Labs Inc.

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

#include <test/jtx.h>
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/Feature.h>

namespace ripple {
namespace test {

class DeliverMin_test : public beast::unit_test::suite
{
public:
    void
    test_convert_all_of_an_asset(FeatureBitset features)
    {
        testcase("Convert all of an asset using DeliverMin");

        using namespace jtx;
        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        {
            Env env(*this, features);
            env.fund(XRP(10000), "alice", "bob", "carol", gw);
            env.trust(USD(100), "alice", "bob", "carol");
            env(pay("alice", "bob", USD(10)), delivermin(USD(10)),  ter(temBAD_AMOUNT));
            env(pay("alice", "bob", USD(10)), delivermin(USD(-5)),
                txflags(tfPartialPayment),                          ter(temBAD_AMOUNT));
            env(pay("alice", "bob", USD(10)), delivermin(XRP(5)),
                txflags(tfPartialPayment),                          ter(temBAD_AMOUNT));
            env(pay("alice", "bob", USD(10)),
                delivermin(Account("carol")["USD"](5)),
                txflags(tfPartialPayment),                          ter(temBAD_AMOUNT));
            env(pay("alice", "bob", USD(10)), delivermin(USD(15)),
                txflags(tfPartialPayment),                          ter(temBAD_AMOUNT));
            env(pay(gw, "carol", USD(50)));
            env(offer("carol", XRP(5), USD(5)));
            env(pay("alice", "bob", USD(10)), paths(XRP),
                delivermin(USD(7)), txflags(tfPartialPayment),
                sendmax(XRP(5)),                                   ter(tecPATH_PARTIAL));
            env.require(balance("alice", XRP(9999.99999)));
            env.require(balance("bob", XRP(10000)));
        }

        {
            Env env(*this, features);
            env.fund(XRP(10000), "alice", "bob", gw);
            env.trust(USD(1000), "alice", "bob");
            env(pay(gw, "bob", USD(100)));
            env(offer("bob", XRP(100), USD(100)));
            env(pay("alice", "alice", USD(10000)), paths(XRP),
                delivermin(USD(100)), txflags(tfPartialPayment),
                sendmax(XRP(100)));
            env.require(balance("alice", USD(100)));
        }

        {
            Env env(*this, features);
            env.fund(XRP(10000), "alice", "bob", "carol", gw);
            env.trust(USD(1000), "bob", "carol");
            env(pay(gw, "bob", USD(200)));
            env(offer("bob", XRP(100), USD(100)));
            env(offer("bob", XRP(1000), USD(100)));
            env(offer("bob", XRP(10000), USD(100)));
            env(pay("alice", "carol", USD(10000)), paths(XRP),
                delivermin(USD(200)), txflags(tfPartialPayment),
                sendmax(XRP(1000)),                                 ter(tecPATH_PARTIAL));
            env(pay("alice", "carol", USD(10000)), paths(XRP),
                delivermin(USD(200)), txflags(tfPartialPayment),
                sendmax(XRP(1100)));
            env.require(balance("bob", USD(0)));
            env.require(balance("carol", USD(200)));
        }

        {
            Env env(*this, features);
            env.fund(XRP(10000), "alice", "bob", "carol", "dan", gw);
            env.trust(USD(1000), "bob", "carol", "dan");
            env(pay(gw, "bob", USD(100)));
            env(pay(gw, "dan", USD(100)));
            env(offer("bob", XRP(100), USD(100)));
            env(offer("bob", XRP(1000), USD(100)));
            env(offer("dan", XRP(100), USD(100)));
            env(pay("alice", "carol", USD(10000)), paths(XRP),
                delivermin(USD(200)), txflags(tfPartialPayment),
                sendmax(XRP(200)));
            env.require(balance("bob", USD(0)));
            env.require(balance("carol", USD(200)));
            env.require(balance("dan", USD(0)));
        }
    }

    void
    run() override
    {
        using namespace jtx;
        auto const sa = supported_amendments();
        test_convert_all_of_an_asset(sa - featureFlow - fix1373 - featureFlowCross);
        test_convert_all_of_an_asset(sa - fix1373 - featureFlowCross);
        test_convert_all_of_an_asset(sa - featureFlowCross);
        test_convert_all_of_an_asset(sa);
    }
};

BEAST_DEFINE_TESTSUITE(DeliverMin,app,ripple);

} //测试
} //涟漪
