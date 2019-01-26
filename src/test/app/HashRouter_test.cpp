
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
    版权所有（c）2012-2015 Ripple Labs Inc.

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

#include <ripple/app/misc/HashRouter.h>
#include <ripple/basics/chrono.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace test {

class HashRouter_test : public beast::unit_test::suite
{
    void
    testNonExpiration()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 2s, 2);

        uint256 const key1(1);
        uint256 const key2(2);
        uint256 const key3(3);

//t＝0
        router.setFlags(key1, 11111);
        BEAST_EXPECT(router.getFlags(key1) == 11111);
        router.setFlags(key2, 22222);
        BEAST_EXPECT(router.getFlags(key2) == 22222);
//KE1：0
//KEY2：0
//关键字3：空

        ++stopwatch;

//因为我们在这里访问key1，
//将不会因另外两个计时周期而过期
        BEAST_EXPECT(router.getFlags(key1) == 11111);
//KE1：1
//KEY2：0
//密钥3空值

        ++stopwatch;

//t＝3
router.setFlags(key3,33333); //强制过期
        BEAST_EXPECT(router.getFlags(key1) == 11111);
        BEAST_EXPECT(router.getFlags(key2) == 0);
    }

    void
    testExpiration()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 2s, 2);

        uint256 const key1(1);
        uint256 const key2(2);
        uint256 const key3(3);
        uint256 const key4(4);
        BEAST_EXPECT(key1 != key2 &&
            key2 != key3 &&
            key3 != key4);

//t＝0
        router.setFlags(key1, 12345);
        BEAST_EXPECT(router.getFlags(key1) == 12345);
//KE1：0
//关键字2：空
//关键字3：空

        ++stopwatch;

//插入会触发过期，
//在访问时更新时间戳，
//所以键1将在第二个之后过期
//调用setflags。
//t＝1
        router.setFlags(key2, 9999);
        BEAST_EXPECT(router.getFlags(key1) == 12345);
        BEAST_EXPECT(router.getFlags(key2) == 9999);
//KE1：1
//KEY2：1
//关键字3：空

        ++stopwatch;
//t＝2
        BEAST_EXPECT(router.getFlags(key2) == 9999);
//KE1：1
//KEY2：2
//关键字3：空

        ++stopwatch;
//t＝3
        router.setFlags(key3, 2222);
        BEAST_EXPECT(router.getFlags(key1) == 0);
        BEAST_EXPECT(router.getFlags(key2) == 9999);
        BEAST_EXPECT(router.getFlags(key3) == 2222);
//KE1：3
//KEY2：3
//密钥3：3

        ++stopwatch;
//t＝4
//不插入，不过期
        router.setFlags(key1, 7654);
        BEAST_EXPECT(router.getFlags(key1) == 7654);
        BEAST_EXPECT(router.getFlags(key2) == 9999);
        BEAST_EXPECT(router.getFlags(key3) == 2222);
//KE1：4
//KEY2：4
//密钥3：4

        ++stopwatch;
        ++stopwatch;

//t＝6
        router.setFlags(key4, 7890);
        BEAST_EXPECT(router.getFlags(key1) == 0);
        BEAST_EXPECT(router.getFlags(key2) == 0);
        BEAST_EXPECT(router.getFlags(key3) == 0);
        BEAST_EXPECT(router.getFlags(key4) == 7890);
//KE1：6
//KEY2：6
//密钥3：6
//KEY4：6
    }

    void testSuppression()
    {
//普通哈希路由器
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 2s, 2);

        uint256 const key1(1);
        uint256 const key2(2);
        uint256 const key3(3);
        uint256 const key4(4);
        BEAST_EXPECT(key1 != key2 &&
            key2 != key3 &&
            key3 != key4);

int flags = 12345;  //忽略此值
        router.addSuppression(key1);
        BEAST_EXPECT(router.addSuppressionPeer(key2, 15));
        BEAST_EXPECT(router.addSuppressionPeer(key3, 20, flags));
        BEAST_EXPECT(flags == 0);

        ++stopwatch;

        BEAST_EXPECT(!router.addSuppressionPeer(key1, 2));
        BEAST_EXPECT(!router.addSuppressionPeer(key2, 3));
        BEAST_EXPECT(!router.addSuppressionPeer(key3, 4, flags));
        BEAST_EXPECT(flags == 0);
        BEAST_EXPECT(router.addSuppressionPeer(key4, 5));
    }

    void
    testSetFlags()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 2s, 2);

        uint256 const key1(1);
        BEAST_EXPECT(router.setFlags(key1, 10));
        BEAST_EXPECT(!router.setFlags(key1, 10));
        BEAST_EXPECT(router.setFlags(key1, 20));
    }

    void
    testRelay()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 1s, 2);

        uint256 const key1(1);

        boost::optional<std::set<HashRouter::PeerShortID>> peers;

        peers = router.shouldRelay(key1);
        BEAST_EXPECT(peers && peers->empty());
        router.addSuppressionPeer(key1, 1);
        router.addSuppressionPeer(key1, 3);
        router.addSuppressionPeer(key1, 5);
//没有动作，因为转播了
        BEAST_EXPECT(!router.shouldRelay(key1));
//过期，但下次搜索后
//对于这个条目，它将被刷新
//相反。但是，继电器不会。
        ++stopwatch;
//获取我们之前添加的同行
        peers = router.shouldRelay(key1);
        BEAST_EXPECT(peers && peers->size() == 3);
        router.addSuppressionPeer(key1, 2);
        router.addSuppressionPeer(key1, 4);
//没有动作，因为转播了
        BEAST_EXPECT(!router.shouldRelay(key1));
//过期，但下次搜索后
//对于这个条目，它将被刷新
//相反。但是，继电器不会。
        ++stopwatch;
//再次继电器
        peers = router.shouldRelay(key1);
        BEAST_EXPECT(peers && peers->size() == 2);
//再次过期
        ++stopwatch;
//确认对等方列表为空。
        peers = router.shouldRelay(key1);
        BEAST_EXPECT(peers && peers->size() == 0);
    }

    void
    testRecover()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 1s, 5);

        uint256 const key1(1);

        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(!router.shouldRecover(key1));
//过期，但下次搜索后
//对于这个条目，它将被刷新
//相反。
        ++stopwatch;
        BEAST_EXPECT(router.shouldRecover(key1));
//过期，但下次搜索后
//对于这个条目，它将被刷新
//相反。
        ++stopwatch;
//再次康复。恢复独立于
//只要条目不过期就可以。
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
//再次过期
        ++stopwatch;
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(!router.shouldRecover(key1));
    }

    void
    testProcess()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 5s, 5);
        uint256 const key(1);
        HashRouter::PeerShortID peer = 1;
        int flags;

        BEAST_EXPECT(router.shouldProcess(key, peer, flags, 1s));
        BEAST_EXPECT(! router.shouldProcess(key, peer, flags, 1s));
        ++stopwatch;
        ++stopwatch;
        BEAST_EXPECT(router.shouldProcess(key, peer, flags, 1s));
    }


public:

    void
    run() override
    {
        testNonExpiration();
        testExpiration();
        testSuppression();
        testSetFlags();
        testRelay();
        testRecover();
        testProcess();
    }
};

BEAST_DEFINE_TESTSUITE(HashRouter, app, ripple);

}
}
