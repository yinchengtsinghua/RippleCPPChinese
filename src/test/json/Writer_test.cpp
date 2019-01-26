
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

#include <ripple/json/json_writer.h>
#include <ripple/json/Writer.h>
#include <test/json/TestOutputSuite.h>
#include <ripple/beast/unit_test.h>

namespace Json {

class JsonWriter_test : public ripple::test::TestOutputSuite
{
public:
    void testTrivial ()
    {
        setup ("trivial");
        BEAST_EXPECT(output_.empty ());
        expectResult("");
    }

    void testNearTrivial ()
    {
        setup ("near trivial");
        BEAST_EXPECT(output_.empty ());
        writer_->output (0);
        expectResult("0");
    }

    void testPrimitives ()
    {
        setup ("true");
        writer_->output (true);
        expectResult ("true");

        setup ("false");
        writer_->output (false);
        expectResult ("false");

        setup ("23");
        writer_->output (23);
        expectResult ("23");

        setup ("23.0");
        writer_->output (23.0);
        expectResult ("23.0");

        setup ("23.5");
        writer_->output (23.5);
        expectResult ("23.5");

        setup ("a string");
        writer_->output ("a string");
        expectResult ("\"a string\"");

        setup ("nullptr");
        writer_->output (nullptr);
        expectResult ("null");
    }

    void testEmpty ()
    {
        setup ("empty array");
        writer_->startRoot (Writer::array);
        writer_->finish ();
        expectResult ("[]");

        setup ("empty object");
        writer_->startRoot (Writer::object);
        writer_->finish ();
        expectResult ("{}");
    }

    void testEscaping ()
    {
        setup ("backslash");
        writer_->output ("\\");
        expectResult ("\"\\\\\"");

        setup ("quote");
        writer_->output ("\"");
        expectResult ("\"\\\"\"");

        setup ("backslash and quote");
        writer_->output ("\\\"");
        expectResult ("\"\\\\\\\"\"");

        setup ("escape embedded");
        writer_->output ("this contains a \\ in the middle of it.");
        expectResult ("\"this contains a \\\\ in the middle of it.\"");

        setup ("remaining escapes");
        writer_->output ("\b\f\n\r\t");
        expectResult ("\"\\b\\f\\n\\r\\t\"");
    }

    void testArray ()
    {
        setup ("empty array");
        writer_->startRoot (Writer::array);
        writer_->append (12);
        writer_->finish ();
        expectResult ("[12]");
    }

    void testLongArray ()
    {
        setup ("long array");
        writer_->startRoot (Writer::array);
        writer_->append (12);
        writer_->append (true);
        writer_->append ("hello");
        writer_->finish ();
        expectResult ("[12,true,\"hello\"]");
    }

    void testEmbeddedArraySimple ()
    {
        setup ("embedded array simple");
        writer_->startRoot (Writer::array);
        writer_->startAppend (Writer::array);
        writer_->finish ();
        writer_->finish ();
        expectResult ("[[]]");
    }

    void testObject ()
    {
        setup ("object");
        writer_->startRoot (Writer::object);
        writer_->set ("hello", "world");
        writer_->finish ();

        expectResult ("{\"hello\":\"world\"}");
    }

    void testComplexObject ()
    {
        setup ("complex object");
        writer_->startRoot (Writer::object);

        writer_->set ("hello", "world");
        writer_->startSet (Writer::array, "array");

        writer_->append (true);
        writer_->append (12);
        writer_->startAppend (Writer::array);
        writer_->startAppend (Writer::object);
        writer_->set ("goodbye", "cruel world.");
        writer_->startSet (Writer::array, "subarray");
        writer_->append (23.5);
        writer_->finishAll ();

        expectResult ("{\"hello\":\"world\",\"array\":[true,12,"
                      "[{\"goodbye\":\"cruel world.\","
                      "\"subarray\":[23.5]}]]}");
    }

    void testJson ()
    {
        setup ("object");
        Json::Value value (Json::objectValue);
        value["foo"] = 23;
        writer_->startRoot (Writer::object);
        writer_->set ("hello", value);
        writer_->finish ();

        expectResult ("{\"hello\":{\"foo\":23}}");
    }

    void run () override
    {
        testTrivial ();
        testNearTrivial ();
        testPrimitives ();
        testEmpty ();
        testEscaping ();
        testArray ();
        testLongArray ();
        testEmbeddedArraySimple ();
        testObject ();
        testComplexObject ();
        testJson();
    }
};

BEAST_DEFINE_TESTSUITE(JsonWriter, ripple_basics, ripple);

} //杰森
