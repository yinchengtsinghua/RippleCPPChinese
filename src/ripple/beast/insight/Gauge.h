
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_INSIGHT_GAUGE_H_INCLUDED
#define BEAST_INSIGHT_GAUGE_H_INCLUDED

#include <ripple/beast/insight/Base.h>
#include <ripple/beast/insight/GaugeImpl.h>

#include <memory>

namespace beast {
namespace insight {

/*测量整数值的度量单位。

    量表是对一个值的即时测量，就像气体量表一样。
    在汽车里。调用者直接设置值，或通过
    指定金额。值保存在客户机中，而不是收集器中。

    这是一个轻量级的引用包装器，复制和分配成本很低。
    当最后一个引用消失时，将不再收集度量。
**/

class Gauge : public Base
{
public:
    using value_type = GaugeImpl::value_type;
    using difference_type = GaugeImpl::difference_type;

    /*创建空度量。
        空度量没有报告任何信息。
    **/

    Gauge ()
    {
    }

    /*创建指定实现的度量引用。
        通常不会直接调用。相反，调用适当的
        收集器接口中的工厂函数。
        见收藏家。
    **/

    explicit Gauge (std::shared_ptr <GaugeImpl> const& impl)
        : m_impl (impl)
    {
    }

    /*设置仪表上的值。
        收集器实现应该结合多个值调用
        如果调用发生在单个
        收集间隔。
    **/

    /*@ {*/
    void set (value_type value) const
    {
        if (m_impl)
            m_impl->set (value);
    }

    Gauge const& operator= (value_type value) const
        { set (value); return *this; }
    /*@ }*/

    /*调整仪表值。*/
    /*@ {*/
    void increment (difference_type amount) const
    {
        if (m_impl)
            m_impl->increment (amount);
    }

    Gauge const&
    operator+= (difference_type amount) const
    {
        increment (amount);
        return *this;
    }

    Gauge const&
    operator-= (difference_type amount) const
    {
        increment (-amount);
        return *this;
    }

    Gauge const&
    operator++ () const
    {
        increment (1);
        return *this;
    }

    Gauge const&
    operator++ (int) const
    {
        increment (1);
        return *this;
    }

    Gauge const&
    operator-- () const
    {
        increment (-1);
        return *this;
    }

    Gauge const&
    operator-- (int) const
    {
        increment (-1);
        return *this;
    }
    /*@ }*/

    std::shared_ptr <GaugeImpl> const&
    impl () const
    {
        return m_impl;
    }

private:
    std::shared_ptr <GaugeImpl> m_impl;
};

}
}

#endif
