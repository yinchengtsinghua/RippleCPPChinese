
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

#include <ripple/basics/Log.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/st.h>
#include <ripple/json/json_reader.h>
#include <ripple/json/to_string.h>
#include <ripple/beast/unit_test.h>
#include <test/jtx.h>

#include <array>
#include <memory>
#include <type_traits>

namespace ripple {

class STObject_test : public beast::unit_test::suite
{
public:
    bool parseJSONString (std::string const& json, Json::Value& to)
    {
        Json::Reader reader;
        return reader.parse(json, to) && to.isObject();
    }

    void testParseJSONArrayWithInvalidChildrenObjects ()
    {
        testcase ("parse json array invalid children");
        try
        {
            /*

            starray/stobject构造并不能完全映射到JSON
            数组/对象。

            Stobject是一个关联容器，将字段映射到值，但是
            Stobject也可以将字段作为其名称存储在
            关联结构。名字很重要，所以要保持
            保真度，需要两个JSON对象来表示它们。

            **/

            std::string faulty ("{\"Template\":[{"
                                    "\"ModifiedNode\":{\"Sequence\":1}, "
                                    "\"DeletedNode\":{\"Sequence\":1}"
                                "}]}");

            std::unique_ptr<STObject> so;
            Json::Value faultyJson;
            bool parsedOK (parseJSONString(faulty, faultyJson));
            unexpected(!parsedOK, "failed to parse");
            STParsedJSONObject parsed ("test", faultyJson);
            BEAST_EXPECT(! parsed.object);
        }
        catch(std::runtime_error& e)
        {
            std::string what(e.what());
            unexpected (what.find("First level children of `Template`") != 0);
        }
    }

    void testParseJSONArray ()
    {
        testcase ("parse json array");
        std::string const json (
            "{\"Template\":[{\"ModifiedNode\":{\"Sequence\":1}}]}");

        Json::Value jsonObject;
        bool parsedOK (parseJSONString(json, jsonObject));
        if (parsedOK)
        {
            STParsedJSONObject parsed ("test", jsonObject);
            BEAST_EXPECT(parsed.object);
            std::string const& serialized (
                to_string (parsed.object->getJson(0)));
            BEAST_EXPECT(serialized == json);
        }
        else
        {
            fail ("Couldn't parse json: " + json);
        }
    }

