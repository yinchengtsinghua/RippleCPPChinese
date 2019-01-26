
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

#ifndef RIPPLE_CORE_LOADEVENT_H_INCLUDED
#define RIPPLE_CORE_LOADEVENT_H_INCLUDED

#include <chrono>
#include <memory>
#include <string>

namespace ripple {

class LoadMonitor;

//vvalco注意到loadEvent和loadMonitor之间的区别是什么？
//vfalc todo将loadEvent重命名为scopedloadsample
//
//这看起来像是一个范围已用时间度量类
//
class LoadEvent
{
public:
//vvalco todo删除对LoadMonitor的依赖。有可能吗？
    LoadEvent (LoadMonitor& monitor,
               std::string const& name,
               bool shouldStart);
    LoadEvent(LoadEvent const&) = delete;

    ~LoadEvent ();

    std::string const&
    name () const;

//等待的时间。
    std::chrono::steady_clock::duration
    waitTime() const;

//跑步的时间。
    std::chrono::steady_clock::duration
    runTime() const;

    void setName (std::string const& name);

//开始测量。如果已经开始，那么
//重新启动，将经过的时间分配给“等待”
//状态。
    void start ();

//停止测量并报告结果。这个
//报告的时间是从上次呼叫到
//开始。
    void stop ();

private:
    LoadMonitor& monitor_;

//代表我们当前的状态
    bool running_;

//与此事件关联的名称（如果有）。
    std::string name_;

//表示上次转换状态的时间
    std::chrono::steady_clock::time_point mark_;

//我们分别等待和奔跑的时间
    std::chrono::steady_clock::duration timeWaiting_;
    std::chrono::steady_clock::duration timeRunning_;
};

} //涟漪

#endif
