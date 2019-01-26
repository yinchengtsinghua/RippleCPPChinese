
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

#include <ripple/basics/random.h>
#include <ripple/ledger/BookDirs.h>
#include <ripple/ledger/Directory.h>
#include <ripple/ledger/Sandbox.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/Protocol.h>
#include <test/jtx.h>
#include <algorithm>

namespace ripple {
namespace test {

struct Directory_test : public beast::unit_test::suite
{
//将[0-15576]映射到一个唯一的3字母货币代码中
    std::string
    currcode (std::size_t i)
    {
//只有17576种可能的组合
        BEAST_EXPECT (i < 17577);

        std::string code;

        for (int j = 0; j != 3; ++j)
        {
            code.push_back ('A' + (i % 26));
            i /= 26;
        }

        return code;
    }

//插入n个空页，编号为[0，…n- 1，在
//指定的目录：
    void
    makePages(
        Sandbox& sb,
        uint256 const& base,
        std::uint64_t n)
    {
        for (std::uint64_t i = 0; i < n; ++i)
        {
            auto p = std::make_shared<SLE>(keylet::page(base, i));

            p->setFieldV256 (sfIndexes, STVector256{});

            if (i + 1 == n)
                p->setFieldU64 (sfIndexNext, 0);
            else
                p->setFieldU64 (sfIndexNext, i + 1);

            if (i == 0)
                p->setFieldU64 (sfIndexPrevious, n - 1);
            else
                p->setFieldU64 (sfIndexPrevious, i - 1);

            sb.insert (p);
        }
    }

    void testDirectoryOrdering()
    {
        using namespace jtx;

        auto gw = Account("gw");
        auto USD = gw["USD"];
        auto alice = Account("alice");
        auto bob = Account("bob");

        {
            testcase ("Directory Ordering (without 'SortedDirectories' amendment");

            Env env(
                *this,
                supported_amendments().reset(featureSortedDirectories));
            env.fund(XRP(10000000), alice, bob, gw);

//插入Alice的400个报价，然后插入Bob的一个：
            for (std::size_t i = 1; i <= 400; ++i)
                env(offer(alice, USD(10), XRP(10)));

//检查爱丽丝的目录：它应该包含一个
//她添加的每个报价的条目。在每一个
//第页，条目应按排序顺序排列。
            {
                auto dir = Dir(*env.current(),
                    keylet::ownerDir(alice));

                std::uint32_t lastSeq = 1;

//通过检查确认订单是连续的
//它们的序列号是：
                for (auto iter = dir.begin(); iter != std::end(dir); ++iter) {
                    BEAST_EXPECT(++lastSeq == (*iter)->getFieldU32(sfSequence));
                }
                BEAST_EXPECT(lastSeq != 1);
            }
        }

        {
            testcase ("Directory Ordering (with 'SortedDirectories' amendment)");

            Env env(*this);
            env.fund(XRP(10000000), alice, gw);

            for (std::size_t i = 1; i <= 400; ++i)
                env(offer(alice, USD(i), XRP(i)));
            env.close();

//检查爱丽丝的目录：它应该包含一个
//她添加的每个报价的条目，以及
//页面条目应按排序顺序排列。
            {
                auto const view = env.closed();

                std::uint64_t page = 0;

                do
                {
                    auto p = view->read(keylet::page(keylet::ownerDir(alice), page));

//确保页面中的条目已排序
                    auto const& v = p->getFieldV256(sfIndexes);
                    BEAST_EXPECT (std::is_sorted(v.begin(), v.end()));

//确保页面包含正确的排序依据
//计算属于这里的序列号。
                    std::uint32_t minSeq = 2 + (page * dirNodeMaxEntries);
                    std::uint32_t maxSeq = minSeq + dirNodeMaxEntries;

                    for (auto const& e : v)
                    {
                        auto c = view->read(keylet::child(e));
                        BEAST_EXPECT(c);
                        BEAST_EXPECT(c->getFieldU32(sfSequence) >= minSeq);
                        BEAST_EXPECT(c->getFieldU32(sfSequence) < maxSeq);
                    }

                    page = p->getFieldU64(sfIndexNext);
                } while (page != 0);
            }

//现在检查一下订货单：应该是我们下的订单
//提议。
            auto book = BookDirs(*env.current(),
                Book({xrpIssue(), USD.issue()}));
            int count = 1;

            for (auto const& offer : book)
            {
                count++;
                BEAST_EXPECT(offer->getFieldAmount(sfTakerPays) == USD(count));
                BEAST_EXPECT(offer->getFieldAmount(sfTakerGets) == XRP(count));
            }
        }
    }

