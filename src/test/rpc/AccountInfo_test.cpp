
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

#include <ripple/protocol/JsonFields.h>     //JSS：：定义
#include <ripple/protocol/Feature.h>
#include <test/jtx.h>

namespace ripple {
namespace test {

class AccountInfo_test : public beast::unit_test::suite
{
public:

    void testErrors()
    {
        using namespace jtx;
        Env env(*this);
        {
//账户信息没有账户。
            auto const info = env.rpc ("json", "account_info", "{ }");
            BEAST_EXPECT(info[jss::result][jss::error_message] ==
                "Missing field 'account'.");
        }
        {
//帐户\帐户标记格式不正确的信息。
            auto const info = env.rpc ("json", "account_info", "{\"account\": "
                "\"n94JNrQYkDrpt62bbSR7nVEhdyAvcJXRAsjEkFYyqRkh9SUTYEqV\"}");
            BEAST_EXPECT(info[jss::result][jss::error_message] ==
                "Disallowed seed.");
        }
        {
//帐户信息，帐户不在分类帐中。
            Account const bogie {"bogie"};
            auto const info = env.rpc ("json", "account_info",
                std::string ("{ ") + "\"account\": \"" + bogie.human() + "\"}");
            BEAST_EXPECT(info[jss::result][jss::error_message] ==
                "Account not found.");
        }
    }

//测试account_info中的“signer_lists”参数。
   void testSignerLists()
    {
        using namespace jtx;
        Env env(*this);
        Account const alice {"alice"};
        env.fund(XRP(1000), alice);

        auto const withoutSigners = std::string ("{ ") +
            "\"account\": \"" + alice.human() + "\"}";

        auto const withSigners = std::string ("{ ") +
            "\"account\": \"" + alice.human() + "\", " +
            "\"signer_lists\": true }";

//爱丽丝还没有签名人。
        {
//没有“签名者列表”参数的帐户信息。
            auto const info = env.rpc ("json", "account_info", withoutSigners);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            BEAST_EXPECT(! info[jss::result][jss::account_data].
                isMember (jss::signer_lists));
        }
        {
//带有“签名者列表”参数的帐户信息。
            auto const info = env.rpc ("json", "account_info", withSigners);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& data = info[jss::result][jss::account_data];
            BEAST_EXPECT(data.isMember (jss::signer_lists));
            auto const& signerLists = data[jss::signer_lists];
            BEAST_EXPECT(signerLists.isArray());
            BEAST_EXPECT(signerLists.size() == 0);
        }

//给爱丽丝一个签名人。
        Account const bogie {"bogie"};

        Json::Value const smallSigners = signers(alice, 2, { { bogie, 3 } });
        env(smallSigners);
        {
//没有“签名者列表”参数的帐户信息。
            auto const info = env.rpc ("json", "account_info", withoutSigners);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            BEAST_EXPECT(! info[jss::result][jss::account_data].
                isMember (jss::signer_lists));
        }
        {
//带有“签名者列表”参数的帐户信息。
            auto const info = env.rpc ("json", "account_info", withSigners);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& data = info[jss::result][jss::account_data];
            BEAST_EXPECT(data.isMember (jss::signer_lists));
            auto const& signerLists = data[jss::signer_lists];
            BEAST_EXPECT(signerLists.isArray());
            BEAST_EXPECT(signerLists.size() == 1);
            auto const& signers = signerLists[0u];
            BEAST_EXPECT(signers.isObject());
            BEAST_EXPECT(signers[sfSignerQuorum.jsonName] == 2);
            auto const& signerEntries = signers[sfSignerEntries.jsonName];
            BEAST_EXPECT(signerEntries.size() == 1);
            auto const& entry0 = signerEntries[0u][sfSignerEntry.jsonName];
            BEAST_EXPECT(entry0[sfSignerWeight.jsonName] == 3);
        }

//给艾丽丝一张签名人的大名单
        Account const demon {"demon"};
        Account const ghost {"ghost"};
        Account const haunt {"haunt"};
        Account const jinni {"jinni"};
        Account const phase {"phase"};
        Account const shade {"shade"};
        Account const spook {"spook"};

        Json::Value const bigSigners = signers(alice, 4, {
            {bogie, 1}, {demon, 1}, {ghost, 1}, {haunt, 1},
            {jinni, 1}, {phase, 1}, {shade, 1}, {spook, 1}, });
        env(bigSigners);
        {
//带有“签名者列表”参数的帐户信息。
            auto const info = env.rpc ("json", "account_info", withSigners);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& data = info[jss::result][jss::account_data];
            BEAST_EXPECT(data.isMember (jss::signer_lists));
            auto const& signerLists = data[jss::signer_lists];
            BEAST_EXPECT(signerLists.isArray());
            BEAST_EXPECT(signerLists.size() == 1);
            auto const& signers = signerLists[0u];
            BEAST_EXPECT(signers.isObject());
            BEAST_EXPECT(signers[sfSignerQuorum.jsonName] == 4);
            auto const& signerEntries = signers[sfSignerEntries.jsonName];
            BEAST_EXPECT(signerEntries.size() == 8);
            for (unsigned i = 0u; i < 8; ++i)
            {
                auto const& entry = signerEntries[i][sfSignerEntry.jsonName];
                BEAST_EXPECT(entry.size() == 2);
                BEAST_EXPECT(entry.isMember(sfAccount.jsonName));
                BEAST_EXPECT(entry[sfSignerWeight.jsonName] == 1);
            }
        }
    }

//测试account_info，version 2 api中的“signer_lists”参数。
   void testSignerListsV2()
    {
        using namespace jtx;
        Env env(*this);
        Account const alice {"alice"};
        env.fund(XRP(1000), alice);

        auto const withoutSigners = std::string ("{ ") +
            "\"jsonrpc\": \"2.0\", "
            "\"ripplerpc\": \"2.0\", "
            "\"id\": 5, "
            "\"method\": \"account_info\", "
            "\"params\": { "
            "\"account\": \"" + alice.human() + "\"}}";

        auto const withSigners = std::string ("{ ") +
            "\"jsonrpc\": \"2.0\", "
            "\"ripplerpc\": \"2.0\", "
            "\"id\": 6, "
            "\"method\": \"account_info\", "
            "\"params\": { "
            "\"account\": \"" + alice.human() + "\", " +
            "\"signer_lists\": true }}";
//爱丽丝还没有签名人。
        {
//没有“签名者列表”参数的帐户信息。
            auto const info = env.rpc ("json2", withoutSigners);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            BEAST_EXPECT(! info[jss::result][jss::account_data].
                isMember (jss::signer_lists));
            BEAST_EXPECT(info.isMember(jss::jsonrpc) && info[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(info.isMember(jss::ripplerpc) && info[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(info.isMember(jss::id) && info[jss::id] == 5);
        }
        {
//带有“签名者列表”参数的帐户信息。
            auto const info = env.rpc ("json2", withSigners);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& data = info[jss::result][jss::account_data];
            BEAST_EXPECT(data.isMember (jss::signer_lists));
            auto const& signerLists = data[jss::signer_lists];
            BEAST_EXPECT(signerLists.isArray());
            BEAST_EXPECT(signerLists.size() == 0);
            BEAST_EXPECT(info.isMember(jss::jsonrpc) && info[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(info.isMember(jss::ripplerpc) && info[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(info.isMember(jss::id) && info[jss::id] == 6);
        }
        {
//将上述两项都作为批处理作业执行
            auto const info = env.rpc ("json2", '[' + withoutSigners + ", "
                                                    + withSigners + ']');
            BEAST_EXPECT(info[0u].isMember(jss::result) &&
                info[0u][jss::result].isMember(jss::account_data));
            BEAST_EXPECT(! info[0u][jss::result][jss::account_data].
                isMember (jss::signer_lists));
            BEAST_EXPECT(info[0u].isMember(jss::jsonrpc) && info[0u][jss::jsonrpc] == "2.0");
            BEAST_EXPECT(info[0u].isMember(jss::ripplerpc) && info[0u][jss::ripplerpc] == "2.0");
            BEAST_EXPECT(info[0u].isMember(jss::id) && info[0u][jss::id] == 5);

            BEAST_EXPECT(info[1u].isMember(jss::result) &&
                info[1u][jss::result].isMember(jss::account_data));
            auto const& data = info[1u][jss::result][jss::account_data];
            BEAST_EXPECT(data.isMember (jss::signer_lists));
            auto const& signerLists = data[jss::signer_lists];
            BEAST_EXPECT(signerLists.isArray());
            BEAST_EXPECT(signerLists.size() == 0);
            BEAST_EXPECT(info[1u].isMember(jss::jsonrpc) && info[1u][jss::jsonrpc] == "2.0");
            BEAST_EXPECT(info[1u].isMember(jss::ripplerpc) && info[1u][jss::ripplerpc] == "2.0");
            BEAST_EXPECT(info[1u].isMember(jss::id) && info[1u][jss::id] == 6);
        }

//给爱丽丝一个签名人。
        Account const bogie {"bogie"};

        Json::Value const smallSigners = signers(alice, 2, { { bogie, 3 } });
        env(smallSigners);
        {
//没有“签名者列表”参数的帐户信息。
            auto const info = env.rpc ("json2", withoutSigners);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            BEAST_EXPECT(! info[jss::result][jss::account_data].
                isMember (jss::signer_lists));
            BEAST_EXPECT(info.isMember(jss::jsonrpc) && info[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(info.isMember(jss::ripplerpc) && info[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(info.isMember(jss::id) && info[jss::id] == 5);
        }
        {
//带有“签名者列表”参数的帐户信息。
            auto const info = env.rpc ("json2", withSigners);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& data = info[jss::result][jss::account_data];
            BEAST_EXPECT(data.isMember (jss::signer_lists));
            auto const& signerLists = data[jss::signer_lists];
            BEAST_EXPECT(signerLists.isArray());
            BEAST_EXPECT(signerLists.size() == 1);
            auto const& signers = signerLists[0u];
            BEAST_EXPECT(signers.isObject());
            BEAST_EXPECT(signers[sfSignerQuorum.jsonName] == 2);
            auto const& signerEntries = signers[sfSignerEntries.jsonName];
            BEAST_EXPECT(signerEntries.size() == 1);
            auto const& entry0 = signerEntries[0u][sfSignerEntry.jsonName];
            BEAST_EXPECT(entry0[sfSignerWeight.jsonName] == 3);
            BEAST_EXPECT(info.isMember(jss::jsonrpc) && info[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(info.isMember(jss::ripplerpc) && info[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(info.isMember(jss::id) && info[jss::id] == 6);
        }

//给艾丽丝一张签名人的大名单
        Account const demon {"demon"};
        Account const ghost {"ghost"};
        Account const haunt {"haunt"};
        Account const jinni {"jinni"};
        Account const phase {"phase"};
        Account const shade {"shade"};
        Account const spook {"spook"};

        Json::Value const bigSigners = signers(alice, 4, {
            {bogie, 1}, {demon, 1}, {ghost, 1}, {haunt, 1},
            {jinni, 1}, {phase, 1}, {shade, 1}, {spook, 1}, });
        env(bigSigners);
        {
//带有“签名者列表”参数的帐户信息。
            auto const info = env.rpc ("json2", withSigners);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& data = info[jss::result][jss::account_data];
            BEAST_EXPECT(data.isMember (jss::signer_lists));
            auto const& signerLists = data[jss::signer_lists];
            BEAST_EXPECT(signerLists.isArray());
            BEAST_EXPECT(signerLists.size() == 1);
            auto const& signers = signerLists[0u];
            BEAST_EXPECT(signers.isObject());
            BEAST_EXPECT(signers[sfSignerQuorum.jsonName] == 4);
            auto const& signerEntries = signers[sfSignerEntries.jsonName];
            BEAST_EXPECT(signerEntries.size() == 8);
            for (unsigned i = 0u; i < 8; ++i)
            {
                auto const& entry = signerEntries[i][sfSignerEntry.jsonName];
                BEAST_EXPECT(entry.size() == 2);
                BEAST_EXPECT(entry.isMember(sfAccount.jsonName));
                BEAST_EXPECT(entry[sfSignerWeight.jsonName] == 1);
            }
            BEAST_EXPECT(info.isMember(jss::jsonrpc) && info[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(info.isMember(jss::ripplerpc) && info[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(info.isMember(jss::id) && info[jss::id] == 6);
        }
    }

    void run() override
    {
        testErrors();
        testSignerLists();
        testSignerListsV2();
    }
};

BEAST_DEFINE_TESTSUITE(AccountInfo,app,ripple);

}
}

