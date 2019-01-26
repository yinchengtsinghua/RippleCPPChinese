
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
#include <ripple/protocol/JsonFields.h>
#include <ripple/overlay/Cluster.h>
#include <ripple/overlay/Overlay.h>
#include <unordered_map>

namespace ripple {

class Peers_test : public beast::unit_test::suite
{
    void testRequest()
    {
        testcase("Basic request");
        using namespace test::jtx;
        Env env {*this};

//在不修改集群的情况下，需要一个空集
//来自此请求
        auto peers = env.rpc("peers")[jss::result];
        BEAST_EXPECT(peers.isMember(jss::cluster) &&
            peers[jss::cluster].size() == 0 );
        BEAST_EXPECT(peers.isMember(jss::peers) &&
            peers[jss::peers].isNull());

//在群集中插入一些节点
        std::unordered_map<std::string, std::string> nodes;
        for(auto i =0; i < 3; ++i)
        {

            auto kp = generateKeyPair (KeyType::secp256k1,
                generateSeed("seed" + std::to_string(i)));

            std::string name = "Node " + std::to_string(i);

            using namespace std::chrono_literals;
            env.app().cluster().update(
                kp.first,
                name,
                200,
                env.timeKeeper().now() - 10s);
            nodes.insert( std::make_pair(
                toBase58(TokenType::NodePublic, kp.first), name));
        }

//发出请求，验证我们创建的节点是否匹配
//有什么报道
        peers = env.rpc("peers")[jss::result];
        if(! BEAST_EXPECT(peers.isMember(jss::cluster)))
            return;
        if(! BEAST_EXPECT(peers[jss::cluster].size() == nodes.size()))
            return;
        for(auto it  = peers[jss::cluster].begin();
                 it != peers[jss::cluster].end();
                 ++it)
        {
            auto key = it.key().asString();
            auto search = nodes.find(key);
            if(! BEAST_EXPECTS(search != nodes.end(), key))
                continue;
            if(! BEAST_EXPECT((*it).isMember(jss::tag)))
                continue;
            auto tag = (*it)[jss::tag].asString();
            BEAST_EXPECTS((*it)[jss::tag].asString() == nodes[key], key);
        }
        BEAST_EXPECT(peers.isMember(jss::peers) && peers[jss::peers].isNull());
    }

public:
    void run () override
    {
        testRequest();
    }
};

BEAST_DEFINE_TESTSUITE (Peers, rpc, ripple);

}  //涟漪
