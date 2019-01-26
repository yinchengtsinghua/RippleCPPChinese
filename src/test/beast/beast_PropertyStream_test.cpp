
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
#include <ripple/beast/unit_test.h>
#include <ripple/beast/utility/PropertyStream.h>
namespace beast {

class PropertyStream_test : public unit_test::suite
{
public:
    using Source = PropertyStream::Source;

    void test_peel_name(std::string s, std::string const& expected,
        std::string const& expected_remainder)
    {
        try
        {
            std::string const peeled_name = Source::peel_name(&s);
            BEAST_EXPECT(peeled_name == expected);
            BEAST_EXPECT(s == expected_remainder);
        }
        catch (...)
        {
            fail("unhandled exception");;
        }
    }

    void test_peel_leading_slash(std::string s, std::string const& expected,
        bool should_be_found)
    {
        try
        {
            bool const found(Source::peel_leading_slash(&s));
            BEAST_EXPECT(found == should_be_found);
            BEAST_EXPECT(s == expected);
        }
        catch (...)
        {
            fail("unhandled exception");;
        }
    }

    void test_peel_trailing_slashstar(std::string s,
        std::string const& expected_remainder, bool should_be_found)
    {
        try
        {
            bool const found(Source::peel_trailing_slashstar(&s));
            BEAST_EXPECT(found == should_be_found);
            BEAST_EXPECT(s == expected_remainder);
        }
        catch (...)
        {
            fail("unhandled exception");;
        }
    }

    void test_find_one(Source& root, Source* expected, std::string const& name)
    {
        try
        {
            Source* source(root.find_one(name));
            BEAST_EXPECT(source == expected);
        }
        catch (...)
        {
            fail("unhandled exception");;
        }
    }

    void test_find_path(Source& root, std::string const& path,
        Source* expected)
    {
        try
        {
            Source* source(root.find_path(path));
            BEAST_EXPECT(source == expected);
        }
        catch (...)
        {
            fail("unhandled exception");;
        }
    }

    void test_find_one_deep(Source& root, std::string const& name,
        Source* expected)
    {
        try
        {
            Source* source(root.find_one_deep(name));
            BEAST_EXPECT(source == expected);
        }
        catch (...)
        {
            fail("unhandled exception");;
        }
    }

    void test_find(Source& root, std::string path, Source* expected,
        bool expected_star)
    {
        try
        {
            auto const result(root.find(path));
            BEAST_EXPECT(result.first == expected);
            BEAST_EXPECT(result.second == expected_star);
        }
        catch (...)
        {
            fail("unhandled exception");;
        }
    }

    void run() override
    {
        Source a("a");
        Source b("b");
        Source c("c");
        Source d("d");
        Source e("e");
        Source f("f");
        Source g("g");

//
//A B D F，E，C G
//

        a.add(b);
        a.add(c);
        c.add(g);
        b.add(d);
        b.add(e);
        d.add(f);

        testcase("peel_name");
        test_peel_name("a", "a", "");
        test_peel_name("foo/bar", "foo", "bar");
        test_peel_name("foo/goo/bar", "foo", "goo/bar");
        test_peel_name("", "", "");

        testcase("peel_leading_slash");
        test_peel_leading_slash("foo/", "foo/", false);
        test_peel_leading_slash("foo", "foo", false);
        test_peel_leading_slash("/foo/", "foo/", true);
        test_peel_leading_slash("/foo", "foo", true);

        testcase("peel_trailing_slashstar");
        /*t眀peel眀u trailing眀slashstar（“/foo/goo/*”，/foo/goo“，true）；
        测试_Peel_Trailing_SlashStar（“foo/goo/*”，“foo/goo”，true）；
        test_peel_trailing_slashstar（“/foo/goo/”，/foo/goo“，false”）；
        测试_Peel_Trailing_SlashStar（“foo/goo”，“foo/goo”，false）；
        测试剥皮拖尾斜线星（“”，“”，假）；
        测试_Peel_Trailing_SlashStar（“/”，“”，false）；
        测试“剥皮”拖尾“斜杠星”（“/*”，“”，真）；
        测试_Peel_Trailing_SlashStar（“/”，“/”，false）；
        测试“剥皮”拖曳“斜杠星”（“**”，“*”，真）；
        测试剥皮拖尾斜杠星（*/", "*", false);


        testcase("find_one");
        test_find_one(a, &b, "b");
        test_find_one(a, nullptr, "d");
        test_find_one(b, &e, "e");
        test_find_one(d, &f, "f");

        testcase("find_path");
        test_find_path(a, "a", nullptr);
        test_find_path(a, "e", nullptr);
        test_find_path(a, "a/b", nullptr);
        test_find_path(a, "a/b/e", nullptr);
        test_find_path(a, "b/e/g", nullptr);
        test_find_path(a, "b/e/f", nullptr);
        test_find_path(a, "b", &b);
        test_find_path(a, "b/e", &e);
        test_find_path(a, "b/d/f", &f);

        testcase("find_one_deep");
        test_find_one_deep(a, "z", nullptr);
        test_find_one_deep(a, "g", &g);
        test_find_one_deep(a, "b", &b);
        test_find_one_deep(a, "d", &d);
        test_find_one_deep(a, "f", &f);

        testcase("find");
        test_find(a, "", &a, false);
        test_find(a, "*", &a, true);
        test_find(a, "/b", &b, false);
        test_find(a, "b", &b, false);
        test_find(a, "d", &d, false);
        test_find(a, "/b*", &b, true);
        test_find(a, "b*", &b, true);
        test_find(a, "d*", &d, true);
        /*t_find（a，“/b/*”，&b，真）；
        测试查找（a，“b/*”，&b，真）；
        测试查找（a，“d/*”，&d，真）；
        测试查找（a，“a”，nullptr，false）；
        测试查找（a，“/d”，nullptr，false）；
        测试查找（a，“/d*”，nullptr，true）；
        测试查找（a，“/d/*”，nullptr，true）；
    }
}；

beast定义测试套件（propertystream、utility、beast）；
}
