
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
#include <test/jtx.h>

namespace ripple {
namespace test {

class AccountOffers_test : public beast::unit_test::suite
{
public:
//测试助手
    static bool checkArraySize(Json::Value const& val, unsigned int size)
    {
        return val.isArray() &&
               val.size() == size;
    }

//测试助手
    static bool checkMarker(Json::Value const& val)
    {
        return val.isMember(jss::marker) &&
               val[jss::marker].isString() &&
               val[jss::marker].asString().size() > 0;
    }

    void testNonAdminMinLimit()
    {
        using namespace jtx;
        Env env {*this, envconfig(no_admin)};
        Account const gw ("G1");
        auto const USD_gw  = gw["USD"];
        Account const bob ("bob");
        auto const USD_bob = bob["USD"];

        env.fund(XRP(10000), gw, bob);
        env.trust(USD_gw(1000), bob);

//这是为了从吉瓦提供一些美元
//Bob帐户，以便正确
//提供给这些美元的报价
        env (pay (gw, bob, USD_gw (10)));
        unsigned const offer_count = 12u;
        for (auto i = 0u; i < offer_count; i++)
        {
            Json::Value jvo = offer(bob, XRP(100 + i), USD_gw(1));
            jvo[sfExpiration.fieldName] = 10000000u;
            env(jvo);
        }

//进行非限制的RPC调用
        auto const jro_nl = env.rpc("account_offers", bob.human())[jss::result][jss::offers];
        BEAST_EXPECT(checkArraySize(jro_nl, offer_count));

//现在做一个下限查询，应该得到“修正”
//至少有10个结果，其中有一个标记集
//总共超过10个
        Json::Value jvParams;
        jvParams[jss::account] = bob.human();
        jvParams[jss::limit]   = 1u;
        auto const  jrr_l = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
        auto const& jro_l = jrr_l[jss::offers];
        BEAST_EXPECT(checkMarker(jrr_l));
        BEAST_EXPECT(checkArraySize(jro_l, 10u));
    }

