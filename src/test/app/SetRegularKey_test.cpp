
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

#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/Feature.h>
#include <test/jtx.h>

namespace ripple {

class SetRegularKey_test : public beast::unit_test::suite
{
public:

    void testDisableMasterKey()
    {
        using namespace test::jtx;
        Env env(*this);
        Account const alice("alice");
        Account const bob("bob");
        env.fund(XRP(10000), alice, bob);

//万能钥匙和普通钥匙
        env(regkey(alice, bob));
        auto const ar = env.le(alice);
        BEAST_EXPECT(ar->isFieldPresent(sfRegularKey) && (ar->getAccountID(sfRegularKey) == bob.id()));

        env(noop(alice));
        env(noop(alice), sig(bob));
        env(noop(alice), sig(alice));

//仅限普通钥匙
        env(fset(alice, asfDisableMaster), sig(alice));
        env(noop(alice));
        env(noop(alice), sig(bob));
        env(noop(alice), sig(alice), ter(tefMASTER_DISABLED));
        env(fclear(alice, asfDisableMaster), sig(alice), ter(tefMASTER_DISABLED));
        env(fclear(alice, asfDisableMaster), sig(bob));
        env(noop(alice), sig(alice));
    }

    void testPasswordSpent()
    {
        using namespace test::jtx;
        Env env(*this);
        Account const alice("alice");
        Account const bob("bob");
        env.fund(XRP(10000), alice, bob);

        auto ar = env.le(alice);
        BEAST_EXPECT(ar->isFieldPresent(sfFlags) && ((ar->getFieldU32(sfFlags) & lsfPasswordSpent) == 0));

        env(regkey(alice, bob), sig(alice), fee(0));

        ar = env.le(alice);
        BEAST_EXPECT(ar->isFieldPresent(sfFlags) && ((ar->getFieldU32(sfFlags) & lsfPasswordSpent) == lsfPasswordSpent));

//fee=0的第二个setRegularKey事务应该失败。
        env(regkey(alice, bob), sig(alice), fee(0), ter(telINSUF_FEE_P));

        env.trust(bob["USD"](1), alice);
        env(pay(bob, alice, bob["USD"](1)));
        ar = env.le(alice);
        BEAST_EXPECT(ar->isFieldPresent(sfFlags) && ((ar->getFieldU32(sfFlags) & lsfPasswordSpent) == 0));
    }

    void testUniversalMaskError()
    {
        using namespace test::jtx;
        Env env(*this);
        Account const alice("alice");
        Account const bob("bob");
        env.fund(XRP(10000), alice, bob);

        auto jv = regkey(alice, bob);
        jv[sfFlags.fieldName] = tfUniversalMask;
        env(jv, ter(temINVALID_FLAG));
    }

    void run() override
    {
        testDisableMasterKey();
        testPasswordSpent();
        testUniversalMaskError();
    }

};

BEAST_DEFINE_TESTSUITE(SetRegularKey,app,ripple);

}

