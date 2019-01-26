
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
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {

//目前，支票似乎相当自给自足。所以
//在jtx上操作的函数在这里本地定义。如果他们是
//其他单元测试需要它们可以放入另一个文件。
namespace test {
namespace jtx {
namespace check {

//创建支票。
Json::Value
create (jtx::Account const& account,
    jtx::Account const& dest, STAmount const& sendMax)
{
    Json::Value jv;
    jv[sfAccount.jsonName] = account.human();
    jv[sfSendMax.jsonName] = sendMax.getJson(0);
    jv[sfDestination.jsonName] = dest.human();
    jv[sfTransactionType.jsonName] = "CheckCreate";
    jv[sfFlags.jsonName] = tfUniversal;
    return jv;
}

//用于指定支票兑现的delivermin的类型。
struct DeliverMin
{
    STAmount value;
    explicit DeliverMin (STAmount const& deliverMin)
    : value (deliverMin) { }
};

//兑现支票。
Json::Value
cash (jtx::Account const& dest,
    uint256 const& checkId, STAmount const& amount)
{
    Json::Value jv;
    jv[sfAccount.jsonName] = dest.human();
    jv[sfAmount.jsonName]  = amount.getJson(0);
    jv[sfCheckID.jsonName] = to_string (checkId);
    jv[sfTransactionType.jsonName] = "CheckCash";
    jv[sfFlags.jsonName] = tfUniversal;
    return jv;
}

Json::Value
cash (jtx::Account const& dest,
    uint256 const& checkId, DeliverMin const& atLeast)
{
    Json::Value jv;
    jv[sfAccount.jsonName] = dest.human();
    jv[sfDeliverMin.jsonName]  = atLeast.value.getJson(0);
    jv[sfCheckID.jsonName] = to_string (checkId);
    jv[sfTransactionType.jsonName] = "CheckCash";
    jv[sfFlags.jsonName] = tfUniversal;
    return jv;
}

//取消支票。
Json::Value
cancel (jtx::Account const& dest, uint256 const& checkId)
{
    Json::Value jv;
    jv[sfAccount.jsonName] = dest.human();
    jv[sfCheckID.jsonName] = to_string (checkId);
    jv[sfTransactionType.jsonName] = "CheckCancel";
    jv[sfFlags.jsonName] = tfUniversal;
    return jv;
}

} //命名空间检查

/*在JTX上设置过期。*/
class expiration
{
private:
    std::uint32_t const expry_;

public:
    explicit expiration (NetClock::time_point const& expiry)
        : expry_{expiry.time_since_epoch().count()}
    {
    }

    void
    operator()(Env&, JTx& jt) const
    {
        jt[sfExpiration.jsonName] = expry_;
    }
};

/*在jtx上设置sourcetag。*/
class source_tag
{
private:
    std::uint32_t const tag_;

public:
    explicit source_tag (std::uint32_t tag)
        : tag_{tag}
    {
    }

    void
    operator()(Env&, JTx& jt) const
    {
        jt[sfSourceTag.jsonName] = tag_;
    }
};

/*在jtx上设置destinationtag。*/
class dest_tag
{
private:
    std::uint32_t const tag_;

public:
    explicit dest_tag (std::uint32_t tag)
        : tag_{tag}
    {
    }

    void
    operator()(Env&, JTx& jt) const
    {
        jt[sfDestinationTag.jsonName] = tag_;
    }
};

/*在JTX上设置invoiceID。*/
class invoice_id
{
private:
    uint256 const id_;

public:
    explicit invoice_id (uint256 const& id)
        : id_{id}
    {
    }

    void
    operator()(Env&, JTx& jt) const
    {
        jt[sfInvoiceID.jsonName] = to_string (id_);
    }
};

} //命名空间JTX
} //命名空间测试

class Check_test : public beast::unit_test::suite
{
//返回帐户检查的帮助程序函数。
    static std::vector<std::shared_ptr<SLE const>>
    checksOnAccount (test::jtx::Env& env, test::jtx::Account account)
    {
        std::vector<std::shared_ptr<SLE const>> result;
        forEachItem (*env.current (), account,
            [&result](std::shared_ptr<SLE const> const& sle)
            {
                if (sle->getType() == ltCHECK)
                     result.push_back (sle);
            });
        return result;
    }

//返回帐户所有者计数的helper函数。
    static std::uint32_t
    ownerCount (test::jtx::Env const& env, test::jtx::Account const& account)
    {
        std::uint32_t ret {0};
        if (auto const sleAccount = env.le(account))
            ret = sleAccount->getFieldU32(sfOwnerCount);
        return ret;
    }

//验证预期DeliveredAmount存在的帮助程序函数。
//
//注意：函数通过调用来推断要操作的事务
//env.tx（），返回最新事务的结果。
    void verifyDeliveredAmount (test::jtx::Env& env, STAmount const& amount)
    {
//获取最近事务的哈希。
        std::string const txHash {env.tx()->getJson (0)[jss::hash].asString()};

//验证DeliveredAmount和Delivered_Amount元数据是否正确。
        env.close();
        Json::Value const meta = env.rpc ("tx", txHash)[jss::result][jss::meta];

//希望有一个deliveredamount字段。
        if (! BEAST_EXPECT(meta.isMember (sfDeliveredAmount.jsonName)))
            return;

//DeliveredAmount和Delivered_金额应同时存在和
//等量。
        BEAST_EXPECT (meta[sfDeliveredAmount.jsonName] == amount.getJson (0));
        BEAST_EXPECT (meta[jss::delivered_amount] == amount.getJson (0));
    }

    void testEnabled()
    {
        testcase ("Enabled");

        using namespace test::jtx;
        Account const alice {"alice"};
        {
//如果未启用检查修正，则不应
//创建、兑现或取消支票。
            Env env {*this, supported_amendments() - featureChecks};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), alice);

            uint256 const checkId {
                getCheckIndex (env.master, env.seq (env.master))};
            env (check::create (env.master, alice, XRP(100)),
                ter(temDISABLED));
            env.close();

            env (check::cash (alice, checkId, XRP(100)),
                ter (temDISABLED));
            env.close();

            env (check::cancel (alice, checkId), ter (temDISABLED));
            env.close();
        }
        {
//如果启用了检查修正，则所有与检查相关的
//应提供设施。
            Env env {*this};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), alice);

            uint256 const checkId1 {
                getCheckIndex (env.master, env.seq (env.master))};
            env (check::create (env.master, alice, XRP(100)));
            env.close();

            env (check::cash (alice, checkId1, XRP(100)));
            env.close();

            uint256 const checkId2 {
                getCheckIndex (env.master, env.seq (env.master))};
            env (check::create (env.master, alice, XRP(100)));
            env.close();

            env (check::cancel (alice, checkId2));
            env.close();
        }
    }

    void testCreateValid()
    {
//探索创建支票的许多有效方法。
        testcase ("Create valid");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        IOU const USD {gw["USD"]};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        STAmount const startBalance {XRP(1000).value()};
        env.fund (startBalance, gw, alice, bob);

//请注意，没有为Alice设置信任线，但Alice可以
//还是开一张美元支票。你不需要钱
//为了写支票必须盖上支票。
        auto writeTwoChecks =
            [&env, &USD, this] (Account const& from, Account const& to)
        {
            std::uint32_t const fromOwnerCount {ownerCount (env, from)};
            std::uint32_t const toOwnerCount   {ownerCount (env, to  )};

            std::size_t const fromCkCount {checksOnAccount (env, from).size()};
            std::size_t const toCkCount   {checksOnAccount (env, to  ).size()};

            env (check::create (from, to, XRP(2000)));
            env.close();

            env (check::create (from, to, USD(50)));
            env.close();

            BEAST_EXPECT (
                checksOnAccount (env, from).size() == fromCkCount + 2);
            BEAST_EXPECT (
                checksOnAccount (env, to  ).size() == toCkCount   + 2);

            env.require (owners (from, fromOwnerCount + 2));
            env.require (owners (to,
                to == from ? fromOwnerCount + 2 : toOwnerCount));
        };
//从
        writeTwoChecks (alice,   bob);
        writeTwoChecks (gw,    alice);
        writeTwoChecks (alice,    gw);

//现在尝试添加各种可选字段。没有
//这些可选字段之间的预期交互；而不是
//过期了，他们就被塞进了分类帐。所以我是
//不看互动。
        using namespace std::chrono_literals;
        std::size_t const aliceCount {checksOnAccount (env, alice).size()};
        std::size_t const bobCount   {checksOnAccount (env,   bob).size()};
        env (check::create (alice, bob, USD(50)), expiration (env.now() + 1s));
        env.close();

        env (check::create (alice, bob, USD(50)), source_tag (2));
        env.close();
        env (check::create (alice, bob, USD(50)), dest_tag (3));
        env.close();
        env (check::create (alice, bob, USD(50)), invoice_id (uint256{4}));
        env.close();
        env (check::create (alice, bob, USD(50)), expiration (env.now() + 1s),
            source_tag (12), dest_tag (13), invoice_id (uint256{4}));
        env.close();

        BEAST_EXPECT (checksOnAccount (env, alice).size() == aliceCount + 5);
        BEAST_EXPECT (checksOnAccount (env, bob  ).size() == bobCount   + 5);

//使用常规键和多重签名创建支票。
        Account const alie {"alie", KeyType::ed25519};
        env (regkey (alice, alie));
        env.close();

        Account const bogie {"bogie", KeyType::secp256k1};
        Account const demon {"demon", KeyType::ed25519};
        env (signers (alice, 2, {{bogie, 1}, {demon, 1}}), sig (alie));
        env.close();

//爱丽丝用她的普通钥匙开支票。
        env (check::create (alice, bob, USD(50)), sig (alie));
        env.close();
        BEAST_EXPECT (checksOnAccount (env, alice).size() == aliceCount + 6);
        BEAST_EXPECT (checksOnAccount (env, bob  ).size() == bobCount   + 6);

//Alice使用多重点火创建支票。
        std::uint64_t const baseFeeDrops {env.current()->fees().base};
        env (check::create (alice, bob, USD(50)),
            msig (bogie, demon), fee (3 * baseFeeDrops));
        env.close();
        BEAST_EXPECT (checksOnAccount (env, alice).size() == aliceCount + 7);
        BEAST_EXPECT (checksOnAccount (env, bob  ).size() == bobCount   + 7);
    }

    void testCreateInvalid()
    {
//探索创建支票的许多无效方法。
        testcase ("Create invalid");

        using namespace test::jtx;

        Account const gw1 {"gateway1"};
        Account const gwF {"gatewayFrozen"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        IOU const USD {gw1["USD"]};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        STAmount const startBalance {XRP(1000).value()};
        env.fund (startBalance, gw1, gwF, alice, bob);

//不良费用。
        env (check::create (alice, bob, USD(50)), fee (drops(-10)),
            ter (temBAD_FEE));
        env.close();

//坏旗
        env (check::create (alice, bob, USD(50)),
            txflags (tfImmediateOrCancel), ter (temINVALID_FLAG));
        env.close();

//检查自己。
        env (check::create (alice, alice, XRP(10)), ter (temREDUNDANT));
        env.close();

//数额不大。
        env (check::create (alice, bob, drops(-1)), ter (temBAD_AMOUNT));
        env.close();

        env (check::create (alice, bob, drops(0)), ter (temBAD_AMOUNT));
        env.close();

        env (check::create (alice, bob, drops(1)));
        env.close();

        env (check::create (alice, bob, USD(-1)), ter (temBAD_AMOUNT));
        env.close();

        env (check::create (alice, bob, USD(0)), ter (temBAD_AMOUNT));
        env.close();

        env (check::create (alice, bob, USD(1)));
        env.close();
        {
            IOU const BAD {gw1, badCurrency()};
            env (check::create (alice, bob, BAD(2)), ter (temBAD_CURRENCY));
            env.close();
        }

//过期不良。
        env (check::create (alice, bob, USD(50)),
            expiration (NetClock::time_point{}), ter (temBAD_EXPIRATION));
        env.close();

//目标不存在。
        Account const bogie {"bogie"};
        env (check::create (alice, bogie, USD(50)), ter (tecNO_DST));
        env.close();

//需要目标标记。
        env (fset (bob, asfRequireDest));
        env.close();

        env (check::create (alice, bob, USD(50)), ter (tecDST_TAG_NEEDED));
        env.close();

        env (check::create (alice, bob, USD(50)), dest_tag(11));
        env.close();

        env (fclear (bob, asfRequireDest));
        env.close();
        {
//全球冻结资产。
            IOU const USF {gwF["USF"]};
            env (fset(gwF, asfGlobalFreeze));
            env.close();

            env (check::create (alice, bob, USF(50)), ter (tecFROZEN));
            env.close();

            env (fclear(gwF, asfGlobalFreeze));
            env.close();

            env (check::create (alice, bob, USF(50)));
            env.close();
        }
        {
//冻结的信任线。支票创建应类似于付款
//面对冻结的信任线时的行为。
            env.trust (USD(1000), alice);
            env.trust (USD(1000), bob);
            env.close();
            env (pay (gw1, alice, USD(25)));
            env (pay (gw1, bob, USD(25)));
            env.close();

//在一个方向上设置信任线冻结可防止Alice
//创建美元支票。但是Bob和Gw1应该仍然能够
//为Alice创建美元支票。
            env (trust(gw1, alice["USD"](0), tfSetFreeze));
            env.close();
            env (check::create (alice, bob, USD(50)), ter (tecFROZEN));
            env.close();
            env (pay (alice, bob, USD(1)), ter (tecPATH_DRY));
            env.close();
            env (check::create (bob, alice, USD(50)));
            env.close();
            env (pay (bob, alice, USD(1)));
            env.close();
            env (check::create (gw1, alice, USD(50)));
            env.close();
            env (pay (gw1, alice, USD(1)));
            env.close();

//清除冻结物。现在检查创建工作。
            env (trust(gw1, alice["USD"](0), tfClearFreeze));
            env.close();
            env (check::create (alice, bob, USD(50)));
            env.close();
            env (check::create (bob, alice, USD(50)));
            env.close();
            env (check::create (gw1, alice, USD(50)));
            env.close();

//另一个方向的冻结不会影响Alice的美元
//检查创建，但阻止Bob和GW1写入检查
//美元兑换爱丽丝。
            env (trust(alice, USD(0), tfSetFreeze));
            env.close();
            env (check::create (alice, bob, USD(50)));
            env.close();
            env (pay (alice, bob, USD(1)));
            env.close();
            env (check::create (bob, alice, USD(50)), ter (tecFROZEN));
            env.close();
            env (pay (bob, alice, USD(1)), ter (tecPATH_DRY));
            env.close();
            env (check::create (gw1, alice, USD(50)), ter (tecFROZEN));
            env.close();
            env (pay (gw1, alice, USD(1)), ter (tecPATH_DRY));
            env.close();

//清除冻结物。
            env(trust(alice, USD(0), tfClearFreeze));
            env.close();
        }

//过期。
        env (check::create (alice, bob, USD(50)),
            expiration (env.now()), ter (tecEXPIRED));
        env.close();

        using namespace std::chrono_literals;
        env (check::create (alice, bob, USD(50)), expiration (env.now() + 1s));
        env.close();

//储备不足。
        Account const cheri {"cheri"};
        env.fund (
            env.current()->fees().accountReserve(1) - drops(1), cheri);

        env (check::create (cheri, bob, USD(50)),
            fee (drops (env.current()->fees().base)),
            ter (tecINSUFFICIENT_RESERVE));
        env.close();

        env (pay (bob, cheri, drops (env.current()->fees().base + 1)));
        env.close();

        env (check::create (cheri, bob, USD(50)));
        env.close();
    }

    void testCashXRP()
    {
//探索许多有效的方法来兑现一张XRP支票。
        testcase ("Cash XRP");

        using namespace test::jtx;

        Account const alice {"alice"};
        Account const bob {"bob"};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        std::uint64_t const baseFeeDrops {env.current()->fees().base};
        STAmount const startBalance {XRP(300).value()};
        env.fund (startBalance, alice, bob);
        {
//基本xrp检查。
            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)));
            env.close();
            env.require (balance (alice, startBalance - drops (baseFeeDrops)));
            env.require (balance (bob,   startBalance));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 0);

            env (check::cash (bob, chkId, XRP(10)));
            env.close();
            env.require (balance (alice,
                startBalance - XRP(10) - drops (baseFeeDrops)));
            env.require (balance (bob,
                startBalance + XRP(10) - drops (baseFeeDrops)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 0);
            BEAST_EXPECT (ownerCount (env, bob  ) == 0);

//使爱丽丝和鲍勃的天平易于思考。
            env (pay (env.master, alice, XRP(10) + drops (baseFeeDrops)));
            env (pay (bob, env.master,   XRP(10) - drops (baseFeeDrops * 2)));
            env.close();
            env.require (balance (alice, startBalance));
            env.require (balance (bob,   startBalance));
        }
        {
//写一张可以咀嚼爱丽丝储备的支票。
            STAmount const reserve {env.current()->fees().accountReserve (0)};
            STAmount const checkAmount {
                startBalance - reserve - drops (baseFeeDrops)};
            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, checkAmount));
            env.close();

