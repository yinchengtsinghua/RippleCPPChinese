
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

#include <ripple/core/ClosureCounter.h>
#include <ripple/beast/unit_test.h>
#include <test/jtx/Env.h>
#include <atomic>
#include <chrono>
#include <thread>

namespace ripple {
namespace test {

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class ClosureCounter_test : public beast::unit_test::suite
{
//我们只是在使用env作为它的日志。
    jtx::Env env {*this};
    beast::Journal j {env.app().journal ("ClosureCounter_test")};

    void testConstruction()
    {
//构建不同类型的闭包计数器。
        {
//计算返回void且不带参数的闭包数。
            ClosureCounter<void> voidCounter;
            BEAST_EXPECT (voidCounter.count() == 0);

            int evidence = 0;
//确保voidCounter.wrap与右值闭包一起工作。
            auto wrapped = voidCounter.wrap ([&evidence] () { ++evidence; });
            BEAST_EXPECT (voidCounter.count() == 1);
            BEAST_EXPECT (evidence == 0);
            BEAST_EXPECT (wrapped);

//wrapped（）应该可以在没有参数的情况下调用。
            (*wrapped)();
            BEAST_EXPECT (evidence == 1);
            (*wrapped)();
            BEAST_EXPECT (evidence == 2);

//销毁包装物应减少作废计数器。
            wrapped = boost::none;
            BEAST_EXPECT (voidCounter.count() == 0);
        }
        {
//count返回void并接受一个int参数的闭包。
            ClosureCounter<void, int> setCounter;
            BEAST_EXPECT (setCounter.count() == 0);

            int evidence = 0;
//确保setcounter.wrap与非常量左值闭包一起工作。
            auto setInt = [&evidence] (int i) { evidence = i; };
            auto wrapped = setCounter.wrap (setInt);

            BEAST_EXPECT (setCounter.count() == 1);
            BEAST_EXPECT (evidence == 0);
            BEAST_EXPECT (wrapped);

//wrapped（）应该可以用一个整型参数调用。
            (*wrapped)(5);
            BEAST_EXPECT (evidence == 5);
            (*wrapped)(11);
            BEAST_EXPECT (evidence == 11);

//销毁包装的内容应减少setcounter。
            wrapped = boost::none;
            BEAST_EXPECT (setCounter.count() == 0);
        }
        {
//count返回int并接受两个int参数的闭包。
            ClosureCounter<int, int, int> sumCounter;
            BEAST_EXPECT (sumCounter.count() == 0);

//确保sumcounter.wrap使用常量左值闭包。
            auto const sum = [] (int i, int j) { return i + j; };
            auto wrapped = sumCounter.wrap (sum);

            BEAST_EXPECT (sumCounter.count() == 1);
            BEAST_EXPECT (wrapped);

//wrapped（）应该可以用两个整数调用。
            BEAST_EXPECT ((*wrapped)(5,  2) ==  7);
            BEAST_EXPECT ((*wrapped)(2, -8) == -6);

//销毁包裹的内容物应减少消耗量。
            wrapped = boost::none;
            BEAST_EXPECT (sumCounter.count() == 0);
        }
    }

//用于测试参数传递的类。
    class TrackedString
    {
    public:
        int copies = {0};
        int moves = {0};
        std::string str;

        TrackedString() = delete;

        explicit TrackedString(char const* rhs)
        : str (rhs) {}

//复制构造函数
        TrackedString (TrackedString const& rhs)
        : copies (rhs.copies + 1)
        , moves (rhs.moves)
        , str (rhs.str) {}

//移动构造函数
        TrackedString (TrackedString&& rhs) noexcept
        : copies (rhs.copies)
        , moves (rhs.moves + 1)
        , str (std::move(rhs.str)) {}

//删除复制和移动工作分配。
        TrackedString& operator=(TrackedString const& rhs) = delete;

//字符串连接
        TrackedString& operator+=(char const* rhs)
        {
            str += rhs;
            return *this;
        }

        friend
        TrackedString operator+(TrackedString const& str, char const* rhs)
        {
            TrackedString ret {str};
            ret.str += rhs;
            return ret;
        }
    };

    void testArgs()
    {
//确保包装的闭包处理右值引用参数
//正确地。
        {
//通过值。
            ClosureCounter<TrackedString, TrackedString> strCounter;
            BEAST_EXPECT (strCounter.count() == 0);

            auto wrapped = strCounter.wrap (
                [] (TrackedString in) { return in += "!"; });

            BEAST_EXPECT (strCounter.count() == 1);
            BEAST_EXPECT (wrapped);

            TrackedString const strValue ("value");
            TrackedString const result = (*wrapped)(strValue);
            BEAST_EXPECT (result.copies == 2);
            BEAST_EXPECT (result.moves == 1);
            BEAST_EXPECT (result.str == "value!");
            BEAST_EXPECT (strValue.str.size() == 5);
        }
        {
//使用常量左值参数。
            ClosureCounter<TrackedString, TrackedString const&> strCounter;
            BEAST_EXPECT (strCounter.count() == 0);

            auto wrapped = strCounter.wrap (
                [] (TrackedString const& in) { return in + "!"; });

            BEAST_EXPECT (strCounter.count() == 1);
            BEAST_EXPECT (wrapped);

            TrackedString const strConstLValue ("const lvalue");
            TrackedString const result = (*wrapped)(strConstLValue);
            BEAST_EXPECT (result.copies == 1);
//野兽期待（result.moves==？）；//移动vs==1，gcc==0
            BEAST_EXPECT (result.str == "const lvalue!");
            BEAST_EXPECT (strConstLValue.str.size() == 12);
        }
        {
//使用非常量左值参数。
            ClosureCounter<TrackedString, TrackedString&> strCounter;
            BEAST_EXPECT (strCounter.count() == 0);

            auto wrapped = strCounter.wrap (
                [] (TrackedString& in) { return in += "!"; });

            BEAST_EXPECT (strCounter.count() == 1);
            BEAST_EXPECT (wrapped);

            TrackedString strLValue ("lvalue");
            TrackedString const result = (*wrapped)(strLValue);
            BEAST_EXPECT (result.copies == 1);
            BEAST_EXPECT (result.moves == 0);
            BEAST_EXPECT (result.str == "lvalue!");
            BEAST_EXPECT (strLValue.str == result.str);
        }
        {
//使用右值参数。
            ClosureCounter<TrackedString, TrackedString&&> strCounter;
            BEAST_EXPECT (strCounter.count() == 0);

            auto wrapped = strCounter.wrap (
                [] (TrackedString&& in) {
//注意，没有一个编译器注意到
//离开范围。因此，如果没有干预，他们会
//复印报税表（2017年6月）。明确的
//std:：move（）是必需的。
                    return std::move(in += "!");
                });

            BEAST_EXPECT (strCounter.count() == 1);
            BEAST_EXPECT (wrapped);

//使绳子足够大，以（可能）避开小绳子。
//优化。
            TrackedString strRValue ("rvalue abcdefghijklmnopqrstuvwxyz");
            TrackedString const result = (*wrapped)(std::move(strRValue));
            BEAST_EXPECT (result.copies == 0);
            BEAST_EXPECT (result.moves == 1);
            BEAST_EXPECT (result.str == "rvalue abcdefghijklmnopqrstuvwxyz!");
            BEAST_EXPECT (strRValue.str.size() == 0);
        }
    }

    void testWrap()
    {
//验证参考计数。
        ClosureCounter<void> voidCounter;
        BEAST_EXPECT (voidCounter.count() == 0);
        {
            auto wrapped1 = voidCounter.wrap ([] () {});
            BEAST_EXPECT (voidCounter.count() == 1);
            {
//复制应增加引用计数。
                auto wrapped2 (wrapped1);
                BEAST_EXPECT (voidCounter.count() == 2);
                {
//移动应增加引用计数。
                    auto wrapped3 (std::move(wrapped2));
                    BEAST_EXPECT (voidCounter.count() == 3);
                    {
//额外的关闭也会增加计数。
                        auto wrapped4 = voidCounter.wrap ([] () {});
                        BEAST_EXPECT (voidCounter.count() == 4);
                    }
                    BEAST_EXPECT (voidCounter.count() == 3);
                }
                BEAST_EXPECT (voidCounter.count() == 2);
            }
            BEAST_EXPECT (voidCounter.count() == 1);
        }
        BEAST_EXPECT (voidCounter.count() == 0);

//计数为0的联接不应停止。
        using namespace std::chrono_literals;
        voidCounter.join("testWrap", 1ms, j);

//在join（）之后包装一个闭包应该返回boost:：none。
        BEAST_EXPECT (voidCounter.wrap ([] () {}) == boost::none);
    }

    void testWaitOnJoin()
    {
//验证参考计数。
        ClosureCounter<void> voidCounter;
        BEAST_EXPECT (voidCounter.count() == 0);

        auto wrapped = (voidCounter.wrap ([] () {}));
        BEAST_EXPECT (voidCounter.count() == 1);

//现在应该停止调用join（），在另一个线程上也可以这样做。
        std::atomic<bool> threadExited {false};
        std::thread localThread ([&voidCounter, &threadExited, this] ()
        {
//应在调用Join后暂停。
            using namespace std::chrono_literals;
            voidCounter.join("testWaitOnJoin", 1ms, j);
            threadExited.store (true);
        });

//等待线程调用voidCounter.join（）。
        while (! voidCounter.joined());

//线程在等待5毫秒后仍应处于活动状态。
//这并不能保证join（）使线程停止，但它
//提高信心。
        using namespace std::chrono_literals;
        std::this_thread::sleep_for (5ms);
        BEAST_EXPECT (threadExited == false);

//销毁包装的内容并期望线程退出
//（异步）。
        wrapped = boost::none;
        BEAST_EXPECT (voidCounter.count() == 0);

//等待线程退出。
        while (threadExited == false);
        localThread.join();
    }

public:
    void run() override
    {
        testConstruction();
        testArgs();
        testWrap();
        testWaitOnJoin();
    }
};

BEAST_DEFINE_TESTSUITE(ClosureCounter, core, ripple);

} //测试
} //涟漪
