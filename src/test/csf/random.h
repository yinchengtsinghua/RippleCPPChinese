
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

#ifndef RIPPLE_TEST_CSF_RANDOM_H_INCLUDED
#define RIPPLE_TEST_CSF_RANDOM_H_INCLUDED

#include <random>
#include <vector>

namespace ripple {
namespace test {
namespace csf {

/*返回基于权重w的随机洗牌向量副本。

    @param v值集
    @param w每个值的权重集
    @param g伪随机数生成器
    @返回一个向量，随机抽取条目，不替换
            根据提供的权重从原始向量。
            即，Res[0]来自重量为w[i]的样品v[i]/和k w[k]
**/

template <class T, class G>
std::vector<T>
random_weighted_shuffle(std::vector<T> v, std::vector<double> w, G& g)
{
    using std::swap;

    for (int i = 0; i < v.size() - 1; ++i)
    {
//选取一个加权为w的随机项
        std::discrete_distribution<> dd(w.begin() + i, w.end());
        auto idx = dd(g);
        std::swap(v[i], v[idx]);
        std::swap(w[i], w[idx]);
    }
    return v;
}

/*生成随机样本的向量

    @param size样本的大小
    @param dist要采样的分布
    @param g伪随机数生成器

    @样本返回向量
**/

template <class RandomNumberDistribution, class Generator>
std::vector<typename RandomNumberDistribution::result_type>
sample( std::size_t size, RandomNumberDistribution dist, Generator& g)
{
    std::vector<typename RandomNumberDistribution::result_type> res(size);
    std::generate(res.begin(), res.end(), [&dist, &g]() { return dist(g); });
    return res;
}

/*根据离散的
    分布

    给定一对随机访问迭代器，每次调用
    选择器实例返回范围内的随机项（开始、结束）
    根据施工时提供的重量。
**/

template <class RAIter, class Generator>
class Selector
{
    RAIter first_, last_;
    std::discrete_distribution<> dd_;
    Generator g_;

public:
    /*构造函数
        @param first random access迭代器到范围的开始
        到范围结尾的@param last random access迭代器
        @param w首先是大小列表权重的向量
        @param g伪随机数生成器
    **/

    Selector(RAIter first, RAIter last, std::vector<double> const& w,
            Generator& g)
      : first_{first}, last_{last}, dd_{w.begin(), w.end()}, g_{g}
    {
        using tag = typename std::iterator_traits<RAIter>::iterator_category;
        static_assert(
                std::is_same<tag, std::random_access_iterator_tag>::value,
                "Selector only supports random access iterators.");
//TODO:允许转发迭代器
    }

    typename std::iterator_traits<RAIter>::value_type
    operator()()
    {
        auto idx = dd_(g_);
        return *(first_ + idx);
    }
};

template <typename Iter, typename Generator>
Selector<Iter,Generator>
makeSelector(Iter first, Iter last, std::vector<double> const& w, Generator& g)
{
    return Selector<Iter, Generator>(first, last, w, g);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//在中未定义的额外利息分配

/*始终返回相同值的常量“分布”
**/

class ConstantDistribution
{
    double t_;

public:
    ConstantDistribution(double const& t) : t_{t}
    {
    }

    template <class Generator>
    inline double
    operator()(Generator& )
    {
        return t_;
    }
};

/*PDF格式的幂律分布

        p（x）=（x/xmin）^-a

    对于a>=1和xmin>=1
 **/

class PowerLawDistribution
{
    double xmin_;
    double a_;
    double inv_;
    std::uniform_real_distribution<double> uf_{0, 1};

public:

    using result_type = double;

    PowerLawDistribution(double xmin, double a) : xmin_{xmin}, a_{a}
    {
        inv_ = 1.0 / (1.0 - a_);
    }

    template <class Generator>
    inline double
    operator()(Generator& g)
    {
//利用CDF的逆变换进行采样
//CDF是p（x<=x）：1-（x/xmin）^（1-a）
        return xmin_ * std::pow(1 - uf_(g), inv_);
    }
};

}  //脑脊液
}  //测试
}  //涟漪

#endif