//鲍勃试图兑现的金额超过支票金额。
            env (check::cash (bob, chkId, checkAmount + drops(1)),
                ter (tecPATH_PARTIAL));
            env.close();
            env (check::cash (
                bob, chkId, check::DeliverMin (checkAmount + drops(1))),
                ter (tecPATH_PARTIAL));
            env.close();

//鲍勃正好兑现支票金额。这是成功的
//因为爱丽丝储备的一个单位在
//支票已消费。
            env (check::cash (bob, chkId, check::DeliverMin (checkAmount)));
            verifyDeliveredAmount (env, drops(checkAmount.mantissa()));
            env.require (balance (alice, reserve));
            env.require (balance (bob,
                startBalance + checkAmount - drops (baseFeeDrops * 3)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 0);
            BEAST_EXPECT (ownerCount (env, bob  ) == 0);

//使爱丽丝和鲍勃的天平易于思考。
            env (pay (env.master, alice, checkAmount + drops (baseFeeDrops)));
            env (pay (bob, env.master, checkAmount - drops (baseFeeDrops * 4)));
            env.close();
            env.require (balance (alice, startBalance));
            env.require (balance (bob,   startBalance));
        }
        {
//写一张超过艾丽丝所能支付的一滴的支票。
            STAmount const reserve {env.current()->fees().accountReserve (0)};
            STAmount const checkAmount {
                startBalance - reserve - drops (baseFeeDrops - 1)};
            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, checkAmount));
            env.close();

