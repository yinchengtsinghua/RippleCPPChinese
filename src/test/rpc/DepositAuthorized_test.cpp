
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
    版权所有（c）2018 Ripple Labs Inc.

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

#include <ripple/protocol/JsonFields.h>     //JSS：：定义
#include <test/jtx.h>

namespace ripple {
namespace test {

class DepositAuthorized_test : public beast::unit_test::suite
{
public:
//为deposit_authorized命令生成参数的helper函数。
    static Json::Value depositAuthArgs (
        jtx::Account const& source,
        jtx::Account const& dest, std::string const& ledger = "")
    {
        Json::Value args {Json::objectValue};
        args[jss::source_account] = source.human();
        args[jss::destination_account] = dest.human();
        if (! ledger.empty())
            args[jss::ledger_index] = ledger;
        return args;
    }

//验证存款授权请求的helper函数是
//成功，返回了预期值。
    void validateDepositAuthResult (Json::Value const& result, bool authorized)
    {
        Json::Value const& results {result[jss::result]};
        BEAST_EXPECT (results[jss::deposit_authorized] == authorized);
        BEAST_EXPECT (results[jss::status] == jss::success);
    }

//测试各种非畸形病例。
    void testValid()
    {
        using namespace jtx;
        Account const alice {"alice"};
        Account const becky {"becky"};
        Account const carol {"carol"};

        Env env(*this);
        env.fund(XRP(1000), alice, becky, carol);
        env.close();

//贝基有权自己存款。
        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (
                becky, becky, "validated").toStyledString()), true);

//艾丽丝现在应该被授权向贝基存款。
        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (
                alice, becky, "validated").toStyledString()), true);

//贝基在当前分类帐中设置depositauth标志。
        env (fset (becky, asfDepositAuth));

//艾丽斯不再被授权在活期分类账中存入贝基。
        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (alice, becky).toStyledString()), false);
        env.close();

//贝基仍有权向自己存款。
        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (
                becky, becky, "validated").toStyledString()), true);

//这不是互惠安排。贝基可以把钱交给爱丽丝。
        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (
                becky, alice, "current").toStyledString()), true);

//Becky为Alice创建存款授权。
        env (deposit::auth (becky, alice));
        env.close();

//艾丽丝现在被授权向贝基存款。
        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (alice, becky, "closed").toStyledString()), true);

//卡罗尔仍无权向贝基存款。
        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (carol, becky).toStyledString()), false);

//贝基清除了储金旗，这样卡罗尔就可以获得授权了。
        env (fclear (becky, asfDepositAuth));
        env.close();

        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (carol, becky).toStyledString()), true);

//艾丽丝仍有权向贝基存款。
        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (alice, becky).toStyledString()), true);
    }

//测试畸形病例。
    void testErrors()
    {
        using namespace jtx;
        Account const alice {"alice"};
        Account const becky {"becky"};

//lambda，用于检查授权存款的（错误）结果。
        auto verifyErr = [this] (
            Json::Value const& result, char const* error, char const* errorMsg)
        {
            BEAST_EXPECT (result[jss::result][jss::status] == jss::error);
            BEAST_EXPECT (result[jss::result][jss::error] == error);
            BEAST_EXPECT (result[jss::result][jss::error_message] == errorMsg);
        };

        Env env(*this);
        {
//缺少源帐户字段。
            Json::Value args {depositAuthArgs (alice, becky)};
            args.removeMember (jss::source_account);
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "invalidParams",
                "Missing field 'source_account'.");
        }
        {
//非字符串源帐户字段。
            Json::Value args {depositAuthArgs (alice, becky)};
            args[jss::source_account] = 7.3;
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "invalidParams",
                "Invalid field 'source_account', not a string.");
        }
        {
//源帐户字段损坏。
            Json::Value args {depositAuthArgs (alice, becky)};
            args[jss::source_account] = "rG1QQv2nh2gr7RCZ!P8YYcBUKCCN633jCn";
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "actMalformed", "Account malformed.");
        }
        {
//缺少目标帐户字段。
            Json::Value args {depositAuthArgs (alice, becky)};
            args.removeMember (jss::destination_account);
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "invalidParams",
                "Missing field 'destination_account'.");
        }
        {
//非字符串目标帐户字段。
            Json::Value args {depositAuthArgs (alice, becky)};
            args[jss::destination_account] = 7.3;
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "invalidParams",
                "Invalid field 'destination_account', not a string.");
        }
        {
//目标帐户字段损坏。
            Json::Value args {depositAuthArgs (alice, becky)};
            args[jss::destination_account] =
                "rP6P9ypfAmc!pw8SZHNwM4nvZHFXDraQas";
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "actMalformed", "Account malformed.");
        }
        {
//请求无效的分类帐。
            Json::Value args {depositAuthArgs (alice, becky, "17")};
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "invalidParams", "ledgerIndexMalformed");
        }
        {
//请求尚未存在的分类帐。
            Json::Value args {depositAuthArgs (alice, becky)};
            args[jss::ledger_index] = 17;
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "lgrNotFound", "ledgerNotFound");
        }
        {
//爱丽丝还没有得到资助。
            Json::Value args {depositAuthArgs (alice, becky)};
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "srcActNotFound",
                "Source account not found.");
        }
        env.fund(XRP(1000), alice);
        env.close();
        {
//贝基还没有得到资助。
            Json::Value args {depositAuthArgs (alice, becky)};
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "dstActNotFound",
                "Destination account not found.");
        }
        env.fund(XRP(1000), becky);
        env.close();
        {
//一旦贝基得到资助，再试一次，看看它是否成功。
            Json::Value args {depositAuthArgs (alice, becky)};
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            validateDepositAuthResult (result, true);
        }
    }

    void run() override
    {
        testValid();
        testErrors();
    }
};

BEAST_DEFINE_TESTSUITE(DepositAuthorized,app,ripple);

}
}

