
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
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/Feature.h>

namespace ripple {

class Ticket_test : public beast::unit_test::suite
{
    static auto constexpr idOne =
      "00000000000000000000000000000000"
      "00000000000000000000000000000001";

///@brief验证创建/取消票据交易的元数据，以及
///返回组成元数据的3或4个节点（受影响的节点）
///
///@param env当前jtx env（将从中提取meta）
///
///@param other_target标志指示目标是否不同
///From帐户是为票据指定的（创建时）
///
///@param expiration标志，表示取消，过期
///导致两个受影响的节点（按顺序）交换。
///
///@retval std:：json对象值的数组大小4表示
///每个元节点条目。当交易是一个不同的取消
///target和account，将有4个完整项，否则最后一个
///entry将是空对象
    auto
    checkTicketMeta(
        test::jtx::Env& env,
        bool other_target = false,
        bool expiration = false)
    {
        using namespace std::string_literals;
        auto const& tx = env.tx ()->getJson (0);
        bool is_cancel = tx[jss::TransactionType] == "TicketCancel";

        auto const& jvm = env.meta ()->getJson (0);
        std::array<Json::Value, 4> retval;

//这些是我们期望的受影响的节点
//几个不同的场景。
//tuple是索引、字段名和标签（ledgerentrytype）
        std::vector<
            std::tuple<std::size_t, std::string, std::string>
        > expected_nodes;

        if (is_cancel && other_target)
        {
            expected_nodes = {
                std::make_tuple(0, sfModifiedNode.fieldName, "AccountRoot"s),
                std::make_tuple(
                    expiration ? 2: 1, sfModifiedNode.fieldName, "AccountRoot"s),
                std::make_tuple(
                    expiration ? 1: 2, sfDeletedNode.fieldName, "Ticket"s),
                std::make_tuple(3, sfDeletedNode.fieldName, "DirectoryNode"s)
            };
        }
        else
        {
            expected_nodes = {
                std::make_tuple(0, sfModifiedNode.fieldName, "AccountRoot"s),
                std::make_tuple(1,
                    is_cancel ?
                        sfDeletedNode.fieldName : sfCreatedNode.fieldName,
                    "Ticket"s),
                std::make_tuple(2,
                    is_cancel ?
                        sfDeletedNode.fieldName : sfCreatedNode.fieldName,
                 "DirectoryNode"s)
            };
        }

        BEAST_EXPECT(jvm.isMember (sfAffectedNodes.fieldName));
        BEAST_EXPECT(jvm[sfAffectedNodes.fieldName].isArray());
        BEAST_EXPECT(
            jvm[sfAffectedNodes.fieldName].size() == expected_nodes.size());

//根据预期验证实际元数据
        for (auto const& it : expected_nodes)
        {
            auto const& idx = std::get<0>(it);
            auto const& field = std::get<1>(it);
            auto const& type = std::get<2>(it);
            BEAST_EXPECT(jvm[sfAffectedNodes.fieldName][idx].isMember(field));
            retval[idx] = jvm[sfAffectedNodes.fieldName][idx][field];
            BEAST_EXPECT(retval[idx][sfLedgerEntryType.fieldName] == type);
        }

        return retval;
    }

    void testTicketNotEnabled ()
    {
        testcase ("Feature Not Enabled");

        using namespace test::jtx;
        Env env {*this, FeatureBitset{}};

        env (ticket::create (env.master), ter(temDISABLED));
        env (ticket::cancel (env.master, idOne), ter (temDISABLED));
    }

    void testTicketCancelNonexistent ()
    {
        testcase ("Cancel Nonexistent");

        using namespace test::jtx;
        Env env {*this, supported_amendments().set(featureTickets)};
        env (ticket::cancel (env.master, idOne), ter (tecNO_ENTRY));
    }

    void testTicketCreatePreflightFail ()
    {
        testcase ("Create/Cancel Ticket with Bad Fee, Fail Preflight");

        using namespace test::jtx;
        Env env {*this, supported_amendments().set(featureTickets)};

        env (ticket::create (env.master), fee (XRP (-1)), ter (temBAD_FEE));
        env (ticket::cancel (env.master, idOne), fee (XRP (-1)), ter (temBAD_FEE));
    }

    void testTicketCreateNonexistent ()
    {
        testcase ("Create Tickets with Nonexistent Accounts");

        using namespace test::jtx;
        Env env {*this, supported_amendments().set(featureTickets)};
        Account alice {"alice"};
        env.memoize (alice);

        env (ticket::create (env.master, alice), ter(tecNO_TARGET));

        env (ticket::create (alice, env.master),
            json (jss::Sequence, 1),
            ter (terNO_ACCOUNT));
    }

