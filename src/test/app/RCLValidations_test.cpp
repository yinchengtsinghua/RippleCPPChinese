
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
    版权所有2017 Ripple Labs Inc.

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

#include <ripple/app/consensus/RCLValidations.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/View.h>
#include <ripple/beast/unit_test.h>
#include <test/jtx.h>

namespace ripple {
namespace test {

class RCLValidations_test : public beast::unit_test::suite
{

    void
    testChangeTrusted()
    {
        testcase("Change validation trusted status");
        auto keys = randomKeyPair(KeyType::secp256k1);
        auto v = std::make_shared<STValidation>(
            uint256(),
            1,
            uint256(),
            NetClock::time_point(),
            keys.first,
            keys.second,
            calcNodeID(keys.first),
            true,
            STValidation::FeeSettings{},
            std::vector<uint256>{});

        BEAST_EXPECT(v->isTrusted());
        v->setUntrusted();
        BEAST_EXPECT(!v->isTrusted());

        RCLValidation rcv{v};
        BEAST_EXPECT(!rcv.trusted());
        rcv.setTrusted();
        BEAST_EXPECT(rcv.trusted());
        rcv.setUntrusted();
        BEAST_EXPECT(!rcv.trusted());
    }

    void
    testRCLValidatedLedger()
    {
        testcase("RCLValidatedLedger ancestry");

        using Seq = RCLValidatedLedger::Seq;
        using ID = RCLValidatedLedger::ID;


//此测试rclValidatedLedger正确实现类型
//Ledgertrie分类帐的要求，及其附加的行为
//只有256个以前的分类账哈希可用于确定祖先。
        Seq const maxAncestors = 256;

//——————————————————————————————————————————————————————————————
//生成两个同意第一个max祖先的分类帐历史记录
//分类帐，然后分开。

        std::vector<std::shared_ptr<Ledger const>> history;

        jtx::Env env(*this);
        Config config;
        auto prev = std::make_shared<Ledger const>(
            create_genesis, config,
            std::vector<uint256>{}, env.app().family());
        history.push_back(prev);
        for (auto i = 0; i < (2*maxAncestors + 1); ++i)
        {
            auto next = std::make_shared<Ledger>(
                *prev,
                env.app().timeKeeper().closeTime());
            next->updateSkipList();
            history.push_back(next);
            prev = next;
        }

//althistory与常规历史的上半部分一致
        Seq const diverge = history.size()/2;
        std::vector<std::shared_ptr<Ledger const>> altHistory(
            history.begin(), history.begin() + diverge);
//提前时钟获取新分类帐
        using namespace std::chrono_literals;
        env.timeKeeper().set(env.timeKeeper().now() + 1200s);
        prev = altHistory.back();
        bool forceHash = true;
        while(altHistory.size() < history.size())
        {
            auto next = std::make_shared<Ledger>(
                *prev,
                env.app().timeKeeper().closeTime());
//在第一次迭代中强制使用其他哈希
            next->updateSkipList();
            if(forceHash)
            {
                next->setImmutable(config);
                forceHash = false;
            }

            altHistory.push_back(next);
            prev = next;
        }

//——————————————————————————————————————————————————————————————


//空分类帐
        {
            RCLValidatedLedger a{RCLValidatedLedger::MakeGenesis{}};
            BEAST_EXPECT(a.seq() == Seq{0});
            BEAST_EXPECT(a[Seq{0}] == ID{0});
            BEAST_EXPECT(a.minSeq() == Seq{0});
        }

//完整历史分类账
        {
            std::shared_ptr<Ledger const> ledger = history.back();
            RCLValidatedLedger a{ledger, env.journal};
            BEAST_EXPECT(a.seq() == ledger->info().seq);
            BEAST_EXPECT(
                a.minSeq() == a.seq() - maxAncestors);
//确保祖传的256个分类帐具有正确的ID
            for(Seq s = a.seq(); s > 0; s--)
            {
                if(s >= a.minSeq())
                    BEAST_EXPECT(a[s] == history[s-1]->info().hash);
                else
                    BEAST_EXPECT(a[s] == ID{0});
            }
        }

//失配试验

//空与非空
        {
            RCLValidatedLedger a{RCLValidatedLedger::MakeGenesis{}};

            for (auto ledger : {history.back(),
                                history[maxAncestors - 1]})
            {
                RCLValidatedLedger b{ledger, env.journal};
                BEAST_EXPECT(mismatch(a, b) == 1);
                BEAST_EXPECT(mismatch(b, a) == 1);
            }
        }
//相同的链条，不同的SEQ
        {
            RCLValidatedLedger a{history.back(), env.journal};
            for(Seq s = a.seq(); s > 0; s--)
            {
                RCLValidatedLedger b{history[s-1], env.journal};
                if(s >= a.minSeq())
                {
                    BEAST_EXPECT(mismatch(a, b) == b.seq() + 1);
                    BEAST_EXPECT(mismatch(b, a) == b.seq() + 1);
                }
                else
                {
                    BEAST_EXPECT(mismatch(a, b) == Seq{1});
                    BEAST_EXPECT(mismatch(b, a) == Seq{1});
                }
            }

        }
//不同的链条，相同的SEQ
        {
//alt history在history.size（）/2处分叉
            for(Seq s = 1; s < history.size(); ++s)
            {
                RCLValidatedLedger a{history[s-1], env.journal};
                RCLValidatedLedger b{altHistory[s-1], env.journal};

                BEAST_EXPECT(a.seq() == b.seq());
                if(s <= diverge)
                {
                    BEAST_EXPECT(a[a.seq()] == b[b.seq()]);
                    BEAST_EXPECT(mismatch(a,b) == a.seq() + 1);
                    BEAST_EXPECT(mismatch(b,a) == a.seq() + 1);
                }
                else
                {
                    BEAST_EXPECT(a[a.seq()] != b[b.seq()]);
                    BEAST_EXPECT(mismatch(a,b) == diverge + 1);
                    BEAST_EXPECT(mismatch(b,a) == diverge + 1);
                }
            }
        }
//不同的链条，不同的SEQ
        {
//围绕发散点比较
            RCLValidatedLedger a{history[diverge], env.journal};
            for(Seq offset = diverge/2; offset < 3*diverge/2; ++offset)
            {
                RCLValidatedLedger b{altHistory[offset-1], env.journal};
                if(offset <= diverge)
                {
                    BEAST_EXPECT(mismatch(a,b) == b.seq() + 1);
                }
                else
                {
                    BEAST_EXPECT(mismatch(a,b) == diverge + 1);
                }
            }
        }
    }

public:
    void
    run() override
    {
        testChangeTrusted();
        testRCLValidatedLedger();
    }
};

BEAST_DEFINE_TESTSUITE(RCLValidations, app, ripple);

}  //命名空间测试
}  //命名空间波纹
