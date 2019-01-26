
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
#include <ripple/basics/TaggedCache.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/clock/manual_clock.h>
#include <test/unit_test/SuiteJournal.h>

namespace ripple {

/*
我想你可以放一些东西进去，确保它们还在那里。让一些
时间过去了，确保他们走了。保持一个强有力的指针指向其中一个，使
当然，即使时间过去了，你也能找到它。创建两个对象
使用相同的键，将两者规范化，并确保获得相同的对象。
把一个物体放进去，但要有一个强有力的指针，使时钟提前很多，
然后用相同的键规范化一个新对象，确保
原始对象。
**/


class TaggedCache_test : public beast::unit_test::suite
{
public:
    void run () override
    {
        using namespace std::chrono_literals;
        using namespace beast::severities;
        test::SuiteJournal journal ("TaggedCache_test", *this);

        TestStopwatch clock;
        clock.set (0);

        using Key = int;
        using Value = std::string;
        using Cache = TaggedCache <Key, Value>;

        Cache c ("test", 1, 1s, clock, journal);

//插入一个项目，检索它，并对其进行老化，以便清除它。
        {
            BEAST_EXPECT(c.getCacheSize() == 0);
            BEAST_EXPECT(c.getTrackSize() == 0);
            BEAST_EXPECT(! c.insert (1, "one"));
            BEAST_EXPECT(c.getCacheSize() == 1);
            BEAST_EXPECT(c.getTrackSize() == 1);

            {
                std::string s;
                BEAST_EXPECT(c.retrieve (1, s));
                BEAST_EXPECT(s == "one");
            }

            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.getCacheSize () == 0);
            BEAST_EXPECT(c.getTrackSize () == 0);
        }

//插入一个项目，保持一个强指针，使其老化，以及
//验证条目是否仍然存在。
        {
            BEAST_EXPECT(! c.insert (2, "two"));
            BEAST_EXPECT(c.getCacheSize() == 1);
            BEAST_EXPECT(c.getTrackSize() == 1);

            {
                Cache::mapped_ptr p (c.fetch (2));
                BEAST_EXPECT(p != nullptr);
                ++clock;
                c.sweep ();
                BEAST_EXPECT(c.getCacheSize() == 0);
                BEAST_EXPECT(c.getTrackSize() == 1);
            }

//既然我们的参考资料不见了，就确保它不见了。
            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.getCacheSize() == 0);
            BEAST_EXPECT(c.getTrackSize() == 0);
        }

//插入相同的键/值对，并确保得到相同的结果
        {
            BEAST_EXPECT(! c.insert (3, "three"));

            {
                Cache::mapped_ptr const p1 (c.fetch (3));
                Cache::mapped_ptr p2 (std::make_shared <Value> ("three"));
                c.canonicalize (3, p2);
                BEAST_EXPECT(p1.get() == p2.get());
            }
            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.getCacheSize() == 0);
            BEAST_EXPECT(c.getTrackSize() == 0);
        }

//把一个物体放进去，但要有一个强有力的指针，使时钟提前很多，
//然后用相同的键规范化一个新对象，确保
//原始对象。
        {
//把物体放进去
            BEAST_EXPECT(! c.insert (4, "four"));
            BEAST_EXPECT(c.getCacheSize() == 1);
            BEAST_EXPECT(c.getTrackSize() == 1);

            {
//保持一个有力的指针指向它
                Cache::mapped_ptr p1 (c.fetch (4));
                BEAST_EXPECT(p1 != nullptr);
                BEAST_EXPECT(c.getCacheSize() == 1);
                BEAST_EXPECT(c.getTrackSize() == 1);
//把钟提前很多
                ++clock;
                c.sweep ();
                BEAST_EXPECT(c.getCacheSize() == 0);
                BEAST_EXPECT(c.getTrackSize() == 1);
//用同一个键将新对象规范化
                Cache::mapped_ptr p2 (std::make_shared <std::string> ("four"));
                BEAST_EXPECT(c.canonicalize (4, p2, false));
                BEAST_EXPECT(c.getCacheSize() == 1);
                BEAST_EXPECT(c.getTrackSize() == 1);
//确保我们得到原始物体
                BEAST_EXPECT(p1.get() == p2.get());
            }

            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.getCacheSize() == 0);
            BEAST_EXPECT(c.getTrackSize() == 0);
        }
    }
};

BEAST_DEFINE_TESTSUITE(TaggedCache,common,ripple);

}
