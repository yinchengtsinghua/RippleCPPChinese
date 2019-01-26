
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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
#include <ripple/app/misc/AmendmentTable.h>

namespace ripple {

class Feature_test : public beast::unit_test::suite
{
    void
    testNoParams()
    {
        testcase ("No Params, None Enabled");

        using namespace test::jtx;
        Env env {*this};

        auto jrr = env.rpc("feature") [jss::result];
        if(! BEAST_EXPECT(jrr.isMember(jss::features)))
            return;
        for(auto const& feature : jrr[jss::features])
        {
            if(! BEAST_EXPECT(feature.isMember(jss::name)))
                return;
//默认配置-因此应禁用、不否决和支持所有配置
            BEAST_EXPECTS(! feature[jss::enabled].asBool(),
                feature[jss::name].asString() + " enabled");
            BEAST_EXPECTS(! feature[jss::vetoed].asBool(),
                feature[jss::name].asString() + " vetoed");
            BEAST_EXPECTS(feature[jss::supported].asBool(),
                feature[jss::name].asString() + " supported");
        }
    }

    void
    testSingleFeature()
    {
        testcase ("Feature Param");

        using namespace test::jtx;
        Env env {*this};

        auto jrr = env.rpc("feature", "CryptoConditions") [jss::result];
        BEAST_EXPECTS(jrr[jss::status] == jss::success, "status");
        jrr.removeMember(jss::status);
        BEAST_EXPECT(jrr.size() == 1);
        auto feature = *(jrr.begin());

        BEAST_EXPECTS(feature[jss::name] == "CryptoConditions", "name");
        BEAST_EXPECTS(! feature[jss::enabled].asBool(), "enabled");
        BEAST_EXPECTS(! feature[jss::vetoed].asBool(), "vetoed");
        BEAST_EXPECTS(feature[jss::supported].asBool(), "supported");

//功能名称区分大小写-此处应有错误
        jrr = env.rpc("feature", "cryptoconditions") [jss::result];
        BEAST_EXPECT(jrr[jss::error] == "badFeature");
        BEAST_EXPECT(jrr[jss::error_message] == "Feature unknown or invalid.");
    }

    void
    testInvalidFeature()
    {
        testcase ("Invalid Feature");

        using namespace test::jtx;
        Env env {*this};

        auto jrr = env.rpc("feature", "AllTheThings") [jss::result];
        BEAST_EXPECT(jrr[jss::error] == "badFeature");
        BEAST_EXPECT(jrr[jss::error_message] == "Feature unknown or invalid.");
    }

    void
    testNonAdmin()
    {
        testcase ("Feature Without Admin");

        using namespace test::jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg) {
            (*cfg)["port_rpc"].set("admin","");
            (*cfg)["port_ws"].set("admin","");
            return cfg;
        })};

        auto jrr = env.rpc("feature") [jss::result];