    void testTicketToSelf ()
    {
        testcase ("Create Tickets with Same Account and Target");

        using namespace test::jtx;
        Env env {*this, supported_amendments().set(featureTickets)};

        env (ticket::create (env.master, env.master));
        auto cr = checkTicketMeta (env);
        auto const& jticket = cr[1];

        BEAST_EXPECT(jticket[sfLedgerIndex.fieldName] ==
            "7F58A0AE17775BA3404D55D406DD1C2E91EADD7AF3F03A26877BCE764CCB75E3");
        BEAST_EXPECT(jticket[sfNewFields.fieldName][jss::Account] ==
            env.master.human());
        BEAST_EXPECT(jticket[sfNewFields.fieldName][jss::Sequence] == 1);
//确认票据中没有保存“target”
        BEAST_EXPECT(! jticket[sfNewFields.fieldName].
            isMember(sfTarget.fieldName));
    }

    void testTicketCancelByCreator ()
    {
        testcase ("Create Ticket and Then Cancel by Creator");

        using namespace test::jtx;
        Env env {*this, supported_amendments().set(featureTickets)};

//创建并验证
        env (ticket::create (env.master));
        auto cr = checkTicketMeta (env);
        auto const& jacct = cr[0];
        auto const& jticket = cr[1];
        BEAST_EXPECT(
            jacct[sfPreviousFields.fieldName][sfOwnerCount.fieldName] == 0);
        BEAST_EXPECT(
            jacct[sfFinalFields.fieldName][sfOwnerCount.fieldName] == 1);
        BEAST_EXPECT(jticket[sfNewFields.fieldName][jss::Sequence] ==
            jacct[sfPreviousFields.fieldName][jss::Sequence]);
        BEAST_EXPECT(jticket[sfLedgerIndex.fieldName] ==
            "7F58A0AE17775BA3404D55D406DD1C2E91EADD7AF3F03A26877BCE764CCB75E3");
        BEAST_EXPECT(jticket[sfNewFields.fieldName][jss::Account] ==
            env.master.human());

//取消
        env (ticket::cancel(env.master, jticket[sfLedgerIndex.fieldName].asString()));
        auto crd = checkTicketMeta (env);
        auto const& jacctd = crd[0];
        BEAST_EXPECT(jacctd[sfFinalFields.fieldName][jss::Sequence] == 3);
        BEAST_EXPECT(
            jacctd[sfFinalFields.fieldName][sfOwnerCount.fieldName] == 0);
    }

    void testTicketInsufficientReserve ()
    {
        testcase ("Create Ticket Insufficient Reserve");

        using namespace test::jtx;
        Env env {*this, supported_amendments().set(featureTickets)};
        Account alice {"alice"};

        env.fund (env.current ()->fees ().accountReserve (0), alice);
        env.close ();

        env (ticket::create (alice), ter (tecINSUFFICIENT_RESERVE));
    }

    void testTicketCancelByTarget ()
    {
        testcase ("Create Ticket and Then Cancel by Target");

        using namespace test::jtx;
        Env env {*this, supported_amendments().set(featureTickets)};
        Account alice {"alice"};

        env.fund (XRP (10000), alice);
        env.close ();

//创建并验证
        env (ticket::create (env.master, alice));
        auto cr = checkTicketMeta (env, true);
        auto const& jacct = cr[0];
        auto const& jticket = cr[1];
        BEAST_EXPECT(
            jacct[sfFinalFields.fieldName][sfOwnerCount.fieldName] == 1);
        BEAST_EXPECT(jticket[sfLedgerEntryType.fieldName] == "Ticket");
        BEAST_EXPECT(jticket[sfLedgerIndex.fieldName] ==
            "C231BA31A0E13A4D524A75F990CE0D6890B800FF1AE75E51A2D33559547AC1A2");
        BEAST_EXPECT(jticket[sfNewFields.fieldName][jss::Account] ==
            env.master.human());
        BEAST_EXPECT(jticket[sfNewFields.fieldName][sfTarget.fieldName] ==
            alice.human());
        BEAST_EXPECT(jticket[sfNewFields.fieldName][jss::Sequence] == 2);

//使用目标帐户取消
        env (ticket::cancel(alice, jticket[sfLedgerIndex.fieldName].asString()));
        auto crd = checkTicketMeta (env, true);
        auto const& jacctd = crd[0];
        auto const& jdir = crd[2];
        BEAST_EXPECT(
            jacctd[sfFinalFields.fieldName][sfOwnerCount.fieldName] == 0);
        BEAST_EXPECT(jdir[sfLedgerIndex.fieldName] ==
            jticket[sfLedgerIndex.fieldName]);
        BEAST_EXPECT(jdir[sfFinalFields.fieldName][jss::Account] ==
            env.master.human());
        BEAST_EXPECT(jdir[sfFinalFields.fieldName][sfTarget.fieldName] ==
            alice.human());
        BEAST_EXPECT(jdir[sfFinalFields.fieldName][jss::Flags] == 0);
        BEAST_EXPECT(jdir[sfFinalFields.fieldName][sfOwnerNode.fieldName] ==
            "0000000000000000");
        BEAST_EXPECT(jdir[sfFinalFields.fieldName][jss::Sequence] == 2);
    }

