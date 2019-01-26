
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

#include <ripple/basics/chrono.h>
#include <ripple/basics/random.h>
#include <ripple/beast/unit_test.h>
#include <ripple/resource/Consumer.h>
#include <ripple/resource/impl/Entry.h>
#include <ripple/resource/impl/Logic.h>
#include <test/unit_test/SuiteJournal.h>

#include <boost/utility/base_from_member.hpp>


namespace ripple {
namespace Resource {

class ResourceManager_test : public beast::unit_test::suite
{
public:
    class TestLogic
        : private boost::base_from_member<TestStopwatch>
        , public Logic

    {
    private:
        using clock_type =
            boost::base_from_member<TestStopwatch>;

    public:
        explicit TestLogic (beast::Journal journal)
            : Logic (beast::insight::NullCollector::New(), member, journal)
        {
        }

        void advance ()
        {
            ++member;
        }

        TestStopwatch& clock ()
        {
            return member;
        }
    };

//——————————————————————————————————————————————————————————————

    void createGossip (Gossip& gossip)
    {
        std::uint8_t const v (10 + rand_int(9));
        std::uint8_t const n (10 + rand_int(9));
        gossip.items.reserve (n);
        for (std::uint8_t i = 0; i < n; ++i)
        {
            Gossip::Item item;
            item.balance = 100 + rand_int(499);
            beast::IP::AddressV4::bytes_type d =
                {{192,0,2,static_cast<std::uint8_t>(v + i)}};
            item.address = beast::IP::Endpoint { beast::IP::AddressV4 {d} };
            gossip.items.push_back (item);
        }
    }

//——————————————————————————————————————————————————————————————

    void testDrop (beast::Journal j)
    {
        testcase ("Warn/drop");

        TestLogic logic (j);

        Charge const fee (dropThreshold + 1);
        beast::IP::Endpoint const addr (
            beast::IP::Endpoint::from_string ("192.0.2.2"));

        {
            Consumer c (logic.newInboundEndpoint (addr));

//在收到警告之前创建加载
            int n = 10000;

            while (--n >= 0)
            {
                if (n == 0)
                {
                    fail ("Loop count exceeded without warning");
                    return;
                }

                if (c.charge (fee) == warn)
                {
                    pass ();
                    break;
                }
                ++logic.clock ();
            }

//创建负载，直到我们被丢弃
            while (--n >= 0)
            {
                if (n == 0)
                {
                    fail ("Loop count exceeded without dropping");
                    return;
                }

                if (c.charge (fee) == drop)
                {
//断开滥用用户的连接
                    BEAST_EXPECT(c.disconnect ());
                    break;
                }
                ++logic.clock ();
            }
        }

//确保消费者在黑名单上呆一段时间。
        {
            Consumer c (logic.newInboundEndpoint (addr));
            logic.periodicActivity();
            if (c.disposition () != drop)
            {
                fail ("Dropped consumer not put on blacklist");
                return;
            }
        }

//确保最终将消费者从黑名单中删除
        bool readmitted = false;
        {
            using namespace std::chrono_literals;
//让消费者有时间重新接受治疗。不应该
//超过有效期。
            auto n = secondsUntilExpiration + 1s;
            while (--n > 0s)
            {
                ++logic.clock ();
                logic.periodicActivity();
                Consumer c (logic.newInboundEndpoint (addr));
                if (c.disposition () != drop)
                {
                    readmitted = true;
                    break;
                }
            }
        }
        if (readmitted == false)
        {
            fail ("Dropped Consumer left on blacklist too long");
            return;
        }
        pass();
    }

    void testImports (beast::Journal j)
    {
        testcase ("Imports");

        TestLogic logic (j);

        Gossip g[5];

        for (int i = 0; i < 5; ++i)
            createGossip (g[i]);

        for (int i = 0; i < 5; ++i)
            logic.importConsumers (std::to_string (i), g[i]);

        pass();
    }

    void testImport (beast::Journal j)
    {
        testcase ("Import");

        TestLogic logic (j);

        Gossip g;
        Gossip::Item item;
        item.balance = 100;
        beast::IP::AddressV4::bytes_type d = {{192, 0, 2, 1}};
        item.address = beast::IP::Endpoint { beast::IP::AddressV4 {d} };
        g.items.push_back (item);

        logic.importConsumers ("g", g);

        pass();
    }

    void testCharges (beast::Journal j)
    {
        testcase ("Charge");

        TestLogic logic (j);

        {
            beast::IP::Endpoint address (beast::IP::Endpoint::from_string ("192.0.2.1"));
            Consumer c (logic.newInboundEndpoint (address));
            Charge fee (1000);
            JLOG(j.info()) <<
                "Charging " << c.to_string() << " " << fee << " per second";
            c.charge (fee);
            for (int i = 0; i < 128; ++i)
            {
                JLOG(j.info()) <<
                    "Time= " << logic.clock().now().time_since_epoch().count() <<
                    ", Balance = " << c.balance();
                logic.advance();
            }
        }

        {
            beast::IP::Endpoint address (beast::IP::Endpoint::from_string ("192.0.2.2"));
            Consumer c (logic.newInboundEndpoint (address));
            Charge fee (1000);
            JLOG(j.info()) <<
                "Charging " << c.to_string() << " " << fee << " per second";
            for (int i = 0; i < 128; ++i)
            {
                c.charge (fee);
                JLOG(j.info()) <<
                    "Time= " << logic.clock().now().time_since_epoch().count() <<
                    ", Balance = " << c.balance();
                logic.advance();
            }
        }

        pass();
    }

    void run() override
    {
        using namespace beast::severities;
        test::SuiteJournal journal ("ResourceManager_test", *this);

        testDrop (journal);
        testCharges (journal);
        testImports (journal);
        testImport (journal);
    }
};

BEAST_DEFINE_TESTSUITE(ResourceManager,resource,ripple);

}
}
