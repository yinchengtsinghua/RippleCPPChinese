
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

#include <test/jtx.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/ledger/ApplyViewImpl.h>
#include <ripple/ledger/OpenView.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/ledger/Sandbox.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Protocol.h>
#include <type_traits>

namespace ripple {
namespace test {

class View_test
    : public beast::unit_test::suite
{
//将小整数转换为键
    static
    Keylet
    k (std::uint64_t id)
    {
        return Keylet{
            ltACCOUNT_ROOT, uint256(id)};
    }

//使用密钥和有效负载创建SLE
    static
    std::shared_ptr<SLE>
    sle (std::uint64_t id,
        std::uint32_t seq = 1)
    {
        auto const le =
            std::make_shared<SLE>(k(id));
        le->setFieldU32(sfSequence, seq);
        return le;
    }

//SLE返回有效载荷
    template <class T>
    static
    std::uint32_t
    seq (std::shared_ptr<T> const& le)
    {
        return le->getFieldU32(sfSequence);
    }

//在SLE上设置有效载荷
    static
    void
    seq (std::shared_ptr<SLE> const& le,
        std::uint32_t seq)
    {
        le->setFieldU32(sfSequence, seq);
    }

//清除所有状态项
    static
    void
    wipe (OpenLedger& openLedger)
    {
        openLedger.modify(
            [](OpenView& view, beast::Journal)
        {
//乱劈！
            boost::optional<uint256> next;
            next.emplace(0);
            for(;;)
            {
                next = view.succ(*next);
                if (! next)
                    break;
                view.rawErase(std::make_shared<SLE>(
                    *view.read(keylet::unchecked(*next))));
            }
            return true;
        });
    }

    static
    void
    wipe (Ledger& ledger)
    {
//乱劈！
        boost::optional<uint256> next;
        next.emplace(0);
        for(;;)
        {
            next = ledger.succ(*next);
            if (! next)
                break;
            ledger.rawErase(std::make_shared<SLE>(
                *ledger.read(keylet::unchecked(*next))));
        }
    }

//测试成功正确性
    void
    succ (ReadView const& v,
        std::uint32_t id,
            boost::optional<
                std::uint32_t> answer)
    {
        auto const next =
            v.succ(k(id).key);
        if (answer)
        {
            if (BEAST_EXPECT(next))
                BEAST_EXPECT(*next ==
                    k(*answer).key);
        }
        else
        {
            BEAST_EXPECT( ! next);
        }
    }

    template <class T>
    static
    std::shared_ptr<
        std::remove_const_t<T>>
    copy (std::shared_ptr<T> const& sp)
    {
        return std::make_shared<
            std::remove_const_t<T>>(*sp);
    }

//应用程序视图的练习分类帐实现
    void
    testLedger()
    {
        using namespace jtx;
        Env env(*this);
        Config config;
        std::shared_ptr<Ledger const> const genesis =
            std::make_shared<Ledger>(
                create_genesis, config,
                std::vector<uint256>{}, env.app().family());
        auto const ledger =
            std::make_shared<Ledger>(
                *genesis,
                env.app().timeKeeper().closeTime());
        wipe(*ledger);
        ReadView& v = *ledger;
        succ(v, 0, boost::none);
        ledger->rawInsert(sle(1, 1));
        BEAST_EXPECT(v.exists(k(1)));
        BEAST_EXPECT(seq(v.read(k(1))) == 1);
        succ(v, 0, 1);
        succ(v, 1, boost::none);
        ledger->rawInsert(sle(2, 2));
        BEAST_EXPECT(seq(v.read(k(2))) == 2);
        ledger->rawInsert(sle(3, 3));
        BEAST_EXPECT(seq(v.read(k(3))) == 3);
        auto s = copy(v.read(k(2)));
        seq(s, 4);
        ledger->rawReplace(std::move(s));
        BEAST_EXPECT(seq(v.read(k(2))) == 4);
        ledger->rawErase(sle(2));
        BEAST_EXPECT(! v.exists(k(2)));
        BEAST_EXPECT(v.exists(k(1)));
        BEAST_EXPECT(v.exists(k(3)));
    }

    void
    testMeta()
    {
        using namespace jtx;
        Env env(*this);
        wipe(env.app().openLedger());
        auto const open = env.current();
        ApplyViewImpl v(&*open, tapNONE);
        succ(v, 0, boost::none);
        v.insert(sle(1));
        BEAST_EXPECT(v.exists(k(1)));
        BEAST_EXPECT(seq(v.read(k(1))) == 1);
        BEAST_EXPECT(seq(v.peek(k(1))) == 1);
        succ(v, 0, 1);
        succ(v, 1, boost::none);
        v.insert(sle(2, 2));
        BEAST_EXPECT(seq(v.read(k(2))) == 2);
        v.insert(sle(3, 3));
        auto s = v.peek(k(3));
        BEAST_EXPECT(seq(s) == 3);
        s = v.peek(k(2));
        seq(s, 4);
        v.update(s);
        BEAST_EXPECT(seq(v.read(k(2))) == 4);
        v.erase(s);
        BEAST_EXPECT(! v.exists(k(2)));
        BEAST_EXPECT(v.exists(k(1)));
        BEAST_EXPECT(v.exists(k(3)));
    }

//练习所有辅助路径
    void
    testMetaSucc()
    {
        using namespace jtx;
        Env env(*this);
        wipe(env.app().openLedger());
        auto const open = env.current();
        ApplyViewImpl v0(&*open, tapNONE);
        v0.insert(sle(1));
        v0.insert(sle(2));
        v0.insert(sle(4));
        v0.insert(sle(7));
        {
            Sandbox v1(&v0);
            v1.insert(sle(3));
            v1.insert(sle(5));
            v1.insert(sle(6));

//V0：12—4—7
//V1: -3-56-

            succ(v0, 0, 1);
            succ(v0, 1, 2);
            succ(v0, 2, 4);
            succ(v0, 3, 4);
            succ(v0, 4, 7);
            succ(v0, 5, 7);
            succ(v0, 6, 7);
            succ(v0, 7, boost::none);

            succ(v1, 0, 1);
            succ(v1, 1, 2);
            succ(v1, 2, 3);
            succ(v1, 3, 4);
            succ(v1, 4, 5);
            succ(v1, 5, 6);
            succ(v1, 6, 7);
            succ(v1, 7, boost::none);

            v1.erase(v1.peek(k(4)));
            succ(v1, 3, 5);

            v1.erase(v1.peek(k(6)));
            succ(v1, 5, 7);
            succ(v1, 6, 7);

//V0：12—7
//V1: -3-5

            v1.apply(v0);
        }

//V0:123-5-7

        succ(v0, 0, 1);
        succ(v0, 1, 2);
        succ(v0, 2, 3);
        succ(v0, 3, 5);
        succ(v0, 4, 5);
        succ(v0, 5, 7);
        succ(v0, 6, 7);
        succ(v0, 7, boost::none);
    }

    void
    testStacked()
    {
        using namespace jtx;
        Env env(*this);
        wipe(env.app().openLedger());
        auto const open = env.current();
        ApplyViewImpl v0 (&*open, tapNONE);
        v0.rawInsert(sle(1, 1));
        v0.rawInsert(sle(2, 2));
        v0.rawInsert(sle(4, 4));

        {
            Sandbox v1(&v0);
            v1.erase(v1.peek(k(2)));
            v1.insert(sle(3, 3));
            auto s = v1.peek(k(4));
            seq(s, 5);
            v1.update(s);
            BEAST_EXPECT(seq(v1.read(k(1))) == 1);
            BEAST_EXPECT(! v1.exists(k(2)));
            BEAST_EXPECT(seq(v1.read(k(3))) == 3);
            BEAST_EXPECT(seq(v1.read(k(4))) == 5);
            {
                Sandbox v2(&v1);
                auto s = v2.peek(k(3));
                seq(s, 6);
                v2.update(s);
                v2.erase(v2.peek(k(4)));
                BEAST_EXPECT(seq(v2.read(k(1))) == 1);
                BEAST_EXPECT(! v2.exists(k(2)));
                BEAST_EXPECT(seq(v2.read(k(3))) == 6);
                BEAST_EXPECT(! v2.exists(k(4)));
//丢弃V2
            }
            BEAST_EXPECT(seq(v1.read(k(1))) == 1);
            BEAST_EXPECT(! v1.exists(k(2)));
            BEAST_EXPECT(seq(v1.read(k(3))) == 3);
            BEAST_EXPECT(seq(v1.read(k(4))) == 5);

            {
                Sandbox v2(&v1);
                auto s = v2.peek(k(3));
                seq(s, 6);
                v2.update(s);
                v2.erase(v2.peek(k(4)));
                BEAST_EXPECT(seq(v2.read(k(1))) == 1);
                BEAST_EXPECT(! v2.exists(k(2)));
                BEAST_EXPECT(seq(v2.read(k(3))) == 6);
                BEAST_EXPECT(! v2.exists(k(4)));
                v2.apply(v1);
            }
            BEAST_EXPECT(seq(v1.read(k(1))) == 1);
            BEAST_EXPECT(! v1.exists(k(2)));
            BEAST_EXPECT(seq(v1.read(k(3))) == 6);
            BEAST_EXPECT(! v1.exists(k(4)));
            v1.apply(v0);
        }
        BEAST_EXPECT(seq(v0.read(k(1))) == 1);
        BEAST_EXPECT(! v0.exists(k(2)));
        BEAST_EXPECT(seq(v0.read(k(3))) == 6);
        BEAST_EXPECT(! v0.exists(k(4)));
    }

//验证上下文信息
    void
    testContext()
    {
        using namespace jtx;
        using namespace std::chrono;
        {
            Env env(*this);
            wipe(env.app().openLedger());
            auto const open = env.current();
            OpenView v0(open.get());
            BEAST_EXPECT(v0.seq() != 98);
            BEAST_EXPECT(v0.seq() == open->seq());
            BEAST_EXPECT(v0.parentCloseTime() != NetClock::time_point{99s});
            BEAST_EXPECT(v0.parentCloseTime() ==
                open->parentCloseTime());
            {
//浅拷贝
                OpenView v1(v0);
                BEAST_EXPECT(v1.seq() == v0.seq());
                BEAST_EXPECT(v1.parentCloseTime() ==
                    v1.parentCloseTime());

                ApplyViewImpl v2(&v1, tapRETRY);
                BEAST_EXPECT(v2.parentCloseTime() ==
                    v1.parentCloseTime());
                BEAST_EXPECT(v2.seq() == v1.seq());
                BEAST_EXPECT(v2.flags() == tapRETRY);

                Sandbox v3(&v2);
                BEAST_EXPECT(v3.seq() == v2.seq());
                BEAST_EXPECT(v3.parentCloseTime() ==
                    v2.parentCloseTime());
                BEAST_EXPECT(v3.flags() == tapRETRY);
            }
            {
                ApplyViewImpl v1(&v0, tapRETRY);
                PaymentSandbox v2(&v1);
                BEAST_EXPECT(v2.seq() == v0.seq());
                BEAST_EXPECT(v2.parentCloseTime() ==
                    v0.parentCloseTime());
                BEAST_EXPECT(v2.flags() == tapRETRY);
                PaymentSandbox v3(&v2);
                BEAST_EXPECT(v3.seq() == v2.seq());
                BEAST_EXPECT(v3.parentCloseTime() ==
                    v2.parentCloseTime());
                BEAST_EXPECT(v3.flags() == v2.flags());
            }
        }
    }

//返回通过SLES找到的密钥列表
    static
    std::vector<uint256>
    sles (ReadView const& ledger)
    {
        std::vector<uint256> v;
        v.reserve (32);
        for(auto const& sle : ledger.sles)
            v.push_back(sle->key());
        return v;
    }

    template <class... Args>
    static
    std::vector<uint256>
    list (Args... args)
    {
        return std::vector<uint256> ({uint256(args)...});
    }

    void
    testSles()
    {
        using namespace jtx;
        Env env(*this);
        Config config;
        std::shared_ptr<Ledger const> const genesis =
            std::make_shared<Ledger> (
                create_genesis, config,
                std::vector<uint256>{}, env.app().family());
        auto const ledger = std::make_shared<Ledger>(
            *genesis,
            env.app().timeKeeper().closeTime());
        auto setup123 = [&ledger, this]()
        {
//删除中间元素
            wipe (*ledger);
            ledger->rawInsert (sle (1));
            ledger->rawInsert (sle (2));
            ledger->rawInsert (sle (3));
            BEAST_EXPECT(sles (*ledger) == list (1, 2, 3));
        };
        {
            setup123 ();
            OpenView view (ledger.get ());
            view.rawErase (sle (1));
            view.rawInsert (sle (4));
            view.rawInsert (sle (5));
            BEAST_EXPECT(sles (view) == list (2, 3, 4, 5));
            auto b = view.sles.begin();
            BEAST_EXPECT(view.sles.upper_bound(uint256(1)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(2)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(3)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(4)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(5)) == b);
        }
        {
            setup123 ();
            OpenView view (ledger.get ());
            view.rawErase (sle (1));
            view.rawErase (sle (2));
            view.rawInsert (sle (4));
            view.rawInsert (sle (5));
            BEAST_EXPECT(sles (view) == list (3, 4, 5));
            auto b = view.sles.begin();
            BEAST_EXPECT(view.sles.upper_bound(uint256(1)) == b);
            BEAST_EXPECT(view.sles.upper_bound(uint256(2)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(3)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(4)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(5)) == b);
        }
        {
            setup123 ();
            OpenView view (ledger.get ());
            view.rawErase (sle (1));
            view.rawErase (sle (2));
            view.rawErase (sle (3));
            view.rawInsert (sle (4));
            view.rawInsert (sle (5));
            BEAST_EXPECT(sles (view) == list (4, 5));
            auto b = view.sles.begin();
            BEAST_EXPECT(view.sles.upper_bound(uint256(1)) == b);
            BEAST_EXPECT(view.sles.upper_bound(uint256(2)) == b);
            BEAST_EXPECT(view.sles.upper_bound(uint256(3)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(4)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(5)) == b);
        }
        {
            setup123 ();
            OpenView view (ledger.get ());
            view.rawErase (sle (3));
            view.rawInsert (sle (4));
            view.rawInsert (sle (5));
            BEAST_EXPECT(sles (view) == list (1, 2, 4, 5));
            auto b = view.sles.begin();
            ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(1)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(2)) == b);
            BEAST_EXPECT(view.sles.upper_bound(uint256(3)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(4)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(5)) == b);
        }
        {
            setup123 ();
            OpenView view (ledger.get ());
            view.rawReplace (sle (1, 10));
            view.rawReplace (sle (3, 30));
            BEAST_EXPECT(sles (view) == list (1, 2, 3));
            BEAST_EXPECT(seq (view.read(k (1))) == 10);
            BEAST_EXPECT(seq (view.read(k (2))) == 1);
            BEAST_EXPECT(seq (view.read(k (3))) == 30);

            view.rawErase (sle (3));
            BEAST_EXPECT(sles (view) == list (1, 2));
            auto b = view.sles.begin();
            ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(1)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(2)) == b);
            BEAST_EXPECT(view.sles.upper_bound(uint256(3)) == b);
            BEAST_EXPECT(view.sles.upper_bound(uint256(4)) == b);
            BEAST_EXPECT(view.sles.upper_bound(uint256(5)) == b);

