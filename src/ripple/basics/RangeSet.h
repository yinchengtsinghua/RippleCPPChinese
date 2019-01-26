
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

#ifndef RIPPLE_BASICS_RANGESET_H_INCLUDED
#define RIPPLE_BASICS_RANGESET_H_INCLUDED

#include <string>
#include <boost/optional.hpp>
#include <boost/icl/closed_interval.hpp>
#include <boost/icl/interval_set.hpp>
#include <boost/serialization/split_free.hpp>

namespace ripple
{

/*域t上的闭合间隔。

    对于一个实例closed interval c，这表示关闭的间隔
    （c.first（），c.last（））。单个元素间隔具有c.first（）==c.last（）。

    这只是BoostInterval容器库间隔的类型别名
    设置，因此用户应该参考该文档以获得可用的支持
    成员和自由函数。
**/

template <class T>
using ClosedInterval = boost::icl::closed_interval<T>;

/*创建闭合范围间隔

    帮助函数创建一个闭合范围间隔而不必限定
    模板参数。
**/

template <class T>
ClosedInterval<T>
range(T low, T high)
{
    return ClosedInterval<T>(low, high);
}

/*域T上的一组闭合间隔。

    用最小数表示域t的一组值
    脱节闭合间隙<t>这对于表示
    t如果缺少一些实例，例如集合1-5、8-9、11-14。

    这只是BoostInterval容器库间隔的类型别名
    设置，因此用户应该参考该文档以获得可用的支持
    成员和自由函数。
**/

template <class T>
using RangeSet = boost::icl::interval_set<T, std::less, ClosedInterval<T>>;


/*将closedinterval转换为样式化字符串

    样式字符串是
        “c.first（）-c.last（）”如果c.first（）！= c）（）
        “c.first（）”如果c.first（）==c.last（））

    @param ci要转换的关闭间隔
    @返回样式字符串
**/

template <class T>
std::string to_string(ClosedInterval<T> const & ci)
{
    if (ci.first() == ci.last())
        return std::to_string(ci.first());
    return std::to_string(ci.first()) + "-" + std::to_string(ci.last());
}

/*将给定的范围集转换为样式化字符串。

    样式化字符串表示是由
    逗号。如果集合为空，则返回字符串“empty”。

    @param rs要转换的范围集
    @返回样式字符串
**/

template <class T>
std::string to_string(RangeSet<T> const & rs)
{
    using ripple::to_string;

    if (rs.empty())
        return "empty";
    std::string res = "";
    for (auto const & interval : rs)
    {
        if (!res.empty())
            res += ",";
        res += to_string(interval);
    }
    return res;
}

/*查找不在小于给定值的集合中的最大值。

    @param rs兴趣集
    @param t必须大于结果的值
    @param minval（默认值为0）允许的最小值
    @返回最大的V，使minv<=v<t和！包含（rs，v）或
            如果不存在这样的V，则无。
**/

template <class T>
boost::optional<T>
prevMissing(RangeSet<T> const & rs, T t, T minVal = 0)
{
    if (rs.empty() || t == minVal)
        return boost::none;
    RangeSet<T> tgt{ ClosedInterval<T>{minVal, t - 1} };
    tgt -= rs;
    if (tgt.empty())
        return boost::none;
    return boost::icl::last(tgt);
}
}  //命名空间波纹


//Boost序列化文档建议放置免费的函数助手
//在boost序列化命名空间中

namespace boost {
namespace serialization {
template <class Archive, class T>
void
save(Archive& ar,
    ripple::ClosedInterval<T> const& ci,
    const unsigned int version)
{
    auto l = ci.lower();
    auto u = ci.upper();
    ar << l << u;
}

template <class Archive, class T>
void
load(Archive& ar, ripple::ClosedInterval<T>& ci, const unsigned int version)
{
    T low, up;
    ar >> low >> up;
    ci = ripple::ClosedInterval<T>{low, up};
}

template <class Archive, class T>
void
serialize(Archive& ar,
    ripple::ClosedInterval<T>& ci,
    const unsigned int version)
{
    split_free(ar, ci, version);
}

template <class Archive, class T>
void
save(Archive& ar, ripple::RangeSet<T> const& rs, const unsigned int version)
{
    auto s = rs.iterative_size();
    ar << s;
    for (auto const& r : rs)
        ar << r;
}

template <class Archive, class T>
void
load(Archive& ar, ripple::RangeSet<T>& rs, const unsigned int version)
{
    rs.clear();
    std::size_t intervals;
    ar >> intervals;
    for (std::size_t i = 0; i < intervals; ++i)
    {
        ripple::ClosedInterval<T> ci;
        ar >> ci;
        rs.insert(ci);
    }
}

template <class Archive, class T>
void
serialize(Archive& ar, ripple::RangeSet<T>& rs, const unsigned int version)
{
    split_free(ar, rs, version);
}

}  //串行化
}  //促进
#endif
