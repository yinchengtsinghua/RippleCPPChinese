
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

#include <ripple/rpc/Status.h>
#include <ripple/basics/contract.h>
#include <ripple/beast/unit_test.h>
#include <algorithm>

namespace ripple {
namespace RPC {

class codeString_test : public beast::unit_test::suite
{
private:
    template <typename Type>
    std::string codeString (Type t)
    {
        return Status (t).codeString();
    }

    void test_OK ()
    {
        testcase ("OK");
        {
            auto s = codeString (Status ());
            expect (s.empty(), "String for OK status");
        }

        {
            auto s = codeString (Status::OK);
            expect (s.empty(), "String for OK status");
        }

        {
            auto s = codeString (0);
            expect (s.empty(), "String for 0 status");
        }

        {
            auto s = codeString (tesSUCCESS);
            expect (s.empty(), "String for tesSUCCESS");
        }

        {
            auto s = codeString (rpcSUCCESS);
            expect (s.empty(), "String for rpcSUCCESS");
        }
    }

    void test_error ()
    {
        testcase ("error");
        {
            auto s = codeString (23);
            expect (s == "23", s);
        }

        {
            auto s = codeString (temBAD_AMOUNT);
            expect (s == "temBAD_AMOUNT: Can only send positive amounts.", s);
        }

        {
            auto s = codeString (rpcBAD_SYNTAX);
            expect (s == "badSyntax: Syntax error.", s);
        }
    }

public:
    void run() override
    {
        test_OK ();
        test_error ();
    }
};

BEAST_DEFINE_TESTSUITE (codeString, Status, RPC);

class fillJson_test : public beast::unit_test::suite
{
private:
    Json::Value value_;

    template <typename Type>
    void fillJson (Type t)
    {
        value_.clear ();
        Status (t).fillJson (value_);
    }

    void test_OK ()
    {
        testcase ("OK");
        fillJson (Status ());
        expect (! value_, "Value for empty status");

        fillJson (0);
        expect (! value_, "Value for 0 status");

        fillJson (Status::OK);
        expect (! value_, "Value for OK status");

        fillJson (tesSUCCESS);
        expect (! value_, "Value for tesSUCCESS");

        fillJson (rpcSUCCESS);
        expect (! value_, "Value for rpcSUCCESS");
    }

    template <typename Type>
    void expectFill (
        std::string const& label,
        Type status,
        Status::Strings messages,
        std::string const& message)
    {
        value_.clear ();
        fillJson (Status (status, messages));

        auto prefix = label + ": ";
        expect (bool (value_), prefix + "No value");

        auto error = value_[jss::error];
        expect (bool (error), prefix + "No error.");

        auto code = error[jss::code].asInt();
        expect (status == code, prefix + "Wrong status " +
                std::to_string (code) + " != " + std::to_string (status));

        auto m = error[jss::message].asString ();
        expect (m == message, m + " != " + message);

        auto d = error[jss::data];
        size_t s1 = d.size(), s2 = messages.size();
        expect (s1 == s2, prefix + "Data sizes differ " +
                std::to_string (s1) + " != " + std::to_string (s2));
        for (auto i = 0; i < std::min (s1, s2); ++i)
        {
            auto ds = d[i].asString();
            expect (ds == messages[i], prefix + ds + " != " +  messages[i]);
        }
    }

    void test_error ()
    {
        testcase ("error");
        expectFill (
            "temBAD_AMOUNT",
            temBAD_AMOUNT,
            {},
            "temBAD_AMOUNT: Can only send positive amounts.");

        expectFill (
            "rpcBAD_SYNTAX",
            rpcBAD_SYNTAX,
            {"An error.", "Another error."},
            "badSyntax: Syntax error.");

        expectFill (
            "integer message",
            23,
            {"Stuff."},
            "23");
    }

    void test_throw ()
    {
        testcase ("throw");
        try
        {
            Throw<Status> (Status(temBAD_PATH, { "path=sdcdfd" }));
        }
        catch (Status const& s)
        {
            expect (s.toTER () == temBAD_PATH, "temBAD_PATH wasn't thrown");
            auto msgs = s.messages ();
            expect (msgs.size () == 1, "Wrong number of messages");
            expect (msgs[0] == "path=sdcdfd", msgs[0]);
        }
        catch (std::exception const&)
        {
            expect (false, "Didn't catch a Status");
        }
    }

public:
    void run() override
    {
        test_OK ();
        test_error ();
        test_throw ();
    }
};

BEAST_DEFINE_TESTSUITE (fillJson, Status, RPC);

} //命名空间RPC
} //涟漪
