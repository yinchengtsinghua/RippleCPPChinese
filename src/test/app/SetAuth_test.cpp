
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

#include <test/jtx.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>

namespace ripple {
namespace test {

struct SetAuth_test : public beast::unit_test::suite
{
//只在信任行上设置tfsetfauth标志
//如果信任线不存在，则应该
//在新规则下创建。
    static
    Json::Value
    auth (jtx::Account const& account,
        jtx::Account const& dest,
            std::string const& currency)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::Account] = account.human();
        jv[jss::LimitAmount] = STAmount(
            { to_currency(currency), dest }).getJson(0);
        jv[jss::TransactionType] = "TrustSet";
        jv[jss::Flags] = tfSetfAuth;
        return jv;
    }

    void testAuth(FeatureBitset features)
    {
//调用方应始终重置FeatureTrustSetAuth。
        BEAST_EXPECT(!features[featureTrustSetAuth]);

        using namespace jtx;
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        {
            Env env(*this, features);
            env.fund(XRP(100000), "alice", gw);
            env(fset(gw, asfRequireAuth));
            env(auth(gw, "alice", "USD"),       ter(tecNO_LINE_REDUNDANT));
        }
        {
            Env env(*this, features | featureTrustSetAuth);

            env.fund(XRP(100000), "alice", "bob", gw);
            env(fset(gw, asfRequireAuth));
            env(auth(gw, "alice", "USD"));
            BEAST_EXPECT(env.le(
                keylet::line(Account("alice").id(),
                    gw.id(), USD.currency)));
            env(trust("alice", USD(1000)));
            env(trust("bob", USD(1000)));
            env(pay(gw, "alice", USD(100)));
env(pay(gw, "bob", USD(100)),       ter(tecPATH_DRY)); //应该是terno-auth
env(pay("alice", "bob", USD(50)),   ter(tecPATH_DRY)); //应该是terno-auth
        }
    }

    void run() override
    {
        using namespace jtx;
        auto const sa = supported_amendments();
        testAuth(sa - featureTrustSetAuth - featureFlow - fix1373 - featureFlowCross);
        testAuth(sa - featureTrustSetAuth - fix1373 - featureFlowCross);
        testAuth(sa - featureTrustSetAuth - featureFlowCross);
        testAuth(sa - featureTrustSetAuth);
    }
};

BEAST_DEFINE_TESTSUITE(SetAuth,test,ripple);

} //测试
} //涟漪
