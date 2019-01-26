
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
  版权所有（c）2016 Ripple Labs Inc.

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

#include <ripple/ledger/CashDiff.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/beast/unit_test.h>
#include <type_traits>

namespace ripple {
namespace test {

class CashDiff_test : public beast::unit_test::suite
{
    static_assert(!std::is_default_constructible<CashDiff>{}, "");
    static_assert(!std::is_copy_constructible<CashDiff>{}, "");
    static_assert(!std::is_copy_assignable<CashDiff>{}, "");
    static_assert(std::is_nothrow_move_constructible<CashDiff>{}, "");
    static_assert(!std::is_move_assignable<CashDiff>{}, "");

//练习差异信任（stamount，stamount）
    void
    testDust ()
    {
        testcase ("diffIsDust (STAmount, STAmount)");

        Issue const usd (Currency (0x5553440000000000), AccountID (0x4985601));
        Issue const usf (Currency (0x5553460000000000), AccountID (0x4985601));

//正面和反面都不是灰尘。
        expect (!diffIsDust (STAmount{usd, 1}, STAmount{usd, -1}));

//不同的问题永远不会尘埃落定。
        expect (!diffIsDust (STAmount{usd, 1}, STAmount{usf, 1}));

//土生土长的和非土生土长的都不是灰尘。
        expect (!diffIsDust (STAmount{usd, 1}, STAmount{1}));

//相等的值总是无稽之谈。
        expect (diffIsDust (STAmount{0}, STAmount{0}));
        {
//测试IOU。
            std::uint64_t oldProbe = 0;
            std::uint64_t newProbe = 10;
            std::uint8_t e10 = 1;
            do
            {
                STAmount large (usd, newProbe + 1);
                STAmount small (usd, newProbe);

                expect (diffIsDust (large, small, e10));
                expect (diffIsDust (large, small, e10+1) == (e10 > 13));

                oldProbe = newProbe;
                newProbe = oldProbe * 10;
                e10 += 1;
            } while (newProbe > oldProbe);
        }
        {
//测试XRP。
//小于等于2的增量总是灰尘。
            expect (diffIsDust (STAmount{2}, STAmount{0}));

            std::uint64_t oldProbe = 0;
            std::uint64_t newProbe = 10;
            std::uint8_t e10 = 0;
            do
            {
//少两滴的差别总是被当作灰尘处理，
//所以用3的增量。
                STAmount large (newProbe + 3);
                STAmount small (newProbe);

                expect (diffIsDust (large, small, e10));
                expect (diffIsDust (large, small, e10+1) == (e10 >= 20));

                oldProbe = newProbe;
                newProbe = oldProbe * 10;
                e10 += 1;
            } while (newProbe > oldProbe);
        }
    }

public:
    void run () override
    {
        testDust();
    }
};

BEAST_DEFINE_TESTSUITE (CashDiff, ledger, ripple);

}  //测试
}  //涟漪
