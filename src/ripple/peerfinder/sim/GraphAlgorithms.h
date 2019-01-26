
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

#ifndef RIPPLE_PEERFINDER_SIM_GRAPHALGORITHMS_H_INCLUDED
#define RIPPLE_PEERFINDER_SIM_GRAPHALGORITHMS_H_INCLUDED

namespace ripple {
namespace PeerFinder {
namespace Sim {

template <typename Vertex>
struct VertexTraits;

/*为连接图中的每个顶点调用一个函数。
    将使用此签名调用函数：
        空隙（顶点和，标准：尺寸直径）；
**/


template <typename Vertex, typename Function>
void breadth_first_traverse (Vertex& start, Function f)
{
    using Traits = VertexTraits <Vertex>;
    using Edges  = typename Traits::Edges;
    using Edge   = typename Traits::Edge;

    using Probe = std::pair <Vertex*, int>;
    using Work = std::deque <Probe>;
    using Visited = std::set <Vertex*>;
    Work work;
    Visited visited;
    work.emplace_back (&start, 0);
    int diameter (0);
    while (! work.empty ())
    {
        Probe const p (work.front());
        work.pop_front ();
        if (visited.find (p.first) != visited.end ())
            continue;
        diameter = std::max (p.second, diameter);
        visited.insert (p.first);
        for (typename Edges::iterator iter (
            Traits::edges (*p.first).begin());
                iter != Traits::edges (*p.first).end(); ++iter)
        {
            Vertex* v (Traits::vertex (*iter));
            if (visited.find (v) != visited.end())
                continue;
            if (! iter->closed())
                work.emplace_back (v, p.second + 1);
        }
        f (*p.first, diameter);
    }
}


}
}
}

#endif
