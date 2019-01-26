
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
    版权所有（c）2012，2015 Ripple Labs Inc.

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

#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/View.h>
#include <ripple/beast/unit_test.h>
#include <test/jtx.h>

namespace ripple {
namespace test {

class SkipList_test : public beast::unit_test::suite
{
    void
    testSkipList()
    {
        jtx::Env env(*this);
        std::vector<std::shared_ptr<Ledger>> history;
        {
            Config config;
            auto prev = std::make_shared<Ledger>(
                create_genesis, config,
                std::vector<uint256>{}, env.app().family());
            history.push_back(prev);
            for (auto i = 0; i < 1023; ++i)
            {
                auto next = std::make_shared<Ledger>(
                    *prev,
                    env.app().timeKeeper().closeTime());
                next->updateSkipList();
                history.push_back(next);
                prev = next;
            }
        }

        {
            auto l = *(std::next(std::begin(history)));
            BEAST_EXPECT((*std::begin(history))->info().seq <
                l->info().seq);
            BEAST_EXPECT(hashOfSeq(*l, l->info().seq + 1,
                env.journal) == boost::none);
            BEAST_EXPECT(hashOfSeq(*l, l->info().seq,
                env.journal) == l->info().hash);
            BEAST_EXPECT(hashOfSeq(*l, l->info().seq - 1,
                env.journal) == l->info().parentHash);
            BEAST_EXPECT(hashOfSeq(*history.back(),
                l->info().seq, env.journal) == boost::none);
        }

//分类帐跳过列表最多可存储前256个哈希值
        for (auto i = history.crbegin();
            i != history.crend(); i += 256)
        {
            for (auto n = i;
                n != std::next(i,
                    (*i)->info().seq - 256 > 1 ? 257 : 256);
                ++n)
            {
                BEAST_EXPECT(hashOfSeq(**i,
                    (*n)->info().seq, env.journal) ==
                        (*n)->info().hash);
            }

//边缘大小写访问超过256
            BEAST_EXPECT(hashOfSeq(**i,
                (*i)->info().seq - 258, env.journal) ==
                    boost::none);
        }

//存储前256个以上的每256个哈希
        for (auto i = history.crbegin();
            i != std::next(history.crend(), -512);
            i += 256)
        {
            for (auto n = std::next(i, 512);
                n != history.crend();
                n += 256)
            {
                BEAST_EXPECT(hashOfSeq(**i,
                    (*n)->info().seq, env.journal) ==
                        (*n)->info().hash);
            }
        }
    }

    void run() override
    {
        testSkipList();
    }
};

BEAST_DEFINE_TESTSUITE(SkipList,ledger,ripple);

}  //测试
}  //涟漪
