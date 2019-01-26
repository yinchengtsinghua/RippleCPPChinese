
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
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/SField.h>
#include <ripple/basics/CountedObject.h>

namespace ripple {

class GetCounts_test : public beast::unit_test::suite
{
    void testGetCounts()
    {
        using namespace test::jtx;
        Env env(*this);

        Json::Value result;
        {
//检查未过账交易记录的计数
            result = env.rpc("get_counts")[jss::result];
            BEAST_EXPECT(result[jss::status] == "success");
            BEAST_EXPECT(! result.isMember("Transaction"));
            BEAST_EXPECT(! result.isMember("STObject"));
            BEAST_EXPECT(! result.isMember("HashRouterEntry"));
            BEAST_EXPECT(
                result.isMember(jss::uptime) &&
                ! result[jss::uptime].asString().empty());
            BEAST_EXPECT(
                result.isMember(jss::dbKBTotal) &&
                result[jss::dbKBTotal].asInt() > 0);
        }

//创建一些交易记录
        env.close();
        Account alice {"alice"};
        Account bob {"bob"};
        env.fund (XRP(10000), alice, bob);
        env.trust (alice["USD"](1000), bob);
        for(auto i=0; i<20; ++i)
        {
            env (pay (alice, bob, alice["USD"](5)));
            env.close();
        }

        {
//检查计数，默认参数
            result = env.rpc("get_counts")[jss::result];
            BEAST_EXPECT(result[jss::status] == "success");
//与CountedObjects报告的值进行比较
            auto const& objectCounts = CountedObjects::getInstance ().getCounts (10);
            for (auto const& it : objectCounts)
            {
                BEAST_EXPECTS(result.isMember(it.first), it.first);
                BEAST_EXPECTS(result[it.first].asInt() == it.second, it.first);
            }
            BEAST_EXPECT(! result.isMember(jss::local_txs));
        }

        {
//以最小阈值100发出请求并验证
//只报告Stobject和NodeObject
            result = env.rpc("get_counts", "100")[jss::result];
            BEAST_EXPECT(result[jss::status] == "success");

//与CountedObjects报告的值进行比较
            auto const& objectCounts = CountedObjects::getInstance ().getCounts (100);
            for (auto const& it : objectCounts)
            {
                BEAST_EXPECTS(result.isMember(it.first), it.first);
                BEAST_EXPECTS(result[it.first].asInt() == it.second, it.first);
            }
            BEAST_EXPECT(! result.isMember("Transaction"));
            BEAST_EXPECT(! result.isMember("STTx"));
            BEAST_EXPECT(! result.isMember("STArray"));
            BEAST_EXPECT(! result.isMember("HashRouterEntry"));
            BEAST_EXPECT(! result.isMember("STLedgerEntry"));
        }

        {
//当有打开的txs时，将存在本地txs字段
            env (pay (alice, bob, alice["USD"](5)));
            result = env.rpc("get_counts")[jss::result];
//故意不叫关门，这样我们就可以开德克萨斯州了
            BEAST_EXPECT(
                result.isMember(jss::local_txs) &&
                result[jss::local_txs].asInt() > 0);
        }
    }

public:
    void run () override
    {
        testGetCounts();
    }
};

BEAST_DEFINE_TESTSUITE(GetCounts,rpc,ripple);

} //涟漪

