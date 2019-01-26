
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

#include <ripple/basics/contract.h>
#include <ripple/protocol/InnerObjectFormats.h>
#include <ripple/protocol/ErrorCodes.h>          //rpc:：containsError（包含错误）
#include <ripple/json/json_reader.h>             //读者：读者
#include <ripple/protocol/STParsedJSON.h>        //StParsedjSonObject对象
#include <ripple/beast/unit_test.h>
#include <test/jtx.h>

namespace ripple {

namespace InnerObjectFormatsUnitTestDetail
{

struct TestJSONTxt
{
    std::string const txt;
    bool const expectFail;
};

static TestJSONTxt const testArray[] =
{

//有效签字人
{R"({
    "Account" : "rDg53Haik2475DJx8bjMDSDPj4VX7htaMd",
    "SignerEntries" :
    [
        {
            "SignerEntry" :
            {
                "Account" : "rnUy2SHTrB9DubsPmkJZUXTf5FcNDGrYEA",
                "SignerWeight" : 4
            }
        },
        {
            "SignerEntry" :
            {
                "Account" : "rPcNzota6B8YBokhYtcTNqQVCngtbnWfux",
                "SignerWeight" : 3
            }
        }
    ],
    "SignerQuorum" : 7,
    "TransactionType" : "SignerListSet"
})", false
},

//签名丢失帐户
{R"({
    "Account" : "rDg53Haik2475DJx8bjMDSDPj4VX7htaMd",
    "SignerEntries" :
    [
        {
            "SignerEntry" :
            {
                "Account" : "rnUy2SHTrB9DubsPmkJZUXTf5FcNDGrYEA",
                "SignerWeight" : 4
            }
        },
        {
            "SignerEntry" :
            {
                "SignerWeight" : 3
            }
        }
    ],
    "SignerQuorum" : 7,
    "TransactionType" : "SignerListSet"
})", true
},

//签名人丢失签名人权重
{R"({
    "Account" : "rDg53Haik2475DJx8bjMDSDPj4VX7htaMd",
    "SignerEntries" :
    [
        {
            "SignerEntry" :
            {
                "Account" : "rnUy2SHTrB9DubsPmkJZUXTf5FcNDGrYEA",
                "SignerWeight" : 4
            }
        },
        {
            "SignerEntry" :
            {
                "Account" : "rPcNzota6B8YBokhYtcTNqQVCngtbnWfux",
            }
        }
    ],
    "SignerQuorum" : 7,
    "TransactionType" : "SignerListSet"
})", true
},

//意外金额的签名
{R"({
    "Account" : "rDg53Haik2475DJx8bjMDSDPj4VX7htaMd",
    "SignerEntries" :
    [
        {
            "SignerEntry" :
            {
                "Account" : "rnUy2SHTrB9DubsPmkJZUXTf5FcNDGrYEA",
                "SignerWeight" : 4
            }
        },
        {
            "SignerEntry" :
            {
                "Amount" : "1000000",
                "Account" : "rPcNzota6B8YBokhYtcTNqQVCngtbnWfux",
                "SignerWeight" : 3
            }
        }
    ],
    "SignerQuorum" : 7,
    "TransactionType" : "SignerListSet"
})", true
},

//无账户和意外金额的签名
{R"({
    "Account" : "rDg53Haik2475DJx8bjMDSDPj4VX7htaMd",
    "SignerEntries" :
    [
        {
            "SignerEntry" :
            {
                "Account" : "rnUy2SHTrB9DubsPmkJZUXTf5FcNDGrYEA",
                "SignerWeight" : 4
            }
        },
        {
            "SignerEntry" :
            {
                "Amount" : "10000000",
                "SignerWeight" : 3
            }
        }
    ],
    "SignerQuorum" : 7,
    "TransactionType" : "SignerListSet"
})", true
},

};

} //命名空间innerbjectformatsunittesdetail


class InnerObjectFormatsParsedJSON_test : public beast::unit_test::suite
{
public:
    void run() override
    {
        using namespace InnerObjectFormatsUnitTestDetail;

//实例化一个jtx：：env，以便执行debuglog写入。
        test::jtx::Env env (*this);

        for (auto const& test : testArray)
        {
            Json::Value req;
            Json::Reader ().parse (test.txt, req);
            if (RPC::contains_error (req))
            {
                Throw<std::runtime_error> (
                    "Internal InnerObjectFormatsParsedJSON error.  Bad JSON.");
            }
            STParsedJSONObject parsed ("request", req);
            bool const noObj = parsed.object == boost::none;
            if ( noObj == test.expectFail )
            {
                pass ();
            }
            else
            {
                std::string errStr ("Unexpected STParsedJSON result on:\n");
                errStr += test.txt;
                fail (errStr);
            }
        }
    }
};

BEAST_DEFINE_TESTSUITE(InnerObjectFormatsParsedJSON,ripple_app,ripple);

} //涟漪
