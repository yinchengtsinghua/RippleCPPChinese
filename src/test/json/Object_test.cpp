
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

#include <ripple/json/Object.h>
#include <test/json/TestOutputSuite.h>
#include <ripple/beast/unit_test.h>

namespace Json {

class JsonObject_test : public ripple::test::TestOutputSuite
{
    void setup (std::string const& testName)
    {
        testcase (testName);
        output_.clear ();
    }

    std::unique_ptr<WriterObject> writerObject_;

    Object& makeRoot()
    {
        writerObject_ = std::make_unique<WriterObject> (
            stringWriterObject (output_));
        return **writerObject_;
    }

    void expectResult (std::string const& expected)
    {
        writerObject_.reset();
        TestOutputSuite::expectResult (expected);
    }

public:
    void testTrivial ()
    {
        setup ("trivial");

        {
            auto& root = makeRoot();
            (void) root;
        }
        expectResult ("{}");
    }

    void testSimple ()
    {
        setup ("simple");
        {
            auto& root = makeRoot();
            root["hello"] = "world";
            root["skidoo"] = 23;
            root["awake"] = false;
            root["temperature"] = 98.6;
        }

        expectResult (
            "{\"hello\":\"world\","
            "\"skidoo\":23,"
            "\"awake\":false,"
            "\"temperature\":98.6}");
    }

    void testOneSub ()
    {
        setup ("oneSub");
        {
            auto& root = makeRoot();
            root.setArray ("ar");
        }
        expectResult ("{\"ar\":[]}");
    }

    void testSubs ()
    {
        setup ("subs");
        {
            auto& root = makeRoot();

            {
//添加一个包含三个条目的数组。
                auto array = root.setArray ("ar");
                array.append (23);
                array.append (false);
                array.append (23.5);
            }

            {
//添加具有一个条目的对象。
                auto obj = root.setObject ("obj");
                obj["hello"] = "world";
            }

            {
//添加另一个包含两个条目的对象。
                Json::Value value;
                value["h"] = "w";
                value["f"] = false;
                root["obj2"] = value;
            }
        }

//JSON：：值的顺序不稳定…
        auto case1 = "{\"ar\":[23,false,23.5],"
                "\"obj\":{\"hello\":\"world\"},"
                "\"obj2\":{\"h\":\"w\",\"f\":false}}";
        auto case2 = "{\"ar\":[23,false,23.5],"
                "\"obj\":{\"hello\":\"world\"},"
                "\"obj2\":{\"f\":false,\"h\":\"w\"}}";
        writerObject_.reset();
        BEAST_EXPECT(output_ == case1 || output_ == case2);
    }

    void testSubsShort ()
    {
        setup ("subsShort");

        {
            auto& root = makeRoot();

            {
//添加一个包含三个条目的数组。
                auto array = root.setArray ("ar");
                array.append (23);
                array.append (false);
                array.append (23.5);
            }

//添加具有一个条目的对象。
            root.setObject ("obj")["hello"] = "world";

            {
//添加另一个包含两个条目的对象。
                auto object = root.setObject ("obj2");
                object.set("h", "w");
                object.set("f", false);
            }
        }
        expectResult (
            "{\"ar\":[23,false,23.5],"
            "\"obj\":{\"hello\":\"world\"},"
            "\"obj2\":{\"h\":\"w\",\"f\":false}}");
    }

    void testFailureObject()
    {
        {
            setup ("object failure assign");
            auto& root = makeRoot();
            auto obj = root.setObject ("o1");
            expectException ([&]() { root["fail"] = "complete"; });
        }
        {
            setup ("object failure object");
            auto& root = makeRoot();
            auto obj = root.setObject ("o1");
            expectException ([&] () { root.setObject ("o2"); });
        }
        {
            setup ("object failure Array");
            auto& root = makeRoot();
            auto obj = root.setArray ("o1");
            expectException ([&] () { root.setArray ("o2"); });
        }
    }

    void testFailureArray()
    {
        {
            setup ("array failure append");
            auto& root = makeRoot();
            auto array = root.setArray ("array");
            auto subarray = array.appendArray ();
            auto fail = [&]() { array.append ("fail"); };
            expectException (fail);
        }
        {
            setup ("array failure appendArray");
            auto& root = makeRoot();
            auto array = root.setArray ("array");
            auto subarray = array.appendArray ();
            auto fail = [&]() { array.appendArray (); };
            expectException (fail);
        }
        {
            setup ("array failure appendObject");
            auto& root = makeRoot();
            auto array = root.setArray ("array");
            auto subarray = array.appendArray ();
            auto fail = [&]() { array.appendObject (); };
            expectException (fail);
        }
    }

    void testKeyFailure ()
    {
        setup ("repeating keys");
        auto& root = makeRoot();
        root.set ("foo", "bar");
        root.set ("baz", 0);
//再次设置钥匙！NDECUG构建
        auto set_again = [&]() { root.set ("foo", "bar"); };
#ifdef NDEBUG
        set_again();
        pass();
#else
        expectException (set_again);
#endif
    }

    void run () override
    {
        testTrivial ();
        testSimple ();

        testOneSub ();
        testSubs ();
        testSubsShort ();

        testFailureObject ();
        testFailureArray ();
        testKeyFailure ();
    }
};

BEAST_DEFINE_TESTSUITE(JsonObject, ripple_basics, ripple);

} //杰森
