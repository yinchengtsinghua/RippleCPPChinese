
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
    版权所有（c）2017 Ripple Labs Inc.

    许可目标使用、复制、修改和/或分发本软件
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

#include <ripple/beast/unit_test.h>
#include <test/csf/Digraph.h>
#include <vector>
#include <string>

namespace ripple {
namespace test {

class Digraph_test : public beast::unit_test::suite
{
public:

    void
    run() override
    {
        using namespace csf;
        using Graph = Digraph<char,std::string>;
        Graph graph;

        BEAST_EXPECT(!graph.connected('a', 'b'));
        BEAST_EXPECT(!graph.edge('a', 'b'));
        BEAST_EXPECT(!graph.disconnect('a', 'b'));

        BEAST_EXPECT(graph.connect('a', 'b', "foobar"));
        BEAST_EXPECT(graph.connected('a', 'b'));
        BEAST_EXPECT(*graph.edge('a', 'b') == "foobar");

        BEAST_EXPECT(!graph.connect('a', 'b', "repeat"));
        BEAST_EXPECT(graph.disconnect('a', 'b'));
        BEAST_EXPECT(graph.connect('a', 'b', "repeat"));
        BEAST_EXPECT(graph.connected('a', 'b'));
        BEAST_EXPECT(*graph.edge('a', 'b') == "repeat");


        BEAST_EXPECT(graph.connect('a', 'c', "tree"));

        {
            std::vector<std::tuple<char, char, std::string>> edges;

            for (auto const & edge : graph.outEdges('a'))
            {
                edges.emplace_back(edge.source, edge.target, edge.data);
            }

            std::vector<std::tuple<char, char, std::string>> expected;
            expected.emplace_back('a', 'b', "repeat");
            expected.emplace_back('a', 'c', "tree");
            BEAST_EXPECT(edges == expected);
            BEAST_EXPECT(graph.outDegree('a') == expected.size());
        }

        BEAST_EXPECT(graph.outEdges('r').size() == 0);
        BEAST_EXPECT(graph.outDegree('r') == 0);
        BEAST_EXPECT(graph.outDegree('c') == 0);

//只有“A”有外缘
        BEAST_EXPECT(graph.outVertices().size() == 1);
        std::vector<char> expected = {'b','c'};

        BEAST_EXPECT((graph.outVertices('a') == expected));
        BEAST_EXPECT(graph.outVertices('b').size() == 0);
        BEAST_EXPECT(graph.outVertices('c').size() == 0);
        BEAST_EXPECT(graph.outVertices('r').size() == 0);

        std::stringstream ss;
        graph.saveDot(ss, [](char v) { return v;});
        std::string expectedDot = "digraph {\n"
        "a -> b;\n"
        "a -> c;\n"
        "}\n";
        BEAST_EXPECT(ss.str() == expectedDot);


    }
};

BEAST_DEFINE_TESTSUITE(Digraph, test, ripple);

}  //命名空间测试
}  //命名空间波纹