//鲍勃试图兑现支票金额。失败是因为
//艾丽丝一点也不愿意为支票提供资金。
            env (check::cash (bob, chkId, checkAmount), ter (tecPATH_PARTIAL));
            env.close();

//鲍勃决定从这张空头支票中得到他能得到的。
            env (check::cash (bob, chkId, check::DeliverMin (drops(1))));
            verifyDeliveredAmount (env, drops(checkAmount.mantissa() - 1));
            env.require (balance (alice, reserve));
            env.require (balance (bob,
                startBalance + checkAmount - drops ((baseFeeDrops * 2) + 1)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 0);
            BEAST_EXPECT (ownerCount (env, bob  ) == 0);

//使爱丽丝和鲍勃的天平易于思考。
            env (pay (env.master, alice,
                checkAmount + drops (baseFeeDrops - 1)));
            env (pay (bob, env.master,
                checkAmount - drops ((baseFeeDrops * 3) + 1)));
            env.close();
            env.require (balance (alice, startBalance));
            env.require (balance (bob,   startBalance));
        }
    }

    void testCashIOU ()
    {
//探索为借据兑现支票的许多有效方法。
        testcase ("Cash IOU");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        IOU const USD {gw["USD"]};
        {
//简单的借据支票兑现金额（失败）。
            Env env {*this};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), gw, alice, bob);

//艾丽斯在拿到钱前写支票。
            uint256 const chkId1 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)));
            env.close();

//鲍勃试图兑现支票。应该失败。
            env (check::cash (bob, chkId1, USD(10)), ter (tecPATH_PARTIAL));
            env.close();

