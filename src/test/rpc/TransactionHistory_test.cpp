
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
#include <test/jtx/Env.h>
#include <test/jtx/envconfig.h>
#include <ripple/protocol/JsonFields.h>
#include <boost/container/static_vector.hpp>
#include <algorithm>

namespace ripple {

class TransactionHistory_test : public beast::unit_test::suite
{
    void
    testBadInput()
    {
        testcase("Invalid request params");
        using namespace test::jtx;
        Env env {*this, envconfig(no_admin)};

        {
//无参数
            auto const result = env.client()
                .invoke("tx_history", {})[jss::result];
            BEAST_EXPECT(result[jss::error] == "invalidParams");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
//测试值为1，大于允许的非管理员限制
            Json::Value params {Json::objectValue};
params[jss::start] = 10001; //对于非管理员，限制为<=10000
            auto const result = env.client()
                .invoke("tx_history", params)[jss::result];
            BEAST_EXPECT(result[jss::error] == "noPermission");
            BEAST_EXPECT(result[jss::status] == "error");
        }
    }

    void testRequest()
    {
        testcase("Basic request");
        using namespace test::jtx;
        Env env {*this};

//创建足够的事务以提供
//历史…
        size_t const numAccounts = 20;
        boost::container::static_vector<Account, numAccounts> accounts;
        for(size_t i = 0; i<numAccounts; ++i)
        {
            accounts.emplace_back("A" + std::to_string(i));
            auto const& acct=accounts.back();
            env.fund(XRP(10000), acct);
            env.close();
            if(i > 0)
            {
                auto const& prev=accounts[i-1];
                env.trust(acct["USD"](1000), prev);
                env(pay(acct, prev, acct["USD"](5)));
            }
            env(offer(acct, XRP(100), acct["USD"](1)));
            env.close();

//验证env（报价）中的最新交易
//在德克萨斯州历史中可用。
            Json::Value params {Json::objectValue};
            params[jss::start] = 0;
            auto result =
                env.client().invoke("tx_history", params)[jss::result];
            if(! BEAST_EXPECT(result[jss::txs].isArray() &&
                    result[jss::txs].size() > 0))
                return;

//在历史记录中搜索与上次优惠匹配的Tx
            bool const txFound = [&] {
                auto const toFind = env.tx()->getJson(0);
                for (auto tx : result[jss::txs])
                {
                    tx.removeMember(jss::inLedger);
                    tx.removeMember(jss::ledger_index);
                    if (toFind == tx)
                        return true;
                }
                return false;
            }();
            BEAST_EXPECT(txFound);
        }

        unsigned int start = 0;
        unsigned int total = 0;
//同时汇总此映射中的事务类型
        std::unordered_map<std::string, unsigned> typeCounts;
        while(start < 120)
        {
            Json::Value params {Json::objectValue};
            params[jss::start] = start;
            auto result =
                env.client().invoke("tx_history", params)[jss::result];
            if(! BEAST_EXPECT(result[jss::txs].isArray() &&
                    result[jss::txs].size() > 0))
                break;
            total += result[jss::txs].size();
            start += 20;
            for (auto const& t : result[jss::txs])
            {
                typeCounts[t[sfTransactionType.fieldName].asString()]++;
            }
        }
        BEAST_EXPECT(total == 117);
        BEAST_EXPECT(typeCounts["AccountSet"] == 20);
        BEAST_EXPECT(typeCounts["TrustSet"] == 19);
        BEAST_EXPECT(typeCounts["Payment"] == 58);
        BEAST_EXPECT(typeCounts["OfferCreate"] == 20);

//另外，尝试使用max non-admin start值的请求
        {
            Json::Value params {Json::objectValue};
params[jss::start] = 10000; //对于非管理员，限制为<=10000
            auto const result = env.client()
                .invoke("tx_history", params)[jss::result];
            BEAST_EXPECT(result[jss::status] == "success");
            BEAST_EXPECT(result[jss::index] == 10000);
        }
    }

public:
    void run () override
    {
        testBadInput();
        testRequest();
    }
};

BEAST_DEFINE_TESTSUITE (TransactionHistory, rpc, ripple);

}  //涟漪
