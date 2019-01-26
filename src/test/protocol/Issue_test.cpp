
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

#include <ripple/basics/UnorderedContainers.h>
#include <ripple/protocol/Book.h>
#include <ripple/protocol/Issue.h>
#include <ripple/beast/unit_test.h>
#include <map>
#include <set>
#include <typeinfo>
#include <unordered_set>

#if BEAST_MSVC
# define STL_SET_HAS_EMPLACE 1
#else
# define STL_SET_HAS_EMPLACE 0
#endif

#ifndef RIPPLE_ASSETS_ENABLE_STD_HASH
# if BEAST_MAC || BEAST_IOS
#  define RIPPLE_ASSETS_ENABLE_STD_HASH 0
# else
#  define RIPPLE_ASSETS_ENABLE_STD_HASH 1
# endif
#endif

namespace ripple {

class Issue_test : public beast::unit_test::suite
{
public:
//比较，uint60的哈希测试（通过基值uint）
    template <typename Unsigned>
    void testUnsigned ()
    {
        Unsigned const u1 (1);
        Unsigned const u2 (2);
        Unsigned const u3 (3);

        BEAST_EXPECT(u1 != u2);
        BEAST_EXPECT(u1 <  u2);
        BEAST_EXPECT(u1 <= u2);
        BEAST_EXPECT(u2 <= u2);
        BEAST_EXPECT(u2 == u2);
        BEAST_EXPECT(u2 >= u2);
        BEAST_EXPECT(u3 >= u2);
        BEAST_EXPECT(u3 >  u2);

        std::hash <Unsigned> hash;

        BEAST_EXPECT(hash (u1) == hash (u1));
        BEAST_EXPECT(hash (u2) == hash (u2));
        BEAST_EXPECT(hash (u3) == hash (u3));
        BEAST_EXPECT(hash (u1) != hash (u2));
        BEAST_EXPECT(hash (u1) != hash (u3));
        BEAST_EXPECT(hash (u2) != hash (u3));
    }

//——————————————————————————————————————————————————————————————

//比较，问题的哈希测试
    template <class Issue>
    void testIssue ()
    {
        Currency const c1 (1); AccountID const i1 (1);
        Currency const c2 (2); AccountID const i2 (2);
        Currency const c3 (3); AccountID const i3 (3);

        BEAST_EXPECT(Issue (c1, i1) != Issue (c2, i1));
        BEAST_EXPECT(Issue (c1, i1) <  Issue (c2, i1));
        BEAST_EXPECT(Issue (c1, i1) <= Issue (c2, i1));
        BEAST_EXPECT(Issue (c2, i1) <= Issue (c2, i1));
        BEAST_EXPECT(Issue (c2, i1) == Issue (c2, i1));
        BEAST_EXPECT(Issue (c2, i1) >= Issue (c2, i1));
        BEAST_EXPECT(Issue (c3, i1) >= Issue (c2, i1));
        BEAST_EXPECT(Issue (c3, i1) >  Issue (c2, i1));
        BEAST_EXPECT(Issue (c1, i1) != Issue (c1, i2));
        BEAST_EXPECT(Issue (c1, i1) <  Issue (c1, i2));
        BEAST_EXPECT(Issue (c1, i1) <= Issue (c1, i2));
        BEAST_EXPECT(Issue (c1, i2) <= Issue (c1, i2));
        BEAST_EXPECT(Issue (c1, i2) == Issue (c1, i2));
        BEAST_EXPECT(Issue (c1, i2) >= Issue (c1, i2));
        BEAST_EXPECT(Issue (c1, i3) >= Issue (c1, i2));
        BEAST_EXPECT(Issue (c1, i3) >  Issue (c1, i2));

        std::hash <Issue> hash;

        BEAST_EXPECT(hash (Issue (c1, i1)) == hash (Issue (c1, i1)));
        BEAST_EXPECT(hash (Issue (c1, i2)) == hash (Issue (c1, i2)));
        BEAST_EXPECT(hash (Issue (c1, i3)) == hash (Issue (c1, i3)));
        BEAST_EXPECT(hash (Issue (c2, i1)) == hash (Issue (c2, i1)));
        BEAST_EXPECT(hash (Issue (c2, i2)) == hash (Issue (c2, i2)));
        BEAST_EXPECT(hash (Issue (c2, i3)) == hash (Issue (c2, i3)));
        BEAST_EXPECT(hash (Issue (c3, i1)) == hash (Issue (c3, i1)));
        BEAST_EXPECT(hash (Issue (c3, i2)) == hash (Issue (c3, i2)));
        BEAST_EXPECT(hash (Issue (c3, i3)) == hash (Issue (c3, i3)));
        BEAST_EXPECT(hash (Issue (c1, i1)) != hash (Issue (c1, i2)));
        BEAST_EXPECT(hash (Issue (c1, i1)) != hash (Issue (c1, i3)));
        BEAST_EXPECT(hash (Issue (c1, i1)) != hash (Issue (c2, i1)));
        BEAST_EXPECT(hash (Issue (c1, i1)) != hash (Issue (c2, i2)));
        BEAST_EXPECT(hash (Issue (c1, i1)) != hash (Issue (c2, i3)));
        BEAST_EXPECT(hash (Issue (c1, i1)) != hash (Issue (c3, i1)));
        BEAST_EXPECT(hash (Issue (c1, i1)) != hash (Issue (c3, i2)));
        BEAST_EXPECT(hash (Issue (c1, i1)) != hash (Issue (c3, i3)));
    }