//艾丽斯几乎得到了足够的资金。鲍勃又试着失败了。
            env (trust (alice, USD(20)));
            env.close();
            env (pay (gw, alice, USD(9.5)));
            env.close();
            env (check::cash (bob, chkId1, USD(10)), ter (tecPATH_PARTIAL));
            env.close();

//爱丽丝得到了最后一笔必要的资金。鲍勃再试一次
//但失败了，因为他没有美元的信托额度。
            env (pay (gw, alice, USD(0.5)));
            env.close();
            env (check::cash (bob, chkId1, USD(10)), ter (tecNO_LINE));
            env.close();

//Bob设置了信任线，但没有达到足够高的限制。
            env (trust (bob, USD(9.5)));
            env.close();
            env (check::cash (bob, chkId1, USD(10)), ter (tecPATH_PARTIAL));
            env.close();

//Bob将信任额度设置得足够高，但要求更多
//比支票的sendmax还多。
            env (trust (bob, USD(10.5)));
            env.close();
            env (check::cash (bob, chkId1, USD(10.5)), ter (tecPATH_PARTIAL));
            env.close();

//鲍勃准确地询问支票金额，支票就结了。
            env (check::cash (bob, chkId1, USD(10)));
            env.close();
            env.require (balance (alice, USD( 0)));
            env.require (balance (bob,   USD(10)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

//鲍勃试图再次兑现同一张支票，但失败了。
            env (check::cash (bob, chkId1, USD(10)), ter (tecNO_ENTRY));
            env.close();

//鲍勃付给爱丽丝7美元，这样他就可以再审一个案子了。
            env (pay (bob, alice, USD(7)));
            env.close();

            uint256 const chkId2 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(7)));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);

//鲍勃兑现支票的金额低于票面金额。那是有效的，
//把支票消费掉，鲍勃得到的就和他要求的一样多。
            env (check::cash (bob, chkId2, USD(5)));
            env.close();
            env.require (balance (alice, USD(2)));
            env.require (balance (bob,   USD(8)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

//爱丽丝写了两张美元支票（2），尽管她只有美元（2）。
            uint256 const chkId3 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(2)));
            env.close();
            uint256 const chkId4 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(2)));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 2);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 2);

//鲍勃将第二张支票兑换成面值。
            env (check::cash (bob, chkId4, USD(2)));
            env.close();
            env.require (balance (alice, USD( 0)));
            env.require (balance (bob,   USD(10)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == 2);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

//Bob不得将最后一张支票兑换成美元（0），他必须
//请改用check：：cancel。
            env (check::cash (bob, chkId3, USD(0)), ter (temBAD_AMOUNT));
            env.close();
            env.require (balance (alice, USD( 0)));
            env.require (balance (bob,   USD(10)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == 2);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

//…所以鲍勃取消了爱丽丝剩下的支票。
            env (check::cancel (bob, chkId3));
            env.close();
            env.require (balance (alice, USD( 0)));
            env.require (balance (bob,   USD(10)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);
        }
        {
//使用delivermin兑现的简单IOU支票（失败）。
            Env env {*this};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), gw, alice, bob);

            env (trust (alice, USD(20)));
            env (trust (bob, USD(20)));
            env.close();
            env (pay (gw, alice, USD(8)));
            env.close();

//爱丽丝提前开了几张支票。
            uint256 const chkId9 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(9)));
            env.close();
            uint256 const chkId8 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(8)));
            env.close();
            uint256 const chkId7 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(7)));
            env.close();
            uint256 const chkId6 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(6)));
            env.close();

//鲍勃试图兑现支票上的金额。
//应该失败，因为爱丽丝没有资金。
            env (check::cash (bob, chkId9, check::DeliverMin (USD(9))),
                ter (tecPATH_PARTIAL));
            env.close();

//鲍勃设定了一个7的交货期，得到了爱丽丝所有的。
            env (check::cash (bob, chkId9, check::DeliverMin (USD(7))));
            verifyDeliveredAmount (env, USD(8));
            env.require (balance (alice, USD(0)));
            env.require (balance (bob,   USD(8)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 3);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 3);
            BEAST_EXPECT (ownerCount (env, alice) == 4);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

//鲍勃付给爱丽丝7美元，这样他就可以用另一张支票了。
            env (pay (bob, alice, USD(7)));
            env.close();

//将delivermin用于检查的sendmax值（而不是
//转移费）应该和设定金额一样有效。
            env (check::cash (bob, chkId7, check::DeliverMin (USD(7))));
            verifyDeliveredAmount (env, USD(7));
            env.require (balance (alice, USD(0)));
            env.require (balance (bob,   USD(8)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 2);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 2);
            BEAST_EXPECT (ownerCount (env, alice) == 3);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

//鲍勃付给爱丽丝8美元，这样他就可以用最后两张支票了。
            env (pay (bob, alice, USD(8)));
            env.close();

//爱丽丝有美元（8）。如果Bob使用美元支票（6）并使用
//delivermin为4，他应该得到支票的sendmax值。
            env (check::cash (bob, chkId6, check::DeliverMin (USD(4))));
            verifyDeliveredAmount (env, USD(6));
            env.require (balance (alice, USD(2)));
            env.require (balance (bob,   USD(6)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == 2);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

//Bob兑现最后一张支票，设置delivermin。
//正好是爱丽丝剩下的美元。
            env (check::cash (bob, chkId8, check::DeliverMin (USD(2))));
            verifyDeliveredAmount (env, USD(2));
            env.require (balance (alice, USD(0)));
            env.require (balance (bob,   USD(8)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);
        }
        {
//检查asfrequeuireauth标志的效果。
            Env env {*this};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), gw, alice, bob);
            env (fset (gw, asfRequireAuth));
            env.close();
            env (trust (gw, alice["USD"](100)), txflags (tfSetfAuth));
            env (trust (alice, USD(20)));
            env.close();
            env (pay (gw, alice, USD(8)));
            env.close();

//爱丽丝给鲍勃开了一张美元支票。鲍勃不能兑现
//因为他无权持有吉布达•伟士[“美元”]。
            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(7)));
            env.close();

            env (check::cash (bob, chkId, USD(7)), ter (tecNO_LINE));
            env.close();

//现在给Bob美元的信托额度。鲍勃仍然不能兑现
//检查，因为他没有被授权。
            env (trust (bob, USD(5)));
            env.close();

            env (check::cash (bob, chkId, USD(7)), ter (tecNO_AUTH));
            env.close();

//Bob获得持有GW[美元]的授权。
            env (trust (gw, bob["USD"](1)), txflags (tfSetfAuth));
            env.close();

//鲍勃试图再次兑现支票，但失败了，因为他的信任
//限制太低。
            env (check::cash (bob, chkId, USD(7)), ter (tecPATH_PARTIAL));
            env.close();

