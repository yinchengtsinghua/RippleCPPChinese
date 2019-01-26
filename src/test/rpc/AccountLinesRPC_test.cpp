
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

#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/TxFlags.h>
#include <test/jtx.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

namespace RPC {

class AccountLinesRPC_test : public beast::unit_test::suite
{
public:
    void testAccountLines()
    {
        testcase ("acccount_lines");

        using namespace test::jtx;
        Env env(*this);
        {
//没有账户的账户行。
            auto const lines = env.rpc ("json", "account_lines", "{ }");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::missing_field_error(jss::account)[jss::error_message]);
        }
        {
//帐户行的帐户格式不正确。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": )"
                R"("n9MJkEKHDhy5eTLuHUQeAAjo382frHNbFK4C8hcwN4nwM2SrLdBj"})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::make_error(rpcBAD_SEED)[jss::error_message]);
        }
        Account const alice {"alice"};
        {
//无资金账户上的账户行。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"("})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::make_error(rpcACT_NOT_FOUND)[jss::error_message]);
        }
        env.fund(XRP(10000), alice);
        env.close();
        LedgerInfo const ledger3Info = env.closed()->info();
        BEAST_EXPECT(ledger3Info.seq == 3);

        {
//爱丽丝有资金，但没有台词。返回空数组。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"("})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 0);
        }
        {
//指定不存在的分类帐。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_index": "nonsense"})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                "ledgerIndexMalformed");
        }
        {
//指定不存在的其他分类帐。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_index": 50000})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                "ledgerNotFound");
        }
//创建信任行与Alice共享。
        Account const gw1 {"gw1"};
        env.fund(XRP(10000), gw1);
        std::vector<IOU> gw1Currencies;

        for (char c = 0; c <= ('Z' - 'A'); ++c)
        {
//GW1货币的名称为“yaa”->“yaz”。
            gw1Currencies.push_back(
                gw1[std::string("YA") + static_cast<char>('A' + c)]);
            IOU const& gw1Currency = gw1Currencies.back();

//建立信任关系。
            env(trust(alice, gw1Currency(100 + c)));
            env(pay(gw1, alice, gw1Currency(50 + c)));
        }
        env.close();
        LedgerInfo const ledger4Info = env.closed()->info();
        BEAST_EXPECT(ledger4Info.seq == 4);

//在另一个分类帐中添加另一组信任行，以便我们可以看到
//历史分类账的差异。
        Account const gw2 {"gw2"};
        env.fund(XRP(10000), gw2);

//GW2需要授权。
        env(fset(gw2, asfRequireAuth));
        env.close();
        std::vector<IOU> gw2Currencies;