    void
    testDirIsEmpty()
    {
        testcase ("dirIsEmpty");

        using namespace jtx;
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const charlie = Account ("charlie");
        auto const gw = Account ("gw");

        Env env(*this);

        env.fund(XRP(1000000), alice, charlie, gw);
        env.close();

//爱丽丝应该有一个空目录。
        BEAST_EXPECT(dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));

//给艾丽斯一个签名者名单，目录里就会有东西。
        env(signers(alice, 1, { { bob, 1} }));
        env.close();
        BEAST_EXPECT(! dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));

        env(signers(alice, jtx::none));
        env.close();
        BEAST_EXPECT(dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));

        std::vector<IOU> const currencies = [this, &gw]()
        {
            std::vector<IOU> c;

            c.reserve((2 * dirNodeMaxEntries) + 3);

            while (c.size() != c.capacity())
                c.push_back(gw[currcode(c.size())]);

            return c;
        }();

//首先，Alices创建了很多信任线，然后
//按不同顺序删除：
        {
            auto cl = currencies;

            for (auto const& c : cl)
            {
                env(trust(alice, c(50)));
                env.close();
            }

            BEAST_EXPECT(! dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));

            std::shuffle (cl.begin(), cl.end(), default_prng());

            for (auto const& c : cl)
            {
                env(trust(alice, c(0)));
                env.close();
            }

            BEAST_EXPECT(dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));
        }

//现在，爱丽丝提出购买货币的建议，创造
//隐含的信任线。
        {
            auto cl = currencies;

            BEAST_EXPECT(dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));

            for (auto c : currencies)
            {
                env(trust(charlie, c(50)));
                env.close();
                env(pay(gw, charlie, c(50)));
                env.close();
                env(offer(alice, c(50), XRP(50)));
                env.close();
            }

            BEAST_EXPECT(! dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));

//现在按随机顺序填写报价单。要约
//条目将被删除并替换为信任
//隐式创建的行。
            std::shuffle (cl.begin(), cl.end(), default_prng());

            for (auto const& c : cl)
            {
                env(offer(charlie, XRP(50), c(50)));
                env.close();
            }
            BEAST_EXPECT(! dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));
//最后，Alice现在把资金寄回
//查理。隐式创建的信任线
//应丢弃：
            std::shuffle (cl.begin(), cl.end(), default_prng());

            for (auto const& c : cl)
            {
                env(pay(alice, charlie, c(50)));
                env.close();
            }

            BEAST_EXPECT(dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));
        }
    }

    void
    testRipd1353()
    {
        testcase("RIPD-1353 Empty Offer Directories");

        using namespace jtx;
        Env env(*this);

        auto const gw = Account{"gateway"};
        auto const alice = Account{"alice"};
        auto const USD = gw["USD"];

        env.fund(XRP(10000), alice, gw);
        env.trust(USD(1000), alice);
        env(pay(gw, alice, USD(1000)));

        auto const firstOfferSeq = env.seq(alice);

//填写三页报价单
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < dirNodeMaxEntries; ++j)
                env(offer(alice, XRP(1), USD(1)));
        env.close();

//删除所有报价。最后删除中间页
        for (auto page : {0, 2, 1})
        {
            for (int i = 0; i < dirNodeMaxEntries; ++i)
            {
                Json::Value cancelOffer;
                cancelOffer[jss::Account] = alice.human();
                cancelOffer[jss::OfferSequence] =
                    Json::UInt(firstOfferSeq + page * dirNodeMaxEntries + i);
                cancelOffer[jss::TransactionType] = "OfferCancel";
                env(cancelOffer);
                env.close();
            }
        }

