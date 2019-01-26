
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

#ifndef RIPPLE_BASICS_TESTSUITE_H_INCLUDED
#define RIPPLE_BASICS_TESTSUITE_H_INCLUDED

#include <ripple/beast/unit_test.h>
#include <string>

namespace ripple {

class TestSuite : public beast::unit_test::suite
{
public:
    template <class S, class T>
    bool expectEquals (S actual, T expected, std::string const& message = "")
    {
        if (actual != expected)
        {
            std::stringstream ss;
            if (!message.empty())
                ss << message << "\n";
            ss << "Actual: " << actual << "\n"
               << "Expected: " << expected;
            fail (ss.str());
            return false;
        }
        pass ();
        return true;

    }

    template <class S, class T>
    bool expectNotEquals(S actual, T expected, std::string const& message = "")
    {
        if (actual == expected)
        {
            std::stringstream ss;
            if (!message.empty())
                ss << message << "\n";
            ss << "Actual: " << actual << "\n"
                << "Expected anything but: " << expected;
            fail(ss.str());
            return false;
        }
        pass();
        return true;

    }

    template <class Collection>
    bool expectCollectionEquals (
        Collection const& actual, Collection const& expected,
        std::string const& message = "")
    {
        auto msg = addPrefix (message);
        bool success = expectEquals (actual.size(), expected.size(),
                                     msg + "Sizes are different");
        using std::begin;
        using std::end;

        auto i = begin (actual);
        auto j = begin (expected);
        auto k = 0;

        for (; i != end (actual) && j != end (expected); ++i, ++j, ++k)
        {
            if (!expectEquals (*i, *j, msg + "Elements at " +
                               std::to_string(k) + " are different."))
                return false;
        }

        return success;
    }

    template <class Exception, class Functor>
    bool expectException (Functor f, std::string const& message = "")
    {
        bool success = false;
        try
        {
            f();
        } catch (Exception const&)
        {
            success = true;
        }
        return expect (success, addPrefix (message) + "no exception thrown");
    }

    template <class Functor>
    bool expectException (Functor f, std::string const& message = "")
    {
        bool success = false;
        try
        {
            f();
        }
        catch (std::exception const&)
        {
            success = true;
        }
        return expect (success, addPrefix (message) + "no exception thrown");
    }

private:
    static std::string addPrefix (std::string const& message)
    {
        std::string msg = message;
        if (!msg.empty())
            msg = ": " + msg;
        return msg;
    }
};

} //涟漪

#endif
