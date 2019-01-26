
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
    版权所有（c）2017 Ripple Labs Inc.

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
#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/STAmount.h>

namespace ripple {

class OwnerInfo_test : public beast::unit_test::suite
{
    void
    testBadInput ()
    {
        testcase ("Bad input to owner_info");

        using namespace test::jtx;
        Env env {*this};

        auto const alice = Account {"alice"};
        env.fund (XRP(10000), alice);
        env.close ();

{ //缺少帐户字段
            auto const result =
                env.rpc ("json", "owner_info", "{}") [jss::result];
            BEAST_EXPECT (result[jss::error] == "invalidParams");
            BEAST_EXPECT (result[jss::error_message] ==
                "Missing field 'account'.");
        }

{ //要求空帐户
            Json::Value params;
            params[jss::account] = "";
            auto const result = env.rpc ("json", "owner_info",
                to_string(params)) [jss::result];
            if (BEAST_EXPECT (
                result.isMember(jss::accepted) &&
                result.isMember(jss::current)))
            {
                BEAST_EXPECT (result[jss::accepted][jss::error] == "badSeed");
                BEAST_EXPECT (result[jss::accepted][jss::error_message] ==
                    "Disallowed seed.");
                BEAST_EXPECT (result[jss::current][jss::error] == "badSeed");
                BEAST_EXPECT (result[jss::current][jss::error_message] ==
                    "Disallowed seed.");
            }
        }

{ //询问不存在的帐户
//这似乎是一个错误，但当前的impl
//（已弃用）不返回错误，只返回空字段。
            Json::Value params;
            params[jss::account] = Account{"bob"}.human();
            auto const result = env.rpc ("json", "owner_info",
                to_string(params)) [jss::result];
            BEAST_EXPECT (result[jss::accepted] == Json::objectValue);
            BEAST_EXPECT (result[jss::current] == Json::objectValue);
            BEAST_EXPECT (result[jss::status] == "success");
        }
    }

    void
    testBasic ()
    {
        testcase ("Basic request for owner_info");

        using namespace test::jtx;
        Env env {*this};

        auto const alice = Account {"alice"};
        auto const gw = Account {"gateway"};
        env.fund (XRP(10000), alice, gw);
        auto const USD = gw["USD"];
        auto const CNY = gw["CNY"];
        env(trust(alice, USD(1000)));
        env(trust(alice, CNY(1000)));
        env(offer(alice, USD(1), XRP(1000)));
        env.close();

        env(pay(gw, alice, USD(50)));
        env(pay(gw, alice, CNY(50)));
        env(offer(alice, CNY(2), XRP(1000)));

        Json::Value params;
        params[jss::account] = alice.human();
        auto const result = env.rpc ("json", "owner_info",
            to_string(params)) [jss::result];
        if (! BEAST_EXPECT (
                result.isMember(jss::accepted) &&
                result.isMember(jss::current)))
        {
            return;
        }

//接受的分类帐分录
        if (! BEAST_EXPECT (result[jss::accepted].isMember(jss::ripple_lines)))
            return;
        auto lines = result[jss::accepted][jss::ripple_lines];
        if (! BEAST_EXPECT (lines.isArray() && lines.size() == 2))
            return;

        BEAST_EXPECT (
            lines[0u][sfBalance.fieldName] ==
            (STAmount{Issue{to_currency("CNY"), noAccount()}, 0}
                 .value().getJson(0)));
        BEAST_EXPECT (
            lines[0u][sfHighLimit.fieldName] ==
            alice["CNY"](1000).value().getJson(0));
        BEAST_EXPECT (
            lines[0u][sfLowLimit.fieldName] ==
            gw["CNY"](0).value().getJson(0));

        BEAST_EXPECT (
            lines[1u][sfBalance.fieldName] ==
            (STAmount{Issue{to_currency("USD"), noAccount()}, 0}
                .value().getJson(0)));
        BEAST_EXPECT (
            lines[1u][sfHighLimit.fieldName] ==
            alice["USD"](1000).value().getJson(0));
        BEAST_EXPECT (
            lines[1u][sfLowLimit.fieldName] ==
            USD(0).value().getJson(0));

        if (! BEAST_EXPECT (result[jss::accepted].isMember(jss::offers)))
            return;
        auto offers = result[jss::accepted][jss::offers];
        if (! BEAST_EXPECT (offers.isArray() && offers.size() == 1))
            return;

        BEAST_EXPECT (
            offers[0u][jss::Account] == alice.human());
        BEAST_EXPECT (
            offers[0u][sfTakerGets.fieldName] == XRP(1000).value().getJson(0));
        BEAST_EXPECT (
            offers[0u][sfTakerPays.fieldName] == USD(1).value().getJson(0));


//当前分类账分录
        if (! BEAST_EXPECT (result[jss::current].isMember(jss::ripple_lines)))
            return;
        lines = result[jss::current][jss::ripple_lines];
        if (! BEAST_EXPECT (lines.isArray() && lines.size() == 2))
            return;

        BEAST_EXPECT (
            lines[0u][sfBalance.fieldName] ==
            (STAmount{Issue{to_currency("CNY"), noAccount()}, -50}
                 .value().getJson(0)));
        BEAST_EXPECT (
            lines[0u][sfHighLimit.fieldName] ==
            alice["CNY"](1000).value().getJson(0));
        BEAST_EXPECT (
            lines[0u][sfLowLimit.fieldName] ==
            gw["CNY"](0).value().getJson(0));

        BEAST_EXPECT (
            lines[1u][sfBalance.fieldName] ==
            (STAmount{Issue{to_currency("USD"), noAccount()}, -50}
                .value().getJson(0)));
        BEAST_EXPECT (
            lines[1u][sfHighLimit.fieldName] ==
            alice["USD"](1000).value().getJson(0));
        BEAST_EXPECT (
            lines[1u][sfLowLimit.fieldName] ==
            gw["USD"](0).value().getJson(0));

        if (! BEAST_EXPECT (result[jss::current].isMember(jss::offers)))
            return;
        offers = result[jss::current][jss::offers];
//当前额外报价1份（共2份）
        if (! BEAST_EXPECT (offers.isArray() && offers.size() == 2))
            return;

        BEAST_EXPECT (
            offers[1u] == result[jss::accepted][jss::offers][0u]);
        BEAST_EXPECT (
            offers[0u][jss::Account] == alice.human());
        BEAST_EXPECT (
            offers[0u][sfTakerGets.fieldName] == XRP(1000).value().getJson(0));
        BEAST_EXPECT (
            offers[0u][sfTakerPays.fieldName] == CNY(2).value().getJson(0));
    }

public:
    void run () override
    {
        testBadInput ();
        testBasic ();
    }
};

BEAST_DEFINE_TESTSUITE(OwnerInfo,app,ripple);

} //涟漪