//既然鲍勃把限额定得很低，他就用
//交付并达到他的信任极限。
            env (check::cash (bob, chkId, check::DeliverMin (USD(4))));
            verifyDeliveredAmount (env, USD(5));
            env.require (balance (alice, USD(3)));
            env.require (balance (bob,   USD(5)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);
        }

//使用普通钥匙和多重签名兑现支票。
//FeatureMultiSign更改签名者列表上的保留，因此
//检查前后。
        FeatureBitset const allSupported {supported_amendments()};
        for (auto const features :
            {allSupported - featureMultiSignReserve,
                allSupported | featureMultiSignReserve})
        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), gw, alice, bob);

//爱丽丝提前开支票。
            uint256 const chkId1 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(1)));
            env.close();

            uint256 const chkId2 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(2)));
            env.close();

            env (trust (alice, USD(20)));
            env (trust (bob, USD(20)));
            env.close();
            env (pay (gw, alice, USD(8)));
            env.close();

//给鲍勃一把普通的钥匙和签名人
            Account const bobby {"bobby", KeyType::secp256k1};
            env (regkey (bob, bobby));
            env.close();

            Account const bogie {"bogie", KeyType::secp256k1};
            Account const demon {"demon", KeyType::ed25519};
            env (signers (bob, 2, {{bogie, 1}, {demon, 1}}), sig (bobby));
            env.close();

//如果启用了FeatureMultiSignReserve，则Bob的签名者列表
//拥有者计数为1，否则为4。
            int const signersCount {features[featureMultiSignReserve] ? 1 : 4};
            BEAST_EXPECT (ownerCount (env, bob) == signersCount + 1);

//鲍勃用他的普通钥匙兑现支票。
            env (check::cash (bob, chkId1, (USD(1))), sig (bobby));
            env.close();
            env.require (balance (alice, USD(7)));
            env.require (balance (bob,   USD(1)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == 2);
            BEAST_EXPECT (ownerCount (env, bob  ) == signersCount + 1);

//鲍勃用多重点火兑现支票。
            std::uint64_t const baseFeeDrops {env.current()->fees().base};
            env (check::cash (bob, chkId2, (USD(2))),
                msig (bogie, demon), fee (3 * baseFeeDrops));
            env.close();
            env.require (balance (alice, USD(5)));
            env.require (balance (bob,   USD(3)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == signersCount + 1);
        }
    }

    void testCashXferFee()
    {
//看看发行人收取转让费时的行为。
        testcase ("Cash with transfer fee");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        IOU const USD {gw["USD"]};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000), gw, alice, bob);

        env (trust (alice, USD(1000)));
        env (trust (bob, USD(1000)));
        env.close();
        env (pay (gw, alice, USD(1000)));
        env.close();

//设置GW的转账率，并查看兑现支票时的后果。
        env (rate (gw, 1.25));
        env.close();

//艾丽斯开了一张最高金额为125美元的支票。最牛
//可得到的是美元（100），因为转移率。
        uint256 const chkId125 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(125)));
        env.close();

//艾丽斯写了另一张支票，直到转账后才能兑现。
//利率变化，这样我们可以看到当支票
//兑现，而不是在创建时兑现。
        uint256 const chkId120 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(120)));
        env.close();

//鲍勃试图兑现这张支票的面值。应该失败。
        env (check::cash (bob, chkId125, USD(125)), ter (tecPATH_PARTIAL));
        env.close();
        env (check::cash (bob, chkId125, check::DeliverMin (USD(101))),
            ter (tecPATH_PARTIAL));
        env.close();

//鲍勃决定接受75美元或以上的任何东西。
//他得到100美元。
        env (check::cash (bob, chkId125, check::DeliverMin (USD(75))));
        verifyDeliveredAmount (env, USD(100));
        env.require (balance (alice, USD(1000 - 125)));
        env.require (balance (bob,   USD(   0 + 100)));
        BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
        BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);

//调整吉布达•伟士的费率…
        env (rate (gw, 1.2));
        env.close();

//鲍勃把第二张支票兑换成低于面值的现金。新的
//汇率适用于实际转移的价值。
        env (check::cash (bob, chkId120, USD(50)));
        env.close();
        env.require (balance (alice, USD(1000 - 125 - 60)));
        env.require (balance (bob,   USD(   0 + 100 + 50)));
        BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
        BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
    }

    void testCashQuality ()
    {
//查看八种可能的质量输入/输出情况。
        testcase ("Cash quality");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        IOU const USD {gw["USD"]};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000), gw, alice, bob);

        env (trust (alice, USD(1000)));
        env (trust (bob, USD(1000)));
        env.close();
        env (pay (gw, alice, USD(1000)));
        env.close();

//
//对两个非发行人之间转移的质量影响。
//

//提供返回QualityInPercent和QualityOutPercent的lambda。
        auto qIn =
            [] (double percent) { return qualityInPercent  (percent); };
        auto qOut =
            [] (double percent) { return qualityOutPercent (percent); };

//有两个测试羊羔：一个用于付款，一个用于支票。
//这显示付款和支票的行为是否相同。
        auto testNonIssuerQPay = [&env, &alice, &bob, &USD]
        (Account const& truster,
            IOU const& iou, auto const& inOrOut, double pct, double amount)
        {
//捕获Bob和Alice的余额，以便在结尾进行测试。
            STAmount const aliceStart {env.balance (alice, USD.issue()).value()};
            STAmount const bobStart   {env.balance (bob,   USD.issue()).value()};

//设置修改后的质量。
            env (trust (truster, iou(1000)), inOrOut (pct));
            env.close();

            env (pay (alice, bob, USD(amount)), sendmax (USD(10)));
            env.close();
            env.require (balance (alice, aliceStart - USD(10)));
            env.require (balance (bob,   bobStart   + USD(10)));

//将质量返回到未修改的状态，这样它就不会
//干扰即将进行的测试。
            env (trust (truster, iou(1000)), inOrOut (0));
            env.close();
        };

        auto testNonIssuerQCheck = [&env, &alice, &bob, &USD]
        (Account const& truster,
            IOU const& iou, auto const& inOrOut, double pct, double amount)
        {
//捕获Bob和Alice的余额，以便在结尾进行测试。
            STAmount const aliceStart {env.balance (alice, USD.issue()).value()};
            STAmount const bobStart   {env.balance (bob,   USD.issue()).value()};

//设置修改后的质量。
            env (trust (truster, iou(1000)), inOrOut (pct));
            env.close();

            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)));
            env.close();

            env (check::cash (bob, chkId, USD(amount)));
            env.close();
            env.require (balance (alice, aliceStart - USD(10)));
            env.require (balance (bob,   bobStart   + USD(10)));

//将质量返回到未修改的状态，这样它就不会
//干扰即将进行的测试。
            env (trust (truster, iou(1000)), inOrOut (0));
            env.close();
        };

