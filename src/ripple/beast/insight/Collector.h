
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

#ifndef BEAST_INSIGHT_COLLECTOR_H_INCLUDED
#define BEAST_INSIGHT_COLLECTOR_H_INCLUDED

#include <ripple/beast/insight/Counter.h>
#include <ripple/beast/insight/Event.h>
#include <ripple/beast/insight/Gauge.h>
#include <ripple/beast/insight/Hook.h>
#include <ripple/beast/insight/Meter.h>

#include <string>

namespace beast {
namespace insight {

/*用于允许收集度量的管理器的接口。

    若要从类中导出度量，请将共享指针传递并保存到此指针
    类构造函数中的接口。创建度量对象
    根据需要（计数器、事件、仪表、仪表和可选挂钩）
    使用接口。

    @见计数器、事件、仪表、挂钩、仪表
    @请参阅nullcollector、statsdcollector
**/

class Collector
{
public:
    using ptr = std::shared_ptr <Collector>;

    virtual ~Collector() = 0;

    /*创建一个钩子。

        在每个收集间隔，在一个实现上调用一个钩子
        定义的线程。这是收集度量的便利设施
        以投票方式。典型的用法是更新所有度量
        对处理程序感兴趣。

        将使用此签名调用处理程序：
            void处理程序（void）

        “见钩”
    **/

    /*@ {*/
    template <class Handler>
    Hook make_hook (Handler handler)
    {
        return make_hook (HookImpl::HandlerType (handler));
    }

    virtual Hook make_hook (HookImpl::HandlerType const& handler) = 0;
    /*@ }*/

    /*创建具有指定名称的计数器。
        见计数器
    **/

    /*@ {*/
    virtual Counter make_counter (std::string const& name) = 0;

    Counter make_counter (std::string const& prefix, std::string const& name)
    {
        if (prefix.empty ())
            return make_counter (name);
        return make_counter (prefix + "." + name);
    }
    /*@ }*/

    /*使用指定的名称创建事件。
        “看事件”
    **/

    /*@ {*/
    virtual Event make_event (std::string const& name) = 0;

    Event make_event (std::string const& prefix, std::string const& name)
    {
        if (prefix.empty ())
            return make_event (name);
        return make_event (prefix + "." + name);
    }
    /*@ }*/

    /*创建具有指定名称的仪表。
        @见规
    **/

    /*@ {*/
    virtual Gauge make_gauge (std::string const& name) = 0;

    Gauge make_gauge (std::string const& prefix, std::string const& name)
    {
        if (prefix.empty ())
            return make_gauge (name);
        return make_gauge (prefix + "." + name);
    }
    /*@ }*/

    /*创建具有指定名称的仪表。
        见表
    **/

    /*@ {*/
    virtual Meter make_meter (std::string const& name) = 0;

    Meter make_meter (std::string const& prefix, std::string const& name)
    {
        if (prefix.empty ())
            return make_meter (name);
        return make_meter (prefix + "." + name);
    }
    /*@ }*/
};

}
}

#endif
