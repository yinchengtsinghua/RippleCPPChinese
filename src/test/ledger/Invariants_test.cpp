
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
#include <ripple/beast/utility/Journal.h>
#include <ripple/app/tx/apply.h>
#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/app/tx/impl/ApplyContext.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <boost/algorithm/string/predicate.hpp>

namespace ripple {

class Invariants_test : public beast::unit_test::suite
{

    class TestSink : public beast::Journal::Sink
    {
    public:
        std::stringstream strm_;

        TestSink () : Sink (beast::severities::kWarning, false) {  }

        void
        write (beast::severities::Severity level,
            std::string const& text) override
        {
            if (level < threshold())
                return;

            strm_ << text << std::endl;
        }
    };

//这是运行失败的不变检查的常见设置/方法。这个
//precheck函数用于使用view操作applycontext
//将导致检查失败的更改。
    using Precheck = std::function <
        bool (
            test::jtx::Account const& a,
            test::jtx::Account const& b,
            ApplyContext& ac)>;

    using TXMod = std::function <
        void (STTx& tx)>;

    void
    doInvariantCheck( bool enabled,
        std::vector<std::string> const& expect_logs,
        Precheck const& precheck,
        XRPAmount fee = XRPAmount{},
        TXMod txmod = [](STTx&){})
    {
        using namespace test::jtx;
        Env env {*this};
        if (! enabled)
        {
            auto& features = env.app().config().features;
            auto it = features.find(featureEnforceInvariants);
            if (it != features.end())
                features.erase(it);
        }

        Account A1 {"A1"};
        Account A2 {"A2"};
        env.fund (XRP (1000), A1, A2);
        env.close();

//Dummy/Empty Tx设置accountContext
        auto tx = STTx {ttACCOUNT_SET, [](STObject&){  } };
        txmod(tx);
        OpenView ov {*env.current()};
        TestSink sink;
        beast::Journal jlog {sink};
        ApplyContext ac {
            env.app(),
            ov,
            tx,
            tesSUCCESS,
            env.current()->fees().base,
            tapNONE,
            jlog
        };

        BEAST_EXPECT(precheck(A1, A2, ac));

        TER tr = tesSUCCESS;
//调用check两次以覆盖tec和tef案例
        for (auto i : {0,1})
        {
            tr = ac.checkInvariants(tr, fee);
            if (enabled)
            {
                BEAST_EXPECT(tr == (i == 0
                    ? TER {tecINVARIANT_FAILED}
                    : TER {tefINVARIANT_FAILED}));
                BEAST_EXPECT(
                    boost::starts_with(sink.strm_.str(), "Invariant failed:") ||
                    boost::starts_with(sink.strm_.str(),
                        "Transaction caused an exception"));
//如果要记录固定失败消息，请取消注释
//日志<“-->”<<sink.strm_.str（）<<std:：endl；
                for (auto const& m : expect_logs)
                {
                    BEAST_EXPECT(sink.strm_.str().find(m) != std::string::npos);
                }
            }
            else
            {
                BEAST_EXPECT(tr == tesSUCCESS);
                BEAST_EXPECT(sink.strm_.str().empty());
            }
        }
    }

    void
    testEnabled ()
    {
        using namespace test::jtx;
        testcase ("feature enabled");
        Env env {*this};

        auto hasInvariants =
            env.app().config().features.find (featureEnforceInvariants);
        BEAST_EXPECT(hasInvariants != env.app().config().features.end());

        BEAST_EXPECT(env.current()->rules().enabled(featureEnforceInvariants));
    }

    void
    testXRPNotCreated (bool enabled)
    {
        using namespace test::jtx;
        testcase << "checks " << (enabled ? "enabled" : "disabled") <<
            " - XRP created";
        doInvariantCheck (enabled,
            {{ "XRP net change was positive: 500" }},
            [](Account const& A1, Account const&, ApplyContext& ac)
            {
//在视图中输入一个帐户并“制造”一些xrp
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                auto amt = sle->getFieldAmount (sfBalance);
                sle->setFieldAmount (sfBalance, amt + 500);
                ac.view().update (sle);
                return true;
            });
    }

    void
    testAccountsNotRemoved(bool enabled)
    {
        using namespace test::jtx;
        testcase << "checks " << (enabled ? "enabled" : "disabled") <<
            " - account root removed";
        doInvariantCheck (enabled,
            {{ "an account root was deleted" }},
            [](Account const& A1, Account const&, ApplyContext& ac)
            {
//从视图中删除帐户
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                ac.view().erase (sle);
                return true;
            });
    }