    void testSequential(bool asAdmin)
    {
        using namespace jtx;
        Env env {*this, asAdmin ? envconfig() : envconfig(no_admin)};
        Account const gw ("G1");
        auto const USD_gw  = gw["USD"];
        Account const bob ("bob");
        auto const USD_bob = bob["USD"];

        env.fund(XRP(10000), gw, bob);
        env.trust(USD_gw(1000), bob);

//这是为了从吉瓦提供一些美元
//Bob帐户，以便正确
//提供给这些美元的报价
        env (pay (gw, bob, USD_gw (10)));

        env(offer(bob, XRP(100), USD_bob(1)));
        env(offer(bob, XRP(100), USD_gw(1)));
        env(offer(bob, XRP(10),  USD_gw(2)));

//进行RPC调用
        auto const jro = env.rpc("account_offers", bob.human())[jss::result][jss::offers];
        if(BEAST_EXPECT(checkArraySize(jro, 3u)))
        {
            BEAST_EXPECT(jro[0u][jss::quality]                   == "100000000");
            BEAST_EXPECT(jro[0u][jss::taker_gets][jss::currency] == "USD");
            BEAST_EXPECT(jro[0u][jss::taker_gets][jss::issuer]   == gw.human());
            BEAST_EXPECT(jro[0u][jss::taker_gets][jss::value]    == "1");
            BEAST_EXPECT(jro[0u][jss::taker_pays]                == "100000000");

            BEAST_EXPECT(jro[1u][jss::quality]                   == "5000000");
            BEAST_EXPECT(jro[1u][jss::taker_gets][jss::currency] == "USD");
            BEAST_EXPECT(jro[1u][jss::taker_gets][jss::issuer]   == gw.human());
            BEAST_EXPECT(jro[1u][jss::taker_gets][jss::value]    == "2");
            BEAST_EXPECT(jro[1u][jss::taker_pays]                == "10000000");

            BEAST_EXPECT(jro[2u][jss::quality]                   == "100000000");
            BEAST_EXPECT(jro[2u][jss::taker_gets][jss::currency] == "USD");
            BEAST_EXPECT(jro[2u][jss::taker_gets][jss::issuer]   == bob.human());
            BEAST_EXPECT(jro[2u][jss::taker_gets][jss::value]    == "1");
            BEAST_EXPECT(jro[2u][jss::taker_pays]                == "100000000");
        }

        {
//现在对相同的数据进行限制（=1）查询
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
            jvParams[jss::limit]   = 1u;
            auto const  jrr_l_1 = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            auto const& jro_l_1 = jrr_l_1[jss::offers];
//限制参数的验证存在差异
//在管理和非管理请求之间。对于管理请求，
//limit参数不受健全默认值的影响，但具有
//非管理员应用了预先配置的限制范围。那是
//为什么在这两个场景中有不同的beast expect（）s？
            BEAST_EXPECT(checkArraySize(jro_l_1, asAdmin ? 1u : 3u));
            BEAST_EXPECT(asAdmin ? checkMarker(jrr_l_1) : (! jrr_l_1.isMember(jss::marker)));
            if (asAdmin)
            {
                BEAST_EXPECT(jro[0u] == jro_l_1[0u]);

//第二项…通过上一个标记
                jvParams[jss::marker] = jrr_l_1[jss::marker];
                auto const  jrr_l_2 = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
                auto const& jro_l_2 = jrr_l_2[jss::offers];
                BEAST_EXPECT(checkMarker(jrr_l_2));
                BEAST_EXPECT(checkArraySize(jro_l_2, 1u));
                BEAST_EXPECT(jro[1u] == jro_l_2[0u]);

//最后一项…通过上一个标记
                jvParams[jss::marker] = jrr_l_2[jss::marker];
                auto const  jrr_l_3 = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
                auto const& jro_l_3 = jrr_l_3[jss::offers];
                BEAST_EXPECT(! jrr_l_3.isMember(jss::marker));
                BEAST_EXPECT(checkArraySize(jro_l_3, 1u));
                BEAST_EXPECT(jro[2u] == jro_l_3[0u]);
            }
            else
            {
                BEAST_EXPECT(jro == jro_l_1);
            }
        }

        {
//现在对相同的数据进行限制（=0）查询
//因为我们在管理端口上操作，所以
//值0未调整为管理员请求的优化范围，因此
//在这种情况下，我们可以得到0个元素。非管理员
//请求，我们将应用限制默认值，因此我们的所有结果
//回来（我们低于最低结果限制）
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
            jvParams[jss::limit]   = 0u;
            auto const  jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            auto const& jro = jrr[jss::offers];
            BEAST_EXPECT(checkArraySize(jro, asAdmin ? 0u : 3u));
            BEAST_EXPECT(asAdmin ? checkMarker(jrr) : (! jrr.isMember(jss::marker)));
        }

    }

    void testBadInput()
    {
        using namespace jtx;
        Env env(*this);
        Account const gw ("G1");
        auto const USD_gw  = gw["USD"];
        Account const bob ("bob");
        auto const USD_bob = bob["USD"];

        env.fund(XRP(10000), gw, bob);
        env.trust(USD_gw(1000), bob);

        {
//无账户字段
            auto const jrr = env.rpc ("account_offers");
            BEAST_EXPECT(jrr[jss::error]         == "badSyntax");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Syntax error.");
        }

        {
//空字符串帐户
            Json::Value jvParams;
            jvParams[jss::account] = "";
            auto const jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "badSeed");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Disallowed seed.");
        }

        {
//虚假账户价值
            auto const jrr = env.rpc ("account_offers", "rNOT_AN_ACCOUNT")[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "actNotFound");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Account not found.");
        }

        {
//坏界限
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
jvParams[jss::limit]   = "0"; //不是整数
            auto const jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Invalid field 'limit', not unsigned integer.");
        }

        {
//无效标记
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
            jvParams[jss::marker]   = "NOT_A_MARKER";
            auto const jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Invalid parameters.");
        }

        {
//无效标记-不是字符串
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
            jvParams[jss::marker]   = 1;
            auto const jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Invalid field 'marker', not string.");
        }

        {
//请求坏的分类帐索引
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
            jvParams[jss::ledger_index]   = 10u;
            auto const jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "lgrNotFound");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "ledgerNotFound");
        }

    }

    void run() override
    {
        testSequential(true);
        testSequential(false);
        testBadInput();
        testNonAdminMinLimit();
    }

};

BEAST_DEFINE_TESTSUITE(AccountOffers,app,ripple);

}
}

