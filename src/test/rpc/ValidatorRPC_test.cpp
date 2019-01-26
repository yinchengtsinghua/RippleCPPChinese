
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

#include <ripple/app/main/BasicApp.h>
#include <ripple/app/misc/ValidatorSite.h>
#include <ripple/basics/base64.h>
#include <ripple/beast/unit_test.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/Sign.h>
#include <test/jtx.h>
#include <test/jtx/TrustedPublisherServer.h>

#include <set>

namespace ripple {

namespace test {

class ValidatorRPC_test : public beast::unit_test::suite
{
    using Validator = TrustedPublisherServer::Validator;

    static
    Validator
    randomValidator ()
    {
        auto const secret = randomSecretKey();
        auto const masterPublic =
            derivePublicKey(KeyType::ed25519, secret);
        auto const signingKeys = randomKeyPair(KeyType::secp256k1);
        return { masterPublic, signingKeys.first, makeManifestString (
            masterPublic, secret, signingKeys.first, signingKeys.second, 1) };
    }

    static
    std::string
    makeManifestString(
        PublicKey const& pk,
        SecretKey const& sk,
        PublicKey const& spk,
        SecretKey const& ssk,
        int seq)
    {
        STObject st(sfGeneric);
        st[sfSequence] = seq;
        st[sfPublicKey] = pk;
        st[sfSigningPubKey] = spk;

        sign(st, HashPrefix::manifest, *publicKeyType(spk), ssk);
        sign(
            st,
            HashPrefix::manifest,
            *publicKeyType(pk),
            sk,
            sfMasterSignature);

        Serializer s;
        st.add(s);

        return base64_encode(
            std::string(static_cast<char const*>(s.data()), s.size()));
    }

public:
    void
    testPrivileges()
    {
        using namespace test::jtx;

        for (bool const isAdmin : {true, false})
        {
            for (std::string cmd : {"validators", "validator_list_sites"})
            {
                Env env{*this, isAdmin ? envconfig() : envconfig(no_admin)};
                auto const jrr = env.rpc(cmd)[jss::result];
                if (isAdmin)
                {
                    BEAST_EXPECT(!jrr.isMember(jss::error));
                    BEAST_EXPECT(jrr[jss::status] == "success");
                }
                else
                {
//当前HTTP/S服务器处理程序返回HTTP 403
//这里是错误代码，而不是nopermission json错误。
//jsonRpcClient只接受该错误并返回空值。
//结果。
                    BEAST_EXPECT(jrr.isNull());
                }
            }

            {
                Env env{*this, isAdmin ? envconfig() : envconfig(no_admin)};
                auto const jrr = env.rpc("server_info")[jss::result];
                BEAST_EXPECT(jrr[jss::status] == "success");
                BEAST_EXPECT(jrr[jss::info].isMember(
                                 jss::validator_list) == isAdmin);
            }

            {
                Env env{*this, isAdmin ? envconfig() : envconfig(no_admin)};
                auto const jrr = env.rpc("server_state")[jss::result];
                BEAST_EXPECT(jrr[jss::status] == "success");
                BEAST_EXPECT(jrr[jss::state].isMember(
                                 jss::validator_list_expires) == isAdmin);
            }
        }
    }

    void
    testStaticUNL()
    {
        using namespace test::jtx;

        std::set<std::string> const keys = {
            "n949f75evCHwgyP4fPVgaHqNHxUVN15PsJEZ3B3HnXPcPjcZAoy7",
            "n9MD5h24qrQqiyBC8aeqqCWvpiBiYQ3jxSr91uiDvmrkyHRdYLUj"};
        Env env{
            *this,
            envconfig([&keys](std::unique_ptr<Config> cfg) {
                for (auto const& key : keys)
                    cfg->section(SECTION_VALIDATORS).append(key);
                return cfg;
            }),
        };

//服务器信息报告自非动态以来的最大过期时间
        {
            auto const jrr = env.rpc("server_info")[jss::result];
            BEAST_EXPECT(
                jrr[jss::info][jss::validator_list][jss::expiration] ==
                "never");
        }
        {
            auto const jrr = env.rpc("server_state")[jss::result];
            BEAST_EXPECT(
                jrr[jss::state][jss::validator_list_expires].asUInt() ==
                NetClock::time_point::max().time_since_epoch().count());
        }
//我们所有的钥匙都在里面
        {
            auto const jrr = env.rpc("validators")[jss::result];
            BEAST_EXPECT(jrr[jss::validator_list][jss::expiration] == "never");
            BEAST_EXPECT(jrr[jss::validation_quorum].asUInt() == keys.size());
            BEAST_EXPECT(jrr[jss::trusted_validator_keys].size() == keys.size());
            BEAST_EXPECT(jrr[jss::publisher_lists].size() == 0);
            BEAST_EXPECT(jrr[jss::local_static_keys].size() == keys.size());
            for (auto const& jKey : jrr[jss::local_static_keys])
            {
                BEAST_EXPECT(keys.count(jKey.asString())== 1);
            }
            BEAST_EXPECT(jrr[jss::signing_keys].size() == 0);
        }
//未配置验证程序站点
        {
            auto const jrr = env.rpc("validator_list_sites")[jss::result];
            BEAST_EXPECT(jrr[jss::validator_sites].size() == 0);
        }
    }