    template <class Set>
    void testIssueSet ()
    {
        Currency const c1 (1);
        AccountID   const i1 (1);
        Currency const c2 (2);
        AccountID   const i2 (2);
        Issue const a1 (c1, i1);
        Issue const a2 (c2, i2);

        {
            Set c;

            c.insert (a1);
            if (! BEAST_EXPECT(c.size () == 1)) return;
            c.insert (a2);
            if (! BEAST_EXPECT(c.size () == 2)) return;

            if (! BEAST_EXPECT(c.erase (Issue (c1, i2)) == 0)) return;
            if (! BEAST_EXPECT(c.erase (Issue (c1, i1)) == 1)) return;
            if (! BEAST_EXPECT(c.erase (Issue (c2, i2)) == 1)) return;
            if (! BEAST_EXPECT(c.empty ())) return;
        }

        {
            Set c;

            c.insert (a1);
            if (! BEAST_EXPECT(c.size () == 1)) return;
            c.insert (a2);
            if (! BEAST_EXPECT(c.size () == 2)) return;

            if (! BEAST_EXPECT(c.erase (Issue (c1, i2)) == 0)) return;
            if (! BEAST_EXPECT(c.erase (Issue (c1, i1)) == 1)) return;
            if (! BEAST_EXPECT(c.erase (Issue (c2, i2)) == 1)) return;
            if (! BEAST_EXPECT(c.empty ())) return;

    #if STL_SET_HAS_EMPLACE
            c.emplace (c1, i1);
            if (! BEAST_EXPECT(c.size() == 1)) return;
            c.emplace (c2, i2);
            if (! BEAST_EXPECT(c.size() == 2)) return;
    #endif
        }
    }

    template <class Map>
    void testIssueMap ()
    {
        Currency const c1 (1);
        AccountID   const i1 (1);
        Currency const c2 (2);
        AccountID   const i2 (2);
        Issue const a1 (c1, i1);
        Issue const a2 (c2, i2);

        {
            Map c;

            c.insert (std::make_pair (a1, 1));
            if (! BEAST_EXPECT(c.size () == 1)) return;
            c.insert (std::make_pair (a2, 2));
            if (! BEAST_EXPECT(c.size () == 2)) return;

            if (! BEAST_EXPECT(c.erase (Issue (c1, i2)) == 0)) return;
            if (! BEAST_EXPECT(c.erase (Issue (c1, i1)) == 1)) return;
            if (! BEAST_EXPECT(c.erase (Issue (c2, i2)) == 1)) return;
            if (! BEAST_EXPECT(c.empty ())) return;
        }

        {
            Map c;

            c.insert (std::make_pair (a1, 1));
            if (! BEAST_EXPECT(c.size () == 1)) return;
            c.insert (std::make_pair (a2, 2));
            if (! BEAST_EXPECT(c.size () == 2)) return;

            if (! BEAST_EXPECT(c.erase (Issue (c1, i2)) == 0)) return;
            if (! BEAST_EXPECT(c.erase (Issue (c1, i1)) == 1)) return;
            if (! BEAST_EXPECT(c.erase (Issue (c2, i2)) == 1)) return;
            if (! BEAST_EXPECT(c.empty ())) return;
        }
    }

    void testIssueSets ()
    {
        testcase ("std::set <Issue>");
        testIssueSet <std::set <Issue>> ();

        testcase ("std::set <Issue>");
        testIssueSet <std::set <Issue>> ();

#if RIPPLE_ASSETS_ENABLE_STD_HASH
        testcase ("std::unordered_set <Issue>");
        testIssueSet <std::unordered_set <Issue>> ();

        testcase ("std::unordered_set <Issue>");
        testIssueSet <std::unordered_set <Issue>> ();
#endif

        testcase ("hash_set <Issue>");
        testIssueSet <hash_set <Issue>> ();

        testcase ("hash_set <Issue>");
        testIssueSet <hash_set <Issue>> ();
    }