    void
    testTypesMatch(bool enabled)
    {
        using namespace test::jtx;
        testcase << "checks " << (enabled ? "enabled" : "disabled") <<
            " - LE types don't match";
        doInvariantCheck (enabled,
            {{ "ledger entry type mismatch" },
             { "XRP net change of -1000000000 doesn't match fee 0" }},
            [](Account const& A1, Account const&, ApplyContext& ac)
            {
//用不同类型的SLE替换表中的条目
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                auto sleNew = std::make_shared<SLE> (ltTICKET, sle->key());
                ac.rawView().rawReplace (sleNew);
                return true;
            });

        doInvariantCheck (enabled,
            {{ "invalid ledger entry type added" }},
            [](Account const& A1, Account const&, ApplyContext& ac)
            {
//在表中添加具有无效类型的SLE的条目
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
//创建一个虚拟的托管分类账条目，然后将类型更改为
//不支持的值，因此有效的类型不变检查
//会失败。
                auto sleNew = std::make_shared<SLE> (
                    keylet::escrow(A1, (*sle)[sfSequence] + 2));
                sleNew->type_ = ltNICKNAME;
                ac.view().insert (sleNew);
                return true;
            });
    }

    void
    testNoXRPTrustLine(bool enabled)
    {
        using namespace test::jtx;
        testcase << "checks " << (enabled ? "enabled" : "disabled") <<
            " - trust lines with XRP not allowed";
        doInvariantCheck (enabled,
            {{ "an XRP trust line was created" }},
            [](Account const& A1, Account const& A2, ApplyContext& ac)
            {
//使用XRP货币创建简单信任SLE
                auto index = getRippleStateIndex (A1, A2, xrpIssue().currency);
                auto const sleNew = std::make_shared<SLE>(
                    ltRIPPLE_STATE, index);
                ac.view().insert (sleNew);
                return true;
            });
    }

    void
    testXRPBalanceCheck(bool enabled)
    {
        using namespace test::jtx;
        testcase << "checks " << (enabled ? "enabled" : "disabled") <<
            " - XRP balance checks";

        doInvariantCheck (enabled,
            {{ "Cannot return non-native STAmount as XRPAmount" }},
            [](Account const& A1, Account const& A2, ApplyContext& ac)
            {
//非本地余额
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                STAmount nonNative (A2["USD"](51));
                sle->setFieldAmount (sfBalance, nonNative);
                ac.view().update (sle);
                return true;
            });

        doInvariantCheck (enabled,
            {{ "incorrect account XRP balance" },
             {  "XRP net change was positive: 99999999000000001" }},
            [](Account const& A1, Account const&, ApplyContext& ac)
            {
//余额超过创世金额
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                sle->setFieldAmount (sfBalance, SYSTEM_CURRENCY_START + 1);
                ac.view().update (sle);
                return true;
            });

        doInvariantCheck (enabled,
            {{ "incorrect account XRP balance" },
             { "XRP net change of -1000000001 doesn't match fee 0" }},
            [](Account const& A1, Account const&, ApplyContext& ac)
            {
//余额为负
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                sle->setFieldAmount (sfBalance, -1);
                ac.view().update (sle);
                return true;
            });
    }

    void
    testTransactionFeeCheck(bool enabled)
    {
        using namespace test::jtx;
        using namespace std::string_literals;
        testcase << "checks " << (enabled ? "enabled" : "disabled") <<
            " - Transaction fee checks";

        doInvariantCheck (enabled,
            {{ "fee paid was negative: -1" },
             { "XRP net change of 0 doesn't match fee -1" }},
            [](Account const&, Account const&, ApplyContext&) { return true; },
            XRPAmount{-1});

        doInvariantCheck (enabled,
            {{ "fee paid exceeds system limit: "s +
                std::to_string(SYSTEM_CURRENCY_START) },
             { "XRP net change of 0 doesn't match fee "s +
                std::to_string(SYSTEM_CURRENCY_START) }},
            [](Account const&, Account const&, ApplyContext&) { return true; },
            XRPAmount{SYSTEM_CURRENCY_START});

         doInvariantCheck (enabled,
            {{ "fee paid is 20 exceeds fee specified in transaction." },
             { "XRP net change of 0 doesn't match fee 20" }},
            [](Account const&, Account const&, ApplyContext&) { return true; },
            XRPAmount{20},
            [](STTx& tx) { tx.setFieldAmount(sfFee, XRPAmount{10}); } );
    }