        for (char c = 0; c <= ('Z' - 'A'); ++c)
        {
//GW2货币的名称为“Zaa”->“Zaz”。
            gw2Currencies.push_back(
                gw2[std::string("ZA") + static_cast<char>('A' + c)]);
            IOU const& gw2Currency = gw2Currencies.back();

//建立信任关系。
            env(trust(alice, gw2Currency(200 + c)));
            env(trust(gw2, gw2Currency(0), alice, tfSetfAuth));
            env.close();
            env(pay(gw2, alice, gw2Currency(100 + c)));
            env.close();

//在GW2信任线上设置标志，以便我们可以查找它们。
            env(trust(alice, gw2Currency(0), gw2, tfSetNoRipple | tfSetFreeze));
        }
        env.close();
        LedgerInfo const ledger58Info = env.closed()->info();
        BEAST_EXPECT(ledger58Info.seq == 58);

//历史分类账的可重复使用测试。
        auto testAccountLinesHistory =
        [this, &env](Account const& account, LedgerInfo const& info, int count)
        {
//按分类帐索引获取帐户行。
            auto const linesSeq = env.rpc ("json", "account_lines",
                R"({"account": ")" + account.human() + R"(", )"
                R"("ledger_index": )" + std::to_string(info.seq) + "}");
            BEAST_EXPECT(linesSeq[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesSeq[jss::result][jss::lines].size() == count);

//按分类帐哈希获取帐户行。
            auto const linesHash = env.rpc ("json", "account_lines",
                R"({"account": ")" + account.human() + R"(", )"
                R"("ledger_hash": ")" + to_string(info.hash) + R"("})");
            BEAST_EXPECT(linesHash[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesHash[jss::result][jss::lines].size() == count);
        };

//Alice在分类帐3中不应该有信任行。
        testAccountLinesHistory (alice, ledger3Info, 0);

//Alice在分类帐4中应该有26个信任行。
        testAccountLinesHistory (alice, ledger4Info, 26);

//Alice在分类帐58中应该有52个信托行。
        testAccountLinesHistory (alice, ledger58Info, 52);

        {
//令人惊讶的是，在
//哪种情况下散列值会赢。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_hash": ")" + to_string(ledger4Info.hash) + R"(", )"
                R"("ledger_index": )" + std::to_string(ledger58Info.seq) + "}");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 26);
        }
        {
//Alice在当前分类帐中应该有52个信托行。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"("})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 52);
        }
        {
//Alice应该和GW1有26条信任线。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("peer": ")" + gw1.human() + R"("})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 26);
        }
        {
//使用格式错误的对等机。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("peer": )"
                R"("n9MJkEKHDhy5eTLuHUQeAAjo382frHNbFK4C8hcwN4nwM2SrLdBj"})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::make_error(rpcBAD_SEED)[jss::error_message]);
        }
        {
//负极限应该失败。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": -1})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::expected_field_message(jss::limit, "unsigned integer"));
        }
        {
//将响应限制为1个信任行。
            auto const linesA = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 1})");
            BEAST_EXPECT(linesA[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesA[jss::result][jss::lines].size() == 1);

//从记号笔停止的地方捡起来。我们应该得到51。
            auto marker = linesA[jss::result][jss::marker].asString();
            auto const linesB = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": ")" + marker + R"("})");
            BEAST_EXPECT(linesB[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesB[jss::result][jss::lines].size() == 51);

//再次从标记停止的位置开始，但设置限制为3。
            auto const linesC = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 3, )"
                R"("marker": ")" + marker + R"("})");
            BEAST_EXPECT(linesC[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesC[jss::result][jss::lines].size() == 3);

//弄乱标记，使其变差并检查错误。
            marker[5] = marker[5] == '7' ? '8' : '7';
            auto const linesD = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": ")" + marker + R"("})");
            BEAST_EXPECT(linesD[jss::result][jss::error_message] ==
                RPC::make_error(rpcINVALID_PARAMS)[jss::error_message]);
        }
        {
//非字符串标记也应该失败。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": true})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::expected_field_message(jss::marker, "string"));
        }
        {
//检查我们期望从Alice到GW2的标志是否存在。
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 1, )"
                R"("peer": ")" + gw2.human() + R"("})");
            auto const& line = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line[jss::freeze].asBool() == true);
            BEAST_EXPECT(line[jss::no_ripple].asBool() == true);
            BEAST_EXPECT(line[jss::peer_authorized].asBool() == true);
        }
        {
//检查GW2到Alice的期望标志是否存在。
            auto const linesA = env.rpc ("json", "account_lines",
                R"({"account": ")" + gw2.human() + R"(", )"
                R"("limit": 1, )"
                R"("peer": ")" + alice.human() + R"("})");
            auto const& lineA = linesA[jss::result][jss::lines][0u];
            BEAST_EXPECT(lineA[jss::freeze_peer].asBool() == true);
            BEAST_EXPECT(lineA[jss::no_ripple_peer].asBool() == true);
            BEAST_EXPECT(lineA[jss::authorized].asBool() == true);

//从返回的标记继续，以确保工作正常。
            BEAST_EXPECT(linesA[jss::result].isMember(jss::marker));
            auto const marker = linesA[jss::result][jss::marker].asString();
            auto const linesB = env.rpc ("json", "account_lines",
                R"({"account": ")" + gw2.human() + R"(", )"
                R"("limit": 25, )"
                R"("marker": ")" + marker + R"(", )"
                R"("peer": ")" + alice.human() + R"("})");
            BEAST_EXPECT(linesB[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesB[jss::result][jss::lines].size() == 25);
            BEAST_EXPECT(! linesB[jss::result].isMember(jss::marker));
        }
    }

    void testAccountLineDelete()
    {
        testcase ("Entry pointed to by marker is removed");
        using namespace test::jtx;
        Env env(*this);

//这里的目标是观察账户行标记行为，如果
//返回标记指向的条目将从分类帐中删除。
//
//显式删除信任行并不容易，因此我们在
//流行时尚。它需要4个演员：
//o网关GW1发行美元
//o Alice出价100美元购买100 XRP。
//o贝基提出以100瑞波币卖出100美元。
//Alice和GW1之间现在将有一个推断的信任线。
//o爱丽丝付给切丽100美元。
//Alice现在应该没有美元，也没有对GW1的信任。
        Account const alice {"alice"};
        Account const becky {"becky"};
        Account const cheri {"cheri"};
        Account const gw1 {"gw1"};
        Account const gw2 {"gw2"};
        env.fund(XRP(10000), alice, becky, cheri, gw1, gw2);
        env.close();

        auto const USD = gw1["USD"];
        auto const EUR = gw2["EUR"];
        env(trust(alice, USD(200)));
        env(trust(becky, EUR(200)));
        env(trust(cheri, EUR(200)));
        env.close();

//贝基从GW1获得100美元。
        env(pay(gw2, becky, EUR(100)));
        env.close();

//艾丽斯提出以100瑞波币购买100欧元。
        env(offer(alice, EUR(100), XRP(100)));
        env.close();

//贝基提议以100欧元的价格购买100瑞波币。
        env(offer(becky, XRP(100), EUR(100)));
        env.close();

//获取Alice的帐户行。限制在1，所以我们得到一个标记。
        auto const linesBeg = env.rpc ("json", "account_lines",
            R"({"account": ")" + alice.human() + R"(", )"
            R"("limit": 1})");
        BEAST_EXPECT(linesBeg[jss::result][jss::lines][0u][jss::currency] == "USD");
        BEAST_EXPECT(linesBeg[jss::result].isMember(jss::marker));

//爱丽丝付给切丽100欧元。
        env(pay(alice, cheri, EUR(100)));
        env.close();

//既然爱丽丝把所有的欧元都付给了切丽，爱丽丝就不应该再这样了。
//与GW1建立信任关系。所以旧标记现在应该是无效的。
        auto const linesEnd = env.rpc ("json", "account_lines",
            R"({"account": ")" + alice.human() + R"(", )"
            R"("marker": ")" +
            linesBeg[jss::result][jss::marker].asString() + R"("})");
        BEAST_EXPECT(linesEnd[jss::result][jss::error_message] ==
                RPC::make_error(rpcINVALID_PARAMS)[jss::error_message]);
    }

