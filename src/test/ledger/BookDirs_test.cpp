
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

#include <test/jtx.h>
#include <ripple/ledger/BookDirs.h>
#include <ripple/protocol/Feature.h>

namespace ripple {
namespace test {

struct BookDirs_test : public beast::unit_test::suite
{
    void test_bookdir(FeatureBitset features)
    {
        using namespace jtx;
        Env env(*this, features);
        auto gw = Account("gw");
        auto USD = gw["USD"];
        env.fund(XRP(1000000), "alice", "bob", "gw");

        {
            Book book(xrpIssue(), USD.issue());
            {
                auto d = BookDirs(*env.current(), book);
                BEAST_EXPECT(std::begin(d) == std::end(d));
                BEAST_EXPECT(std::distance(d.begin(), d.end()) == 0);
            }
            {
                auto d = BookDirs(*env.current(), reversed(book));
                BEAST_EXPECT(std::distance(d.begin(), d.end()) == 0);
            }
        }

        {
            env(offer("alice", Account("alice")["USD"](50), XRP(10)));
            auto d = BookDirs(*env.current(),
                Book(Account("alice")["USD"].issue(), xrpIssue()));
            BEAST_EXPECT(std::distance(d.begin(), d.end()) == 1);
        }

        {
            env(offer("alice", gw["CNY"](50), XRP(10)));
            auto d = BookDirs(*env.current(),
                Book(gw["CNY"].issue(), xrpIssue()));
            BEAST_EXPECT(std::distance(d.begin(), d.end()) == 1);
        }

        {
            env.trust(Account("bob")["CNY"](10), "alice");
            env(pay("bob", "alice", Account("bob")["CNY"](10)));
            env(offer("alice", USD(50), Account("bob")["CNY"](10)));
            auto d = BookDirs(*env.current(),
                Book(USD.issue(), Account("bob")["CNY"].issue()));
            BEAST_EXPECT(std::distance(d.begin(), d.end()) == 1);
        }

        {
            auto AUD = gw["AUD"];
            for (auto i = 1, j = 3; i <= 3; ++i, --j)
                for (auto k = 0; k < 80; ++k)
                    env(offer("alice", AUD(i), XRP(j)));

            auto d = BookDirs(*env.current(),
                Book(AUD.issue(), xrpIssue()));
            BEAST_EXPECT(std::distance(d.begin(), d.end()) == 240);
            auto i = 1, j = 3, k = 0;
            for (auto const& e : d)
            {
                BEAST_EXPECT(e->getFieldAmount(sfTakerPays) == AUD(i));
                BEAST_EXPECT(e->getFieldAmount(sfTakerGets) == XRP(j));
                if (++k % 80 == 0)
                {
                    ++i;
                    --j;
                }
            }
        }
    }

    void run() override
    {
        using namespace jtx;
        auto const sa = supported_amendments();
        test_bookdir(sa - featureFlow - fix1373 - featureFlowCross);
        test_bookdir(sa                         - featureFlowCross);
        test_bookdir(sa);
    }
};

BEAST_DEFINE_TESTSUITE(BookDirs,ledger,ripple);

} //测试
} //涟漪
