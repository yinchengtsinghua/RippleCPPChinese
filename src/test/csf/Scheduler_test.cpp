
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

#include <ripple/beast/unit_test.h>
#include <test/csf/Scheduler.h>
#include <set>

namespace ripple {
namespace test {

class Scheduler_test : public beast::unit_test::suite
{
public:

    void
    run() override
    {
        using namespace std::chrono_literals;
        csf::Scheduler scheduler;
        std::set<int> seen;

        scheduler.in(1s, [&]{ seen.insert(1);});
        scheduler.in(2s, [&]{ seen.insert(2);});
        auto token = scheduler.in(3s, [&]{ seen.insert(3);});
        scheduler.at(scheduler.now() + 4s, [&]{ seen.insert(4);});
        scheduler.at(scheduler.now() + 8s, [&]{ seen.insert(8);});

        auto start = scheduler.now();

//处理第一个事件
        BEAST_EXPECT(seen.empty());
        BEAST_EXPECT(scheduler.step_one());
        BEAST_EXPECT(seen == std::set<int>({1}));
        BEAST_EXPECT(scheduler.now() == (start + 1s));

//如果步进到当前时间，则不处理
        BEAST_EXPECT(scheduler.step_until(scheduler.now()));
        BEAST_EXPECT(seen == std::set<int>({1}));
        BEAST_EXPECT(scheduler.now() == (start + 1s));

//处理下一个事件
        BEAST_EXPECT(scheduler.step_for(1s));
        BEAST_EXPECT(seen == std::set<int>({1,2}));
        BEAST_EXPECT(scheduler.now() == (start + 2s));

//不处理取消的事件，但提前时钟
        scheduler.cancel(token);
        BEAST_EXPECT(scheduler.step_for(1s));
        BEAST_EXPECT(seen == std::set<int>({1,2}));
        BEAST_EXPECT(scheduler.now() == (start + 3s));

//处理到3个整数
        BEAST_EXPECT(scheduler.step_while([&]() { return seen.size() < 3; }));
        BEAST_EXPECT(seen == std::set<int>({1,2,4}));
        BEAST_EXPECT(scheduler.now() == (start + 4s));

//处理剩下的
        BEAST_EXPECT(scheduler.step());
        BEAST_EXPECT(seen == std::set<int>({1,2,4,8}));
        BEAST_EXPECT(scheduler.now() == (start + 8s));

//剩下的再处理不前进
        BEAST_EXPECT(!scheduler.step());
        BEAST_EXPECT(seen == std::set<int>({1,2,4,8}));
        BEAST_EXPECT(scheduler.now() == (start + 8s));
    }
};

BEAST_DEFINE_TESTSUITE(Scheduler, test, ripple);

}  //命名空间测试
}  //命名空间波纹
