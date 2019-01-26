
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
#include <ripple/protocol/JsonFields.h>

namespace ripple {

class TransactionEntry_test : public beast::unit_test::suite
{
    void
    testBadInput()
    {
        testcase("Invalid request params");
        using namespace test::jtx;
        Env env {*this};

        {
//无参数
            auto const result = env.client()
                .invoke("transaction_entry", {})[jss::result];
            BEAST_EXPECT(result[jss::error] == "fieldNotFoundTransaction");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value params {Json::objectValue};
            params[jss::ledger] = 20;
            auto const result = env.client()
                .invoke("transaction_entry", params)[jss::result];
            BEAST_EXPECT(result[jss::error] == "lgrNotFound");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value params {Json::objectValue};
            params[jss::ledger] = "current";
            params[jss::tx_hash] = "DEADBEEF";
            auto const result = env.client()
                .invoke("transaction_entry", params)[jss::result];
            BEAST_EXPECT(result[jss::error] == "notYetImplemented");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value params {Json::objectValue};
            params[jss::ledger] = "closed";
            params[jss::tx_hash] = "DEADBEEF";
            auto const result = env.client()
                .invoke("transaction_entry", params)[jss::result];
            BEAST_EXPECT(! result[jss::ledger_hash].asString().empty());
            BEAST_EXPECT(result[jss::error] == "transactionNotFound");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        std::string const txHash {
            "E2FE8D4AF3FCC3944DDF6CD8CDDC5E3F0AD50863EF8919AFEF10CB6408CD4D05"};

//命令行格式
        {
//无参数
            Json::Value const result {env.rpc ("transaction_entry")};
            BEAST_EXPECT(result[jss::ledger_hash].asString().empty());
            BEAST_EXPECT(result[jss::error] == "badSyntax");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
//一个论点
            Json::Value const result {env.rpc ("transaction_entry", txHash)};
            BEAST_EXPECT(result[jss::error] == "badSyntax");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
//第一个参数字符太少
            Json::Value const result {env.rpc (
                "transaction_entry", txHash.substr (1), "closed")};
            BEAST_EXPECT(result[jss::error] == "invalidParams");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
//第一个参数的字符太多
            Json::Value const result {env.rpc (
                "transaction_entry", txHash + "A", "closed")};
            BEAST_EXPECT(result[jss::error] == "invalidParams");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
//第二个参数无效
            Json::Value const result {env.rpc (
                "transaction_entry", txHash, "closer")};
            BEAST_EXPECT(result[jss::error] == "invalidParams");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
//0的分类帐索引无效
            Json::Value const result {env.rpc (
                "transaction_entry", txHash, "0")};
            BEAST_EXPECT(result[jss::error] == "invalidParams");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
//三论证
            Json::Value const result {env.rpc (
                "transaction_entry", txHash, "closed", "extra")};
            BEAST_EXPECT(result[jss::error] == "badSyntax");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
//结构有效，但找不到事务。
            Json::Value const result {env.rpc (
                "transaction_entry", txHash, "closed")};
            BEAST_EXPECT(
                ! result[jss::result][jss::ledger_hash].asString().empty());
            BEAST_EXPECT(
                result[jss::result][jss::error] == "transactionNotFound");
            BEAST_EXPECT(result[jss::result][jss::status] == "error");
        }
    }

    void testRequest()
    {
        testcase("Basic request");
        using namespace test::jtx;
        Env env {*this};

        auto check_tx = [this, &env]
            (int index, std::string const txhash, std::string const type = "")
            {
//使用分类帐索引查找的第一个请求
                Json::Value const resIndex {[&env, index, &txhash] ()
                {
                    Json::Value params {Json::objectValue};
                    params[jss::ledger_index] = index;
                    params[jss::tx_hash] = txhash;
                    return env.client()
                        .invoke("transaction_entry", params)[jss::result];
                }()};

                if(! BEAST_EXPECTS(resIndex.isMember(jss::tx_json), txhash))
                    return;

                BEAST_EXPECT(resIndex[jss::tx_json][jss::hash] == txhash);
                if(! type.empty())
                {
                    BEAST_EXPECTS(
                        resIndex[jss::tx_json][jss::TransactionType] == type,
                        txhash + " is " +
                            resIndex[jss::tx_json][jss::TransactionType].asString());
                }

//第二个使用分类账哈希查找和验证的请求
//两个响应都匹配
                {
                    Json::Value params {Json::objectValue};
                    params[jss::ledger_hash] = resIndex[jss::ledger_hash];
                    params[jss::tx_hash] = txhash;
                    Json::Value const resHash = env.client()
                        .invoke("transaction_entry", params)[jss::result];
                    BEAST_EXPECT(resHash == resIndex);
                }

//将命令行窗体与索引一起使用。
                {
                    Json::Value const clIndex {env.rpc (
                        "transaction_entry", txhash, std::to_string (index))};
                    BEAST_EXPECT (clIndex["result"] == resIndex);
                }

//将命令行窗体与分类帐哈希一起使用。
                {
                    Json::Value const clHash {env.rpc (
                        "transaction_entry", txhash,
                        resIndex[jss::ledger_hash].asString())};
                    BEAST_EXPECT (clHash["result"] == resIndex);
                }
            };

        Account A1 {"A1"};
        Account A2 {"A2"};

        env.fund(XRP(10000), A1);
        auto fund_1_tx =
            boost::lexical_cast<std::string>(env.tx()->getTransactionID());

        env.fund(XRP(10000), A2);
        auto fund_2_tx =
            boost::lexical_cast<std::string>(env.tx()->getTransactionID());

        env.close();

//这些实际上是账户集txs，因为fund有两个txs和
//env.tx只报告最后一个
        check_tx(env.closed()->seq(), fund_1_tx);
        check_tx(env.closed()->seq(), fund_2_tx);

        env.trust(A2["USD"](1000), A1);
//信托Tx实际上是一种支付，因为信托方法
//在trustset之后用付款方式退款..所以忽略类型
//在下面的支票中
        auto trust_tx =
            boost::lexical_cast<std::string>(env.tx()->getTransactionID());

        env(pay(A2, A1, A2["USD"](5)));
        auto pay_tx =
            boost::lexical_cast<std::string>(env.tx()->getTransactionID());
        env.close();

        check_tx(env.closed()->seq(), trust_tx);
        check_tx(env.closed()->seq(), pay_tx, "Payment");

        env(offer(A2, XRP(100), A2["USD"](1)));
        auto offer_tx =
            boost::lexical_cast<std::string>(env.tx()->getTransactionID());

        env.close();

        check_tx(env.closed()->seq(), offer_tx, "OfferCreate");
    }

public:
    void run () override
    {
        testBadInput();
        testRequest();
    }
};

BEAST_DEFINE_TESTSUITE (TransactionEntry, rpc, ripple);

}  //涟漪
