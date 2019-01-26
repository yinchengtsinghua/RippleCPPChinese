
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

#include <ripple/peerfinder/impl/Livecache.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/clock/manual_clock.h>
#include <test/beast/IPEndpointCommon.h>
#include <test/unit_test/SuiteJournal.h>
#include <boost/algorithm/string.hpp>

namespace ripple {
namespace PeerFinder {

bool operator== (Endpoint const& a, Endpoint const& b)
{
    return (a.hops == b.hops &&
            a.address == b.address);
}

class Livecache_test : public beast::unit_test::suite
{
    TestStopwatch clock_;
    test::SuiteJournal journal_;

public:
    Livecache_test()
    : journal_ ("Livecache_test", *this)
    { }

//将地址添加为端点
    template <class C>
    inline void add (beast::IP::Endpoint ep, C& c, int hops = 0)
    {
        Endpoint cep {ep, hops};
        c.insert (cep);
    }

    void testBasicInsert ()
    {
        testcase ("Basic Insert");
        Livecache <> c (clock_, journal_);
        BEAST_EXPECT(c.empty());

        for (auto i = 0; i < 10; ++i)
            add(beast::IP::randomEP(true), c);

        BEAST_EXPECT(! c.empty());
        BEAST_EXPECT(c.size() == 10);

        for (auto i = 0; i < 10; ++i)
            add(beast::IP::randomEP(false), c);

        BEAST_EXPECT(! c.empty());
        BEAST_EXPECT(c.size() == 20);
    }

    void testInsertUpdate ()
    {
        testcase ("Insert/Update");
        Livecache <> c (clock_, journal_);

        auto ep1 = Endpoint {beast::IP::randomEP(), 2};
        c.insert(ep1);
        BEAST_EXPECT(c.size() == 1);
//第三个位置列表将包含条目
        BEAST_EXPECT((c.hops.begin()+2)->begin()->hops == 2);

        auto ep2 = Endpoint {ep1.address, 4};
//这不会改变该条目具有更高的跃点
        c.insert(ep2);
        BEAST_EXPECT(c.size() == 1);
//仍在第三名名单中
        BEAST_EXPECT((c.hops.begin()+2)->begin()->hops == 2);

        auto ep3 = Endpoint {ep1.address, 2};
//这不会更改该项与现有项具有相同的跃点
        c.insert(ep3);
        BEAST_EXPECT(c.size() == 1);
//仍在第三名名单中
        BEAST_EXPECT((c.hops.begin()+2)->begin()->hops == 2);

        auto ep4 = Endpoint {ep1.address, 1};
        c.insert(ep4);
        BEAST_EXPECT(c.size() == 1);
//现在在第二位置列表
        BEAST_EXPECT((c.hops.begin()+1)->begin()->hops == 1);
    }

    void testExpire ()
    {
        testcase ("Expire");
        using namespace std::chrono_literals;
        Livecache <> c (clock_, journal_);

        auto ep1 = Endpoint {beast::IP::randomEP(), 1};
        c.insert(ep1);
        BEAST_EXPECT(c.size() == 1);
        c.expire();
        BEAST_EXPECT(c.size() == 1);
//验证是否提前到到期前1秒
//让我们的入口完好无损
        clock_.advance(Tuning::liveCacheSecondsToLive - 1s);
        c.expire();
        BEAST_EXPECT(c.size() == 1);
//现在提前到到期日
        clock_.advance(1s);
        c.expire();
        BEAST_EXPECT(c.empty());
    }

    void testHistogram ()
    {
        testcase ("Histogram");
        constexpr auto num_eps = 40;
        Livecache <> c (clock_, journal_);
        for (auto i = 0; i < num_eps; ++i)
            add(
                beast::IP::randomEP(true),
                c,
                ripple::rand_int(0, safe_cast<int>(Tuning::maxHops + 1)));
        auto h = c.hops.histogram();
        if(! BEAST_EXPECT(! h.empty()))
            return;
        std::vector <std::string> v;
        boost::split (v, h, boost::algorithm::is_any_of (","));
        auto sum = 0;
        for (auto const& n : v)
        {
            auto val = boost::lexical_cast<int>(boost::trim_copy(n));
            sum += val;
            BEAST_EXPECT(val >= 0);
        }
        BEAST_EXPECT(sum == num_eps);
    }


    void testShuffle ()
    {
        testcase ("Shuffle");
        Livecache <> c (clock_, journal_);
        for (auto i = 0; i < 100; ++i)
            add(
                beast::IP::randomEP(true),
                c,
                ripple::rand_int(0, safe_cast<int>(Tuning::maxHops + 1)));

        using at_hop = std::vector <ripple::PeerFinder::Endpoint>;
        using all_hops = std::array <at_hop, 1 + Tuning::maxHops + 1>;

        auto cmp_EP = [](Endpoint const& a, Endpoint const& b) {
            return (b.hops < a.hops || (b.hops == a.hops && b.address < a.address));
        };
        all_hops before;
        all_hops before_sorted;
        for (auto i = std::make_pair(0, c.hops.begin());
                i.second != c.hops.end(); ++i.first, ++i.second)
        {
            std::copy ((*i.second).begin(), (*i.second).end(),
                std::back_inserter (before[i.first]));
            std::copy ((*i.second).begin(), (*i.second).end(),
                std::back_inserter (before_sorted[i.first]));
            std::sort(
                before_sorted[i.first].begin(),
                before_sorted[i.first].end(),
                cmp_EP);
        }

        c.hops.shuffle();

        all_hops after;
        all_hops after_sorted;
        for (auto i = std::make_pair(0, c.hops.begin());
                i.second != c.hops.end(); ++i.first, ++i.second)
        {
            std::copy ((*i.second).begin(), (*i.second).end(),
                std::back_inserter (after[i.first]));
            std::copy ((*i.second).begin(), (*i.second).end(),
                std::back_inserter (after_sorted[i.first]));
            std::sort(
                after_sorted[i.first].begin(),
                after_sorted[i.first].end(),
                cmp_EP);
        }

//每个跃点存储桶应包含相同的项
//排序前后，尽管顺序不同
        bool all_match = true;
        for (auto i = 0; i < before.size(); ++i)
        {
            BEAST_EXPECT(before[i].size() == after[i].size());
            all_match = all_match && (before[i] == after[i]);
            BEAST_EXPECT(before_sorted[i] == after_sorted[i]);
        }
        BEAST_EXPECT(! all_match);
    }

    void run () override
    {
        testBasicInsert ();
        testInsertUpdate ();
        testExpire ();
        testHistogram ();
        testShuffle ();
    }
};

BEAST_DEFINE_TESTSUITE(Livecache,peerfinder,ripple);

}
}