//PCT量
        testNonIssuerQPay   (alice, gw["USD"],  qIn,  50,     10);
        testNonIssuerQCheck (alice, gw["USD"],  qIn,  50,     10);

//这是质量影响结果的唯一情况。
        testNonIssuerQPay   (bob,   gw["USD"],  qIn,  50,      5);
        testNonIssuerQCheck (bob,   gw["USD"],  qIn,  50,      5);

        testNonIssuerQPay   (gw, alice["USD"],  qIn,  50,     10);
        testNonIssuerQCheck (gw, alice["USD"],  qIn,  50,     10);

        testNonIssuerQPay   (gw,   bob["USD"],  qIn,  50,     10);
        testNonIssuerQCheck (gw,   bob["USD"],  qIn,  50,     10);

        testNonIssuerQPay   (alice, gw["USD"], qOut, 200,     10);
        testNonIssuerQCheck (alice, gw["USD"], qOut, 200,     10);

        testNonIssuerQPay   (bob,   gw["USD"], qOut, 200,     10);
        testNonIssuerQCheck (bob,   gw["USD"], qOut, 200,     10);

        testNonIssuerQPay   (gw, alice["USD"], qOut, 200,     10);
        testNonIssuerQCheck (gw, alice["USD"], qOut, 200,     10);

        testNonIssuerQPay   (gw,   bob["USD"], qOut, 200,     10);
        testNonIssuerQCheck (gw,   bob["USD"], qOut, 200,     10);

//
//发行人与非发行人之间转让的质量影响。
//

//有两个测试羔羊，原因与之前相同。
        auto testIssuerQPay = [&env, &gw, &alice, &USD]
        (Account const& truster, IOU const& iou,
            auto const& inOrOut, double pct,
            double amt1, double max1, double amt2, double max2)
        {
//捕捉爱丽丝的平衡，这样我们可以在最后测试。它不
//看看网关的平衡是什么意思。
            STAmount const aliceStart {env.balance (alice, USD.issue()).value()};

//设置修改后的质量。
            env (trust (truster, iou(1000)), inOrOut (pct));
            env.close();

//爱丽丝支付GW。
            env (pay (alice, gw, USD(amt1)), sendmax (USD(max1)));
            env.close();
            env.require (balance (alice, aliceStart - USD(10)));

//GW付钱给爱丽丝。
            env (pay (gw, alice, USD(amt2)), sendmax (USD(max2)));
            env.close();
            env.require (balance (alice, aliceStart));

//将质量返回到未修改的状态，这样它就不会
//干扰即将进行的测试。
            env (trust (truster, iou(1000)), inOrOut (0));
            env.close();
        };

        auto testIssuerQCheck = [&env, &gw, &alice, &USD]
        (Account const& truster, IOU const& iou,
            auto const& inOrOut, double pct,
            double amt1, double max1, double amt2, double max2)
        {
//捕捉爱丽丝的平衡，这样我们可以在最后测试。它不
//看看发行人的余额有什么意义。
            STAmount const aliceStart {env.balance (alice, USD.issue()).value()};

//设置修改后的质量。
            env (trust (truster, iou(1000)), inOrOut (pct));
            env.close();

//爱丽丝给吉布达•伟士写支票。GW兑现。
            uint256 const chkAliceId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, gw, USD(max1)));
            env.close();

            env (check::cash (gw, chkAliceId, USD(amt1)));
            env.close();
            env.require (balance (alice, aliceStart - USD(10)));

//吉布达•伟士给爱丽丝写支票。爱丽丝兑现。
            uint256 const chkGwId {getCheckIndex (gw, env.seq (gw))};
            env (check::create (gw, alice, USD(max2)));
            env.close();

            env (check::cash (alice, chkGwId, USD(amt2)));
            env.close();
            env.require (balance (alice, aliceStart));

//将质量返回到未修改的状态，这样它就不会
//干扰即将进行的测试。
            env (trust (truster, iou(1000)), inOrOut (0));
            env.close();
        };

//第一种情况是质量影响结果的唯一情况。
//PCT最大值1最大值2最大值2
        testIssuerQPay   (alice, gw["USD"],  qIn,  50,   10,  10,   5,  10);
        testIssuerQCheck (alice, gw["USD"],  qIn,  50,   10,  10,   5,  10);

        testIssuerQPay   (gw, alice["USD"],  qIn,  50,   10,  10,  10,  10);
        testIssuerQCheck (gw, alice["USD"],  qIn,  50,   10,  10,  10,  10);

        testIssuerQPay   (alice, gw["USD"], qOut, 200,   10,  10,  10,  10);
        testIssuerQCheck (alice, gw["USD"], qOut, 200,   10,  10,  10,  10);

        testIssuerQPay   (gw, alice["USD"], qOut, 200,   10,  10,  10,  10);
        testIssuerQCheck (gw, alice["USD"], qOut, 200,   10,  10,  10,  10);
    }

    void testCashInvalid()
    {
//探索许多兑现支票失败的方法。
        testcase ("Cash invalid");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const zoe {"zoe"};
        IOU const USD {gw["USD"]};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000), gw, alice, bob, zoe);

//现在建立爱丽丝的信任线。
        env (trust (alice, USD(20)));
        env.close();
        env (pay (gw, alice, USD(20)));
        env.close();

//在鲍勃得到一个信托额度之前，让他兑现一张支票。
//应该失败。
        {
            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(20)));
            env.close();

            env (check::cash (bob, chkId, USD(20)), ter (tecNO_LINE));
            env.close();
        }

//现在建立鲍勃的信任线。
        env (trust (bob, USD(20)));
        env.close();

//鲍勃试图从爱丽丝那里兑现一张不存在的支票。
        {
            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::cash (bob, chkId, USD(20)), ter (tecNO_ENTRY));
            env.close();
        }

//爱丽丝提前开支票。
        uint256 const chkIdU {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(20)));
        env.close();

        uint256 const chkIdX {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, XRP(10)));
        env.close();

        using namespace std::chrono_literals;
        uint256 const chkIdExp {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, XRP(10)), expiration (env.now() + 1s));
        env.close();

        uint256 const chkIdFroz1 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(1)));
        env.close();

        uint256 const chkIdFroz2 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(2)));
        env.close();

        uint256 const chkIdFroz3 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(3)));
        env.close();

        uint256 const chkIdFroz4 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(4)));
        env.close();

        uint256 const chkIdNoDest1 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(1)));
        env.close();

        uint256 const chkIdHasDest2 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(2)), dest_tag (7));
        env.close();

