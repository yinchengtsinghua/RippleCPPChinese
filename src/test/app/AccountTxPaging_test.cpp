
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
#include <test/jtx.h>
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/JsonFields.h>
#include <cstdlib>

namespace ripple {

class AccountTxPaging_test : public beast::unit_test::suite
{
    bool
    checkTransaction (Json::Value const& tx, int sequence, int ledger)
    {
        return (
            tx[jss::tx][jss::Sequence].asInt() == sequence &&
            tx[jss::tx][jss::ledger_index].asInt() == ledger);
    }

    auto next(
        test::jtx::Env & env,
        test::jtx::Account const& account,
        int ledger_min,
        int ledger_max,
        int limit,
        bool forward,
        Json::Value const& marker = Json::nullValue)
    {
        Json::Value jvc;
        jvc[jss::account] = account.human();
        jvc[jss::ledger_index_min] = ledger_min;
        jvc[jss::ledger_index_max] = ledger_max;
        jvc[jss::forward] = forward;
        jvc[jss::limit] = limit;
        if (marker)
            jvc[jss::marker] = marker;

        return env.rpc("json", "account_tx", to_string(jvc))[jss::result];
    }

    void
    testAccountTxPaging ()
    {
        testcase("Paging for Single Account");
        using namespace test::jtx;

        Env env(*this);
        Account A1 {"A1"};
        Account A2 {"A2"};
        Account A3 {"A3"};

        env.fund(XRP(10000), A1, A2, A3);
        env.close();

        env.trust(A3["USD"](1000), A1);
        env.trust(A2["USD"](1000), A1);
        env.trust(A3["USD"](1000), A2);
        env.close();

        for (auto i = 0; i < 5; ++i)
        {
            env(pay(A2, A1, A2["USD"](2)));
            env(pay(A3, A1, A3["USD"](2)));
            env(offer(A1, XRP(11), A1["USD"](1)));
            env(offer(A2, XRP(10), A2["USD"](1)));
            env(offer(A3, XRP(9),  A3["USD"](1)));
            env.close();
        }

        /*A3的顺序/分类账如下：
         *序列分类帐索引
         ＊3＞3
         ＊1＞3
         ＊2＞4
         ＊2＞4
         ＊2＞5
         ＊3＞5
         ＊4＞6
         ＊5＞6
         ＊6＞7
         ＊7＞7
         ＊8＞8
         ＊9＞8
         ＊10＞9
         ＊11＞9
        **/


//以多种方式浏览结果。
        {
//限值=2，3批，给出前6个TXS
            auto jrr = next(env, A3, 2, 5, 2, true);
            auto txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 2))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 3, 3));
            BEAST_EXPECT(checkTransaction (txs[1u], 1, 3));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

            jrr = next(env, A3, 2, 5, 2, true, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 2))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 2, 4));
            BEAST_EXPECT(checkTransaction (txs[1u], 2, 4));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

            jrr = next(env, A3, 2, 5, 2, true, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 2))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 2, 5));
            BEAST_EXPECT(checkTransaction (txs[1u], 3, 5));
            BEAST_EXPECT(! jrr[jss::marker]);
        }

        {
//限制1，3个请求给出前3个TXS
            auto jrr = next(env, A3, 3, 9, 1, true);
            auto txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 1))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 3, 3));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

            jrr = next(env, A3, 3, 9, 1, true, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 1))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 1, 3));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

            jrr = next(env, A3, 3, 9, 1, true, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 1))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 2, 4));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

//继续限制3，直到所有TXS结束
            jrr = next(env, A3, 3, 9, 3, true, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 3))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 2, 4));
            BEAST_EXPECT(checkTransaction (txs[1u], 2, 5));
            BEAST_EXPECT(checkTransaction (txs[2u], 3, 5));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

            jrr = next(env, A3, 3, 9, 3, true, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 3))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 4, 6));
            BEAST_EXPECT(checkTransaction (txs[1u], 5, 6));
            BEAST_EXPECT(checkTransaction (txs[2u], 6, 7));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

            jrr = next(env, A3, 3, 9, 3, true, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 3))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 7, 7));
            BEAST_EXPECT(checkTransaction (txs[1u], 8, 8));
            BEAST_EXPECT(checkTransaction (txs[2u], 9, 8));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

            jrr = next(env, A3, 3, 9, 3, true, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 2))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 10, 9));
            BEAST_EXPECT(checkTransaction (txs[1u], 11, 9));
            BEAST_EXPECT(! jrr[jss::marker]);
        }

        {
//限制2个，下降，2个批次，最后4个TXS
            auto jrr = next(env, A3, 3, 9, 2, false);
            auto txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 2))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 11, 9));
            BEAST_EXPECT(checkTransaction (txs[1u], 10, 9));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

            jrr = next(env, A3, 3, 9, 2, false, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 2))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 9, 8));
            BEAST_EXPECT(checkTransaction (txs[1u], 8, 8));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

//继续执行限值3，直到看到所有热膨胀系数。
            jrr = next(env, A3, 3, 9, 3, false, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 3))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 7, 7));
            BEAST_EXPECT(checkTransaction (txs[1u], 6, 7));
            BEAST_EXPECT(checkTransaction (txs[2u], 5, 6));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

            jrr = next(env, A3, 3, 9, 3, false, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 3))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 4, 6));
            BEAST_EXPECT(checkTransaction (txs[1u], 3, 5));
            BEAST_EXPECT(checkTransaction (txs[2u], 2, 5));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

            jrr = next(env, A3, 3, 9, 3, false, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 3))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 2, 4));
            BEAST_EXPECT(checkTransaction (txs[1u], 2, 4));
            BEAST_EXPECT(checkTransaction (txs[2u], 1, 3));
            if(! BEAST_EXPECT(jrr[jss::marker]))
                return;

            jrr = next(env, A3, 3, 9, 3, false, jrr[jss::marker]);
            txs = jrr[jss::transactions];
            if (! BEAST_EXPECT(txs.isArray() && txs.size() == 1))
                return;
            BEAST_EXPECT(checkTransaction (txs[0u], 3, 3));
            BEAST_EXPECT(! jrr[jss::marker]);
        }
    }

public:
    void
    run() override
    {
        testAccountTxPaging();
    }
};

BEAST_DEFINE_TESTSUITE(AccountTxPaging,app,ripple);

}