//当前HTTP/S服务器处理程序在此处返回HTTP 403错误代码
//而不是nopermission json错误。jsonrpcclient就是吃这个
//错误并返回空结果。
        BEAST_EXPECT(jrr.isNull());
    }

    void
    testSomeEnabled()
    {
        testcase ("No Params, Some Enabled");

        using namespace test::jtx;
        Env env {*this,
            FeatureBitset(featureEscrow, featureCryptoConditions)};

        auto jrr = env.rpc("feature") [jss::result];
        if(! BEAST_EXPECT(jrr.isMember(jss::features)))
            return;
        for(auto it = jrr[jss::features].begin();
            it != jrr[jss::features].end(); ++it)
        {
            uint256 id;
            id.SetHexExact(it.key().asString().c_str());
            if(! BEAST_EXPECT((*it).isMember(jss::name)))
                return;
            bool expectEnabled = env.app().getAmendmentTable().isEnabled(id);
            bool expectSupported = env.app().getAmendmentTable().isSupported(id);
            BEAST_EXPECTS((*it)[jss::enabled].asBool() == expectEnabled,
                (*it)[jss::name].asString() + " enabled");
            BEAST_EXPECTS(! (*it)[jss::vetoed].asBool(),
                (*it)[jss::name].asString() + " vetoed");
            BEAST_EXPECTS((*it)[jss::supported].asBool() == expectSupported,
                (*it)[jss::name].asString() + " supported");
        }
    }

    void
    testWithMajorities()
    {
        testcase ("With Majorities");

        using namespace test::jtx;
        Env env {*this, envconfig(validator, "")};

        auto jrr = env.rpc("feature") [jss::result];
        if(! BEAST_EXPECT(jrr.isMember(jss::features)))
            return;

//在这一点上，没有多数，因此没有与
//修正投票
        for(auto const& feature : jrr[jss::features])
        {
            if(! BEAST_EXPECT(feature.isMember(jss::name)))
                return;
            BEAST_EXPECTS(! feature.isMember(jss::majority),
                feature[jss::name].asString() + " majority");
            BEAST_EXPECTS(! feature.isMember(jss::count),
                feature[jss::name].asString() + " count");
            BEAST_EXPECTS(! feature.isMember(jss::threshold),
                feature[jss::name].asString() + " threshold");
            BEAST_EXPECTS(! feature.isMember(jss::validations),
                feature[jss::name].asString() + " validations");
            BEAST_EXPECTS(! feature.isMember(jss::vote),
                feature[jss::name].asString() + " vote");
        }

        auto majorities = getMajorityAmendments (*env.closed());
        if(! BEAST_EXPECT(majorities.empty()))
            return;

//关闭分类帐，直到出现修订。
        for (auto i = 0; i <= 256; ++i)
        {
            env.close();
            majorities = getMajorityAmendments (*env.closed());
            if (! majorities.empty())
                break;
        }

//至少有5个修正案。不要做精确的比较
//为了避免将来增加更多的修改而进行维护。
        BEAST_EXPECT(majorities.size() >= 5);

        jrr = env.rpc("feature") [jss::result];
        if(! BEAST_EXPECT(jrr.isMember(jss::features)))
            return;
        for(auto const& feature : jrr[jss::features])
        {
            if(! BEAST_EXPECT(feature.isMember(jss::name)))
                return;
            BEAST_EXPECTS(feature.isMember(jss::majority),
                feature[jss::name].asString() + " majority");
            BEAST_EXPECTS(feature.isMember(jss::count),
                feature[jss::name].asString() + " count");
            BEAST_EXPECTS(feature.isMember(jss::threshold),
                feature[jss::name].asString() + " threshold");
            BEAST_EXPECTS(feature.isMember(jss::validations),
                feature[jss::name].asString() + " validations");
            BEAST_EXPECTS(feature.isMember(jss::vote),
                feature[jss::name].asString() + " vote");
            BEAST_EXPECT(feature[jss::vote] == 256);
            BEAST_EXPECT(feature[jss::majority] == 2740);
        }

    }

    void
    testVeto()
    {
        testcase ("Veto");

        using namespace test::jtx;
        Env env {*this,
            FeatureBitset(featureCryptoConditions)};

        auto jrr = env.rpc("feature", "CryptoConditions") [jss::result];
        if(! BEAST_EXPECTS(jrr[jss::status] == jss::success, "status"))
            return;
        jrr.removeMember(jss::status);
        if(! BEAST_EXPECT(jrr.size() == 1))
            return;
        auto feature = *(jrr.begin());
        BEAST_EXPECTS(feature[jss::name] == "CryptoConditions", "name");
        BEAST_EXPECTS(! feature[jss::vetoed].asBool(), "vetoed");

        jrr = env.rpc("feature", "CryptoConditions", "reject") [jss::result];
        if(! BEAST_EXPECTS(jrr[jss::status] == jss::success, "status"))
            return;
        jrr.removeMember(jss::status);
        if(! BEAST_EXPECT(jrr.size() == 1))
            return;
        feature = *(jrr.begin());
        BEAST_EXPECTS(feature[jss::name] == "CryptoConditions", "name");
        BEAST_EXPECTS(feature[jss::vetoed].asBool(), "vetoed");

        jrr = env.rpc("feature", "CryptoConditions", "accept") [jss::result];
        if(! BEAST_EXPECTS(jrr[jss::status] == jss::success, "status"))
            return;
        jrr.removeMember(jss::status);
        if(! BEAST_EXPECT(jrr.size() == 1))
            return;
        feature = *(jrr.begin());
        BEAST_EXPECTS(feature[jss::name] == "CryptoConditions", "name");
        BEAST_EXPECTS(! feature[jss::vetoed].asBool(), "vetoed");

//接受或拒绝以外的任何内容都是错误的
        jrr = env.rpc("feature", "CryptoConditions", "maybe");
        BEAST_EXPECT(jrr[jss::error] == "invalidParams");
        BEAST_EXPECT(jrr[jss::error_message] == "Invalid parameters.");
    }

public:

    void run() override
    {
        testNoParams();
        testSingleFeature();
        testInvalidFeature();
        testNonAdmin();
        testSomeEnabled();
        testWithMajorities();
        testVeto();
    }
};

BEAST_DEFINE_TESTSUITE(Feature,rpc,ripple);

} //涟漪
