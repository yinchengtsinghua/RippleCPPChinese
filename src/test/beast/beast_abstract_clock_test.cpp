
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

//模块：../impl/chrono_io.cpp

#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/clock/manual_clock.h>
#include <ripple/beast/unit_test.h>
#include <sstream>
#include <string>
#include <thread>

namespace beast {

class abstract_clock_test : public unit_test::suite
{
public:
    template <class Clock>
    void test (std::string name, abstract_clock<Clock>& c)
    {
        testcase (name);

        auto const t1 (c.now ());
        std::this_thread::sleep_for (
            std::chrono::milliseconds (1500));
        auto const t2 (c.now ());

        log <<
            "t1= " << t1.time_since_epoch().count() <<
            ", t2= " << t2.time_since_epoch().count() <<
            ", elapsed= " << (t2 - t1).count() << std::endl;

        pass ();
    }

    void test_manual ()
    {
        testcase ("manual");

        using clock_type = manual_clock<std::chrono::steady_clock>;
        clock_type c;

        std::stringstream ss;

        auto c1 = c.now().time_since_epoch();
        c.set (clock_type::time_point (std::chrono::seconds(1)));
        auto c2 = c.now().time_since_epoch();
        c.set (clock_type::time_point (std::chrono::seconds(2)));
        auto c3 = c.now().time_since_epoch();

        log <<
            "[" << c1.count () << "," << c2.count () <<
            "," << c3.count () << "]" << std::endl;

        pass ();
    }

    void run () override
    {
        test ("steady_clock", get_abstract_clock<
            std::chrono::steady_clock>());
        test ("system_clock", get_abstract_clock<
            std::chrono::system_clock>());
        test ("high_resolution_clock", get_abstract_clock<
            std::chrono::high_resolution_clock>());

        test_manual ();
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL(abstract_clock,chrono,beast);

}