            view.rawInsert (sle (5));
            view.rawInsert (sle (4));
            view.rawInsert (sle (3));
            BEAST_EXPECT(sles (view) == list (1, 2, 3, 4, 5));
            b = view.sles.begin();
            ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(1)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(2)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(3)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(4)) == b); ++b;
            BEAST_EXPECT(view.sles.upper_bound(uint256(5)) == b);
        }
    }

    void
    testFlags()
    {
        using namespace jtx;
        Env env(*this);

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");
        auto const gw = Account("gateway");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        env.fund(XRP(10000), alice, bob, carol, gw);
        env.trust(USD(100), alice, bob, carol);
        {
//全球冻结。
            env(pay(gw, alice, USD(50)));
            env(offer(alice, XRP(5), USD(5)));

//现在冻结GW。
            env(fset (gw, asfGlobalFreeze));
            env.close();
            env(offer(alice, XRP(4), USD(5)), ter(tecFROZEN));
            env.close();

//如果冻结，Alice的美元余额应该为零。
            BEAST_EXPECT(USD(0) == accountHolds (*env.closed(),
                alice, USD.currency, gw, fhZERO_IF_FROZEN, env.journal));

//解冻gw然后再试一次。
            env(fclear (gw, asfGlobalFreeze));
            env.close();
            env(offer("alice", XRP(4), USD(5)));
        }
        {
//局部冻结。
            env(pay(gw, bob, USD(50)));
            env.close();

//现在吉布达•伟士冻结了鲍勃的美元信托额度。
            env(trust(gw, USD(100), bob, tfSetFreeze));
            env.close();

//如果冻结，鲍勃的余额应该为零。
            BEAST_EXPECT(USD(0) == accountHolds (*env.closed(),
                bob, USD.currency, gw, fhZERO_IF_FROZEN, env.journal));

//吉布达•伟士解除了鲍勃的信任。鲍勃把钱拿回来了。
            env(trust(gw, USD(100), bob, tfClearFreeze));
            env.close();
            BEAST_EXPECT(USD(50) == accountHolds (*env.closed(),
                bob, USD.currency, gw, fhZERO_IF_FROZEN, env.journal));
        }
        {
//帐号（）。
            env(pay(gw, carol, USD(50)));
            env.close();

//卡罗尔没有欧元。
            BEAST_EXPECT(EUR(0) == accountHolds (*env.closed(),
                carol, EUR.currency, gw, fhZERO_IF_FROZEN, env.journal));

//但是卡罗尔有美元。
            BEAST_EXPECT(USD(50) == accountHolds (*env.closed(),
                carol, USD.currency, gw, fhZERO_IF_FROZEN, env.journal));

//卡罗尔的xrp余额应该是她的持有量减去储备量。
            auto const carolsXRP = accountHolds (*env.closed(), carol,
                xrpCurrency(), xrpAccount(), fhZERO_IF_FROZEN, env.journal);
//卡罗尔的xrp余额：10000
//基本储备：200
//1条信托额度乘以准备金：1*50
//-----
//卡罗尔可用余额：9750
            BEAST_EXPECT(carolsXRP == XRP(9750));

//卡罗尔应该能够花*比她的xrp余额更多的钱在
//吃了她保留的食物的费用。
            env(noop(carol), fee(carolsXRP + XRP(10)));
            env.close();

//卡罗尔的xrp余额现在应该显示为零。
            BEAST_EXPECT(XRP(0) == accountHolds (*env.closed(),
                carol, xrpCurrency(), gw, fhZERO_IF_FROZEN, env.journal));
        }
        {
//会计凭证（）。
//网关拥有他们声称拥有的任何资金。
            auto const gwUSD = accountFunds(
                *env.closed(), gw, USD(314159), fhZERO_IF_FROZEN, env.journal);
            BEAST_EXPECT(gwUSD == USD(314159));

//卡罗尔有来自网关的资金。
            auto carolsUSD = accountFunds(
                *env.closed(), carol, USD(0), fhZERO_IF_FROZEN, env.journal);
            BEAST_EXPECT(carolsUSD == USD(50));

//如果卡罗尔的资金被冻结，她就没有资金了…
            env(fset (gw, asfGlobalFreeze));
            env.close();
            carolsUSD = accountFunds(
                *env.closed(), carol, USD(0), fhZERO_IF_FROZEN, env.journal);
            BEAST_EXPECT(carolsUSD == USD(0));

//…除非查询忽略冻结状态。
            carolsUSD = accountFunds(
                *env.closed(), carol, USD(0), fhIGNORE_FREEZE, env.journal);
            BEAST_EXPECT(carolsUSD == USD(50));

//只是为了保持整洁，解冻吉布达·伟士。
            env(fclear (gw, asfGlobalFreeze));
            env.close();
        }
    }

    void
    testTransferRate()
    {
        using namespace jtx;
        Env env(*this);

        auto const gw1 = Account("gw1");

        env.fund(XRP(10000), gw1);
        env.close();

        auto rdView = env.closed();
//在GW1上未设置速率的情况下进行测试。
        BEAST_EXPECT(transferRate (*rdView, gw1) == parityRate);

        env(rate(gw1, 1.02));
        env.close();

        rdView = env.closed();
        BEAST_EXPECT(transferRate (*rdView, gw1) == Rate{ 1020000000 });
    }

    void
    testAreCompatible()
    {
//此测试需要不兼容的分类帐。好消息是我们可以
//同时构造和管理两个不同的env实例
//时间。所以我们可以使用两个env实例互相生成
//不兼容的分类帐。
        using namespace jtx;
        auto const alice = Account("alice");
        auto const bob = Account("bob");

//第一个EV。
        Env eA(*this);

        eA.fund(XRP(10000), alice);
        eA.close();
        auto const rdViewA3 = eA.closed();

        eA.fund(XRP(10000), bob);
        eA.close();
        auto const rdViewA4 = eA.closed();

//这两个env不能共享相同的端口，因此修改配置
//第二个env使用更高的端口号
        Env eB {*this, envconfig(port_increment, 3)};

//制作与第一个分类账不兼容的分类账。注释
//鲍勃的资金先于爱丽丝。
        eB.fund(XRP(10000), bob);
        eB.close();
        auto const rdViewB3 = eB.closed();

        eB.fund(XRP(10000), alice);
        eB.close();
        auto const rdViewB4 = eB.closed();

//检查兼容性。
        auto jStream = eA.journal.error();
        BEAST_EXPECT(  areCompatible (*rdViewA3, *rdViewA4, jStream, ""));
        BEAST_EXPECT(  areCompatible (*rdViewA4, *rdViewA3, jStream, ""));
        BEAST_EXPECT(  areCompatible (*rdViewA4, *rdViewA4, jStream, ""));
        BEAST_EXPECT(! areCompatible (*rdViewA3, *rdViewB4, jStream, ""));
        BEAST_EXPECT(! areCompatible (*rdViewA4, *rdViewB3, jStream, ""));
        BEAST_EXPECT(! areCompatible (*rdViewA4, *rdViewB4, jStream, ""));

//尝试其他接口。
//注意，不同的接口有不同的结果。
        auto const& iA3 = rdViewA3->info();
        auto const& iA4 = rdViewA4->info();

        BEAST_EXPECT(  areCompatible (iA3.hash, iA3.seq, *rdViewA4, jStream, ""));
        BEAST_EXPECT(  areCompatible (iA4.hash, iA4.seq, *rdViewA3, jStream, ""));
        BEAST_EXPECT(  areCompatible (iA4.hash, iA4.seq, *rdViewA4, jStream, ""));
        BEAST_EXPECT(! areCompatible (iA3.hash, iA3.seq, *rdViewB4, jStream, ""));
        BEAST_EXPECT(  areCompatible (iA4.hash, iA4.seq, *rdViewB3, jStream, ""));
        BEAST_EXPECT(! areCompatible (iA4.hash, iA4.seq, *rdViewB4, jStream, ""));
    }

    void
    testRegressions()
    {
        using namespace jtx;

//创建包含1个项目的分类帐，将
//应用程序视图，然后另一个应用程序视图，
//删除项目，应用。
        {
            Env env(*this);
            Config config;
            std::shared_ptr<Ledger const> const genesis =
                std::make_shared<Ledger>(
                    create_genesis, config,
                    std::vector<uint256>{}, env.app().family());
            auto const ledger =
                std::make_shared<Ledger>(
                    *genesis,
                    env.app().timeKeeper().closeTime());
            wipe(*ledger);
            ledger->rawInsert(sle(1));
            ReadView& v0 = *ledger;
            ApplyViewImpl v1(&v0, tapNONE);
            {
                Sandbox v2(&v1);
                v2.erase(v2.peek(k(1)));
                v2.apply(v1);
            }
            BEAST_EXPECT(! v1.exists(k(1)));
        }

//确保OpenLedger:：Empty Works
        {
            Env env(*this);
            BEAST_EXPECT(env.app().openLedger().empty());
            env.fund(XRP(10000), Account("test"));
            BEAST_EXPECT(! env.app().openLedger().empty());
        }
    }

    void run() override
    {
//这是最好的工作，否则
        BEAST_EXPECT(k(0).key < k(1).key);

        testLedger();
        testMeta();
        testMetaSucc();
        testStacked();
        testContext();
        testSles();
        testFlags();
        testTransferRate();
        testAreCompatible();
        testRegressions();
    }
};

