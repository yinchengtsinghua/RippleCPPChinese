
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
    版权所有（c）2012，2013 Ripple Labs Inc.

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

#include <test/jtx/balance.h>
#include <test/jtx/Env.h>
#include <test/jtx/fee.h>
#include <test/jtx/flags.h>
#include <test/jtx/pay.h>
#include <test/jtx/trust.h>
#include <test/jtx/require.h>
#include <test/jtx/seq.h>
#include <test/jtx/sig.h>
#include <test/jtx/utility.h>
#include <test/jtx/JSONRPCClient.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/consensus/LedgerTiming.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Slice.h>
#include <ripple/json/to_string.h>
#include <ripple/net/HTTPClient.h>
#include <ripple/net/RPCCall.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/SystemParameters.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/protocol/Feature.h>
#include <memory>

namespace ripple {
namespace test {
namespace jtx {

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Env::AppBundle::AppBundle(beast::unit_test::suite& suite,
    std::unique_ptr<Config> config,
    std::unique_ptr<Logs> logs)
{
    using namespace beast::severities;
//使用Kfatal阈值减少来自Stobject的噪声。
    setDebugLogSink (std::make_unique<SuiteJournalSink>(
        "Debug", kFatal, suite));
    auto timeKeeper_ =
        std::make_unique<ManualTimeKeeper>();
    timeKeeper = timeKeeper_.get();
//所以我们不必调用config:：setup
    HTTPClient::initializeSSLContext(*config, debugLog());
    owned = make_Application(std::move(config),
        std::move(logs), std::move(timeKeeper_));
    app = owned.get();
    app->logs().threshold(kError);
    if(! app->setup())
        Throw<std::runtime_error> ("Env::AppBundle: setup failed");
    timeKeeper->set(
        app->getLedgerMaster().getClosedLedger()->info().closeTime);
    /*->DoStart（错误/*不启动计时器*/）；
    线程=标准：：线程（
        [&]（）app->run（）；）；

    client=makejsonrpcclient（app->config（））；
}

env:：appbundle:：~appbundle（）。
{
    客户机。
    //确保所有作业完成，否则测试
    //可能无法获得他们期望的覆盖范围。
    app->getJobQueue（）.rendezvous（）；
    app->signalStop（）；
    线程；

    //在套件超出范围之前移除debugLink。
    setdebugglogsink（空指针）；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std:：shared_ptr<readview const>
Env:：关闭（）
{
    返回app（）.getledgermaster（）.getClosedledger（）；
}

无效
ENV：：CLOSE（网络时钟：：时间点关闭时间，
    boost：：可选<std:：chrono:：millises>conensusdelay）
{
    //向上取整到下一个可分辨值
    使用名称空间std:：chrono-literals；
    closeTime+=closed（）->info（）.closeTimeResolution-1s；
    timekeeper（）。设置（closetime）；
    //除非我们需要模拟
    //特定的共识延迟。
    如果（同意延迟）
        app（）.getops（）.acceptledger（consensusdelay）；
    其他的
    {
        rpc（“分类账接受”）；
        //vfalc无错误检查？
    }
    timekeeper（）。设置（
        closed（）->info（）.closetime）；
}

无效
env:：memoize（帐户常数和帐户）
{
    映射模板（account.id（），account）；
}

帐户常量
env：：查找（accountid const&id）const
{
    auto const iter=地图查找（id）；
    if（iter==map.end（））
        抛出<std:：runtime_error>（）
            “env:：lookup：：未知帐户ID”）；
    返回iter->second；
}

帐户常量
env:：lookup（std:：string const&base58id）const
{
    自动常量帐户=
        parsebase58<accountid>（base58id）；
    如果（！）帐户）
        抛出<std:：runtime_error>（）
            “env:：lookup:帐户ID无效”）；
    退货查询（*account）；
}

漂亮的数量
环境：余额（账户常数和账户）常数
{
    auto const sle=le（账户）；
    如果（！）系统性红斑狼疮
        返回XRP（0）；
    返回{
        SLE->GetFieldAmount（sfBalance）
            “”
}

漂亮的数量
环境：余额（账户常数和账户，
    发布常量和发布）常量
{
    if（isxrp（发行货币）
        返回余额（账户）；
    auto const sle=le（keylet:：line（
        account.id（），issue））；
    如果（！）系统性红斑狼疮
        返回stamount（issue，0），
            account.name（）
    auto amount=sle->getfieldamount（sfbalance）；
    金额.设置发行人（发行.帐户）；
    如果（account.id（）>issue.account）
        amount.negate（）；
    返回金额，
        查找（issue.account）.name（）
}

STD:：UIT32
env：：seq（account const&account）const
{
    auto const sle=le（账户）；
    如果（！）系统性红斑狼疮
        抛出<std:：runtime_error>（）
            “账户根丢失”）；
    返回sle->getfieldu32（sfsequence）；
}

std:：shared&ptr<sle const>
环境：Le（帐户常数和帐户）常数
{
    返回le（keylet:：account（account.id（））；
}

std:：shared&ptr<sle const>
环境：：le（keylet const&k）const
{
    返回电流（）->读取（K）；
}

无效
env:：fund（bool setdefaultripple，
    StamCount常量和金额，
        账户常数和账户）
{
    memoize（账户）；
    if（设置默认纹波）
    {
        //vfalc note费用公式是否正确？
        申请（支付（主账户、账户、金额+
            删除（current（）->fees（）.base）），
                jtx：：seq（jtx：：autofill），
                    费用（jtx:：autofill）
                        sig（jtx:：autofill））；
        应用（fset（account，asfDefaultRipple）
            jtx：：seq（jtx：：autofill），
                费用（jtx:：autofill）
                    sig（jtx:：autofill））；
        要求（标志（account、asfDefaultRipple））；
    }
    其他的
    {
        应用（支付（主账户、账户、金额）
            jtx：：seq（jtx：：autofill），
                费用（jtx:：autofill）
                    sig（jtx:：autofill））；
        要求（nflags（account，asfDefaultRipple））；
    }
    要求（jtx:：balance（account，amount））；
}

无效
env:：trust（stamount const&amount，
    账户常数和账户）
{
    自动常数开始=余额（账户）；
    应用（jtx:：trust（account，amount）
        jtx：：seq（jtx：：autofill），
            费用（jtx:：autofill）
                sig（jtx:：autofill））；
    申请（支付（主账户，账户，
        删除（current（）->fees（）.base）），
            jtx：：seq（jtx：：autofill），
                费用（jtx:：autofill）
                    sig（jtx:：autofill））；
    test.expect（balance（account）==开始）；
}

std:：pair<ter，bool>
env:：parseResult（json:：value const&jr）
{
    彼得；
    if（jr.isObject（）&&jr.ismember（jss:：result）&&
        jr[jss:：result].ismember（jss:：engine_result_code））
        ter=ter:：fromint（jr[jss:：result][jss:：engine_result_code].asint（））；
    其他的
        ter=t无效；
    返回std：：使配对（ter，
        是否成功（ter）是否索赔（ter））；
}

无效
提交（jtx const&jt）
{
    布尔申请；
    如果（J.STX）
    {
        txid_u=jt.stx->getTransactionId（）；
        串行器S；
        STX-> ADS（S）；
        auto const jr=rpc（“提交”，strhex（s.slice（））；

        std：：tie（ter_u，didapply）=parseResult（jr）；
    }
    其他的
    {
        //解析失败或jtx是
        //否则缺少stx字段。
        term畸形；
        didapply=错误；
    }
    返回后置条件（jt，ter_uu，didapply）；
}

无效
env:：sign_and_submit（jtx const&jt，json:：value params）
{
    布尔申请；

    自动常量帐户=
        查找（jt.jv[jss:：account].asstring（））；
    auto const&passphrase=account.name（）；

    JSO:：价值JR；
    if（params.isNull（））
    {
        //使用命令行界面
        auto const jv=boost:：lexical_cast<std:：string>（jt.jv）；
        jr=rpc（“提交”，密码，合资企业）；
    }
    其他的
    {
        //使用提供的参数，然后直接执行
        //到（RPC）客户端。
        断言（params.isObject（））；
        如果（！）params.ismember（jss:：secret）&&
            ！参数ismember（jss:：key_type）&&
            ！参数ismember（jss:：seed）&&
            ！参数ismember（jss:：seed_hex）&&
            ！params.ismember（jss:：passphrase）
        {
            params[jss:：secret]=passphrase；
        }
        参数[jss:：tx_json]=jt.jv；
        jr=client（）.invoke（“提交”，参数）；
    }
    TXIDI. SetHex
        jr[jss:：result][jss:：tx_json][jss:：hash].asstring（））；

    std：：tie（ter_u，didapply）=parseResult（jr）；

    返回后置条件（jt，ter_uu，didapply）；
}

无效
环境：后置条件（jtx const&jt、ter ter ter、bool didapply）
{
    如果（jt.ter&！测试。预期（ter==*jt.ter，
        “应用：”+transtoken（ter）+
            “（”+transhuman（ter）+“）！=“+”
                transtoken（*jt.ter）+“（+
                    异人（*jt.ter）+”））
    {
        test.log<<pretty（jt.jv）<<std:：endl；
        //如果
        //我们没有得到预期的结果。
        返回；
    }
    如果（TraceEi）
    {
        如果（Trace>＞0）
            ——TraceEi；
        test.log<<pretty（jt.jv）<<std:：endl；
    }
    for（auto const&f:jt.要求）
        f（*此）；
}

std:：shared_ptr<stobject const>
EnV:：元（）
{
    关闭（）；
    auto const item=closed（）->txread（txid_u）；
    返回item.second；
}

std:：shared_ptr<sttx const>
EXV:TX（）常数
{
    return current（）->txread（txid_uu）.first；
}

无效
环境：自动填充信号（JTX&JT）
{
    汽车和合资企业=合资企业；
    如果（J.签字人）
        返回jt.signer（*this，jt）；
    如果（！）J.FILYSIG）
        返回；
    自动常量帐户=
        查找（jv[jss:：account].asstring（））；
    如果（！）app（）.checksigs（））
    {
        合资企业[JSS:：SigningPubKey]=
            strhex（account.pk（）.slice（））；
        //假信号，否则sttx无效
        jv[jss:：txsignature]=“00”；
        返回；
    }
    auto const ar=le（账户）；
    如果（ar和ar->isFieldPresent（sfRegularKey））
        jtx：：签名（jv，lookup（
            ar->getaccountid（sfregularkey））；
    其他的
        JTX：签字（合资企业，账户）；
}

无效
env:：autofill（jtx和jt）
{
    汽车和合资企业=合资企业；
    如果（J.FILIIAL费）
        jtx：：填写费用（jv，*current（））；
    如果（J.FiLySeq）
        jtx:：fill_seq（jv，*current（））；
    //必须排在最后
    尝试
    {
        自动填充信号（JT）；
    }
    catch（分析错误常量&）
    {
        test.log<“分析失败：\n”<<
            漂亮（合资）<<std:：endl；
        重新抛出（）；
    }
}

std:：shared_ptr<sttx const>
环境：ST（JTX常量和JT）
{
    //解析必须成功，因为我们
    //自己生成了JSON。
    boost：：可选<stobject>obj；
    尝试
    {
        obj=jtx:：parse（jt.jv）；
    }
    catch（jtx:：parse_error const&）
    {
        test.log<“异常：分析错误\n”<<
            漂亮（jt.jv）<<std:：endl；
        重新抛出（）；
    }

    尝试
    {
        返回消毒器（sttx std:：move（*obj））；
    }
    catch（std:：exception const&）
    {
    }
    返回null pTR；
}

JSON:：价值
env:：do_rpc（std:：vector<std:：string>const&args）
{
    返回rpcclient（args，app（）.config（），app（）.logs（））.second；
}

无效
env：：启用功能（uint256 const功能）
{
    //必须为功能调用env:：close（）。
    //启用以发生。
    app（）.config（）.features.insert（功能）；
}

} //JTX

}／／检验
} /纹波