    void testIssueMaps ()
    {
        testcase ("std::map <Issue, int>");
        testIssueMap <std::map <Issue, int>> ();

        testcase ("std::map <Issue, int>");
        testIssueMap <std::map <Issue, int>> ();

#if RIPPLE_ASSETS_ENABLE_STD_HASH
        testcase ("std::unordered_map <Issue, int>");
        testIssueMap <std::unordered_map <Issue, int>> ();

        testcase ("std::unordered_map <Issue, int>");
        testIssueMap <std::unordered_map <Issue, int>> ();

        testcase ("hash_map <Issue, int>");
        testIssueMap <hash_map <Issue, int>> ();

        testcase ("hash_map <Issue, int>");
        testIssueMap <hash_map <Issue, int>> ();

#endif
    }

//——————————————————————————————————————————————————————————————

//比较，书籍散列测试
    template <class Book>
    void testBook ()
    {
        Currency const c1 (1); AccountID const i1 (1);
        Currency const c2 (2); AccountID const i2 (2);
        Currency const c3 (3); AccountID const i3 (3);

        Issue a1 (c1, i1);
        Issue a2 (c1, i2);
        Issue a3 (c2, i2);
        Issue a4 (c3, i2);

        BEAST_EXPECT(Book (a1, a2) != Book (a2, a3));
        BEAST_EXPECT(Book (a1, a2) <  Book (a2, a3));
        BEAST_EXPECT(Book (a1, a2) <= Book (a2, a3));
        BEAST_EXPECT(Book (a2, a3) <= Book (a2, a3));
        BEAST_EXPECT(Book (a2, a3) == Book (a2, a3));
        BEAST_EXPECT(Book (a2, a3) >= Book (a2, a3));
        BEAST_EXPECT(Book (a3, a4) >= Book (a2, a3));
        BEAST_EXPECT(Book (a3, a4) >  Book (a2, a3));

        std::hash <Book> hash;

//日志<<std:：hex<<hash（book（a1，a2））；
//日志<<std:：hex<<hash（book（a1，a2））；
//
//日志<<std:：hex<<hash（book（a1，a3））；
//日志<<std:：hex<<hash（book（a1，a3））；
//
//日志<<std:：hex<<hash（book（a1，a4））；
//日志<<std:：hex<<hash（book（a1，a4））；
//
//日志<<std:：hex<<hash（book（a2，a3））；
//日志<<std:：hex<<hash（book（a2，a3））；
//
//日志<<std:：hex<<hash（book（a2，a4））；
//日志<<std:：hex<<hash（book（a2，a4））；
//
//日志<<std:：hex<<hash（book（a3，a4））；
//日志<<std:：hex<<hash（book（a3，a4））；

        BEAST_EXPECT(hash (Book (a1, a2)) == hash (Book (a1, a2)));
        BEAST_EXPECT(hash (Book (a1, a3)) == hash (Book (a1, a3)));
        BEAST_EXPECT(hash (Book (a1, a4)) == hash (Book (a1, a4)));
        BEAST_EXPECT(hash (Book (a2, a3)) == hash (Book (a2, a3)));
        BEAST_EXPECT(hash (Book (a2, a4)) == hash (Book (a2, a4)));
        BEAST_EXPECT(hash (Book (a3, a4)) == hash (Book (a3, a4)));

        BEAST_EXPECT(hash (Book (a1, a2)) != hash (Book (a1, a3)));
        BEAST_EXPECT(hash (Book (a1, a2)) != hash (Book (a1, a4)));
        BEAST_EXPECT(hash (Book (a1, a2)) != hash (Book (a2, a3)));
        BEAST_EXPECT(hash (Book (a1, a2)) != hash (Book (a2, a4)));
        BEAST_EXPECT(hash (Book (a1, a2)) != hash (Book (a3, a4)));
    }

//——————————————————————————————————————————————————————————————

    template <class Set>
    void testBookSet ()
    {
        Currency const c1 (1);
        AccountID   const i1 (1);
        Currency const c2 (2);
        AccountID   const i2 (2);
        Issue const a1 (c1, i1);
        Issue const a2 (c2, i2);
        Book  const b1 (a1, a2);
        Book  const b2 (a2, a1);

        {
            Set c;

            c.insert (b1);
            if (! BEAST_EXPECT(c.size () == 1)) return;
            c.insert (b2);
            if (! BEAST_EXPECT(c.size () == 2)) return;

            if (! BEAST_EXPECT(c.erase (Book (a1, a1)) == 0)) return;
            if (! BEAST_EXPECT(c.erase (Book (a1, a2)) == 1)) return;
            if (! BEAST_EXPECT(c.erase (Book (a2, a1)) == 1)) return;
            if (! BEAST_EXPECT(c.empty ())) return;
        }

        {
            Set c;

            c.insert (b1);
            if (! BEAST_EXPECT(c.size () == 1)) return;
            c.insert (b2);
            if (! BEAST_EXPECT(c.size () == 2)) return;

            if (! BEAST_EXPECT(c.erase (Book (a1, a1)) == 0)) return;
            if (! BEAST_EXPECT(c.erase (Book (a1, a2)) == 1)) return;
            if (! BEAST_EXPECT(c.erase (Book (a2, a1)) == 1)) return;
            if (! BEAST_EXPECT(c.empty ())) return;

    #if STL_SET_HAS_EMPLACE
            c.emplace (a1, a2);
            if (! BEAST_EXPECT(c.size() == 1)) return;
            c.emplace (a2, a1);
            if (! BEAST_EXPECT(c.size() == 2)) return;
    #endif
        }
    }