class GetAmendments_test
    : public beast::unit_test::suite
{
    void
    testGetAmendments()
    {
        using namespace jtx;
        Env env {*this, envconfig(validator, "")};

//从没有修改开始。
        auto majorities = getMajorityAmendments (*env.closed());
        BEAST_EXPECT(majorities.empty());

//现在关闭分类帐，直到修订显示。
        int i = 0;
        for (i = 0; i <= 256; ++i)
        {
            env.close();
            majorities = getMajorityAmendments (*env.closed());
            if (! majorities.empty())
                break;
        }

//至少有5个修正案。不要做精确的比较
//为了避免将来增加更多的修改而进行维护。
        BEAST_EXPECT(i == 254);
        BEAST_EXPECT(majorities.size() >= 5);

//目前还不应启用任何修订。
        auto enableds = getEnabledAmendments(*env.closed());
        BEAST_EXPECT(enableds.empty());

//现在等待2周，模块256分类账，以便修改
//启用。通过每80分钟关闭分类帐来加快进程，
//这将使我们在256个分类账之后的2周内。
        for (i = 0; i <= 256; ++i)
        {
            using namespace std::chrono_literals;
            env.close(80min);
            enableds = getEnabledAmendments(*env.closed());
            if (! enableds.empty())
                break;
        }
        BEAST_EXPECT(i == 255);
        BEAST_EXPECT(enableds.size() >= 5);
    }

    void run() override
    {
        testGetAmendments();
    }
};

BEAST_DEFINE_TESTSUITE(View,ledger,ripple);
BEAST_DEFINE_TESTSUITE(GetAmendments,ledger,ripple);

}  //测试
}  //涟漪
