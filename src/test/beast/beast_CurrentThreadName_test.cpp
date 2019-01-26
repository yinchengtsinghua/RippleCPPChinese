
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
    版权所有（c）2017 Ripple Labs Inc.

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

#include <ripple/beast/core/CurrentThreadName.h>
#include <ripple/beast/unit_test.h>
#include <atomic>
#include <thread>

namespace ripple {
namespace test {

class CurrentThreadName_test : public beast::unit_test::suite
{
private:
    static void exerciseName (
        std::string myName, std::atomic<bool>* stop, std::atomic<int>* state)
    {
//验证线程在创建时没有名称。
        auto const initialThreadName = beast::getCurrentThreadName();

//设置新名称。
        beast::setCurrentThreadName (myName);

//向调用者指示已设置名称。
        *state = 1;

//如果有一个初始线程名，那么我们失败了。
        if (initialThreadName)
            return;

//等待所有线程都有它们的名称。
        while (! *stop);

//确保我们之前设置的线程名仍然存在
//（例如，不被另一个线程覆盖）。
        if (beast::getCurrentThreadName() == myName)
            *state = 2;
    }

public:
    void
    run() override
    {
//用两个不同的名称制作两个不同的线程。确保
//当线程
//出口。
        std::atomic<bool> stop {false};

        std::atomic<int> stateA {0};
        std::thread tA (exerciseName, "tA", &stop, &stateA);

        std::atomic<int> stateB {0};
        std::thread tB (exerciseName, "tB", &stop, &stateB);

//等待两个线程都设置了名称。
        while (stateA == 0 || stateB == 0);

        stop = true;
        tA.join();
        tB.join();

//两个线程退出时仍应具有预期的名称。
        BEAST_EXPECT (stateA == 2);
        BEAST_EXPECT (stateB == 2);
    }
};

BEAST_DEFINE_TESTSUITE(CurrentThreadName,core,beast);

} //测试
} //涟漪
