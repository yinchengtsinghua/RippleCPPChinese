
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
#include <test/jtx/Env.h>
#include <test/jtx/PathSet.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/core/LexicalCast.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/SField.h>

namespace ripple {

class Discrepancy_test : public beast::unit_test::suite
{
//这是从JS/Coffee移植的遗留测试。总账
//状态最初是通过保存的分类帐文件和相关的
//条目已转换为等效的jtx/env设置。
//使用path和sendmax付款并查询交易
//核实余额变动净额是否与收取的费用相符。
    void
    testXRPDiscrepancy (FeatureBitset features)
    {
        testcase ("Discrepancy test : XRP Discrepancy");
        using namespace test::jtx;
        Env env {*this, features};

        Account A1 {"A1"};
        Account A2 {"A2"};
        Account A3 {"A3"};
        Account A4 {"A4"};
        Account A5 {"A5"};
        Account A6 {"A6"};
        Account A7 {"A7"};

        env.fund(XRP(2000), A1);
        env.fund(XRP(1000), A2, A6, A7);
        env.fund(XRP(5000), A3);
        env.fund(XRP(1000000), A4);
        env.fund(XRP(600000), A5);
        env.close();

        env(trust(A1, A3["CNY"](200000)));
        env(pay(A3, A1, A3["CNY"](31)));
        env.close();

        env(trust(A1, A2["JPY"](1000000)));
        env(pay(A2, A1, A2["JPY"](729117)));
        env.close();

        env(trust(A4, A2["JPY"](10000000)));
        env(pay(A2, A4, A2["JPY"](470056)));
        env.close();

        env(trust(A5, A3["CNY"](50000)));
        env(pay(A3, A5, A3["CNY"](8683)));
        env.close();

        env(trust(A6, A3["CNY"](3000)));
        env(pay(A3, A6, A3["CNY"](293)));
        env.close();

        env(trust(A7, A6["CNY"](50000)));
        env(pay(A6, A7, A6["CNY"](261)));
        env.close();

        env(offer(A4, XRP(49147), A2["JPY"](34501)));
        env(offer(A5, A3["CNY"](3150), XRP(80086)));
        env(offer(A7, XRP(1233), A6["CNY"](25)));
        env.close();

        test::PathSet payPaths {
            test::Path {A2["JPY"], A2},
            test::Path {XRP, A2["JPY"], A2},
            test::Path {A6, XRP, A2["JPY"], A2} };

        env(pay(A1, A1, A2["JPY"](1000)),
            json(payPaths.json()),
            txflags(tfPartialPayment),
            sendmax(A3["CNY"](56)));
        env.close();

        Json::Value jrq2;
        jrq2[jss::binary] = false;
        jrq2[jss::transaction] = env.tx()->getJson(0)[jss::hash];
        jrq2[jss::id] = 3;
        auto jrr = env.rpc ("json", "tx", to_string(jrq2))[jss::result];
        uint64_t fee { jrr[jss::Fee].asUInt() };
        auto meta = jrr[jss::meta];
        uint64_t sumPrev {0};
        uint64_t sumFinal {0};
        BEAST_EXPECT(meta[sfAffectedNodes.fieldName].size() == 9);
        for(auto const& an : meta[sfAffectedNodes.fieldName])
        {
            Json::Value node;
            if(an.isMember(sfCreatedNode.fieldName))
                node = an[sfCreatedNode.fieldName];
            else if(an.isMember(sfModifiedNode.fieldName))
                node = an[sfModifiedNode.fieldName];
            else if(an.isMember(sfDeletedNode.fieldName))
                node = an[sfDeletedNode.fieldName];

            if(node && node[sfLedgerEntryType.fieldName] == "AccountRoot")
            {
                Json::Value prevFields =
                    node.isMember(sfPreviousFields.fieldName) ?
                    node[sfPreviousFields.fieldName] :
                    node[sfNewFields.fieldName];
                Json::Value finalFields =
                    node.isMember(sfFinalFields.fieldName) ?
                    node[sfFinalFields.fieldName] :
                    node[sfNewFields.fieldName];
                if(prevFields)
                    sumPrev += beast::lexicalCastThrow<std::uint64_t>(
                        prevFields[sfBalance.fieldName].asString());
                if(finalFields)
                    sumFinal += beast::lexicalCastThrow<std::uint64_t>(
                        finalFields[sfBalance.fieldName].asString());
            }
        }
//余额（最终和上一个）的差额应为
//收费
        BEAST_EXPECT(sumPrev-sumFinal == fee);
    }

public:
    void run () override
    {
        using namespace test::jtx;
        auto const sa = supported_amendments();
        testXRPDiscrepancy (sa - featureFlow - fix1373 - featureFlowCross);
        testXRPDiscrepancy (sa               - fix1373 - featureFlowCross);
        testXRPDiscrepancy (sa                         - featureFlowCross);
        testXRPDiscrepancy (sa);
    }
};

BEAST_DEFINE_TESTSUITE (Discrepancy, app, ripple);

} //涟漪
