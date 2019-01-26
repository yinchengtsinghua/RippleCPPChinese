
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
    版权所有（c）2015 Ripple Labs Inc.

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

#include <ripple/protocol/STAccount.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

struct STAccount_test : public beast::unit_test::suite
{
    void
    testSTAccount()
    {
        {
//测试默认构造函数。
            STAccount const defaultAcct;
            BEAST_EXPECT(defaultAcct.getSType() == STI_ACCOUNT);
            BEAST_EXPECT(defaultAcct.getText() == "");
            BEAST_EXPECT(defaultAcct.isDefault() == true);
            BEAST_EXPECT(defaultAcct.value() == AccountID {});
            {
#ifdef NDEBUG //限定，因为序列化在调试生成中断言。
                Serializer s;
defaultAcct.add (s); //调试生成中的断言
                BEAST_EXPECT(s.size() == 1);
                BEAST_EXPECT(s.getHex() == "00");
                SerialIter sit (s.slice ());
                STAccount const deserializedDefault (sit, sfAccount);
                BEAST_EXPECT(deserializedDefault.isEquivalent (defaultAcct));
#endif //调试程序
            }
            {
//构造反序列化的默认statcount。
                Serializer s;
                s.addVL (nullptr, 0);
                SerialIter sit (s.slice ());
                STAccount const deserializedDefault (sit, sfAccount);
                BEAST_EXPECT(deserializedDefault.isEquivalent (defaultAcct));
            }

//从sfield测试构造函数。
            STAccount const sfAcct {sfAccount};
            BEAST_EXPECT(sfAcct.getSType() == STI_ACCOUNT);
            BEAST_EXPECT(sfAcct.getText() == "");
            BEAST_EXPECT(sfAcct.isDefault());
            BEAST_EXPECT(sfAcct.value() == AccountID {});
            BEAST_EXPECT(sfAcct.isEquivalent (defaultAcct));
            {
                Serializer s;
                sfAcct.add (s);
                BEAST_EXPECT(s.size() == 1);
                BEAST_EXPECT(s.getHex() == "00");
                SerialIter sit (s.slice ());
                STAccount const deserializedSf (sit, sfAccount);
                BEAST_EXPECT(deserializedSf.isEquivalent(sfAcct));
            }

//从sfield和accountID测试构造函数。
            STAccount const zeroAcct {sfAccount, AccountID{}};
            BEAST_EXPECT(zeroAcct.getText() == "rrrrrrrrrrrrrrrrrrrrrhoLvTp");
            BEAST_EXPECT(! zeroAcct.isDefault());
            BEAST_EXPECT(zeroAcct.value() == AccountID {0});
            BEAST_EXPECT(! zeroAcct.isEquivalent (defaultAcct));
            BEAST_EXPECT(! zeroAcct.isEquivalent (sfAcct));
            {
                Serializer s;
                zeroAcct.add (s);
                BEAST_EXPECT(s.size() == 21);
                BEAST_EXPECT(s.getHex() ==
                    "140000000000000000000000000000000000000000");
                SerialIter sit (s.slice ());
                STAccount const deserializedZero (sit, sfAccount);
                BEAST_EXPECT(deserializedZero.isEquivalent (zeroAcct));
            }
            {
//从不完全是160位的VL构造。
                Serializer s;
                const std::uint8_t bits128[] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                s.addVL (bits128, sizeof (bits128));
                SerialIter sit (s.slice ());
                try
                {
//构造大小不正确的staccount应该引发。
                    STAccount const deserializedBadSize (sit, sfAccount);
                }
                catch (std::runtime_error const& ex)
                {
                    BEAST_EXPECT(ex.what() == std::string("Invalid STAccount size"));
                }

            }

//有趣的是，相同的值，但不同的类型是等效的！
            STAccount const regKey {sfRegularKey, AccountID{}};
            BEAST_EXPECT(regKey.isEquivalent (zeroAcct));

//测试分配。
            STAccount assignAcct;
            BEAST_EXPECT(assignAcct.isEquivalent (defaultAcct));
            BEAST_EXPECT(assignAcct.isDefault());
            assignAcct = AccountID{};
            BEAST_EXPECT(! assignAcct.isEquivalent (defaultAcct));
            BEAST_EXPECT(assignAcct.isEquivalent (zeroAcct));
            BEAST_EXPECT(! assignAcct.isDefault());
        }
    }

    void
    run() override
    {
        testSTAccount();
    }
};

BEAST_DEFINE_TESTSUITE(STAccount,protocol,ripple);

}