    void
    testNoBadOffers(bool enabled)
    {
        using namespace test::jtx;
        testcase << "checks " << (enabled ? "enabled" : "disabled") <<
            " - no bad offers";

        doInvariantCheck (enabled,
            {{ "offer with a bad amount" }},
            [](Account const& A1, Account const&, ApplyContext& ac)
            {
//带负买主付款的报价
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                auto const offer_index =
                    getOfferIndex (A1.id(), (*sle)[sfSequence]);
                auto sleNew = std::make_shared<SLE> (ltOFFER, offer_index);
                sleNew->setAccountID (sfAccount, A1.id());
                sleNew->setFieldU32 (sfSequence, (*sle)[sfSequence]);
                sleNew->setFieldAmount (sfTakerPays, XRP(-1));
                ac.view().insert (sleNew);
                return true;
            });

        doInvariantCheck (enabled,
            {{ "offer with a bad amount" }},
            [](Account const& A1, Account const&, ApplyContext& ac)
            {
//带消极接受者的报价
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                auto const offer_index =
                    getOfferIndex (A1.id(), (*sle)[sfSequence]);
                auto sleNew = std::make_shared<SLE> (ltOFFER, offer_index);
                sleNew->setAccountID (sfAccount, A1.id());
                sleNew->setFieldU32 (sfSequence, (*sle)[sfSequence]);
                sleNew->setFieldAmount (sfTakerPays, A1["USD"](10));
                sleNew->setFieldAmount (sfTakerGets, XRP(-1));
                ac.view().insert (sleNew);
                return true;
            });

        doInvariantCheck (enabled,
            {{ "offer with a bad amount" }},
            [](Account const& A1, Account const&, ApplyContext& ac)
            {
//向XRP提供XRP
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                auto const offer_index =
                    getOfferIndex (A1.id(), (*sle)[sfSequence]);
                auto sleNew = std::make_shared<SLE> (ltOFFER, offer_index);
                sleNew->setAccountID (sfAccount, A1.id());
                sleNew->setFieldU32 (sfSequence, (*sle)[sfSequence]);
                sleNew->setFieldAmount (sfTakerPays, XRP(10));
                sleNew->setFieldAmount (sfTakerGets, XRP(11));
                ac.view().insert (sleNew);
                return true;
            });
    }

    void
    testNoZeroEscrow(bool enabled)
    {
        using namespace test::jtx;
        testcase << "checks " << (enabled ? "enabled" : "disabled") <<
            " - no zero escrow";

        doInvariantCheck (enabled,
            {{ "Cannot return non-native STAmount as XRPAmount" }},
            [](Account const& A1, Account const& A2, ApplyContext& ac)
            {
//非本地金额托管
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                auto sleNew = std::make_shared<SLE> (
                    keylet::escrow(A1, (*sle)[sfSequence] + 2));
                STAmount nonNative (A2["USD"](51));
                sleNew->setFieldAmount (sfAmount, nonNative);
                ac.view().insert (sleNew);
                return true;
            });

        doInvariantCheck (enabled,
            {{ "XRP net change of -1000000 doesn't match fee 0"},
             {  "escrow specifies invalid amount" }},
            [](Account const& A1, Account const&, ApplyContext& ac)
            {
//负金额托管
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                auto sleNew = std::make_shared<SLE> (
                    keylet::escrow(A1, (*sle)[sfSequence] + 2));
                sleNew->setFieldAmount (sfAmount, XRP(-1));
                ac.view().insert (sleNew);
                return true;
            });

        doInvariantCheck (enabled,
            {{ "XRP net change was positive: 100000000000000001" },
             {  "escrow specifies invalid amount" }},
            [](Account const& A1, Account const&, ApplyContext& ac)
            {
//托管金额过大
                auto const sle = ac.view().peek (keylet::account(A1.id()));
                if(! sle)
                    return false;
                auto sleNew = std::make_shared<SLE> (
                    keylet::escrow(A1, (*sle)[sfSequence] + 2));
                sleNew->setFieldAmount (sfAmount, SYSTEM_CURRENCY_START + 1);
                ac.view().insert (sleNew);
                return true;
            });
    }

public:
    void run () override
    {
        testEnabled ();

//现在运行每个不变量检查测试
//功能启用和禁用
        for(auto const& b : {false, true})
        {
            testXRPNotCreated (b);
            testAccountsNotRemoved (b);
            testTypesMatch (b);
            testNoXRPTrustLine (b);
            testXRPBalanceCheck (b);
            testTransactionFeeCheck(b);
            testNoBadOffers (b);
            testNoZeroEscrow (b);
        }
    }
};

BEAST_DEFINE_TESTSUITE (Invariants, ledger, ripple);

}  //涟漪

