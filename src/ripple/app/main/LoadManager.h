
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

#ifndef RIPPLE_APP_MAIN_LOADMANAGER_H_INCLUDED
#define RIPPLE_APP_MAIN_LOADMANAGER_H_INCLUDED

#include <ripple/core/Stoppable.h>
#include <memory>
#include <mutex>
#include <thread>

namespace ripple {

class Application;

/*管理加载源。

    这个对象创建一个关联的线程来维护时钟。

    当服务器被某个特定的对等机超载时，它会发出警告。
    第一。这使得友好的同行减少了资源消耗，
    或者断开与服务器的连接。

    使用警告系统，而不是仅仅下降，因为敌意
    不管怎样，对等端都可以重新连接。
**/

class LoadManager : public Stoppable
{
    LoadManager (Application& app, Stoppable& parent, beast::Journal journal);

public:
    LoadManager () = delete;
    LoadManager (LoadManager const&) = delete;
    LoadManager& operator=(LoadManager const&) = delete;

    /*摧毁经理。

        析构函数仅在线程停止后返回。
    **/

    ~LoadManager ();

    /*打开死锁检测。

        死锁检测器以禁用状态开始。在这个函数之后
        调用时，它将使用单独的线程报告死锁
        复位功能不至少每10秒调用一次。

        @参见重置读锁检测器
    **/

//vvalco注意到死锁检测器似乎处于“待命”状态。
//在程序启动期间，如果
//初始化操作进行得很长？
//
    void activateDeadlockDetector ();

    /*重置死锁检测计时器。

        专用线程监视死锁计时器，如果太多
        时间过去了，它将产生日志警告。
    **/

    void resetDeadlockDetector ();

//——————————————————————————————————————————————————————————————

//可停止构件
    void onPrepare () override;

    void onStart () override;

    void onStop () override;

private:
    void run ();

private:
    Application& app_;
    beast::Journal journal_;

    std::thread thread_;
std::mutex mutex_;          //守卫死锁，武装起来，停止。

UptimeClock::time_point deadLock_;  //检测服务器死锁。
    bool armed_;
    bool stop_;

    friend
    std::unique_ptr<LoadManager>
    make_LoadManager(Application& app, Stoppable& parent, beast::Journal journal);
};

std::unique_ptr<LoadManager>
make_LoadManager (
    Application& app, Stoppable& parent, beast::Journal journal);

} //涟漪

#endif
