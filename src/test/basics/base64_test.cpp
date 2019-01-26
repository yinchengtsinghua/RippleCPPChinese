
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
    版权所有（c）2012-2018 Ripple Labs Inc.

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

//
//版权所有（c）2016-2017 Vinnie Falco（gmail.com上的Vinnie.dot Falco）
//
//在Boost软件许可证1.0版下分发。（见附件
//文件许可证_1_0.txt或复制到http://www.boost.org/license_1_0.txt）
//
//官方存储库：https://github.com/boostorg/beast
//

#include <ripple/basics/base64.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

class base64_test : public beast::unit_test::suite
{
public:
    void
    check (std::string const& in, std::string const& out)
    {
        auto const encoded = base64_encode (in);
        BEAST_EXPECT(encoded == out);
        BEAST_EXPECT(base64_decode (encoded) == in);
    }

    void
    run() override
    {
        check ("",       "");
        check ("f",      "Zg==");
        check ("fo",     "Zm8=");
        check ("foo",    "Zm9v");
        check ("foob",   "Zm9vYg==");
        check ("fooba",  "Zm9vYmE=");
        check ("foobar", "Zm9vYmFy");

        check(
            "Man is distinguished, not only by his reason, but by this singular passion from "
            "other animals, which is a lust of the mind, that by a perseverance of delight "
            "in the continued and indefatigable generation of knowledge, exceeds the short "
            "vehemence of any carnal pleasure."
            ,
            "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz"
            "IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg"
            "dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu"
            "dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo"
            "ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4="
            );

        std::string const notBase64 = "not_base64!!";
        std::string const truncated = "not";
        BEAST_EXPECT(base64_decode(notBase64) == base64_decode(truncated));
    }
};

BEAST_DEFINE_TESTSUITE(base64, ripple_basics, ripple);

} //涟漪