    void testParseJSONEdgeCases()
    {
        testcase("parse json object");

        {
            std::string const goodJson(
                R"({"CloseResolution":19,"Method":250,)"
                R"("TransactionResult":"tecFROZEN"})");

            Json::Value jv;
            if (BEAST_EXPECT(parseJSONString(goodJson, jv)))
            {
                STParsedJSONObject parsed("test", jv);
                if (BEAST_EXPECT(parsed.object))
                {
                    std::string const& serialized(
                        to_string(parsed.object->getJson(0)));
                    BEAST_EXPECT(serialized == goodJson);
                }
            }
        }

        {
            std::string const goodJson(
                R"({"CloseResolution":19,"Method":"250",)"
                R"("TransactionResult":"tecFROZEN"})");
            std::string const expectedJson(
                R"({"CloseResolution":19,"Method":250,)"
                R"("TransactionResult":"tecFROZEN"})");

            Json::Value jv;
            if (BEAST_EXPECT(parseJSONString(goodJson, jv)))
            {
//整数值总是被解析为int，
//除非它们太大。我们要一个小的uint。
                jv["CloseResolution"] = Json::UInt(19);
                STParsedJSONObject parsed("test", jv);
                if (BEAST_EXPECT(parsed.object))
                {
                    std::string const& serialized(
                        to_string(parsed.object->getJson(0)));
                    BEAST_EXPECT(serialized == expectedJson);
                }
            }
        }

        {
            std::string const json(
                R"({"CloseResolution":19,"Method":250,)"
                R"("TransactionResult":"terQUEUED"})");

            Json::Value jv;
            if (BEAST_EXPECT(parseJSONString(json, jv)))
            {
                STParsedJSONObject parsed("test", jv);
                BEAST_EXPECT(!parsed.object);
                BEAST_EXPECT(parsed.error);
                BEAST_EXPECT(parsed.error[jss::error] == "invalidParams");
                BEAST_EXPECT(parsed.error[jss::error_message] ==
                    "Field 'test.TransactionResult' is out of range.");
            }
        }

        {
            std::string const json(
                R"({"CloseResolution":19,"Method":"pony",)"
                R"("TransactionResult":"tesSUCCESS"})");

            Json::Value jv;
            if (BEAST_EXPECT(parseJSONString(json, jv)))
            {
                STParsedJSONObject parsed("test", jv);
                BEAST_EXPECT(!parsed.object);
                BEAST_EXPECT(parsed.error);
                BEAST_EXPECT(parsed.error[jss::error] == "invalidParams");
                BEAST_EXPECT(parsed.error[jss::error_message] ==
                    "Field 'test.Method' has bad type.");
            }
        }

        {
            std::string const json(
                R"({"CloseResolution":19,"Method":3294967296,)"
                R"("TransactionResult":"tesSUCCESS"})");

            Json::Value jv;
            if (BEAST_EXPECT(parseJSONString(json, jv)))
            {
                STParsedJSONObject parsed("test", jv);
                BEAST_EXPECT(!parsed.object);
                BEAST_EXPECT(parsed.error);
                BEAST_EXPECT(parsed.error[jss::error] == "invalidParams");
                BEAST_EXPECT(parsed.error[jss::error_message] ==
                    "Field 'test.Method' is out of range.");
            }
        }

        {
            std::string const json(
                R"({"CloseResolution":-10,"Method":42,)"
                R"("TransactionResult":"tesSUCCESS"})");

            Json::Value jv;
            if (BEAST_EXPECT(parseJSONString(json, jv)))
            {
                STParsedJSONObject parsed("test", jv);
                BEAST_EXPECT(!parsed.object);
                BEAST_EXPECT(parsed.error);
                BEAST_EXPECT(parsed.error[jss::error] == "invalidParams");
                BEAST_EXPECT(parsed.error[jss::error_message] ==
                    "Field 'test.CloseResolution' is out of range.");
            }
        }

        {
            std::string const json(
                R"({"CloseResolution":19,"Method":3.141592653,)"
                R"("TransactionResult":"tesSUCCESS"})");

            Json::Value jv;
            if (BEAST_EXPECT(parseJSONString(json, jv)))
            {
                STParsedJSONObject parsed("test", jv);
                BEAST_EXPECT(!parsed.object);
                BEAST_EXPECT(parsed.error);
                BEAST_EXPECT(parsed.error[jss::error] == "invalidParams");
                BEAST_EXPECT(parsed.error[jss::error_message] ==
                    "Field 'test.Method' has bad type.");
            }
        }
    }