    void
    testDynamicUNL()
    {
        using namespace test::jtx;

        auto toStr = [](PublicKey const& publicKey) {
            return toBase58(TokenType::NodePublic, publicKey);
        };

//发布服务器清单/签名密钥
        auto const publisherSecret = randomSecretKey();
        auto const publisherPublic =
            derivePublicKey(KeyType::ed25519, publisherSecret);
        auto const publisherSigningKeys = randomKeyPair(KeyType::secp256k1);
        auto const manifest = makeManifestString(
            publisherPublic,
            publisherSecret,
            publisherSigningKeys.first,
            publisherSigningKeys.second,
            1);

//将在已发布列表中的验证程序密钥
        std::vector<Validator> validators = {randomValidator(), randomValidator()};
        std::set<std::string> expectedKeys;
        for (auto const& val : validators)
            expectedKeys.insert(toStr(val.masterPublic));


//——————————————————————————————————————————————————————————————
//发布者列表网站不可用
        {
//发布者网站信息
            using namespace std::string_literals;
            std::string siteURI =
"http://“s+getenvlocalhostaddr（）+”：1234/验证程序“；

            Env env{
                *this,
                envconfig([&](std::unique_ptr<Config> cfg) {
                    cfg->section(SECTION_VALIDATOR_LIST_SITES).append(siteURI);
                    cfg->section(SECTION_VALIDATOR_LIST_KEYS)
                        .append(strHex(publisherPublic));
                    return cfg;
                }),
            };

            env.app().validatorSites().start();
            env.app().validatorSites().join();

            {
                auto const jrr = env.rpc("server_info")[jss::result];
                BEAST_EXPECT(
                    jrr[jss::info][jss::validator_list][jss::expiration] ==
                    "unknown");
            }
            {
                auto const jrr = env.rpc("server_state")[jss::result];
                BEAST_EXPECT(
                    jrr[jss::state][jss::validator_list_expires].asInt() == 0);
            }
            {
                auto const jrr = env.rpc("validators")[jss::result];
                BEAST_EXPECT(jrr[jss::validation_quorum].asUInt() ==
                    std::numeric_limits<std::uint32_t>::max());
                BEAST_EXPECT(jrr[jss::local_static_keys].size() == 0);
                BEAST_EXPECT(jrr[jss::trusted_validator_keys].size() == 0);
                BEAST_EXPECT(
                    jrr[jss::validator_list][jss::expiration] == "unknown");

                if (BEAST_EXPECT(jrr[jss::publisher_lists].size() == 1))
                {
                    auto jp = jrr[jss::publisher_lists][0u];
                    BEAST_EXPECT(jp[jss::available] == false);
                    BEAST_EXPECT(jp[jss::list].size() == 0);
                    BEAST_EXPECT(!jp.isMember(jss::seq));
                    BEAST_EXPECT(!jp.isMember(jss::expiration));
                    BEAST_EXPECT(!jp.isMember(jss::version));
                    BEAST_EXPECT(
                        jp[jss::pubkey_publisher] == strHex(publisherPublic));
                }
                BEAST_EXPECT(jrr[jss::signing_keys].size() == 0);
            }
            {
                auto const jrr = env.rpc("validator_list_sites")[jss::result];
                if (BEAST_EXPECT(jrr[jss::validator_sites].size() == 1))
                {
                    auto js = jrr[jss::validator_sites][0u];
                    BEAST_EXPECT(js[jss::refresh_interval_min].asUInt() == 5);
                    BEAST_EXPECT(js[jss::uri] == siteURI);
                    BEAST_EXPECT(js.isMember(jss::last_refresh_time));
                    BEAST_EXPECT(js[jss::last_refresh_status] == "invalid");
                }
            }
        }
//——————————————————————————————————————————————————————————————
//发布者列表网站可用
        {
            using namespace std::chrono_literals;
            NetClock::time_point const expiration{3600s};

//管理服务器的单线程IO服务
            struct Worker : BasicApp
            {
                Worker() : BasicApp(1) {}
            };
            Worker w;

            TrustedPublisherServer server(
                w.get_io_service(),
                publisherSigningKeys,
                manifest,
                1,
                expiration,
                1,
                validators);

            std::stringstream uri;
uri << "http://“<<server.local_endpoint（）<<”/validators“；
            auto siteURI = uri.str();

            Env env{
                *this,
                envconfig([&](std::unique_ptr<Config> cfg) {
                    cfg->section(SECTION_VALIDATOR_LIST_SITES).append(siteURI);
                    cfg->section(SECTION_VALIDATOR_LIST_KEYS)
                        .append(strHex(publisherPublic));
                    return cfg;
                }),
            };

            env.app().validatorSites().start();
            env.app().validatorSites().join();
            hash_set<NodeID> startKeys;
            for (auto const& val : validators)
                startKeys.insert(calcNodeID(val.masterPublic));

            env.app().validators().updateTrusted(startKeys);

            {
                auto const jrr = env.rpc("server_info")[jss::result];
                BEAST_EXPECT(
                    jrr[jss::info][jss::validator_list][jss::expiration] ==
                    to_string(expiration));
            }
            {
                auto const jrr = env.rpc("server_state")[jss::result];
                BEAST_EXPECT(
                    jrr[jss::state][jss::validator_list_expires].asUInt() ==
                    expiration.time_since_epoch().count());
            }
            {
                auto const jrr = env.rpc("validators")[jss::result];
                BEAST_EXPECT(jrr[jss::validation_quorum].asUInt() == 2);
                BEAST_EXPECT(
                    jrr[jss::validator_list][jss::expiration] ==
                    to_string(expiration));
                BEAST_EXPECT(jrr[jss::local_static_keys].size() == 0);

                BEAST_EXPECT(jrr[jss::trusted_validator_keys].size() ==
                    expectedKeys.size());
                for (auto const& jKey : jrr[jss::trusted_validator_keys])
                {
                    BEAST_EXPECT(expectedKeys.count(jKey.asString()) == 1);
                }

                if (BEAST_EXPECT(jrr[jss::publisher_lists].size() == 1))
                {
                    auto jp = jrr[jss::publisher_lists][0u];
                    BEAST_EXPECT(jp[jss::available] == true);
                    if (BEAST_EXPECT(jp[jss::list].size() == 2))
                    {
//检查条目
                        std::set<std::string> foundKeys;
                        for (auto const& k : jp[jss::list])
                        {
                            foundKeys.insert(k.asString());
                        }
                        BEAST_EXPECT(foundKeys == expectedKeys);
                    }
                    BEAST_EXPECT(jp[jss::seq].asUInt() == 1);
                    BEAST_EXPECT(
                        jp[jss::pubkey_publisher] == strHex(publisherPublic));
                    BEAST_EXPECT(jp[jss::expiration] == to_string(expiration));
                    BEAST_EXPECT(jp[jss::version] == 1);
                }
                auto jsk = jrr[jss::signing_keys];
                BEAST_EXPECT(jsk.size() == 2);
                for (auto const& val : validators)
                {
                    BEAST_EXPECT(jsk.isMember(toStr(val.masterPublic)));
                    BEAST_EXPECT(
                        jsk[toStr(val.masterPublic)] ==
                        toStr(val.signingPublic));
                }
            }
            {
                auto const jrr = env.rpc("validator_list_sites")[jss::result];
                if (BEAST_EXPECT(jrr[jss::validator_sites].size() == 1))
                {
                    auto js = jrr[jss::validator_sites][0u];
                    BEAST_EXPECT(js[jss::refresh_interval_min].asUInt() == 5);
                    BEAST_EXPECT(js[jss::uri] == siteURI);
                    BEAST_EXPECT(js[jss::last_refresh_status] == "accepted");
//更新的实际时间因运行而异，因此
//只要核实时间就行了
                    BEAST_EXPECT(js.isMember(jss::last_refresh_time));
                }
            }
        }
    }

    void
    test_validation_create()
    {
        using namespace test::jtx;
        Env env{*this};
        auto result = env.rpc("validation_create");
        BEAST_EXPECT(result.isMember(jss::result) &&
                     result[jss::result][jss::status] == "success");
        result = env.rpc("validation_create",
                         "BAWL MAN JADE MOON DOVE GEM SON NOW HAD ADEN GLOW TIRE");
        BEAST_EXPECT(result.isMember(jss::result) &&
                     result[jss::result][jss::status] == "success");
    }

    void
    run() override
    {
        testPrivileges();
        testStaticUNL();
        testDynamicUNL();
        test_validation_create();
    }
};

BEAST_DEFINE_TESTSUITE(ValidatorRPC, app, ripple);

}  //命名空间测试
}  //命名空间波纹
