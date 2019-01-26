
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
#include <ripple/protocol/JsonFields.h>

namespace ripple {

class AccountCurrencies_test : public beast::unit_test::suite
{
    void
    testBadInput ()
    {
        testcase ("Bad input to account_currencies");

        using namespace test::jtx;
        Env env {*this};

        auto const alice = Account {"alice"};
        env.fund (XRP(10000), alice);
        env.close ();

{ //无效的分类帐（哈希）
            Json::Value params;
            params[jss::ledger_hash] = 1;
            auto const result = env.rpc ("json", "account_currencies",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "invalidParams");
            BEAST_EXPECT (result[jss::error_message] ==
                "ledgerHashNotString");
        }

{ //缺少帐户字段
            auto const result =
                env.rpc ("json", "account_currencies", "{}") [jss::result];
            BEAST_EXPECT (result[jss::error] == "invalidParams");
            BEAST_EXPECT (result[jss::error_message] ==
                "Missing field 'account'.");
        }

{ //严格模式，比特币令牌无效
            Json::Value params;
params[jss::account] = "llIIOO"; //这些在比特币字母表中是无效的
            params[jss::strict] = true;
            auto const result = env.rpc ("json", "account_currencies",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "actMalformed");
            BEAST_EXPECT (result[jss::error_message] ==
                "Account malformed.");
        }

{ //严格模式，使用格式正确的比特币令牌
            Json::Value params;
            params[jss::account] = base58EncodeTokenBitcoin (
                TokenType::AccountID, alice.id().data(), alice.id().size());
            params[jss::strict] = true;
            auto const result = env.rpc ("json", "account_currencies",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "actBitcoin");
            BEAST_EXPECT (result[jss::error_message] ==
                "Account is bitcoin address.");
        }

{ //询问不存在的帐户
            Json::Value params;
            params[jss::account] = Account{"bob"}.human();
            auto const result = env.rpc ("json", "account_currencies",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "actNotFound");
            BEAST_EXPECT (result[jss::error_message] ==
                "Account not found.");
        }
    }

    void
    testBasic ()
    {
        testcase ("Basic request for account_currencies");

        using namespace test::jtx;
        Env env {*this};

        auto const alice = Account {"alice"};
        auto const gw = Account {"gateway"};
        env.fund (XRP(10000), alice, gw);
        char currencySuffix {'A'};
std::vector<boost::optional<IOU>> gwCurrencies (26); //A—Z
        std::generate (gwCurrencies.begin(), gwCurrencies.end(),
            [&]()
            {
                auto gwc = gw[std::string("US") + currencySuffix++];
                env (trust (alice, gwc (100)));
                return gwc;
            });
        env.close ();

        Json::Value params;
        params[jss::account] = alice.human();
        auto result = env.rpc ("json", "account_currencies",
            boost::lexical_cast<std::string>(params)) [jss::result];

        auto arrayCheck =
            [&result] (
                Json::StaticString const& fld,
                std::vector<boost::optional<IOU>> const& expected) -> bool
            {
                bool stat =
                    result.isMember (fld) &&
                    result[fld].isArray() &&
                    result[fld].size() == expected.size();
                for (size_t i = 0; stat && i < expected.size(); ++i)
                {
                    Currency foo;
                    stat &= (
                        to_string(expected[i].value().currency) ==
                        result[fld][i].asString()
                    );
                }
                return stat;
            };

        BEAST_EXPECT (arrayCheck (jss::receive_currencies, gwCurrencies));
        BEAST_EXPECT (arrayCheck (jss::send_currencies, {}));

//现在对每种货币进行支付
        for (auto const& c : gwCurrencies)
            env (pay (gw, alice, c.value()(50)));

//发送货币现在应该填充
        result = env.rpc ("json", "account_currencies",
            boost::lexical_cast<std::string>(params)) [jss::result];
        BEAST_EXPECT (arrayCheck (jss::receive_currencies, gwCurrencies));
        BEAST_EXPECT (arrayCheck (jss::send_currencies, gwCurrencies));

//冻结美元信托额度并验证收到的货币
//不改变
        env(trust(alice, gw["USD"](100), tfSetFreeze));
        result = env.rpc ("account_lines", alice.human());
        for (auto const l : result[jss::lines])
            BEAST_EXPECT(
                l[jss::freeze].asBool() == (l[jss::currency] == "USD"));
        result = env.rpc ("json", "account_currencies",
            boost::lexical_cast<std::string>(params)) [jss::result];
        BEAST_EXPECT (arrayCheck (jss::receive_currencies, gwCurrencies));
        BEAST_EXPECT (arrayCheck (jss::send_currencies, gwCurrencies));
//清除冻结
        env(trust(alice, gw["USD"](100), tfClearFreeze));

//为美国支付从Alice到GW的信托额度
        env (pay (gw, alice, gw["USA"](50)));
//美国现在应该从接收货币中消失了
        result = env.rpc ("json", "account_currencies",
            boost::lexical_cast<std::string>(params)) [jss::result];
        decltype(gwCurrencies) gwCurrenciesNoUSA (gwCurrencies.begin() + 1,
            gwCurrencies.end());
        BEAST_EXPECT (arrayCheck (jss::receive_currencies, gwCurrenciesNoUSA));
        BEAST_EXPECT (arrayCheck (jss::send_currencies, gwCurrencies));

//从gw到alice增加信任，然后耗尽信任线
//所以给爱丽丝寄货币的话，现在就不用美国了
        env (trust (gw, alice["USA"] (100)));
        env (pay (alice, gw, alice["USA"](200)));
        result = env.rpc ("json", "account_currencies",
            boost::lexical_cast<std::string>(params)) [jss::result];
        BEAST_EXPECT (arrayCheck (jss::receive_currencies, gwCurrencies));
        BEAST_EXPECT (arrayCheck (jss::send_currencies, gwCurrenciesNoUSA));
    }

public:
    void run () override
    {
        testBadInput ();
        testBasic ();
    }
};

BEAST_DEFINE_TESTSUITE(AccountCurrencies,app,ripple);

} //涟漪