//IOU和XRP支票兑现的相同失败案例集。
        auto failingCases = [&env, &gw, &alice, &bob] (
            uint256 const& chkId, STAmount const& amount)
        {
//不良费用。
            env (check::cash (bob, chkId, amount), fee (drops(-10)),
                ter (temBAD_FEE));
            env.close();

//坏旗
            env (check::cash (bob, chkId, amount),
                txflags (tfImmediateOrCancel), ter (temINVALID_FLAG));
            env.close();

//同时缺少金额和交货时间。
            {
                Json::Value tx {check::cash (bob, chkId, amount)};
                tx.removeMember (sfAmount.jsonName);
                env (tx, ter (temMALFORMED));
                env.close();
            }
//金额和交货最小值都存在。
            {
                Json::Value tx {check::cash (bob, chkId, amount)};
                tx[sfDeliverMin.jsonName]  = amount.getJson(0);
                env (tx, ter (temMALFORMED));
                env.close();
            }

//负或零金额。
            {
                STAmount neg {amount};
                neg.negate();
                env (check::cash (bob, chkId, neg), ter (temBAD_AMOUNT));
                env.close();
                env (check::cash (bob, chkId, amount.zeroed()),
                    ter (temBAD_AMOUNT));
                env.close();
            }

//不良货币
            if (! amount.native()) {
                Issue const badIssue {badCurrency(), amount.getIssuer()};
                STAmount badAmount {amount};
                badAmount.setIssue (Issue {badCurrency(), amount.getIssuer()});
                env (check::cash (bob, chkId, badAmount),
                    ter (temBAD_CURRENCY));
                env.close();
            }

//不是目的地兑现支票。
            env (check::cash (alice, chkId, amount), ter (tecNO_PERMISSION));
            env.close();
            env (check::cash (gw, chkId, amount), ter (tecNO_PERMISSION));
            env.close();

//货币不匹配。
            {
                IOU const wrongCurrency {gw["EUR"]};
                STAmount badAmount {amount};
                badAmount.setIssue (wrongCurrency.issue());
                env (check::cash (bob, chkId, badAmount),
                    ter (temMALFORMED));
                env.close();
            }

//颁发者不匹配。
            {
                IOU const wrongIssuer {alice["USD"]};
                STAmount badAmount {amount};
                badAmount.setIssue (wrongIssuer.issue());
                env (check::cash (bob, chkId, badAmount),
                    ter (temMALFORMED));
                env.close();
            }

//金额大于sendmax。
            env (check::cash (bob, chkId, amount + amount),
                ter (tecPATH_PARTIAL));
            env.close();

//delivermin大于sendmax。
            env (check::cash (bob, chkId, check::DeliverMin (amount + amount)),
                ter (tecPATH_PARTIAL));
            env.close();
        };

        failingCases (chkIdX, XRP(10));
        failingCases (chkIdU, USD(20));

//确认这两张支票确实可以兑现。
        env (check::cash (bob, chkIdU, USD(20)));
        env.close();
        env (check::cash (bob, chkIdX, check::DeliverMin (XRP(10))));
        verifyDeliveredAmount (env, XRP(10));

//尝试兑现过期支票。
        env (check::cash (bob, chkIdExp, XRP(10)), ter (tecEXPIRED));
        env.close();

//取消过期支票。任何人都可以取消过期支票。
        env (check::cancel (zoe, chkIdExp));
        env.close();

//我们能用冻结的货币兑现支票吗？
        {
            env (pay (bob, alice, USD(20)));
            env.close();
            env.require (balance (alice, USD(20)));
            env.require (balance (bob,   USD( 0)));

//全球冻结
            env (fset (gw, asfGlobalFreeze));
            env.close();

            env (check::cash (bob, chkIdFroz1, USD(1)), ter (tecPATH_PARTIAL));
            env.close();
            env (check::cash (bob, chkIdFroz1, check::DeliverMin (USD(0.5))),
                ter (tecPATH_PARTIAL));
            env.close();

            env (fclear (gw, asfGlobalFreeze));
            env.close();

//不再冻结。成功。
            env (check::cash (bob, chkIdFroz1, USD(1)));
            env.close();
            env.require (balance (alice, USD(19)));
            env.require (balance (bob,   USD( 1)));

//冻结个别信托额度。
            env (trust(gw, alice["USD"](0), tfSetFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz2, USD(2)), ter (tecPATH_PARTIAL));
            env.close();
            env (check::cash (bob, chkIdFroz2, check::DeliverMin (USD(1))),
                ter (tecPATH_PARTIAL));
            env.close();

//清除冻结物。现在检查兑现工作。
            env (trust(gw, alice["USD"](0), tfClearFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz2, USD(2)));
            env.close();
            env.require (balance (alice, USD(17)));
            env.require (balance (bob,   USD( 3)));

//冻结鲍勃的信任线。鲍勃不能兑现支票。
            env (trust(gw, bob["USD"](0), tfSetFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz3, USD(3)), ter (tecFROZEN));
            env.close();
            env (check::cash (bob, chkIdFroz3, check::DeliverMin (USD(1))),
                ter (tecFROZEN));
            env.close();

//清除冻结物。现在再次检查兑现工作。
            env (trust(gw, bob["USD"](0), tfClearFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz3, check::DeliverMin (USD(1))));
            verifyDeliveredAmount (env, USD(3));
            env.require (balance (alice, USD(14)));
            env.require (balance (bob,   USD( 6)));

//把鲍勃的冻伤钻头转到另一个方向。检查
//兑现失败。
            env (trust (bob, USD(20), tfSetFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz4, USD(4)), ter (terNO_LINE));
            env.close();
            env (check::cash (bob, chkIdFroz4, check::DeliverMin (USD(1))),
                ter (terNO_LINE));
            env.close();

//清除鲍勃的冻结位，支票应该可以兑现。
            env (trust (bob, USD(20), tfClearFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz4, USD(4)));
            env.close();
            env.require (balance (alice, USD(10)));
            env.require (balance (bob,   USD(10)));
        }
        {
//在Bob的帐户上设置RequiredEst标志（在检查之后
//然后兑现一张没有目的地标签的支票。
            env (fset (bob, asfRequireDest));
            env.close();
            env (check::cash (bob, chkIdNoDest1, USD(1)),
                ter (tecDST_TAG_NEEDED));
            env.close();
            env (check::cash (bob, chkIdNoDest1, check::DeliverMin (USD(0.5))),
                ter (tecDST_TAG_NEEDED));
            env.close();

//鲍勃可以用目的地标签兑现支票。
            env (check::cash (bob, chkIdHasDest2, USD(2)));
            env.close();
            env.require (balance (alice, USD( 8)));
            env.require (balance (bob,   USD(12)));

//清除Bob帐户上的RequiredEst标志，以便
//兑现支票，不带目的地标签。
            env (fclear (bob, asfRequireDest));
            env.close();
            env (check::cash (bob, chkIdNoDest1, USD(1)));
            env.close();
            env.require (balance (alice, USD( 7)));
            env.require (balance (bob,   USD(13)));
        }
    }

    void testCancelValid()
    {
//探索许多取消支票的方法。
        testcase ("Cancel valid");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const zoe {"zoe"};
        IOU const USD {gw["USD"]};

//FeatureMultiSign更改签名者列表上的保留，因此
//检查前后。
        FeatureBitset const allSupported {supported_amendments()};
        for (auto const features :
            {allSupported - featureMultiSignReserve,
                allSupported | featureMultiSignReserve})
        {
            Env env {*this, features};

            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), gw, alice, bob, zoe);