    template <class Map>
    void testBookMap ()
    {
        Currency const c1 (1);
        AccountID   const i1 (1);
        Currency const c2 (2);
        AccountID   const i2 (2);
        Issue const a1 (c1, i1);
        Issue const a2 (c2, i2);
        Book  const b1 (a1, a2);
        Book  const b2 (a2, a1);

//类型名映射：：值类型值类型；
//std:：pair<book const，int>value_type；

        {
            Map c;

//c.插入（值_类型（b1，1））；
            c.insert (std::make_pair (b1, 1));
            if (! BEAST_EXPECT(c.size () == 1)) return;
//c.插入（值_类型（b2，2））；
            c.insert (std::make_pair (b2, 1));
            if (! BEAST_EXPECT(c.size () == 2)) return;

            if (! BEAST_EXPECT(c.erase (Book (a1, a1)) == 0)) return;
            if (! BEAST_EXPECT(c.erase (Book (a1, a2)) == 1)) return;
            if (! BEAST_EXPECT(c.erase (Book (a2, a1)) == 1)) return;
            if (! BEAST_EXPECT(c.empty ())) return;
        }

        {
            Map c;

//c.插入（值_类型（b1，1））；
            c.insert (std::make_pair (b1, 1));
            if (! BEAST_EXPECT(c.size () == 1)) return;
//c.插入（值_类型（b2，2））；
            c.insert (std::make_pair (b2, 1));
            if (! BEAST_EXPECT(c.size () == 2)) return;

            if (! BEAST_EXPECT(c.erase (Book (a1, a1)) == 0)) return;
            if (! BEAST_EXPECT(c.erase (Book (a1, a2)) == 1)) return;
            if (! BEAST_EXPECT(c.erase (Book (a2, a1)) == 1)) return;
            if (! BEAST_EXPECT(c.empty ())) return;
        }
    }

    void testBookSets ()
    {
        testcase ("std::set <Book>");
        testBookSet <std::set <Book>> ();

        testcase ("std::set <Book>");
        testBookSet <std::set <Book>> ();

#if RIPPLE_ASSETS_ENABLE_STD_HASH
        testcase ("std::unordered_set <Book>");
        testBookSet <std::unordered_set <Book>> ();

        testcase ("std::unordered_set <Book>");
        testBookSet <std::unordered_set <Book>> ();
#endif

        testcase ("hash_set <Book>");
        testBookSet <hash_set <Book>> ();

        testcase ("hash_set <Book>");
        testBookSet <hash_set <Book>> ();
    }

    void testBookMaps ()
    {
        testcase ("std::map <Book, int>");
        testBookMap <std::map <Book, int>> ();

        testcase ("std::map <Book, int>");
        testBookMap <std::map <Book, int>> ();

#if RIPPLE_ASSETS_ENABLE_STD_HASH
        testcase ("std::unordered_map <Book, int>");
        testBookMap <std::unordered_map <Book, int>> ();

        testcase ("std::unordered_map <Book, int>");
        testBookMap <std::unordered_map <Book, int>> ();

        testcase ("hash_map <Book, int>");
        testBookMap <hash_map <Book, int>> ();

        testcase ("hash_map <Book, int>");
        testBookMap <hash_map <Book, int>> ();
#endif
    }

//——————————————————————————————————————————————————————————————

    void run() override
    {
        testcase ("Currency");
        testUnsigned <Currency> ();

        testcase ("AccountID");
        testUnsigned <AccountID> ();

//---

        testcase ("Issue");
        testIssue <Issue> ();

        testcase ("Issue");
        testIssue <Issue> ();

        testIssueSets ();
        testIssueMaps ();

//---

        testcase ("Book");
        testBook <Book> ();

        testcase ("Book");
        testBook <Book> ();

        testBookSets ();
        testBookMaps ();
    }
};

BEAST_DEFINE_TESTSUITE(Issue,protocol,ripple);

}
