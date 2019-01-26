
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
#include <test/jtx.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/JsonFields.h>

namespace ripple {

namespace test {

class SetTrust_test : public beast::unit_test::suite
{
public:


    void testFreeTrustlines(bool thirdLineCreatesLE, bool createOnHighAcct)
    {
        if (thirdLineCreatesLE)
            testcase("Allow two free trustlines");
        else
            testcase("Dynamic reserve for trustline");

        using namespace jtx;
        Env env(*this);

        auto const gwA = Account{ "gwA" };
        auto const gwB = Account{ "gwB" };
        auto const acctC = Account{ "acctC" };
        auto const acctD = Account{ "acctD" };

        auto const & creator  = createOnHighAcct ? acctD : acctC;
        auto const & assistor = createOnHighAcct ? acctC : acctD;

        auto const txFee = env.current()->fees().base;
        auto const baseReserve = env.current()->fees().accountReserve(0);
        auto const threelineReserve = env.current()->fees().accountReserve(3);

        env.fund(XRP(10000), gwA, gwB, assistor);

//基金创建者…
        /*.基金（基本准备金/*足以持有账户*/
            +折扣（3*txfee）/*并支付3笔交易*/, creator);


        env(trust(creator, gwA["USD"](100)), require(lines(creator,1)));
        env(trust(creator, gwB["USD"](100)), require(lines(creator,2)));

        if (thirdLineCreatesLE)
        {
//创建者没有足够的信任线
            env(trust(creator, assistor["USD"](100)),
                ter(tecNO_LINE_INSUF_RESERVE), require(lines(creator, 2)));
        }
        else
        {
//首先建立与助理相反的信任方向
            env(trust(assistor,creator["USD"](100)), require(lines(creator,3)));

//创建者没有足够的空间在上创建其他方向
//现有信任行分类帐条目
            env(trust(creator,assistor["USD"](100)),ter(tecINSUF_RESERVE_LINE));
        }

//基金创建者要支付的额外金额
        env(pay(env.master,creator,STAmount{ threelineReserve - baseReserve }));

        if (thirdLineCreatesLE)
        {
            env(trust(creator,assistor["USD"](100)),require(lines(creator, 3)));
        }
        else
        {
            env(trust(creator, assistor["USD"](100)),require(lines(creator,3)));

            Json::Value jv;
            jv["account"] = creator.human();
            auto const lines = env.rpc("json","account_lines", to_string(jv));
//验证所有行的创建者限制为100
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 3);
            for (auto const & line : lines[jss::result][jss::lines])
            {
                BEAST_EXPECT(line[jss::limit] == "100");
            }
        }
    }

    Json::Value trust_explicit_amt(jtx::Account const & a, STAmount const & amt)
    {
        Json::Value jv;
        jv[jss::Account] = a.human();
        jv[jss::LimitAmount] = amt.getJson(0);
        jv[jss::TransactionType] = "TrustSet";
        jv[jss::Flags] = 0;
        return jv;
    }

    void testMalformedTransaction()
    {
        testcase("SetTrust checks for malformed transactions");

        using namespace jtx;
        Env env{ *this };

        auto const gw = Account{ "gateway" };
        auto const alice = Account{ "alice" };
        env.fund(XRP(10000), gw, alice);

//需要有效的tf标志
        for (std::uint64_t badFlag = 1u ;
            badFlag <= std::numeric_limits<std::uint32_t>::max(); badFlag *= 2)
        {
            if( badFlag & tfTrustSetMask)
                env(trust(alice, gw["USD"](100),
                    static_cast<std::uint32_t>(badFlag)), ter(temINVALID_FLAG));
        }

//信托金额不能是xrp
        env(trust_explicit_amt(alice, drops(10000)), ter(temBAD_LIMIT));

//信托金额不能为Badconcurrency IOU
        env(trust_explicit_amt(alice, gw[ to_string(badCurrency())](100)),
            ter(temBAD_CURRENCY));

//信任金额不能为负数
        env(trust(alice, gw["USD"](-1000)), ter(temBAD_LIMIT));

//信任金额不能来自无效的颁发者
        env(trust_explicit_amt(alice, STAmount{Issue{to_currency("USD"),
            noAccount()}, 100 }), ter(temDST_NEEDED));

//信任不能是对自己
        env(trust(alice, alice["USD"](100)), ter(temDST_IS_SRC));

//如果lsfrequeuireauth不需要，则不应设置tfsetauth标志
        env(trust(alice, gw["USD"](100), tfSetfAuth), ter(tefNO_AUTH_REQUIRED));
    }

    void testModifyQualityOfTrustline(bool createQuality, bool createOnHighAcct)
    {
        testcase << "SetTrust " << (createQuality ? "creates" : "removes")
            << " quality of trustline for "
            << (createOnHighAcct ? "high" : "low" )
            << " account" ;

        using namespace jtx;
        Env env{ *this };

        auto const alice = Account{ "alice" };
        auto const bob = Account{ "bob" };

        auto const & fromAcct = createOnHighAcct ? alice : bob;
        auto const & toAcct = createOnHighAcct ? bob : alice;

        env.fund(XRP(10000), fromAcct, toAcct);


        auto txWithoutQuality = trust(toAcct, fromAcct["USD"](100));
        txWithoutQuality["QualityIn"] = "0";
        txWithoutQuality["QualityOut"] = "0";

        auto txWithQuality = txWithoutQuality;
        txWithQuality["QualityIn"] = "1000";
        txWithQuality["QualityOut"] = "1000";

        auto & tx1 = createQuality ? txWithQuality : txWithoutQuality;
        auto & tx2 = createQuality ? txWithoutQuality : txWithQuality;

        auto check_quality = [&](const bool exists)
        {
            Json::Value jv;
            jv["account"] = toAcct.human();
            auto const lines = env.rpc("json","account_lines", to_string(jv));
            auto quality = exists ? 1000 : 0;
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 1);
            BEAST_EXPECT(lines[jss::result][jss::lines][0u][jss::quality_in]
                == quality);
            BEAST_EXPECT(lines[jss::result][jss::lines][0u][jss::quality_out]
                == quality);
        };


        env(tx1, require(lines(toAcct, 1)), require(lines(fromAcct, 1)));
        check_quality(createQuality);

        env(tx2, require(lines(toAcct, 1)), require(lines(fromAcct, 1)));
        check_quality(!createQuality);

    }

    void run() override
    {
        testFreeTrustlines(true, false);
        testFreeTrustlines(false, true);
        testFreeTrustlines(false, true);
//真的，真的情况并不重要，因为创建了一个托管行分类账
//输入要求创建者保留
//独立于终结点的高/低帐户ID
        testMalformedTransaction();
        testModifyQualityOfTrustline(false, false);
        testModifyQualityOfTrustline(false, true);
        testModifyQualityOfTrustline(true, false);
        testModifyQualityOfTrustline(true, true);
    }
};
BEAST_DEFINE_TESTSUITE(SetTrust, app, ripple);
} //测试
} //涟漪