//测试API V2
    void testAccountLines2()
    {
        testcase ("V2: acccount_lines");

        using namespace test::jtx;
        Env env(*this);
        {
//帐户行的JSON2格式不正确（缺少ID字段）。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0")"
                " }");
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
        }
        {
//没有账户的账户行。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5)"
                " }");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::missing_field_error(jss::account)[jss::error_message]);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
//帐户行的帐户格式不正确。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": )"
                R"("n9MJkEKHDhy5eTLuHUQeAAjo382frHNbFK4C8hcwN4nwM2SrLdBj"}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::make_error(rpcBAD_SEED)[jss::error_message]);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        Account const alice {"alice"};
        {
//无资金账户上的账户行。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"("}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::make_error(rpcACT_NOT_FOUND)[jss::error_message]);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        env.fund(XRP(10000), alice);
        env.close();
        LedgerInfo const ledger3Info = env.closed()->info();
        BEAST_EXPECT(ledger3Info.seq == 3);

        {
//爱丽丝有资金，但没有台词。返回空数组。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"("}})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 0);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
//指定不存在的分类帐。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_index": "nonsense"}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                "ledgerIndexMalformed");
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
//指定不存在的其他分类帐。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_index": 50000}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                "ledgerNotFound");
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
//创建信任行与Alice共享。
        Account const gw1 {"gw1"};
        env.fund(XRP(10000), gw1);
        std::vector<IOU> gw1Currencies;

        for (char c = 0; c <= ('Z' - 'A'); ++c)
        {
//GW1货币的名称为“yaa”->“yaz”。
            gw1Currencies.push_back(
                gw1[std::string("YA") + static_cast<char>('A' + c)]);
            IOU const& gw1Currency = gw1Currencies.back();

//建立信任关系。
            env(trust(alice, gw1Currency(100 + c)));
            env(pay(gw1, alice, gw1Currency(50 + c)));
        }
        env.close();
        LedgerInfo const ledger4Info = env.closed()->info();
        BEAST_EXPECT(ledger4Info.seq == 4);

//在另一个分类帐中添加另一组信任行，以便我们可以看到
//历史分类账的差异。
        Account const gw2 {"gw2"};
        env.fund(XRP(10000), gw2);

//GW2需要授权。
        env(fset(gw2, asfRequireAuth));
        env.close();
        std::vector<IOU> gw2Currencies;

        for (char c = 0; c <= ('Z' - 'A'); ++c)
        {
//GW2货币的名称为“Zaa”->“Zaz”。
            gw2Currencies.push_back(
                gw2[std::string("ZA") + static_cast<char>('A' + c)]);
            IOU const& gw2Currency = gw2Currencies.back();

//建立信任关系。
            env(trust(alice, gw2Currency(200 + c)));
            env(trust(gw2, gw2Currency(0), alice, tfSetfAuth));
            env.close();
            env(pay(gw2, alice, gw2Currency(100 + c)));
            env.close();

//在GW2信任线上设置标志，以便我们可以查找它们。
            env(trust(alice, gw2Currency(0), gw2, tfSetNoRipple | tfSetFreeze));
        }
        env.close();
        LedgerInfo const ledger58Info = env.closed()->info();
        BEAST_EXPECT(ledger58Info.seq == 58);

//历史分类账的可重复使用测试。
        auto testAccountLinesHistory =
        [this, &env](Account const& account, LedgerInfo const& info, int count)
        {
//按分类帐索引获取帐户行。
            auto const linesSeq = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + account.human() + R"(", )"
                R"("ledger_index": )" + std::to_string(info.seq) + "}}");
            BEAST_EXPECT(linesSeq[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesSeq[jss::result][jss::lines].size() == count);
            BEAST_EXPECT(linesSeq.isMember(jss::jsonrpc) && linesSeq[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesSeq.isMember(jss::ripplerpc) && linesSeq[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesSeq.isMember(jss::id) && linesSeq[jss::id] == 5);

//按分类帐哈希获取帐户行。
            auto const linesHash = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + account.human() + R"(", )"
                R"("ledger_hash": ")" + to_string(info.hash) + R"("}})");
            BEAST_EXPECT(linesHash[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesHash[jss::result][jss::lines].size() == count);
            BEAST_EXPECT(linesHash.isMember(jss::jsonrpc) && linesHash[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesHash.isMember(jss::ripplerpc) && linesHash[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesHash.isMember(jss::id) && linesHash[jss::id] == 5);
        };

//Alice在分类帐3中不应该有信任行。
        testAccountLinesHistory (alice, ledger3Info, 0);

//Alice在分类帐4中应该有26个信任行。
        testAccountLinesHistory (alice, ledger4Info, 26);

//Alice在分类帐58中应该有52个信托行。
        testAccountLinesHistory (alice, ledger58Info, 52);

        {
//令人惊讶的是，在
//哪种情况下散列值会赢。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_hash": ")" + to_string(ledger4Info.hash) + R"(", )"
                R"("ledger_index": )" + std::to_string(ledger58Info.seq) + "}}");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 26);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
//Alice在当前分类帐中应该有52个信托行。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"("}})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 52);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
//Alice应该和GW1有26条信任线。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("peer": ")" + gw1.human() + R"("}})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 26);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
//使用格式错误的对等机。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("peer": )"
                R"("n9MJkEKHDhy5eTLuHUQeAAjo382frHNbFK4C8hcwN4nwM2SrLdBj"}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::make_error(rpcBAD_SEED)[jss::error_message]);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
//负极限应该失败。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": -1}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::expected_field_message(jss::limit, "unsigned integer"));
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
//将响应限制为1个信任行。
            auto const linesA = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 1}})");
            BEAST_EXPECT(linesA[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesA[jss::result][jss::lines].size() == 1);
            BEAST_EXPECT(linesA.isMember(jss::jsonrpc) && linesA[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesA.isMember(jss::ripplerpc) && linesA[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesA.isMember(jss::id) && linesA[jss::id] == 5);

//从记号笔停止的地方捡起来。我们应该得到51。
            auto marker = linesA[jss::result][jss::marker].asString();
            auto const linesB = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": ")" + marker + R"("}})");
            BEAST_EXPECT(linesB[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesB[jss::result][jss::lines].size() == 51);
            BEAST_EXPECT(linesB.isMember(jss::jsonrpc) && linesB[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesB.isMember(jss::ripplerpc) && linesB[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesB.isMember(jss::id) && linesB[jss::id] == 5);

//再次从标记停止的位置开始，但设置限制为3。
            auto const linesC = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 3, )"
                R"("marker": ")" + marker + R"("}})");
            BEAST_EXPECT(linesC[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesC[jss::result][jss::lines].size() == 3);
            BEAST_EXPECT(linesC.isMember(jss::jsonrpc) && linesC[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesC.isMember(jss::ripplerpc) && linesC[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesC.isMember(jss::id) && linesC[jss::id] == 5);

//弄乱标记，使其变差并检查错误。
            marker[5] = marker[5] == '7' ? '8' : '7';
            auto const linesD = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": ")" + marker + R"("}})");
            BEAST_EXPECT(linesD[jss::error][jss::message] ==
                RPC::make_error(rpcINVALID_PARAMS)[jss::error_message]);
            BEAST_EXPECT(linesD.isMember(jss::jsonrpc) && linesD[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesD.isMember(jss::ripplerpc) && linesD[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesD.isMember(jss::id) && linesD[jss::id] == 5);
        }
        {
//非字符串标记也应该失败。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": true}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::expected_field_message(jss::marker, "string"));
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
//检查我们期望从Alice到GW2的标志是否存在。
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 1, )"
                R"("peer": ")" + gw2.human() + R"("}})");
            auto const& line = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line[jss::freeze].asBool() == true);
            BEAST_EXPECT(line[jss::no_ripple].asBool() == true);
            BEAST_EXPECT(line[jss::peer_authorized].asBool() == true);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
//检查GW2到Alice的期望标志是否存在。
            auto const linesA = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + gw2.human() + R"(", )"
                R"("limit": 1, )"
                R"("peer": ")" + alice.human() + R"("}})");
            auto const& lineA = linesA[jss::result][jss::lines][0u];
            BEAST_EXPECT(lineA[jss::freeze_peer].asBool() == true);
            BEAST_EXPECT(lineA[jss::no_ripple_peer].asBool() == true);
            BEAST_EXPECT(lineA[jss::authorized].asBool() == true);
            BEAST_EXPECT(linesA.isMember(jss::jsonrpc) && linesA[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesA.isMember(jss::ripplerpc) && linesA[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesA.isMember(jss::id) && linesA[jss::id] == 5);

//从返回的标记继续，以确保工作正常。
            BEAST_EXPECT(linesA[jss::result].isMember(jss::marker));
            auto const marker = linesA[jss::result][jss::marker].asString();
            auto const linesB = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + gw2.human() + R"(", )"
                R"("limit": 25, )"
                R"("marker": ")" + marker + R"(", )"
                R"("peer": ")" + alice.human() + R"("}})");
            BEAST_EXPECT(linesB[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesB[jss::result][jss::lines].size() == 25);
            BEAST_EXPECT(! linesB[jss::result].isMember(jss::marker));
            BEAST_EXPECT(linesB.isMember(jss::jsonrpc) && linesB[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesB.isMember(jss::ripplerpc) && linesB[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesB.isMember(jss::id) && linesB[jss::id] == 5);
        }
    }

//测试API V2
    void testAccountLineDelete2()
    {
        testcase ("V2: account_lines with removed marker");

        using namespace test::jtx;
        Env env(*this);

//这里的目标是观察账户行标记行为，如果
//返回标记指向的条目将从分类帐中删除。
//
//显式删除信任行并不容易，因此我们在
//流行时尚。它需要4个演员：
//o网关GW1发行欧元
//o Alice提议以100 XRP的价格购买100欧元。
//o贝基提出以100瑞波币卖出100欧元。
//Alice和GW2之间现在将有一个推断的信任线。
//o爱丽丝付给切丽100欧元。
//爱丽丝现在应该没有欧元，也没有对GW2的信任。
        Account const alice {"alice"};
        Account const becky {"becky"};
        Account const cheri {"cheri"};
        Account const gw1 {"gw1"};
        Account const gw2 {"gw2"};
        env.fund(XRP(10000), alice, becky, cheri, gw1, gw2);
        env.close();

        auto const USD = gw1["USD"];
        auto const EUR = gw2["EUR"];
        env(trust(alice, USD(200)));
        env(trust(becky, EUR(200)));
        env(trust(cheri, EUR(200)));
        env.close();

//贝基从GW1获得100欧元。
        env(pay(gw2, becky, EUR(100)));
        env.close();

//艾丽斯提出以100瑞波币购买100欧元。
        env(offer(alice, EUR(100), XRP(100)));
        env.close();

//贝基提议以100欧元的价格购买100瑞波币。
        env(offer(becky, XRP(100), EUR(100)));
        env.close();

//获取Alice的帐户行。限制在1，所以我们得到一个标记。
        auto const linesBeg = env.rpc ("json2", "{ "
            R"("method" : "account_lines",)"
            R"("jsonrpc" : "2.0",)"
            R"("ripplerpc" : "2.0",)"
            R"("id" : 5,)"
            R"("params": )"
            R"({"account": ")" + alice.human() + R"(", )"
            R"("limit": 1}})");
        BEAST_EXPECT(linesBeg[jss::result][jss::lines][0u][jss::currency] == "USD");
        BEAST_EXPECT(linesBeg[jss::result].isMember(jss::marker));
        BEAST_EXPECT(linesBeg.isMember(jss::jsonrpc) && linesBeg[jss::jsonrpc] == "2.0");
        BEAST_EXPECT(linesBeg.isMember(jss::ripplerpc) && linesBeg[jss::ripplerpc] == "2.0");
        BEAST_EXPECT(linesBeg.isMember(jss::id) && linesBeg[jss::id] == 5);

//爱丽丝付给奇瑞100美元。
        env(pay(alice, cheri, EUR(100)));
        env.close();

//既然爱丽丝把所有的欧元都付给了切丽，爱丽丝就不应该再这样了。
//与GW1建立信任关系。所以旧标记现在应该是无效的。
        auto const linesEnd = env.rpc ("json2", "{ "
            R"("method" : "account_lines",)"
            R"("jsonrpc" : "2.0",)"
            R"("ripplerpc" : "2.0",)"
            R"("id" : 5,)"
            R"("params": )"
            R"({"account": ")" + alice.human() + R"(", )"
            R"("marker": ")" +
            linesBeg[jss::result][jss::marker].asString() + R"("}})");
        BEAST_EXPECT(linesEnd[jss::error][jss::message] ==
                RPC::make_error(rpcINVALID_PARAMS)[jss::error_message]);
        BEAST_EXPECT(linesEnd.isMember(jss::jsonrpc) && linesEnd[jss::jsonrpc] == "2.0");
        BEAST_EXPECT(linesEnd.isMember(jss::ripplerpc) && linesEnd[jss::ripplerpc] == "2.0");
        BEAST_EXPECT(linesEnd.isMember(jss::id) && linesEnd[jss::id] == 5);
    }

    void run () override
    {
        testAccountLines();
        testAccountLineDelete();
        testAccountLines2();
        testAccountLineDelete2();
    }
};

BEAST_DEFINE_TESTSUITE(AccountLinesRPC,app,ripple);

} //RPC
} //涟漪