    void testTicketWithExpiration ()
    {
        testcase ("Create Ticket with Future Expiration");

        using namespace test::jtx;
        Env env {*this, supported_amendments().set(featureTickets)};

//创建并验证
        using namespace std::chrono_literals;
        uint32_t expire =
            (env.timeKeeper ().closeTime () + 60s)
            .time_since_epoch ().count ();
        env (ticket::create (env.master, expire));
        auto cr = checkTicketMeta (env);
        auto const& jacct = cr[0];
        auto const& jticket = cr[1];
        BEAST_EXPECT(
            jacct[sfPreviousFields.fieldName][sfOwnerCount.fieldName] == 0);
        BEAST_EXPECT(
            jacct[sfFinalFields.fieldName][sfOwnerCount.fieldName] == 1);
        BEAST_EXPECT(jticket[sfNewFields.fieldName][jss::Sequence] ==
            jacct[sfPreviousFields.fieldName][jss::Sequence]);
        BEAST_EXPECT(
            jticket[sfNewFields.fieldName][sfExpiration.fieldName] == expire);
    }

    void testTicketZeroExpiration ()
    {
        testcase ("Create Ticket with Zero Expiration");

        using namespace test::jtx;
        Env env {*this, supported_amendments().set(featureTickets)};

//创建并验证
        env (ticket::create (env.master, 0u), ter (temBAD_EXPIRATION));
    }

    void testTicketWithPastExpiration ()
    {
        testcase ("Create Ticket with Past Expiration");

        using namespace test::jtx;
        Env env {*this, supported_amendments().set(featureTickets)};

        env.timeKeeper ().adjustCloseTime (days {2});
        env.close ();

//创建并验证
        uint32_t expire = 60;
        env (ticket::create (env.master, expire));
//如果过期，我们只能
//返回一个元节点条目
        auto const& jvm = env.meta ()->getJson (0);
        BEAST_EXPECT(jvm.isMember(sfAffectedNodes.fieldName));
        BEAST_EXPECT(jvm[sfAffectedNodes.fieldName].isArray());
        BEAST_EXPECT(jvm[sfAffectedNodes.fieldName].size() == 1);
        BEAST_EXPECT(jvm[sfAffectedNodes.fieldName][0u].
            isMember(sfModifiedNode.fieldName));
        auto const& jacct =
            jvm[sfAffectedNodes.fieldName][0u][sfModifiedNode.fieldName];
        BEAST_EXPECT(
            jacct[sfLedgerEntryType.fieldName] == "AccountRoot");
        BEAST_EXPECT(jacct[sfFinalFields.fieldName][jss::Account] ==
            env.master.human());
    }

    void testTicketAllowExpiration ()
    {
        testcase ("Create Ticket and Allow to Expire");

        using namespace test::jtx;
        Env env {*this, supported_amendments().set(featureTickets)};

//创建并验证
        uint32_t expire =
            (env.timeKeeper ().closeTime () + std::chrono::hours {3})
            .time_since_epoch().count();
        env (ticket::create (env.master, expire));
        auto cr = checkTicketMeta (env);
        auto const& jacct = cr[0];
        auto const& jticket = cr[1];
        BEAST_EXPECT(
            jacct[sfPreviousFields.fieldName][sfOwnerCount.fieldName] == 0);
        BEAST_EXPECT(
            jacct[sfFinalFields.fieldName][sfOwnerCount.fieldName] == 1);
        BEAST_EXPECT(
            jticket[sfNewFields.fieldName][sfExpiration.fieldName] == expire);
        BEAST_EXPECT(jticket[sfLedgerIndex.fieldName] ==
            "7F58A0AE17775BA3404D55D406DD1C2E91EADD7AF3F03A26877BCE764CCB75E3");

        Account alice {"alice"};
        env.fund (XRP (10000), alice);
        env.close ();

//现在试着用Alice帐户取消，它不起作用。
        auto jv = ticket::cancel(alice, jticket[sfLedgerIndex.fieldName].asString());
        env (jv, ter (tecNO_PERMISSION));

//将分类帐时间提前到触发到期
        env.timeKeeper ().adjustCloseTime (days {3});
        env.close ();

//现在再试一次-取消成功，因为票证已过期
        env (jv);
        auto crd = checkTicketMeta (env, true, true);
        auto const& jticketd = crd[1];
        BEAST_EXPECT(
            jticketd[sfFinalFields.fieldName][sfExpiration.fieldName] == expire);
    }

public:
    void run () override
    {
        testTicketNotEnabled ();
        testTicketCancelNonexistent ();
        testTicketCreatePreflightFail ();
        testTicketCreateNonexistent ();
        testTicketToSelf ();
        testTicketCancelByCreator ();
        testTicketInsufficientReserve ();
        testTicketCancelByTarget ();
        testTicketWithExpiration ();
        testTicketZeroExpiration ();
        testTicketWithPastExpiration ();
        testTicketAllowExpiration ();
    }
};

BEAST_DEFINE_TESTSUITE (Ticket, tx, ripple);

}  //涟漪