//所有的报价都被取消了，所以这本书
//应没有条目且为空：
        {
            Sandbox sb(env.closed().get(), tapNONE);
            uint256 const bookBase = getBookBase({xrpIssue(), USD.issue()});

            BEAST_EXPECT(dirIsEmpty (sb, keylet::page(bookBase)));
            BEAST_EXPECT (!sb.succ(bookBase, getQualityNext(bookBase)));
        }

//艾丽斯把她要的美元还给了门口
//取消了她的信任。她的所有者目录
//现在应为空：
        {
            env.trust(USD(0), alice);
            env(pay(alice, gw, alice["USD"](1000)));
            env.close();
            BEAST_EXPECT(dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));
        }
    }

    void
    testEmptyChain()
    {
        testcase("Empty Chain on Delete");

        using namespace jtx;
        Env env(*this);

        auto const gw = Account{"gateway"};
        auto const alice = Account{"alice"};
        auto const USD = gw["USD"];

        env.fund(XRP(10000), alice);
        env.close();

        uint256 base;
        base.SetHex("fb71c9aa3310141da4b01d6c744a98286af2d72ab5448d5adc0910ca0c910880");

        uint256 item;
        item.SetHex("bad0f021aa3b2f6754a8fe82a5779730aa0bbbab82f17201ef24900efc2c7312");

        {
//创建一个由三页组成的链：
            Sandbox sb(env.closed().get(), tapNONE);
            makePages (sb, base, 3);

//在中间页插入项目：
            {
                auto p = sb.peek (keylet::page(base, 1));
                BEAST_EXPECT(p);

                STVector256 v;
                v.push_back (item);
                p->setFieldV256 (sfIndexes, v);
                sb.update(p);
            }

//现在，尝试从中间删除项目
//页。这将导致删除所有页面：
            BEAST_EXPECT (sb.dirRemove (keylet::page(base, 0), 1, keylet::unchecked(item), false));
            BEAST_EXPECT (!sb.peek(keylet::page(base, 2)));
            BEAST_EXPECT (!sb.peek(keylet::page(base, 1)));
            BEAST_EXPECT (!sb.peek(keylet::page(base, 0)));
        }

        {
//创建一个包含四页的链：
            Sandbox sb(env.closed().get(), tapNONE);
            makePages (sb, base, 4);

//现在在第1页和第2页添加项目：
            {
                auto p1 = sb.peek (keylet::page(base, 1));
                BEAST_EXPECT(p1);

                STVector256 v1;
                v1.push_back (~item);
                p1->setFieldV256 (sfIndexes, v1);
                sb.update(p1);

                auto p2 = sb.peek (keylet::page(base, 2));
                BEAST_EXPECT(p2);

                STVector256 v2;
                v2.push_back (item);
                p2->setFieldV256 (sfIndexes, v2);
                sb.update(p2);
            }

//现在，尝试从第2页删除该项。
//这将导致第2页和第3页
//删除：
            BEAST_EXPECT (sb.dirRemove (keylet::page(base, 0), 2, keylet::unchecked(item), false));
            BEAST_EXPECT (!sb.peek(keylet::page(base, 3)));
            BEAST_EXPECT (!sb.peek(keylet::page(base, 2)));

            auto p1 = sb.peek(keylet::page(base, 1));
            BEAST_EXPECT (p1);
            BEAST_EXPECT (p1->getFieldU64 (sfIndexNext) == 0);
            BEAST_EXPECT (p1->getFieldU64 (sfIndexPrevious) == 0);

            auto p0 = sb.peek(keylet::page(base, 0));
            BEAST_EXPECT (p0);
            BEAST_EXPECT (p0->getFieldU64 (sfIndexNext) == 1);
            BEAST_EXPECT (p0->getFieldU64 (sfIndexPrevious) == 1);
        }
    }

    void run() override
    {
        testDirectoryOrdering();
        testDirIsEmpty();
        testRipd1353();
        testEmptyChain();
    }
};

BEAST_DEFINE_TESTSUITE_PRIO(Directory,ledger,ripple,1);

}
}
