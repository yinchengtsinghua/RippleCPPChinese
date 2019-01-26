
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

#ifndef BEAST_INSIGHT_METER_H_INCLUDED
#define BEAST_INSIGHT_METER_H_INCLUDED

#include <memory>

#include <ripple/beast/insight/Base.h>
#include <ripple/beast/insight/MeterImpl.h>

namespace beast {
namespace insight {

/*测量整数值的度量单位。

    一个仪表可以被认为是一个只递增的计数器。

    这是一个轻量级的引用包装器，复制和分配成本很低。
    当最后一个引用消失时，将不再收集度量。
**/

class Meter : public Base
{
public:
    using value_type = MeterImpl::value_type;

    /*创建空度量。
        空度量没有报告任何信息。
    **/

    Meter ()
        { }

    /*创建指定实现的度量引用。
        通常不会直接调用。相反，调用适当的
        收集器接口中的工厂函数。
        见收藏家。
    **/

    explicit Meter (std::shared_ptr <MeterImpl> const& impl)
        : m_impl (impl)
        { }

    /*增加仪表。*/
    /*@ {*/
    void increment (value_type amount) const
    {
        if (m_impl)
            m_impl->increment (amount);
    }

    Meter const& operator+= (value_type amount) const
    {
        increment (amount);
        return *this;
    }

    Meter const& operator++ () const
    {
        increment (1);
        return *this;
    }

    Meter const& operator++ (int) const
    {
        increment (1);
        return *this;
    }
    /*@ }*/

    std::shared_ptr <MeterImpl> const& impl () const
    {
        return m_impl;
    }

private:
    std::shared_ptr <MeterImpl> m_impl;
};

}
}

#endif
