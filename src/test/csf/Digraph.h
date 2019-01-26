
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

#ifndef RIPPLE_TEST_CSF_DIGRAPH_H_INCLUDED
#define RIPPLE_TEST_CSF_DIGRAPH_H_INCLUDED

#include <boost/container/flat_map.hpp>
#include <boost/optional.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>
#include <fstream>
#include <unordered_map>
#include <type_traits>

namespace ripple {
namespace test {
namespace csf {

namespace detail {
//当图形不需要边缘数据时使用虚拟类
struct NoEdgeData
{
};

}  //命名空间详细信息

/*有向图

使用邻接列表表示边缘的基本有向图。

顶点的实例唯一地标识图形中的顶点。实例
EdgeData是在连接两个顶点的边缘中存储的任何数据。

Vertex和EdgeData都应该是轻量级的，并且复制成本低。

**/

template <class Vertex, class EdgeData = detail::NoEdgeData>
class Digraph
{
    using Links = boost::container::flat_map<Vertex, EdgeData>;
    using Graph = boost::container::flat_map<Vertex, Links>;
    Graph graph_;

//允许返回未知顶点的空ITerAbles
    Links empty;

public:
    /*连接两个顶点

        @param source源顶点
        @param target目标顶点
        @param e边缘数据
        @如果创建了边，返回true

    **/

    bool
    connect(Vertex source, Vertex target, EdgeData e)
    {
        return graph_[source].emplace(target, e).second;
    }

    /*使用默认的构造边缘数据连接两个顶点

        @param source源顶点
        @param target目标顶点
        @如果创建了边，返回true

    **/

    bool
    connect(Vertex source, Vertex target)
    {
        return connect(source, target, EdgeData{});
    }

    /*断开两个顶点

        @param source源顶点
        @param target目标顶点
        @如果删除了边，返回真值

        如果源未连接到目标，则此函数不执行任何操作。
    **/

    bool
    disconnect(Vertex source, Vertex target)
    {
        auto it = graph_.find(source);
        if (it != graph_.end())
        {
            return it->second.erase(target) > 0;
        }
        return false;
    }

    /*返回两个顶点之间的边数据

        @param source源顶点
        @param target目标顶点
        @return可选的<edge>如果不存在边，则为boost：：none

    **/

    boost::optional<EdgeData>
    edge(Vertex source, Vertex target) const
    {
        auto it = graph_.find(source);
        if (it != graph_.end())
        {
            auto edgeIt = it->second.find(target);
            if (edgeIt != it->second.end())
                return edgeIt->second;
        }
        return boost::none;
    }

    /*检查是否连接了两个顶点

        @param source源顶点
        @param target目标顶点
        @如果源有一个到目标的外缘，返回true
    **/

    bool
    connected(Vertex source, Vertex target) const
    {
        return edge(source, target) != boost::none;
    }

    /*图中顶点的范围

        @返回顶点上的一个增强变换范围，其中边在
                图表
    **/

    auto
    outVertices() const
    {
        return boost::adaptors::transform(
            graph_,
            [](typename Graph::value_type const& v) { return v.first; });
    }

    /*目标顶点范围

        @param source源顶点
        @返回源的目标顶点上的增强变换范围。
     **/

    auto
    outVertices(Vertex source) const
    {
        auto transform = [](typename Links::value_type const& link) {
            return link.first;
        };
        auto it = graph_.find(source);
        if (it != graph_.end())
            return boost::adaptors::transform(it->second, transform);

        return boost::adaptors::transform(empty, transform);
    }

    /*与边关联的顶点和数据
    **/

    struct Edge
    {
        Vertex source;
        Vertex target;
        EdgeData data;
    };

    /*外缘范围

        @param source源顶点
        @返回所有边缘的边缘类型的增强变换范围
                来源。
    **/

    auto
    outEdges(Vertex source) const
    {
        auto transform = [source](typename Links::value_type const& link) {
            return Edge{source, link.first, link.second};
        };

        auto it = graph_.find(source);
        if (it != graph_.end())
            return boost::adaptors::transform(it->second, transform);

        return boost::adaptors::transform(empty, transform);
    }

    /*顶点出度

        @param source源顶点
        @返回源的传出边缘数
    **/

    std::size_t
    outDegree(Vertex source) const
    {
        auto it = graph_.find(source);
        if (it != graph_.end())
            return it->second.size();
        return 0;
    }

    /*保存graphviz点文件

        保存图形的graphviz点描述
        @param filename输出文件（创建）
        @param vertexname可调用的t vertexname（vertex const&）
                          返回文件中顶点的目标名称
                          T必须是可排斥的
    **/

    template <class VertexName>
    void
    saveDot(std::ostream & out, VertexName&& vertexName) const
    {
        out << "digraph {\n";
        for (auto const& vData : graph_)
        {
            auto const fromName = vertexName(vData.first);
            for (auto const& eData : vData.second)
            {
                auto const toName = vertexName(eData.first);
                out << fromName << " -> " << toName << ";\n";
            }
        }
        out << "}\n";
    }

    template <class VertexName>
    void
    saveDot(std::string const& fileName, VertexName&& vertexName) const
    {
        std::ofstream out(fileName);
        saveDot(out, std::forward<VertexName>(vertexName));
    }
};

}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹
#endif
