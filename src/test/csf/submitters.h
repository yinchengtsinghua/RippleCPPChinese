
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
#ifndef RIPPLE_TEST_CSF_SUBMITTERS_H_INCLUDED
#define RIPPLE_TEST_CSF_SUBMITTERS_H_INCLUDED

#include <test/csf/SimTime.h>
#include <test/csf/Scheduler.h>
#include <test/csf/Peer.h>
#include <test/csf/Tx.h>
#include <type_traits>
namespace ripple {
namespace test {
namespace csf {

//提交者是模拟向网络提交事务的类。

/*将速率表示为计数/持续时间*/
struct Rate
{
    std::size_t count;
    SimDuration duration;

    double
    inv() const
    {
        return duration.count()/double(count);
    }
};

/*向指定的对等方提交事务

    提交从开始时开始的连续事务，然后根据
    成功调用distribution（），直到停止。

    @tparam distribution是STL中的“uniformrandombitgenerator”
            被随机分布用于生成随机样本
    @tparam generator是具有成员的对象

            T operator（）（发电机和G）

            它产生以同步时间单位到下一个单位的延迟t
            交易。对于当前的SimDuration定义，这是
            当前的纳秒数。提交者内部强制转换
            算术T到SimDuration：：表示允许使用标准的单位
            作为分发的库分发。
**/

template <class Distribution, class Generator, class Selector>
class Submitter
{
    Distribution dist_;
    SimTime stop_;
    std::uint32_t nextID_ = 0;
    Selector selector_;
    Scheduler & scheduler_;
    Generator & g_;

//将生成的持续时间转换为SimDuration
    static SimDuration
    asDuration(SimDuration d)
    {
        return d;
    }

    template <class T>
    static
    std::enable_if_t<std::is_arithmetic<T>::value, SimDuration>
    asDuration(T t)
    {
        return SimDuration{static_cast<SimDuration::rep>(t)};
    }

    void
    submit()
    {
        selector_()->submit(Tx{nextID_++});
        if (scheduler_.now() < stop_)
        {
            scheduler_.in(asDuration(dist_(g_)), [&]() { submit(); });
        }
    }

public:
    Submitter(
        Distribution dist,
        SimTime start,
        SimTime end,
        Selector & selector,
        Scheduler & s,
        Generator & g)
        : dist_{dist}, stop_{end}, selector_{selector}, scheduler_{s}, g_{g}
    {
        scheduler_.at(start, [&]() { submit(); });
    }
};

template <class Distribution, class Generator, class Selector>
Submitter<Distribution, Generator, Selector>
makeSubmitter(
    Distribution dist,
    SimTime start,
    SimTime end,
    Selector& sel,
    Scheduler& s,
    Generator& g)
{
    return Submitter<Distribution, Generator, Selector>(
            dist, start ,end, sel, s, g);
}

}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹

#endif
