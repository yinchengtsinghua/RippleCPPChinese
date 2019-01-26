
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
    版权所有2015 Ripple Labs Inc.

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

#include <ripple/basics/BasicConfig.h>
#include <ripple/overlay/Cluster.h>
#include <ripple/overlay/ClusterNode.h>
#include <ripple/protocol/SecretKey.h>
#include <test/jtx/TestSuite.h>
#include <test/unit_test/SuiteJournal.h>

namespace ripple {
namespace tests {

class cluster_test : public ripple::TestSuite
{
    test::SuiteJournal journal_;

public:
    cluster_test ()
    : journal_ ("cluster_test", *this)
    { }

    std::unique_ptr<Cluster>
    create (std::vector<PublicKey> const& nodes)
    {
        auto cluster = std::make_unique <Cluster> (journal_);

        for (auto const& n : nodes)
            cluster->update (n, "Test");

        return cluster;
    }

    PublicKey
    randomNode ()
    {
        return derivePublicKey (
            KeyType::secp256k1,
            randomSecretKey());
    }

    void
    testMembership ()
    {
//网络上的服务器
        std::vector<PublicKey> network;

        while (network.size () != 128)
            network.push_back (randomNode());

        {
            testcase ("Membership: Empty cluster");

            auto c = create ({});

            for (auto const& n : network)
                BEAST_EXPECT(!c->member (n));
        }

        {
            testcase ("Membership: Non-empty cluster and none present");

            std::vector<PublicKey> cluster;
            while (cluster.size () != 32)
                cluster.push_back (randomNode());

            auto c = create (cluster);

            for (auto const& n : network)
                BEAST_EXPECT(!c->member (n));
        }

        {
            testcase ("Membership: Non-empty cluster and some present");

            std::vector<PublicKey> cluster (
                network.begin (), network.begin () + 16);

            while (cluster.size () != 32)
                cluster.push_back (randomNode());

            auto c = create (cluster);

            for (auto const& n : cluster)
                BEAST_EXPECT(c->member (n));

            for (auto const& n : network)
            {
                auto found = std::find (
                    cluster.begin (), cluster.end (), n);
                BEAST_EXPECT(static_cast<bool>(c->member (n)) ==
                    (found != cluster.end ()));
            }
        }

        {
            testcase ("Membership: Non-empty cluster and all present");

            std::vector<PublicKey> cluster (
                network.begin (), network.begin () + 32);

            auto c = create (cluster);

            for (auto const& n : cluster)
                BEAST_EXPECT(c->member (n));

            for (auto const& n : network)
            {
                auto found = std::find (
                    cluster.begin (), cluster.end (), n);
                BEAST_EXPECT(static_cast<bool>(c->member (n)) ==
                    (found != cluster.end ()));
            }
        }
    }

    void
    testUpdating ()
    {
        testcase ("Updating");

        auto c = create ({});

        auto const node = randomNode ();
        auto const name = toBase58(
            TokenType::NodePublic,
            node);
        std::uint32_t load = 0;
        NetClock::time_point tick = {};

//初始更新
        BEAST_EXPECT(c->update (node, "", load, tick));
        {
            auto member = c->member (node);
            BEAST_EXPECT(static_cast<bool>(member));
            BEAST_EXPECT(member->empty ());
        }

//更新太快：应失败
        BEAST_EXPECT(! c->update (node, name, load, tick));
        {
            auto member = c->member (node);
            BEAST_EXPECT(static_cast<bool>(member));
            BEAST_EXPECT(member->empty ());
        }

        using namespace std::chrono_literals;

//更新名称（空更新为非空）
        tick += 1s;
        BEAST_EXPECT(c->update (node, name, load, tick));
        {
            auto member = c->member (node);
            BEAST_EXPECT(static_cast<bool>(member));
            BEAST_EXPECT(member->compare(name) == 0);
        }

//更新名称（非空不会变为空）
        tick += 1s;
        BEAST_EXPECT(c->update (node, "", load, tick));
        {
            auto member = c->member (node);
            BEAST_EXPECT(static_cast<bool>(member));
            BEAST_EXPECT(member->compare(name) == 0);
        }

//更新名称（非空更新为新的非空）
        tick += 1s;
        BEAST_EXPECT(c->update (node, "test", load, tick));
        {
            auto member = c->member (node);
            BEAST_EXPECT(static_cast<bool>(member));
            BEAST_EXPECT(member->compare("test") == 0);
        }
    }

    void
    testConfigLoad ()
    {
        testcase ("Config Load");

        auto c = std::make_unique <Cluster> (journal_);

//网络上的服务器
        std::vector<PublicKey> network;

        while (network.size () != 8)
            network.push_back (randomNode());

        auto format = [](
            PublicKey const &publicKey,
            char const* comment = nullptr)
        {
            auto ret = toBase58(
                TokenType::NodePublic,
                publicKey);

            if (comment)
                ret += comment;

            return ret;
        };

        Section s1;

//正确（空）配置
        BEAST_EXPECT(c->load (s1));
        BEAST_EXPECT(c->size() == 0);

//正确配置
        s1.append (format (network[0]));
        s1.append (format (network[1], "    "));
        s1.append (format (network[2], " Comment"));
        s1.append (format (network[3], " Multi Word Comment"));
        s1.append (format (network[4], "  Leading Whitespace"));
        s1.append (format (network[5], " Trailing Whitespace  "));
        s1.append (format (network[6], "  Leading & Trailing Whitespace  "));
        s1.append (format (network[7], "  Leading,  Trailing  &  Internal  Whitespace  "));

        BEAST_EXPECT(c->load (s1));

        for (auto const& n : network)
            BEAST_EXPECT(c->member (n));

//配置不正确
        Section s2;
        s2.append ("NotAPublicKey");
        BEAST_EXPECT(!c->load (s2));

        Section s3;
        s3.append (format (network[0], "!"));
        BEAST_EXPECT(!c->load (s3));

        Section s4;
        s4.append (format (network[0], "!  Comment"));
        BEAST_EXPECT(!c->load (s4));

//检查当我们遇到
//不正确或不可解析的条目：
        auto const node1 = randomNode();
        auto const node2 = randomNode ();

        Section s5;
        s5.append (format (node1, "XXX"));
        s5.append (format (node2));
        BEAST_EXPECT(!c->load (s5));
        BEAST_EXPECT(!c->member (node1));
        BEAST_EXPECT(!c->member (node2));
    }

    void
    run() override
    {
        testMembership ();
        testUpdating ();
        testConfigLoad ();
    }
};

BEAST_DEFINE_TESTSUITE(cluster,overlay,ripple);

} //测验
} //涟漪
