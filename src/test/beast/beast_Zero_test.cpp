
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
    版权所有2014，nikolaos d.bogalis<nikb@bogalis.net>

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

#include <ripple/beast/utility/Zero.h>

#include <ripple/beast/unit_test.h>

namespace beast {

struct adl_tester {};

int signum (adl_tester)
{
    return 0;
}


namespace inner_adl_test {

struct adl_tester2 {};

int signum (adl_tester2)
{
    return 0;
}

}  //细节

class Zero_test : public beast::unit_test::suite
{
private:
    struct IntegerWrapper
    {
        int value;

        IntegerWrapper (int v)
            : value (v)
        {
        }

        int signum() const
        {
            return value;
        }
    };

public:
    void expect_same(bool result, bool correct, char const* message)
    {
        expect(result == correct, message);
    }

    void
    test_lhs_zero (IntegerWrapper x)
    {
        expect_same (x >= zero, x.signum () >= 0,
            "lhs greater-than-or-equal-to");
        expect_same (x > zero, x.signum () > 0,
            "lhs greater than");
        expect_same (x == zero, x.signum () == 0,
            "lhs equal to");
        expect_same (x != zero, x.signum () != 0,
            "lhs not equal to");
        expect_same (x < zero, x.signum () < 0,
            "lhs less than");
        expect_same (x <= zero, x.signum () <= 0,
            "lhs less-than-or-equal-to");
    }

    void
    test_lhs_zero ()
    {
        testcase ("lhs zero");

        test_lhs_zero(-7);
        test_lhs_zero(0);
        test_lhs_zero(32);
    }

    void
    test_rhs_zero (IntegerWrapper x)
    {
        expect_same (zero >= x, 0 >= x.signum (),
            "rhs greater-than-or-equal-to");
        expect_same (zero > x, 0 > x.signum (),
            "rhs greater than");
        expect_same (zero == x, 0 == x.signum (),
            "rhs equal to");
        expect_same (zero != x, 0 != x.signum (),
            "rhs not equal to");
        expect_same (zero < x, 0 < x.signum (),
            "rhs less than");
        expect_same (zero <= x, 0 <= x.signum (),
            "rhs less-than-or-equal-to");
    }

    void
    test_rhs_zero ()
    {
        testcase ("rhs zero");

        test_rhs_zero(-4);
        test_rhs_zero(0);
        test_rhs_zero(64);
    }

    void
    test_adl ()
    {
      expect (adl_tester{} == zero, "ADL failure!");
      expect (inner_adl_test::adl_tester2{} == zero, "ADL failure!");
    }

    void
    run() override
    {
        test_lhs_zero ();
        test_rhs_zero ();
        test_adl ();
    }

};

BEAST_DEFINE_TESTSUITE(Zero, types, beast);

}