    void testSerialization ()
    {
        testcase ("serialization");

        unexpected (sfGeneric.isUseful (), "sfGeneric must not be useful");

        SField const& sfTestVL = SField::getField (STI_VL, 255);
        SField const& sfTestH256 = SField::getField (STI_HASH256, 255);
        SField const& sfTestU32 = SField::getField (STI_UINT32, 255);
        SField const& sfTestV256 = SField::getField(STI_VECTOR256, 255);
        SField const& sfTestObject = SField::getField (STI_OBJECT, 255);

        SOTemplate elements;
        elements.push_back (SOElement (sfFlags, SOE_REQUIRED));
        elements.push_back (SOElement (sfTestVL, SOE_REQUIRED));
        elements.push_back (SOElement (sfTestH256, SOE_OPTIONAL));
        elements.push_back (SOElement (sfTestU32, SOE_REQUIRED));
        elements.push_back (SOElement (sfTestV256, SOE_OPTIONAL));

        STObject object1 (elements, sfTestObject);
        STObject object2 (object1);

        unexpected (object1.getSerializer () != object2.getSerializer (),
            "STObject error 1");

        unexpected (object1.isFieldPresent (sfTestH256) ||
            !object1.isFieldPresent (sfTestVL), "STObject error");

        object1.makeFieldPresent (sfTestH256);

        unexpected (!object1.isFieldPresent (sfTestH256), "STObject Error 2");

        unexpected (object1.getFieldH256 (sfTestH256) != uint256 (),
            "STObject error 3");

        if (object1.getSerializer () == object2.getSerializer ())
        {
            log <<
                "O1: " << object1.getJson (0) << '\n' <<
                "O2: " << object2.getJson (0) << std::endl;
            fail ("STObject error 4");
        }
        else
        {
            pass ();
        }

        object1.makeFieldAbsent (sfTestH256);

        unexpected (object1.isFieldPresent (sfTestH256), "STObject error 5");

        unexpected (object1.getFlags () != 0, "STObject error 6");

        unexpected (object1.getSerializer () != object2.getSerializer (),
            "STObject error 7");

        STObject copy (object1);

        unexpected (object1.isFieldPresent (sfTestH256), "STObject error 8");

        unexpected (copy.isFieldPresent (sfTestH256), "STObject error 9");

        unexpected (object1.getSerializer () != copy.getSerializer (),
            "STObject error 10");

        copy.setFieldU32 (sfTestU32, 1);

        unexpected (object1.getSerializer () == copy.getSerializer (),
            "STObject error 11");

        for (int i = 0; i < 1000; i++)
        {
            Blob j (i, 2);

            object1.setFieldVL (sfTestVL, j);

            Serializer s;
            object1.add (s);
            SerialIter it (s.slice());

            STObject object3 (elements, it, sfTestObject);

            unexpected (object1.getFieldVL (sfTestVL) != j, "STObject error");

            unexpected (object3.getFieldVL (sfTestVL) != j, "STObject error");
        }

        {
            std::vector<uint256> uints;
            uints.reserve(5);
            for (int i = 0; i < uints.capacity(); ++i)
            {
                uints.emplace_back(i);
            }
            object1.setFieldV256(sfTestV256, STVector256(uints));

            Serializer s;
            object1.add(s);
            SerialIter it(s.slice());

            STObject object3(elements, it, sfTestObject);

            auto const& uints1 = object1.getFieldV256(sfTestV256);
            auto const& uints3 = object3.getFieldV256(sfTestV256);

            BEAST_EXPECT(uints1 == uints3);
        }
    }

//练习字段访问器
    void
    testFields()
    {
        testcase ("fields");

        auto const& sf1 = sfSequence;
        auto const& sf2 = sfExpiration;
        auto const& sf3 = sfQualityIn;
        auto const& sf4 = sfSignature;
        auto const& sf5 = sfPublicKey;

//读取自由对象

        {
            auto const st = [&]()
            {
                STObject st(sfGeneric);
                st.setFieldU32(sf1, 1);
                st.setFieldU32(sf2, 2);
                return st;
            }();

            BEAST_EXPECT(st[sf1] == 1);
            BEAST_EXPECT(st[sf2] == 2);
            except<STObject::FieldErr>([&]()
                { st[sf3]; });
            BEAST_EXPECT(*st[~sf1] == 1);
            BEAST_EXPECT(*st[~sf2] == 2);
            BEAST_EXPECT(st[~sf3] == boost::none);
            BEAST_EXPECT(!! st[~sf1]);
            BEAST_EXPECT(!! st[~sf2]);
            BEAST_EXPECT(! st[~sf3]);
            BEAST_EXPECT(st[sf1] != st[sf2]);
            BEAST_EXPECT(st[~sf1] != st[~sf2]);
        }

//读取模板化对象

        auto const sot = [&]()
        {
            SOTemplate sot;
            sot.push_back(SOElement(sf1, SOE_REQUIRED));
            sot.push_back(SOElement(sf2, SOE_OPTIONAL));
            sot.push_back(SOElement(sf3, SOE_DEFAULT));
            sot.push_back(SOElement(sf4, SOE_OPTIONAL));
            sot.push_back(SOElement(sf5, SOE_DEFAULT));
            return sot;
        }();

        {
            auto const st = [&]()
            {
                STObject st(sot, sfGeneric);
                st.setFieldU32(sf1, 1);
                st.setFieldU32(sf2, 2);
                return st;
            }();

            BEAST_EXPECT(st[sf1] == 1);
            BEAST_EXPECT(st[sf2] == 2);
            BEAST_EXPECT(st[sf3] == 0);
            BEAST_EXPECT(*st[~sf1] == 1);
            BEAST_EXPECT(*st[~sf2] == 2);
            BEAST_EXPECT(*st[~sf3] == 0);
            BEAST_EXPECT(!! st[~sf1]);
            BEAST_EXPECT(!! st[~sf2]);
            BEAST_EXPECT(!! st[~sf3]);
        }

//写自由对象

        {
            STObject st(sfGeneric);
            unexcept([&]() { st[sf1]; });
            except([&](){ return st[sf1] == 0; });
            BEAST_EXPECT(st[~sf1] == boost::none);
            BEAST_EXPECT(st[~sf1] == boost::optional<std::uint32_t>{});
            BEAST_EXPECT(st[~sf1] != boost::optional<std::uint32_t>(1));
            BEAST_EXPECT(! st[~sf1]);
            st[sf1] = 2;
            BEAST_EXPECT(st[sf1] == 2);
            BEAST_EXPECT(st[~sf1] != boost::none);
            BEAST_EXPECT(st[~sf1] == boost::optional<std::uint32_t>(2));
            BEAST_EXPECT(!! st[~sf1]);
            st[sf1] = 1;
            BEAST_EXPECT(st[sf1] == 1);
            BEAST_EXPECT(!! st[sf1]);
            BEAST_EXPECT(!! st[~sf1]);
            st[sf1] = 0;
            BEAST_EXPECT(! st[sf1]);
            BEAST_EXPECT(!! st[~sf1]);
            st[~sf1] = boost::none;
            BEAST_EXPECT(! st[~sf1]);
            BEAST_EXPECT(st[~sf1] == boost::none);
            BEAST_EXPECT(st[~sf1] == boost::optional<std::uint32_t>{});
            st[~sf1] = boost::none;
            BEAST_EXPECT(! st[~sf1]);
            except([&]() { return st[sf1] == 0; });
            except([&]() { return *st[~sf1]; });
            st[sf1] = 1;
            BEAST_EXPECT(st[sf1] == 1);
            BEAST_EXPECT(!! st[sf1]);
            BEAST_EXPECT(!! st[~sf1]);
            st[sf1] = 3;
            st[sf2] = st[sf1];
            BEAST_EXPECT(st[sf1] == 3);
            BEAST_EXPECT(st[sf2] == 3);
            BEAST_EXPECT(st[sf2] == st[sf1]);
            st[sf1] = 4;
            st[sf2] = st[sf1];
            BEAST_EXPECT(st[sf1] == 4);
            BEAST_EXPECT(st[sf2] == 4);
            BEAST_EXPECT(st[sf2] == st[sf1]);
        }

//写入模板化对象

        {
            STObject st(sot, sfGeneric);
            BEAST_EXPECT(!! st[~sf1]);
            BEAST_EXPECT(st[~sf1] != boost::none);
            BEAST_EXPECT(st[sf1] == 0);
            BEAST_EXPECT(*st[~sf1] == 0);
            BEAST_EXPECT(! st[~sf2]);
            BEAST_EXPECT(st[~sf2] == boost::none);
            except([&]() { return st[sf2] == 0; });
            BEAST_EXPECT(!! st[~sf3]);
            BEAST_EXPECT(st[~sf3] != boost::none);
            BEAST_EXPECT(st[sf3] == 0);
            except([&]() { st[~sf1] = boost::none; });
            st[sf1] = 1;
            BEAST_EXPECT(st[sf1] == 1);
            BEAST_EXPECT(*st[~sf1] == 1);
            BEAST_EXPECT(!! st[~sf1]);
            st[sf1] = 0;
            BEAST_EXPECT(st[sf1] == 0);
            BEAST_EXPECT(*st[~sf1] == 0);
            BEAST_EXPECT(!! st[~sf1]);
            st[sf2] = 2;
            BEAST_EXPECT(st[sf2] == 2);
            BEAST_EXPECT(*st[~sf2] == 2);
            BEAST_EXPECT(!! st[~sf2]);
            st[~sf2] = boost::none;
            except([&]() { return *st[~sf2]; });
            BEAST_EXPECT(! st[~sf2]);
            st[sf3] = 3;
            BEAST_EXPECT(st[sf3] == 3);
            BEAST_EXPECT(*st[~sf3] == 3);
            BEAST_EXPECT(!! st[~sf3]);
            st[sf3] = 2;
            BEAST_EXPECT(st[sf3] == 2);
            BEAST_EXPECT(*st[~sf3] == 2);
            BEAST_EXPECT(!! st[~sf3]);
            st[sf3] = 0;
            BEAST_EXPECT(st[sf3] == 0);
            BEAST_EXPECT(*st[~sf3] == 0);
            BEAST_EXPECT(!! st[~sf3]);
            except([&]() { st[~sf3] = boost::none; });
            BEAST_EXPECT(st[sf3] == 0);
            BEAST_EXPECT(*st[~sf3] == 0);
            BEAST_EXPECT(!! st[~sf3]);
        }

//强制运算符Boost：：可选

        {
            STObject st(sfGeneric);
            auto const v = ~st[~sf1];
            static_assert(std::is_same<
                std::decay_t<decltype(v)>,
                    boost::optional<std::uint32_t>>::value, "");
        }

//UDT标量字段

        {
            STObject st(sfGeneric);
            st[sfAmount] = STAmount{};
            st[sfAccount] = AccountID{};
            st[sfDigest] = uint256{};
            [&](STAmount){}(st[sfAmount]);
            [&](AccountID){}(st[sfAccount]);
            [&](uint256){}(st[sfDigest]);
        }

//stblob和slice

        {
            {
                STObject st(sfGeneric);
                Buffer b(1);
                BEAST_EXPECT(! b.empty());
                st[sf4] = std::move(b);
                BEAST_EXPECT(b.empty());
                BEAST_EXPECT(Slice(st[sf4]).size() == 1);
                st[~sf4] = boost::none;
                BEAST_EXPECT(! ~st[~sf4]);
                b = Buffer{2};
                st[sf4] = Slice(b);
                BEAST_EXPECT(b.size() == 2);
                BEAST_EXPECT(Slice(st[sf4]).size() == 2);
                st[sf5] = st[sf4];
                BEAST_EXPECT(Slice(st[sf4]).size() == 2);
                BEAST_EXPECT(Slice(st[sf5]).size() == 2);
            }
            {
                STObject st(sot, sfGeneric);
                BEAST_EXPECT(st[sf5] == Slice{});
                BEAST_EXPECT(!! st[~sf5]);
                BEAST_EXPECT(!! ~st[~sf5]);
                Buffer b(1);
                st[sf5] = std::move(b);
                BEAST_EXPECT(b.empty());
                BEAST_EXPECT(Slice(st[sf5]).size() == 1);
                st[~sf4] = boost::none;
                BEAST_EXPECT(! ~st[~sf4]);
            }
        }

//UDT斑点

        {
            STObject st(sfGeneric);
            BEAST_EXPECT(! st[~sf5]);
            auto const kp = generateKeyPair(
                KeyType::secp256k1,
                    generateSeed("masterpassphrase"));
            st[sf5] = kp.first;
            BEAST_EXPECT(st[sf5] != PublicKey{});
            st[~sf5] = boost::none;
#if 0
            pk = st[sf5];
            BEAST_EXPECT(pk.size() == 0);
#endif
        }

//按引用字段

        {
            auto const& sf = sfIndexes;
            STObject st(sfGeneric);
            std::vector<uint256> v;
            v.emplace_back(1);
            v.emplace_back(2);
            st[sf] = v;
            st[sf] = std::move(v);
            auto const& cst = st;
            BEAST_EXPECT(cst[sf].size() == 2);
            BEAST_EXPECT(cst[~sf]->size() == 2);
            BEAST_EXPECT(cst[sf][0] == 1);
            BEAST_EXPECT(cst[sf][1] == 2);
            static_assert(std::is_same<decltype(cst[sfIndexes]),
                std::vector<uint256> const&>::value, "");
        }

//按引用字段默认

        {
            auto const& sf1 = sfIndexes;
            auto const& sf2 = sfHashes;
            auto const& sf3 = sfAmendments;
            auto const sot = [&]()
            {
                SOTemplate sot;
                sot.push_back(SOElement(sf1, SOE_REQUIRED));
                sot.push_back(SOElement(sf2, SOE_OPTIONAL));
                sot.push_back(SOElement(sf3, SOE_DEFAULT));
                return sot;
            }();
            STObject st(sot, sfGeneric);
            auto const& cst(st);
            BEAST_EXPECT(cst[sf1].size() == 0);
            BEAST_EXPECT(! cst[~sf2]);
            BEAST_EXPECT(cst[sf3].size() == 0);
            std::vector<uint256> v;
            v.emplace_back(1);
            st[sf1] = v;
            BEAST_EXPECT(cst[sf1].size() == 1);
            BEAST_EXPECT(cst[sf1][0] == uint256{1});
            st[sf2] = v;
            BEAST_EXPECT(cst[sf2].size() == 1);
            BEAST_EXPECT(cst[sf2][0] == uint256{1});
            st[~sf2] = boost::none;
            BEAST_EXPECT(! st[~sf2]);
            st[sf3] = v;
            BEAST_EXPECT(cst[sf3].size() == 1);
            BEAST_EXPECT(cst[sf3][0] == uint256{1});
            st[sf3] = std::vector<uint256>{};
            BEAST_EXPECT(cst[sf3].size() == 0);
        }
    }

