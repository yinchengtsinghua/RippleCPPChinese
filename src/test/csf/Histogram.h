
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
#ifndef RIPPLE_TEST_CSF_HISTOGRAM_H_INCLUDED
#define RIPPLE_TEST_CSF_HISTOGRAM_H_INCLUDED

#include <map>
#include <chrono>
#include <algorithm>

namespace ripple {
namespace test {
namespace csf {

/*基本柱状图。

    满足的类型“t”的柱状图
      -默认结构：t
      -比较：t a，b；bool res=a<b
      ——加：t a，b；t c=a+b；
      -乘法：t a，std:：size_t b；t c=a*b；
      —除数：t a；std:：size_t b；t c=a/b；


**/

template <class T, class Compare = std::less<T>>
class Histogram
{
//TODO:如果此值变为
//不可缩放的
    std::map<T, std::size_t, Compare> counts_;
    std::size_t samples = 0;
public:
    /*插入一个示例*/
    void
    insert(T const & s)
    {
        ++counts_[s];
        ++samples;
    }

    /*样品数量*/
    std::size_t
    size() const
    {
        return samples;
    }

    /*不同样本（箱）的数量*/
    std::size_t
    numBins() const
    {
        return counts_.size();
    }

    /*最小观测值*/
    T
    minValue() const
    {
        return counts_.empty() ? T{} : counts_.begin()->first;
    }

    /*最大观测值*/
    T
    maxValue() const
    {
        return counts_.empty() ? T{} : counts_.rbegin()->first;
    }

    /*柱状图平均值*/
    T
    avg() const
    {
        T tmp{};
        if(samples == 0)
            return tmp;
//因为计数是经过分类的，所以不需要太担心数字
//错误
        for (auto const& it : counts_)
        {
            tmp += it.first * it.second;
        }
        return tmp/samples;
    }

    /*计算给定的分布百分比。

        @param p percentile介于0和1之间，例如0.50是第50个百分点
                 如果百分比落在两个箱子之间，则使用最近的箱子。
        @返回给定的分布百分比
    **/

    T
    percentile(float p) const
    {
        assert(p >= 0 && p <=1);
        std::size_t pos = std::round(p * samples);

        if(counts_.empty())
            return T{};

        auto it = counts_.begin();
        std::size_t cumsum = it->second;
        while (it != counts_.end() && cumsum < pos)
        {
            ++it;
            cumsum += it->second;
        }
        return it->first;
    }
};

}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹

#endif
