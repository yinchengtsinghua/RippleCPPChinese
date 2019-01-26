
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

#include <ripple/beast/unit_test.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <test/jtx.h>

namespace ripple {

namespace test {

class AccountTX_test : public beast::unit_test::suite
{
    void
    testParameters()
    {
        using namespace test::jtx;

        Env env(*this);
        Account A1{"A1"};
        env.fund(XRP(10000), A1);
        env.close();

//分类帐3有两个与帐户融资相关联的TXS
//所有其他分类账都没有TXS

        auto hasTxs = [](Json::Value const& j) {
            return j.isMember(jss::result) &&
                (j[jss::result][jss::status] == "success") &&
                (j[jss::result][jss::transactions].size() == 2) &&
                (j[jss::result][jss::transactions][0u][jss::tx]
                  [jss::TransactionType] == "AccountSet") &&
                (j[jss::result][jss::transactions][1u][jss::tx]
                  [jss::TransactionType] == "Payment");
        };

        auto noTxs = [](Json::Value const& j) {
            return j.isMember(jss::result) &&
                (j[jss::result][jss::status] == "success") &&
                (j[jss::result][jss::transactions].size() == 0);
        };

        auto isErr = [](Json::Value const& j, error_code_i code) {
            return j.isMember(jss::result) &&
                j[jss::result].isMember(jss::error) &&
                j[jss::result][jss::error] == RPC::get_error_info(code).token;
        };

        Json::Value jParms;

        BEAST_EXPECT(isErr(
            env.rpc("json", "account_tx", to_string(jParms)),
            rpcINVALID_PARAMS));

        jParms[jss::account] = "0xDEADBEEF";

        BEAST_EXPECT(isErr(
            env.rpc("json", "account_tx", to_string(jParms)),
            rpcACT_MALFORMED));

        jParms[jss::account] = A1.human();
        BEAST_EXPECT(hasTxs(env.rpc("json", "account_tx", to_string(jParms))));

//分类帐最小/最大索引
        {
            Json::Value p{jParms};
            p[jss::ledger_index_min] = -1;
            p[jss::ledger_index_max] = -1;
            BEAST_EXPECT(hasTxs(env.rpc("json", "account_tx", to_string(p))));

            p[jss::ledger_index_min] = 0;
            p[jss::ledger_index_max] = 100;
            BEAST_EXPECT(hasTxs(env.rpc("json", "account_tx", to_string(p))));

            p[jss::ledger_index_min] = 1;
            p[jss::ledger_index_max] = 2;
            BEAST_EXPECT(noTxs(env.rpc("json", "account_tx", to_string(p))));

            p[jss::ledger_index_min] = 2;
            p[jss::ledger_index_max] = 1;
            BEAST_EXPECT(isErr(
                env.rpc("json", "account_tx", to_string(p)),
                rpcLGR_IDXS_INVALID));
        }

//仅分类帐索引最小值
        {
            Json::Value p{jParms};
            p[jss::ledger_index_min] = -1;
            BEAST_EXPECT(hasTxs(env.rpc("json", "account_tx", to_string(p))));

            p[jss::ledger_index_min] = 1;
            BEAST_EXPECT(hasTxs(env.rpc("json", "account_tx", to_string(p))));

            p[jss::ledger_index_min] = env.current()->info().seq;
            BEAST_EXPECT(isErr(
                env.rpc("json", "account_tx", to_string(p)),
                rpcLGR_IDXS_INVALID));
        }

//仅限于分类帐索引最大值
        {
            Json::Value p{jParms};
            p[jss::ledger_index_max] = -1;
            BEAST_EXPECT(hasTxs(env.rpc("json", "account_tx", to_string(p))));

            p[jss::ledger_index_max] = env.current()->info().seq;
            BEAST_EXPECT(hasTxs(env.rpc("json", "account_tx", to_string(p))));

            p[jss::ledger_index_max] = env.closed()->info().seq;
            BEAST_EXPECT(hasTxs(env.rpc("json", "account_tx", to_string(p))));

            p[jss::ledger_index_max] = env.closed()->info().seq - 1 ;
            BEAST_EXPECT(noTxs(env.rpc("json", "account_tx", to_string(p))));
        }

//Ledger序列
        {
            Json::Value p{jParms};

            p[jss::ledger_index] = env.closed()->info().seq;
            BEAST_EXPECT(hasTxs(env.rpc("json", "account_tx", to_string(p))));

            p[jss::ledger_index] = env.closed()->info().seq - 1;
            BEAST_EXPECT(noTxs(env.rpc("json", "account_tx", to_string(p))));

            p[jss::ledger_index] = env.current()->info().seq;
            BEAST_EXPECT(isErr(
                env.rpc("json", "account_tx", to_string(p)),
                rpcLGR_NOT_VALIDATED));

            p[jss::ledger_index] = env.current()->info().seq + 1;
            BEAST_EXPECT(isErr(
                env.rpc("json", "account_tx", to_string(p)),
                rpcLGR_NOT_FOUND));
        }

//分类哈希
        {
            Json::Value p{jParms};

            p[jss::ledger_hash] = to_string(env.closed()->info().hash);
            BEAST_EXPECT(hasTxs(env.rpc("json", "account_tx", to_string(p))));

            p[jss::ledger_hash] = to_string(env.closed()->info().parentHash);
            BEAST_EXPECT(noTxs(env.rpc("json", "account_tx", to_string(p))));
        }
    }

public:
    void
    run() override
    {
        testParameters();
    }
};
BEAST_DEFINE_TESTSUITE(AccountTX, app, ripple);

}  //命名空间测试

}  //命名空间波纹