//爱丽丝提前开支票。
//三张没有过期的普通支票。
            uint256 const chkId1 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)));
            env.close();

            uint256 const chkId2 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)));
            env.close();

            uint256 const chkId3 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)));
            env.close();

//三张10分钟后到期的支票。
            using namespace std::chrono_literals;
            uint256 const chkIdNotExp1 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)), expiration (env.now()+600s));
            env.close();

            uint256 const chkIdNotExp2 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)), expiration (env.now()+600s));
            env.close();

            uint256 const chkIdNotExp3 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)), expiration (env.now()+600s));
            env.close();

//一秒钟内到期的三张支票。
            uint256 const chkIdExp1 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)), expiration (env.now()+1s));
            env.close();

            uint256 const chkIdExp2 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)), expiration (env.now()+1s));
            env.close();

            uint256 const chkIdExp3 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)), expiration (env.now()+1s));
            env.close();

//使用普通钥匙和多重点火取消两项检查。
            uint256 const chkIdReg {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)));
            env.close();

            uint256 const chkIdMSig {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 11);
            BEAST_EXPECT (ownerCount (env, alice) == 11);

//创建者、目的地和外部人员取消检查。
            env (check::cancel (alice, chkId1));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 10);
            BEAST_EXPECT (ownerCount (env, alice) == 10);

            env (check::cancel (bob, chkId2));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 9);
            BEAST_EXPECT (ownerCount (env, alice) == 9);

            env (check::cancel (zoe, chkId3), ter (tecNO_PERMISSION));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 9);
            BEAST_EXPECT (ownerCount (env, alice) == 9);

//创建者、目的地和外部人员取消未过期的支票。
            env (check::cancel (alice, chkIdNotExp1));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 8);
            BEAST_EXPECT (ownerCount (env, alice) == 8);

            env (check::cancel (bob, chkIdNotExp2));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 7);
            BEAST_EXPECT (ownerCount (env, alice) == 7);

            env (check::cancel (zoe, chkIdNotExp3), ter (tecNO_PERMISSION));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 7);
            BEAST_EXPECT (ownerCount (env, alice) == 7);

//创建者、目的地和局外人取消过期的支票。
            env (check::cancel (alice, chkIdExp1));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 6);
            BEAST_EXPECT (ownerCount (env, alice) == 6);

            env (check::cancel (bob, chkIdExp2));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 5);
            BEAST_EXPECT (ownerCount (env, alice) == 5);

            env (check::cancel (zoe, chkIdExp3));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 4);
            BEAST_EXPECT (ownerCount (env, alice) == 4);

//使用普通钥匙和多重签名取消支票。
            Account const alie {"alie", KeyType::ed25519};
            env (regkey (alice, alie));
            env.close();

            Account const bogie {"bogie", KeyType::secp256k1};
            Account const demon {"demon", KeyType::ed25519};
            env (signers (alice, 2, {{bogie, 1}, {demon, 1}}), sig (alie));
            env.close();

//如果启用了FeatureMultiSignReserve，那么Alices的签名者列表
//拥有者计数为1，否则为4。
            int const signersCount {features[featureMultiSignReserve] ? 1 : 4};

//爱丽丝用普通钥匙取消支票。
            env (check::cancel (alice, chkIdReg), sig (alie));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 3);
            BEAST_EXPECT (ownerCount (env, alice) == signersCount + 3);

//Alice使用多重点火取消支票。
            std::uint64_t const baseFeeDrops {env.current()->fees().base};
            env (check::cancel (alice, chkIdMSig),
                msig (bogie, demon), fee (3 * baseFeeDrops));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 2);
            BEAST_EXPECT (ownerCount (env, alice) == signersCount + 2);

//创建者和目的地取消剩余的未过期支票。
            env (check::cancel (alice, chkId3), sig (alice));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == signersCount + 1);

            env (check::cancel (bob, chkIdNotExp3));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == signersCount + 0);
        }
    }

    void testCancelInvalid()
    {
//探索许多取消支票失败的方法。
        testcase ("Cancel invalid");

        using namespace test::jtx;

        Account const alice {"alice"};
        Account const bob {"bob"};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000), alice, bob);

//不良费用。
        env (check::cancel (bob, getCheckIndex (alice, env.seq (alice))),
            fee (drops(-10)), ter (temBAD_FEE));
        env.close();

//坏旗
        env (check::cancel (bob, getCheckIndex (alice, env.seq (alice))),
            txflags (tfImmediateOrCancel), ter (temINVALID_FLAG));
        env.close();

//不存在的检查。
        env (check::cancel (bob, getCheckIndex (alice, env.seq (alice))),
            ter (tecNO_ENTRY));
        env.close();
    }

    void testFix1623Enable ()
    {
        testcase ("Fix1623 enable");

        using namespace test::jtx;

        auto testEnable = [this] (FeatureBitset const& features, bool hasFields)
        {
//除非启用了fix1623，“tx”rpc命令应返回
//支票上既没有“已交付金额”也没有“已交付金额”
//交易。
            Account const alice {"alice"};
            Account const bob {"bob"};

            Env env {*this, features};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), alice, bob);
            env.close();

            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(200)));
            env.close();

            env (check::cash (bob, chkId, check::DeliverMin (XRP(100))));

//获取最近事务的哈希。
            std::string const txHash {
                env.tx()->getJson (0)[jss::hash].asString()};

//deliveredAmount和delivered_金额存在或
//不存在于基于fix1623的“tx”返回的元数据中。
            env.close();
            Json::Value const meta =
                env.rpc ("tx", txHash)[jss::result][jss::meta];

            BEAST_EXPECT(meta.isMember (
                sfDeliveredAmount.jsonName) == hasFields);
            BEAST_EXPECT(meta.isMember (jss::delivered_amount) == hasFields);
        };

//同时运行禁用和启用的案例。
        testEnable (supported_amendments() - fix1623, false);
        testEnable (supported_amendments(),           true);
    }

public:
    void run () override
    {
        testEnabled();
        testCreateValid();
        testCreateInvalid();
        testCashXRP();
        testCashIOU();
        testCashXferFee();
        testCashQuality();
        testCashInvalid();
        testCancelValid();
        testCancelInvalid();
        testFix1623Enable();
    }
};

BEAST_DEFINE_TESTSUITE (Check, tx, ripple);

}  //涟漪