    void
    testMalformed()
    {
        testcase ("Malformed serialized forms");

        try
        {
            std::array<std::uint8_t, 5> const payload {{ 0xee, 0xee, 0xe1, 0xee, 0xee }};
            SerialIter sit{makeSlice(payload)};
            auto obj = std::make_shared<STArray>(sit, sfMetadata);
            BEAST_EXPECT(!obj);
        }
        catch (std::exception const& e)
        {
            BEAST_EXPECT(strcmp(e.what(),
                "Duplicate field detected") == 0);
        }

        try
        {
            std::array<std::uint8_t, 3> const payload {{ 0xe2, 0xe1, 0xe2 }};
            SerialIter sit{makeSlice(payload)};
            auto obj = std::make_shared<STObject>(sit, sfMetadata);
               BEAST_EXPECT(!obj);
        }
        catch (std::exception const& e)
        {
            BEAST_EXPECT(strcmp(e.what(),
                "Duplicate field detected") == 0);
        }

        try
        {
            std::array<std::uint8_t, 250> const payload
            {{
                0x12, 0x00, 0x65, 0x24, 0x00, 0x00, 0x00, 0x00, 0x20, 0x1e, 0x00, 0x4f,
                0x00, 0x00, 0x20, 0x1f, 0x03, 0xf6, 0x00, 0x00, 0x20, 0x20, 0x00, 0x00,
                0x00, 0x00, 0x35, 0x00, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68,
                0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x73, 0x00, 0x81, 0x14,
                0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x24, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe5, 0xfe, 0xf3, 0xe7, 0xe5, 0x65,
                0x24, 0x00, 0x00, 0x00, 0x00, 0x20, 0x1e, 0x00, 0x6f, 0x00, 0x00, 0x20,
                0x1f, 0x03, 0xf6, 0x00, 0x00, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x35,
                0x00, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x40, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x02, 0x00, 0x12, 0x00, 0x65, 0x24, 0x00, 0x00, 0x00,
                0x00, 0x20, 0x1e, 0x00, 0x4f, 0x00, 0x00, 0x20, 0x1f, 0x03, 0xf6, 0x00,
                0x00, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x35, 0x24, 0x59, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
                0x00, 0x54, 0x72, 0x61, 0x6e, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x65, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe5,
                0xfe, 0xf3, 0xe7, 0xe5, 0x65, 0x24, 0x00, 0x00, 0x00, 0x00, 0x20, 0x1e,
                0x00, 0x6f, 0x00, 0x00, 0x20, 0xf6, 0x00, 0x00, 0x03, 0x1f, 0x20, 0x20,
                0x00, 0x00, 0x00, 0x00, 0x35, 0x00, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x73, 0x00,
                0x81, 0x14, 0x00, 0x10, 0x00, 0x73, 0x00, 0x81, 0x14, 0x00, 0x10, 0x00,
                0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x00, 0xe5, 0xfe
            }};

            SerialIter sit{makeSlice(payload)};
            auto obj = std::make_shared<STTx>(sit);
            BEAST_EXPECT(!obj);
        }
        catch (std::exception const& e)
        {
            BEAST_EXPECT(strcmp(e.what(),
                "Duplicate field detected") == 0);
        }
    }

    void
    run() override
    {
//实例化一个jtx：：env，以便执行debuglog写入。
        test::jtx::Env env (*this);

        testFields();
        testSerialization();
        testParseJSONArray();
        testParseJSONArrayWithInvalidChildrenObjects();
        testParseJSONEdgeCases();
        testMalformed();
    }
};

BEAST_DEFINE_TESTSUITE(STObject,protocol,ripple);

} //涟漪
