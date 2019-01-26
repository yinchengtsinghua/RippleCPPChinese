
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

#include <ripple/basics/chrono.h>
#include <ripple/basics/KeyCache.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/clock/manual_clock.h>

namespace ripple {

class KeyCache_test : public beast::unit_test::suite
{
public:
    void run () override
    {
        using namespace std::chrono_literals;
        TestStopwatch clock;
        clock.set (0);

        using Key = std::string;
        using Cache = KeyCache <Key>;

//插入一个项目，检索它，并对其进行老化，以便清除它。
        {
            Cache c ("test", clock, 1, 2s);

            BEAST_EXPECT(c.size () == 0);
            BEAST_EXPECT(c.insert ("one"));
            BEAST_EXPECT(! c.insert ("one"));
            BEAST_EXPECT(c.size () == 1);
            BEAST_EXPECT(c.exists ("one"));
            BEAST_EXPECT(c.touch_if_exists ("one"));
            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.size () == 1);
            BEAST_EXPECT(c.exists ("one"));
            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.size () == 0);
            BEAST_EXPECT(! c.exists ("one"));
            BEAST_EXPECT(! c.touch_if_exists ("one"));
        }

//插入两项，其中一项过期
        {
            Cache c ("test", clock, 2, 2s);

            BEAST_EXPECT(c.insert ("one"));
            BEAST_EXPECT(c.size  () == 1);
            BEAST_EXPECT(c.insert ("two"));
            BEAST_EXPECT(c.size  () == 2);
            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.size () == 2);
            BEAST_EXPECT(c.touch_if_exists ("two"));
            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.size () == 1);
            BEAST_EXPECT(c.exists ("two"));
        }

//插入三个项目（1个超出限制），扫描
        {
            Cache c ("test", clock, 2, 3s);

            BEAST_EXPECT(c.insert ("one"));
            ++clock;
            BEAST_EXPECT(c.insert ("two"));
            ++clock;
            BEAST_EXPECT(c.insert ("three"));
            ++clock;
            BEAST_EXPECT(c.size () == 3);
            c.sweep ();
            BEAST_EXPECT(c.size () < 3);
        }
    }
};

BEAST_DEFINE_TESTSUITE(KeyCache,common,ripple);

}
