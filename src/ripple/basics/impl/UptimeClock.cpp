
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

#include <ripple/basics/UptimeClock.h>

namespace ripple {

std::atomic<UptimeClock::rep>  UptimeClock::now_{0};      //启动后的秒数
std::atomic<bool>              UptimeClock::stop_{false}; //停止更新线程

//在波纹状关闭时，取消并等待更新线程
UptimeClock::update_thread::~update_thread()
{
    if (joinable())
    {
        stop_ = true;
//此join（）可能需要1个，但只会发生
//一次在波纹停机。
        join();
    }
}

//启动更新线程
UptimeClock::update_thread
UptimeClock::start_clock()
{
    return update_thread{[]
                         {
                             using namespace std;
                             using namespace std::chrono;

//每秒钟醒来，立即更新\u
                             auto next = system_clock::now() + 1s;
                             while (!stop_)
                             {
                                 this_thread::sleep_until(next);
                                 next += 1s;
                                 ++now_;
                             }
                         }};
}

//这实际上测量的是从第一次使用开始的时间，而不是从波纹开始的时间。
//然而，这两个时代之间的差别只有一小部分时间
//不重要。

UptimeClock::time_point
UptimeClock::now()
{
//第一次使用时启动更新线程
    static const auto init = start_clock();

//返回自波纹启动后的秒数
    return time_point{duration{now_}};
}

} //涟漪
