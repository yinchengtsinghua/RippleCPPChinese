
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#include <ripple/beast/clock/manual_clock.h>
#include <ripple/beast/unit_test.h>

#include <ripple/beast/container/aged_set.h>
#include <ripple/beast/container/aged_map.h>
#include <ripple/beast/container/aged_multiset.h>
#include <ripple/beast/container/aged_multimap.h>
#include <ripple/beast/container/aged_unordered_set.h>
#include <ripple/beast/container/aged_unordered_map.h>
#include <ripple/beast/container/aged_unordered_multiset.h>
#include <ripple/beast/container/aged_unordered_multimap.h>

#include <vector>
#include <list>

#ifndef BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR
# ifdef _MSC_VER
#  define BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR 0
# else
#  define BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR 1
# endif
#endif

#ifndef BEAST_CONTAINER_EXTRACT_NOREF
# ifdef _MSC_VER
#  define BEAST_CONTAINER_EXTRACT_NOREF 1
# else
#  define BEAST_CONTAINER_EXTRACT_NOREF 1
# endif
#endif

namespace beast {

class aged_associative_container_test_base : public unit_test::suite
{
public:
    template <class T>
    struct CompT
    {
        explicit CompT (int)
        {
        }

        CompT (CompT const&)
        {
        }

        bool operator() (T const& lhs, T const& rhs) const
        {
            return m_less (lhs, rhs);
        }

    private:
        CompT () = delete;
        std::less <T> m_less;
    };

    template <class T>
    class HashT
    {
    public:
        explicit HashT (int)
        {
        }

        std::size_t operator() (T const& t) const
        {
            return m_hash (t);
        }

    private:
        HashT() = delete;
        std::hash <T> m_hash;
    };

    template <class T>
    struct EqualT
    {
    public:
        explicit EqualT (int)
        {
        }

        bool operator() (T const& lhs, T const& rhs) const
        {
            return m_eq (lhs, rhs);
        }

    private:
        EqualT() = delete;
        std::equal_to <T> m_eq;
    };

    template <class T>
    struct AllocT
    {
        using value_type = T;

//使用std:：true_type:：type=propagate_on_container_swap:；

        template <class U>
        struct rebind
        {
            using other = AllocT <U>;
        };

        explicit AllocT (int)
        {
        }

        AllocT (AllocT const&) = default;

        template <class U>
        AllocT (AllocT <U> const&)
        {
        }

        template <class U>
        bool operator== (AllocT <U> const&) const
        {
            return true;
        }

        template <class U>
        bool operator!= (AllocT <U> const& o) const
        {
            return !(*this == o);
        }


        T* allocate (std::size_t n, T const* = 0)
        {
            return static_cast <T*> (
                ::operator new (n * sizeof(T)));
        }

        void deallocate (T* p, std::size_t)
        {
            ::operator delete (p);
        }

#if ! BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR
        AllocT ()
        {
        }
#else
    private:
        AllocT() = delete;
#endif
    };

//——————————————————————————————————————————————————————————————

//命令
    template <class Base, bool IsUnordered>
    class MaybeUnordered : public Base
    {
    public:
        using Comp = std::less <typename Base::Key>;
        using MyComp = CompT <typename Base::Key>;

    protected:
        static std::string name_ordered_part()
        {
            return "";
        }
    };

//无序的
    template <class Base>
    class MaybeUnordered <Base, true> : public Base
    {
    public:
        using Hash = std::hash <typename Base::Key>;
        using Equal = std::equal_to <typename Base::Key>;
        using MyHash = HashT <typename Base::Key>;
        using MyEqual = EqualT <typename Base::Key>;

    protected:
        static std::string name_ordered_part()
        {
            return "unordered_";
        }
    };

//独特的
    template <class Base, bool IsMulti>
    class MaybeMulti : public Base
    {
    public:
    protected:
        static std::string name_multi_part()
        {
            return "";
        }
    };

//多种
    template <class Base>
    class MaybeMulti <Base, true> : public Base
    {
    public:
    protected:
        static std::string name_multi_part()
        {
            return "multi";
        }
    };

//设置
    template <class Base, bool IsMap>
    class MaybeMap : public Base
    {
    public:
        using T = void;
        using Value = typename Base::Key;
        using Values = std::vector <Value>;

        static typename Base::Key const& extract (Value const& value)
        {
            return value;
        }

        static Values values()
        {
            Values v {
                "apple",
                "banana",
                "cherry",
                "grape",
                "orange",
            };
            return v;
        }

    protected:
        static std::string name_map_part()
        {
            return "set";
        }
    };

//地图
    template <class Base>
    class MaybeMap <Base, true> : public Base
    {
    public:
        using T = int;
        using Value = std::pair <typename Base::Key, T>;
        using Values = std::vector <Value>;

        static typename Base::Key const& extract (Value const& value)
        {
            return value.first;
        }

        static Values values()
        {
            Values v {
                std::make_pair ("apple",  1),
                std::make_pair ("banana", 2),
                std::make_pair ("cherry", 3),
                std::make_pair ("grape",  4),
                std::make_pair ("orange", 5)
            };
            return v;
        }

    protected:
        static std::string name_map_part()
        {
            return "map";
        }
    };

//——————————————————————————————————————————————————————————————

//命令
    template <
        class Base,
        bool IsUnordered = Base::is_unordered::value
    >
    struct ContType
    {
        template <
            class Compare = std::less <typename Base::Key>,
            class Allocator = std::allocator <typename Base::Value>
        >
        using Cont = detail::aged_ordered_container <
            Base::is_multi::value, Base::is_map::value, typename Base::Key,
                typename Base::T, typename Base::Clock, Compare, Allocator>;
    };

//无序的
    template <
        class Base
    >
    struct ContType <Base, true>
    {
        template <
            class Hash = std::hash <typename Base::Key>,
            class KeyEqual = std::equal_to <typename Base::Key>,
            class Allocator = std::allocator <typename Base::Value>
        >
        using Cont = detail::aged_unordered_container <
            Base::is_multi::value, Base::is_map::value,
                typename Base::Key, typename Base::T, typename Base::Clock,
                    Hash, KeyEqual, Allocator>;
    };

//——————————————————————————————————————————————————————————————

    struct TestTraitsBase
    {
        using Key = std::string;
        using Clock = std::chrono::steady_clock;
        using ManualClock = manual_clock<Clock>;
    };

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    struct TestTraitsHelper
        : MaybeUnordered <MaybeMulti <MaybeMap <
            TestTraitsBase, IsMap>, IsMulti>, IsUnordered>
    {
    private:
        using Base = MaybeUnordered <MaybeMulti <MaybeMap <
            TestTraitsBase, IsMap>, IsMulti>, IsUnordered>;

    public:
        using typename Base::Key;

        using is_unordered = std::integral_constant <bool, IsUnordered>;
        using is_multi = std::integral_constant <bool, IsMulti>;
        using is_map = std::integral_constant <bool, IsMap>;

        using Alloc = std::allocator <typename Base::Value>;
        using MyAlloc = AllocT <typename Base::Value>;

        static std::string name()
        {
            return std::string ("aged_") +
                Base::name_ordered_part() +
                    Base::name_multi_part() +
                        Base::name_map_part();
        }
    };

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    struct TestTraits
        : TestTraitsHelper <IsUnordered, IsMulti, IsMap>
        , ContType <TestTraitsHelper <IsUnordered, IsMulti, IsMap>>
    {
    };

    template <class Cont>
    static std::string name (Cont const&)
    {
        return TestTraits <
            Cont::is_unordered,
            Cont::is_multi,
            Cont::is_map>::name();
    }

    template <class Traits>
    struct equal_value
    {
        bool operator() (typename Traits::Value const& lhs,
            typename Traits::Value const& rhs)
        {
            return Traits::extract (lhs) == Traits::extract (rhs);
        }
    };

    template <class Cont>
    static
    std::vector <typename Cont::value_type>
    make_list (Cont const& c)
    {
        return std::vector <typename Cont::value_type> (
            c.begin(), c.end());
    }

//——————————————————————————————————————————————————————————————

    template <
        class Container,
        class Values
    >
    typename std::enable_if <
        Container::is_map::value && ! Container::is_multi::value>::type
    checkMapContents (Container& c, Values const& v);

    template <
        class Container,
        class Values
    >
    typename std::enable_if <!
        (Container::is_map::value && ! Container::is_multi::value)>::type
    checkMapContents (Container, Values const&)
    {
    }

//无序的
    template <
        class C,
        class Values
    >
    typename std::enable_if <
        std::remove_reference <C>::type::is_unordered::value>::type
    checkUnorderedContentsRefRef (C&& c, Values const& v);

    template <
        class C,
        class Values
    >
    typename std::enable_if <!
        std::remove_reference <C>::type::is_unordered::value>::type
    checkUnorderedContentsRefRef (C&&, Values const&)
    {
    }

    template <class C, class Values>
    void checkContentsRefRef (C&& c, Values const& v);

    template <class Cont, class Values>
    void checkContents (Cont& c, Values const& v);

    template <class Cont>
    void checkContents (Cont& c);

//——————————————————————————————————————————————————————————————

//命令
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testConstructEmpty ();

//无序的
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testConstructEmpty ();

//命令
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testConstructRange ();

//无序的
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testConstructRange ();

//命令
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testConstructInitList ();

//无序的
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testConstructInitList ();

//——————————————————————————————————————————————————————————————

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void
    testCopyMove ();

//——————————————————————————————————————————————————————————————

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void
    testIterator ();

//无序容器没有反向迭代器
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testReverseIterator();

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testReverseIterator()
    {
    }

//——————————————————————————————————————————————————————————————

    template <class Container, class Values>
    void checkInsertCopy (Container& c, Values const& v);

    template <class Container, class Values>
    void checkInsertMove (Container& c, Values const& v);

    template <class Container, class Values>
    void checkInsertHintCopy (Container& c, Values const& v);

    template <class Container, class Values>
    void checkInsertHintMove (Container& c, Values const& v);

    template <class Container, class Values>
    void checkEmplace (Container& c, Values const& v);

    template <class Container, class Values>
    void checkEmplaceHint (Container& c, Values const& v);

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void testModifiers();

//——————————————————————————————————————————————————————————————

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void
    testChronological ();

//——————————————————————————————————————————————————————————————

//地图，无序地图
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsMap && ! IsMulti>::type
    testArrayCreate();

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! (IsMap && ! IsMulti)>::type
    testArrayCreate()
    {
    }

//——————————————————————————————————————————————————————————————

//清除测试的帮助程序
    template <class Container, class Values>
    void reverseFillAgedContainer(Container& c, Values const& v);

    template <class Iter>
    Iter nextToEndIter (Iter const beginIter, Iter const endItr);

//——————————————————————————————————————————————————————————————

    template <class Container, class Iter>
    bool doElementErase (Container& c, Iter const beginItr, Iter const endItr);

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void testElementErase();

//——————————————————————————————————————————————————————————————

    template <class Container, class BeginEndSrc>
    void doRangeErase (Container& c, BeginEndSrc const& beginEndSrc);

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void testRangeErase();

//——————————————————————————————————————————————————————————————

//命令
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testCompare ();

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testCompare ()
    {
    }

//——————————————————————————————————————————————————————————————

//命令
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testObservers();

//无序的
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testObservers();

//——————————————————————————————————————————————————————————————

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void testMaybeUnorderedMultiMap ();

    template <bool IsUnordered, bool IsMulti>
    void testMaybeUnorderedMulti();

    template <bool IsUnordered>
    void testMaybeUnordered();
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//通过at（）和运算符[]检查内容
//地图，无序地图
template <
    class Container,
    class Values
>
typename std::enable_if <
    Container::is_map::value && ! Container::is_multi::value>::type
aged_associative_container_test_base::
checkMapContents (Container& c, Values const& v)
{
    if (v.empty())
    {
        BEAST_EXPECT(c.empty());
        BEAST_EXPECT(c.size() == 0);
        return;
    }

    try
    {
//确保没有引发异常
        for (auto const& e : v)
            c.at (e.first);
        for (auto const& e : v)
            BEAST_EXPECT(c.operator[](e.first) == e.second);
    }
    catch (std::out_of_range const&)
    {
        fail ("caught exception");
    }
}

//无序的
template <
    class C,
    class Values
>
typename std::enable_if <
    std::remove_reference <C>::type::is_unordered::value>::type
aged_associative_container_test_base::
checkUnorderedContentsRefRef (C&& c, Values const& v)
{
    using Cont = typename std::remove_reference <C>::type;
    using Traits = TestTraits <
        Cont::is_unordered::value,
            Cont::is_multi::value,
                Cont::is_map::value
                >;
    using size_type = typename Cont::size_type;
    auto const hash (c.hash_function());
    auto const key_eq (c.key_eq());
    for (size_type i (0); i < c.bucket_count(); ++i)
    {
        auto const last (c.end(i));
        for (auto iter (c.begin (i)); iter != last; ++iter)
        {
            auto const match (std::find_if (v.begin(), v.end(),
                [iter](typename Values::value_type const& e)
                {
                    return Traits::extract (*iter) ==
                        Traits::extract (e);
                }));
            BEAST_EXPECT(match != v.end());
            BEAST_EXPECT(key_eq (Traits::extract (*iter),
                Traits::extract (*match)));
            BEAST_EXPECT(hash (Traits::extract (*iter)) ==
                hash (Traits::extract (*match)));
        }
    }
}

template <class C, class Values>
void
aged_associative_container_test_base::
checkContentsRefRef (C&& c, Values const& v)
{
    using Cont = typename std::remove_reference <C>::type;
    using Traits = TestTraits <
        Cont::is_unordered::value,
            Cont::is_multi::value,
                Cont::is_map::value
                >;
    using size_type = typename Cont::size_type;

    BEAST_EXPECT(c.size() == v.size());
    BEAST_EXPECT(size_type (std::distance (
        c.begin(), c.end())) == v.size());
    BEAST_EXPECT(size_type (std::distance (
        c.cbegin(), c.cend())) == v.size());
    BEAST_EXPECT(size_type (std::distance (
        c.chronological.begin(), c.chronological.end())) == v.size());
    BEAST_EXPECT(size_type (std::distance (
        c.chronological.cbegin(), c.chronological.cend())) == v.size());
    BEAST_EXPECT(size_type (std::distance (
        c.chronological.rbegin(), c.chronological.rend())) == v.size());
    BEAST_EXPECT(size_type (std::distance (
        c.chronological.crbegin(), c.chronological.crend())) == v.size());

    checkUnorderedContentsRefRef (c, v);
}

template <class Cont, class Values>
void
aged_associative_container_test_base::
checkContents (Cont& c, Values const& v)
{
    checkContentsRefRef (c, v);
    checkContentsRefRef (const_cast <Cont const&> (c), v);
    checkMapContents (c, v);
}

template <class Cont>
void
aged_associative_container_test_base::
checkContents (Cont& c)
{
    using Traits = TestTraits <
        Cont::is_unordered::value,
            Cont::is_multi::value,
                Cont::is_map::value
                >;
    using Values = typename Traits::Values;
    checkContents (c, Values());
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//施工
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//命令
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testConstructEmpty ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    using Value = typename Traits::Value;
    using Key = typename Traits::Key;
    using T = typename Traits::T;
    using Clock = typename Traits::Clock;
    using Comp = typename Traits::Comp;
    using Alloc = typename Traits::Alloc;
    using MyComp = typename Traits::MyComp;
    using MyAlloc = typename Traits::MyAlloc;
    typename Traits::ManualClock clock;

//测试用例（traits:：name（）+“empty”）；
    testcase ("empty");

    {
        typename Traits::template Cont <Comp, Alloc> c (
            clock);
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyComp, Alloc> c (
            clock, MyComp(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <Comp, MyAlloc> c (
            clock, MyAlloc(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyComp, MyAlloc> c (
            clock, MyComp(1), MyAlloc(1));
        checkContents (c);
    }
}

//无序的
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <IsUnordered>::type
aged_associative_container_test_base::
testConstructEmpty ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    using Value = typename Traits::Value;
    using Key = typename Traits::Key;
    using T = typename Traits::T;
    using Clock = typename Traits::Clock;
    using Hash = typename Traits::Hash;
    using Equal = typename Traits::Equal;
    using Alloc = typename Traits::Alloc;
    using MyHash = typename Traits::MyHash;
    using MyEqual = typename Traits::MyEqual;
    using MyAlloc = typename Traits::MyAlloc;
    typename Traits::ManualClock clock;

//测试用例（traits:：name（）+“empty”）；
    testcase ("empty");
    {
        typename Traits::template Cont <Hash, Equal, Alloc> c (
            clock);
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyHash, Equal, Alloc> c (
            clock, MyHash(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <Hash, MyEqual, Alloc> c (
            clock, MyEqual (1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <Hash, Equal, MyAlloc> c (
            clock, MyAlloc (1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyHash, MyEqual, Alloc> c (
            clock, MyHash(1), MyEqual(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyHash, Equal, MyAlloc> c (
            clock, MyHash(1), MyAlloc(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <Hash, MyEqual, MyAlloc> c (
            clock, MyEqual(1), MyAlloc(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyHash, MyEqual, MyAlloc> c (
            clock, MyHash(1), MyEqual(1), MyAlloc(1));
        checkContents (c);
    }
}

//命令
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testConstructRange ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    using Value = typename Traits::Value;
    using Key = typename Traits::Key;
    using T = typename Traits::T;
    using Clock = typename Traits::Clock;
    using Comp = typename Traits::Comp;
    using Alloc = typename Traits::Alloc;
    using MyComp = typename Traits::MyComp;
    using MyAlloc = typename Traits::MyAlloc;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

//测试用例（traits:：name（）+“range”）；
    testcase ("range");

    {
        typename Traits::template Cont <Comp, Alloc> c (
            v.begin(), v.end(),
            clock);
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyComp, Alloc> c (
            v.begin(), v.end(),
            clock, MyComp(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <Comp, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyAlloc(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyComp, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyComp(1), MyAlloc(1));
        checkContents (c, v);

    }

//掉期

    {
        typename Traits::template Cont <Comp, Alloc> c1 (
            v.begin(), v.end(),
            clock);
        typename Traits::template Cont <Comp, Alloc> c2 (
            clock);
        std::swap (c1, c2);
        checkContents (c2, v);
    }
}

//无序的
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <IsUnordered>::type
aged_associative_container_test_base::
testConstructRange ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    using Value = typename Traits::Value;
    using Key = typename Traits::Key;
    using T = typename Traits::T;
    using Clock = typename Traits::Clock;
    using Hash = typename Traits::Hash;
    using Equal = typename Traits::Equal;
    using Alloc = typename Traits::Alloc;
    using MyHash = typename Traits::MyHash;
    using MyEqual = typename Traits::MyEqual;
    using MyAlloc = typename Traits::MyAlloc;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

//测试用例（traits:：name（）+“range”）；
    testcase ("range");

    {
        typename Traits::template Cont <Hash, Equal, Alloc> c (
            v.begin(), v.end(),
            clock);
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyHash, Equal, Alloc> c (
            v.begin(), v.end(),
            clock, MyHash(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <Hash, MyEqual, Alloc> c (
            v.begin(), v.end(),
            clock, MyEqual (1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <Hash, Equal, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyAlloc (1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyHash, MyEqual, Alloc> c (
            v.begin(), v.end(),
            clock, MyHash(1), MyEqual(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyHash, Equal, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyHash(1), MyAlloc(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <Hash, MyEqual, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyEqual(1), MyAlloc(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyHash, MyEqual, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyHash(1), MyEqual(1), MyAlloc(1));
        checkContents (c, v);
    }
}

//命令
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testConstructInitList ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    using Value = typename Traits::Value;
    using Key = typename Traits::Key;
    using T = typename Traits::T;
    using Clock = typename Traits::Clock;
    using Comp = typename Traits::Comp;
    using Alloc = typename Traits::Alloc;
    using MyComp = typename Traits::MyComp;
    using MyAlloc = typename Traits::MyAlloc;
    typename Traits::ManualClock clock;

//测试用例（traits:：name（）+“init list”）；
    testcase ("init-list");

//沃尔法科托多

    pass();
}

//无序的
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <IsUnordered>::type
aged_associative_container_test_base::
testConstructInitList ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    using Value = typename Traits::Value;
    using Key = typename Traits::Key;
    using T = typename Traits::T;
    using Clock = typename Traits::Clock;
    using Hash = typename Traits::Hash;
    using Equal = typename Traits::Equal;
    using Alloc = typename Traits::Alloc;
    using MyHash = typename Traits::MyHash;
    using MyEqual = typename Traits::MyEqual;
    using MyAlloc = typename Traits::MyAlloc;
    typename Traits::ManualClock clock;

//测试用例（traits:：name（）+“init list”）；
    testcase ("init-list");

//沃尔法科托多
    pass();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//复制/移动构造并分配
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testCopyMove ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    using Value = typename Traits::Value;
    using Alloc = typename Traits::Alloc;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

//测试用例（traits:：name（）+“copy/move”）；
    testcase ("copy/move");

//复制

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (c);
        checkContents (c, v);
        checkContents (c2, v);
        BEAST_EXPECT(c == c2);
        unexpected (c != c2);
    }

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (c, Alloc());
        checkContents (c, v);
        checkContents (c2, v);
        BEAST_EXPECT(c == c2);
        unexpected (c != c2);
    }

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (
            clock);
        c2 = c;
        checkContents (c, v);
        checkContents (c2, v);
        BEAST_EXPECT(c == c2);
        unexpected (c != c2);
    }

//移动

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (
            std::move (c));
        checkContents (c2, v);
    }

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (
            std::move (c), Alloc());
        checkContents (c2, v);
    }

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (
            clock);
        c2 = std::move (c);
        checkContents (c2, v);
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//迭代器构造和赋值
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testIterator()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    using Value = typename Traits::Value;
    using Alloc = typename Traits::Alloc;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

//测试用例（traits:：name（）+“迭代器”）；
    testcase ("iterator");

    typename Traits::template Cont <> c {clock};

    using iterator = decltype (c.begin());
    using const_iterator = decltype (c.cbegin());

//应该能够从迭代器构造或分配迭代器。
    iterator nnIt_0 {c.begin()};
    iterator nnIt_1 {nnIt_0};
    BEAST_EXPECT(nnIt_0 == nnIt_1);
    iterator nnIt_2;
    nnIt_2 = nnIt_1;
    BEAST_EXPECT(nnIt_1 == nnIt_2);

//应该能够从
//常数迭代器
    const_iterator ccIt_0 {c.cbegin()};
    const_iterator ccIt_1 {ccIt_0};
    BEAST_EXPECT(ccIt_0 == ccIt_1);
    const_iterator ccIt_2;
    ccIt_2 = ccIt_1;
    BEAST_EXPECT(ccIt_1 == ccIt_2);

//迭代器和常量迭代器之间的比较可以
    BEAST_EXPECT(nnIt_0 == ccIt_0);
    BEAST_EXPECT(ccIt_1 == nnIt_1);

//应该能够从迭代器构造常量迭代器。
    const_iterator ncIt_3 {c.begin()};
    const_iterator ncIt_4 {nnIt_0};
    BEAST_EXPECT(ncIt_3 == ncIt_4);
    const_iterator ncIt_5;
    ncIt_5 = nnIt_2;
    BEAST_EXPECT(ncIt_5 == ncIt_4);

//这些都不应该编译，因为它们构造或分配给
//带有常量迭代器的非常量迭代器。

//迭代器cnit_0_c.cbegin（）

//迭代器cnit_1_ccit_0_

//迭代器cnit_2；
//cnit_2=ccit_2；
}

template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testReverseIterator()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    using Value = typename Traits::Value;
    using Alloc = typename Traits::Alloc;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

//测试用例（traits:：name（）+“reverse_iterators”）；
    testcase ("reverse_iterator");

    typename Traits::template Cont <> c {clock};

    using iterator = decltype (c.begin());
    using const_iterator = decltype (c.cbegin());
    using reverse_iterator = decltype (c.rbegin());
    using const_reverse_iterator = decltype (c.crbegin());

//命名解码器环
//构造自------++-----构造类型
///\/\--字符对
//赛义特
//R（后退）或F（前进）--
//^-^------C（常量）或N（非常量）

//应该能够从
//反转迭代器。
    reverse_iterator rNrNit_0 {c.rbegin()};
    reverse_iterator rNrNit_1 {rNrNit_0};
    BEAST_EXPECT(rNrNit_0 == rNrNit_1);
    reverse_iterator xXrNit_2;
    xXrNit_2 = rNrNit_1;
    BEAST_EXPECT(rNrNit_1 == xXrNit_2);

//应该能够从
//常量反向迭代器
    const_reverse_iterator rCrCit_0 {c.crbegin()};
    const_reverse_iterator rCrCit_1 {rCrCit_0};
    BEAST_EXPECT(rCrCit_0 == rCrCit_1);
    const_reverse_iterator xXrCit_2;
    xXrCit_2 = rCrCit_1;
    BEAST_EXPECT(rCrCit_1 == xXrCit_2);

//Reverse_迭代器和Const_Reverse_迭代器比较正常
    BEAST_EXPECT(rNrNit_0 == rCrCit_0);
    BEAST_EXPECT(rCrCit_1 == rNrNit_1);

//应该能够从
//反向迭代器
    const_reverse_iterator rNrCit_0 {c.rbegin()};
    const_reverse_iterator rNrCit_1 {rNrNit_0};
    BEAST_EXPECT(rNrCit_0 == rNrCit_1);
    xXrCit_2 = rNrNit_1;
    BEAST_EXPECT(rNrCit_1 == xXrCit_2);

//标准允许这些转换：
//o Reverse_迭代器可以从迭代器显式构造。
//o const_reverse_迭代器可以从const_迭代器显式构造。
//应该能够从
//非反向迭代器。
    reverse_iterator fNrNit_0 {c.begin()};
    const_reverse_iterator fNrCit_0 {c.begin()};
    BEAST_EXPECT(fNrNit_0 == fNrCit_0);
    const_reverse_iterator fCrCit_0 {c.cbegin()};
    BEAST_EXPECT(fNrCit_0 == fCrCit_0);

//这些都不应该编译，因为它们构造了一个非反转的
//反向迭代器的迭代器。
//迭代器rnfnit_0_c.rbegin（）
//const_迭代器rnfcit_0_c.rbegin（）
//const_迭代器rcfcit_0_c.crbegin（）

//不能将迭代器赋给反向迭代器或
//反之亦然。所以下面的行不应该编译。
    iterator xXfNit_0;
//xxfnit_0=xkrnit_2；
//xxrnit_2=xxfnit_0；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//修饰语
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————


template <class Container, class Values>
void
aged_associative_container_test_base::
checkInsertCopy (Container& c, Values const& v)
{
    for (auto const& e : v)
        c.insert (e);
    checkContents (c, v);
}

template <class Container, class Values>
void
aged_associative_container_test_base::
checkInsertMove (Container& c, Values const& v)
{
    Values v2 (v);
    for (auto& e : v2)
        c.insert (std::move (e));
    checkContents (c, v);
}

template <class Container, class Values>
void
aged_associative_container_test_base::
checkInsertHintCopy (Container& c, Values const& v)
{
    for (auto const& e : v)
        c.insert (c.cend(), e);
    checkContents (c, v);
}

template <class Container, class Values>
void
aged_associative_container_test_base::
checkInsertHintMove (Container& c, Values const& v)
{
    Values v2 (v);
    for (auto& e : v2)
        c.insert (c.cend(), std::move (e));
    checkContents (c, v);
}

template <class Container, class Values>
void
aged_associative_container_test_base::
checkEmplace (Container& c, Values const& v)
{
    for (auto const& e : v)
        c.emplace (e);
    checkContents (c, v);
}

template <class Container, class Values>
void
aged_associative_container_test_base::
checkEmplaceHint (Container& c, Values const& v)
{
    for (auto const& e : v)
        c.emplace_hint (c.cend(), e);
    checkContents (c, v);
}

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testModifiers()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());
    auto const l (make_list (v));

//测试用例（traits:：name（）+“modify”）；
    testcase ("modify");

    {
        typename Traits::template Cont <> c (clock);
        checkInsertCopy (c, v);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertCopy (c, l);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertMove (c, v);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertMove (c, l);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertHintCopy (c, v);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertHintCopy (c, l);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertHintMove (c, v);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertHintMove (c, l);
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//按时间顺序排列
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testChronological ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    using Value = typename Traits::Value;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

//测试用例（traits:：name（）+“计时”）；
    testcase ("chronological");

    typename Traits::template Cont <> c (
        v.begin(), v.end(), clock);

    BEAST_EXPECT(std::equal (
        c.chronological.cbegin(), c.chronological.cend(),
            v.begin(), v.end(), equal_value <Traits> ()));

//使用非常量迭代器测试touch（）。
    for (auto iter (v.crbegin()); iter != v.crend(); ++iter)
    {
        using iterator = typename decltype (c)::iterator;
        iterator found (c.find (Traits::extract (*iter)));

        BEAST_EXPECT(found != c.cend());
        if (found == c.cend())
            return;
        c.touch (found);
    }

    BEAST_EXPECT(std::equal (
        c.chronological.cbegin(), c.chronological.cend(),
            v.crbegin(), v.crend(), equal_value <Traits> ()));

//使用常量迭代器测试touch（）。
    for (auto iter (v.cbegin()); iter != v.cend(); ++iter)
    {
        using const_iterator = typename decltype (c)::const_iterator;
        const_iterator found (c.find (Traits::extract (*iter)));

        BEAST_EXPECT(found != c.cend());
        if (found == c.cend())
            return;
        c.touch (found);
    }

    BEAST_EXPECT(std::equal (
        c.chronological.cbegin(), c.chronological.cend(),
            v.cbegin(), v.cend(), equal_value <Traits> ()));

    {
//因为不允许触摸（反向\u迭代器pos），所以以下内容
//不应为任何过时的容器类型编译行。
//c.touch（c.rbegin（））；
//c.touch（c.crbegin（））；
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//通过运算符[]创建元素
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//地图，无序地图
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <IsMap && ! IsMulti>::type
aged_associative_container_test_base::
testArrayCreate()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    typename Traits::ManualClock clock;
    auto v (Traits::values());

//测试用例（traits:：name（）+“array create”）；
    testcase ("array create");

    {
//复制构造键
        typename Traits::template Cont <> c (clock);
        for (auto e : v)
            c [e.first] = e.second;
        checkContents (c, v);
    }

    {
//移动构造键
        typename Traits::template Cont <> c (clock);
        for (auto e : v)
            c [std::move (e.first)] = e.second;
        checkContents (c, v);
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//清除测试的帮助程序
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Container, class Values>
void
aged_associative_container_test_base::
reverseFillAgedContainer (Container& c, Values const& values)
{
//以防传入的容器不是空的。
    c.clear();

//c.clock（）返回一个抽象的时钟，因此动态转换为手动时钟。
//vfalc注这是粗略的
    using ManualClock = TestTraitsBase::ManualClock;
    ManualClock& clk (dynamic_cast <ManualClock&> (c.clock()));
    clk.set (0);

    Values rev (values);
    std::sort (rev.begin (), rev.end ());
    std::reverse (rev.begin (), rev.end ());
    for (auto& v : rev)
    {
//按相反的顺序添加值，以便按时间顺序反转。
        ++clk;
        c.insert (v);
    }
}

//在enditer之前获取一个迭代器。我们必须使用operator++因为
//不能将运算符--与无序容器迭代器一起使用。
template <class Iter>
Iter
aged_associative_container_test_base::
nextToEndIter (Iter beginIter, Iter const endIter)
{
    if (beginIter == endIter)
    {
        fail ("Internal test failure. Cannot advance beginIter");
        return beginIter;
    }

//
    Iter nextToEnd = beginIter;
    do
    {
        nextToEnd = beginIter++;
    } while (beginIter != endIter);
    return nextToEnd;
}

//元素擦除测试的实现
//
//此测试接受：
//o我们将从中删除元素的容器
//o迭代器进入定义擦除范围的容器
//
//此实现不声明传递，因为它希望
//调用方检查容器的大小和返回的迭代器
//
//请注意，此测试适用于老化的关联容器，因为
//仅擦除使对已擦除元素的引用和迭代器无效
//（见23.2.4/13）。因此传入的结束迭代器保持有效通过
//整个测试。
template <class Container, class Iter>
bool aged_associative_container_test_base::
doElementErase (Container& c, Iter const beginItr, Iter const endItr)
{
    auto it (beginItr);
    size_t count = c.size();
    while (it != endItr)
    {
        auto expectIt = it;
        ++expectIt;
        it = c.erase (it);

        if (it != expectIt)
        {
            fail ("Unexpected returned iterator from element erase");
            return false;
        }

        --count;
        if (count != c.size())
        {
            fail ("Failed to erase element");
            return false;
        }

        if (c.empty ())
        {
            if (it != endItr)
            {
                fail ("Erase of last element didn't produce end");
                return false;
            }
        }
    }
   return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//删除单个元素
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testElementErase ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;

//测试用例（traits:：name（）+“元素擦除”
    testcase ("element erase");

//制作并填充容器
    typename Traits::ManualClock clock;
    typename Traits::template Cont <> c {clock};
    reverseFillAgedContainer (c, Traits::values());

    {
//测试标准迭代器
        auto tempContainer (c);
        if (! doElementErase (tempContainer,
            tempContainer.cbegin(), tempContainer.cend()))
return; //测试失败

        BEAST_EXPECT(tempContainer.empty());
        pass();
    }
    {
//测试时间迭代器
        auto tempContainer (c);
        auto& chron (tempContainer.chronological);
        if (! doElementErase (tempContainer, chron.begin(), chron.end()))
return; //测试失败

        BEAST_EXPECT(tempContainer.empty());
        pass();
    }
    {
//测试标准迭代器部分擦除
        auto tempContainer (c);
        BEAST_EXPECT(tempContainer.size() > 2);
        if (! doElementErase (tempContainer, ++tempContainer.begin(),
            nextToEndIter (tempContainer.begin(), tempContainer.end())))
return; //测试失败

        BEAST_EXPECT(tempContainer.size() == 2);
        pass();
    }
    {
//测试时间迭代器部分擦除
        auto tempContainer (c);
        BEAST_EXPECT(tempContainer.size() > 2);
        auto& chron (tempContainer.chronological);
        if (! doElementErase (tempContainer, ++chron.begin(),
            nextToEndIter (chron.begin(), chron.end())))
return; //测试失败

        BEAST_EXPECT(tempContainer.size() == 2);
        pass();
    }
    {
        auto tempContainer (c);
        BEAST_EXPECT(tempContainer.size() > 4);
//不允许使用Erase（Reverse_Iterator）。以下都不是
//应该为任何过时的容器类型编译。
//c.erase（c.rbegin（））；
//c.erase（c.crbegin（））；
//c.erase（c.rbegin（），++c.rbegin（））；
//c.erase（c.crbegin（），++c.crbegin（））；
    }
}

//范围擦除测试的实现
//
//此测试接受：
//
//o含有2种以上元素的容器和
//o要在传递的容器中请求begin（）和end（）迭代器的对象
//
//这个特殊的接口允许将容器本身作为
//第二个参数或容器的“时间”元素。两个
//迭代器的源需要在容器上进行测试。
//
//测试定位迭代器，这样基于范围的删除将离开第一个
//以及容器中的最后一个元素。然后验证容器
//以预期的内容结束。
//
template <class Container, class BeginEndSrc>
void
aged_associative_container_test_base::
doRangeErase (Container& c, BeginEndSrc const& beginEndSrc)
{
    BEAST_EXPECT(c.size () > 2);
    auto itBeginPlusOne (beginEndSrc.begin ());
    auto const valueFront = *itBeginPlusOne;
    ++itBeginPlusOne;

//在end（）之前获取一个迭代器
    auto itBack (nextToEndIter (itBeginPlusOne, beginEndSrc.end ()));
    auto const valueBack = *itBack;

//删除除第一个和最后一个元素以外的所有元素
    auto const retIter = c.erase (itBeginPlusOne, itBack);

    BEAST_EXPECT(c.size() == 2);
    BEAST_EXPECT(valueFront == *(beginEndSrc.begin()));
    BEAST_EXPECT(valueBack == *(++beginEndSrc.begin()));
    BEAST_EXPECT(retIter == (++beginEndSrc.begin()));
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//删除元素范围
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testRangeErase ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;

//测试用例（traits:：name（）+“元素擦除”
    testcase ("range erase");

//制作并填充容器
    typename Traits::ManualClock clock;
    typename Traits::template Cont <> c {clock};
    reverseFillAgedContainer (c, Traits::values());

//不用费心用反向迭代器测试范围擦除。
    {
        auto tempContainer (c);
        doRangeErase (tempContainer, tempContainer);
    }
    {
        auto tempContainer (c);
        doRangeErase (tempContainer, tempContainer.chronological);
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//容器范围比较
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//命令
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testCompare ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    using Value = typename Traits::Value;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

//测试用例（traits:：name（）+“array create”）；
    testcase ("array create");

    typename Traits::template Cont <> c1 (
        v.begin(), v.end(), clock);

    typename Traits::template Cont <> c2 (
        v.begin(), v.end(), clock);
    c2.erase (c2.cbegin());

    expect      (c1 != c2);
    unexpected  (c1 == c2);
    expect      (c1 <  c2);
    expect      (c1 <= c2);
    unexpected  (c1 >  c2);
    unexpected  (c1 >= c2);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//观察员
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//命令
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testObservers()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    typename Traits::ManualClock clock;

//测试用例（traits:：name（）+“observers”）；
    testcase ("observers");

    typename Traits::template Cont <> c (clock);
    c.key_comp();
    c.value_comp();

    pass();
}

//无序的
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <IsUnordered>::type
aged_associative_container_test_base::
testObservers()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;
    typename Traits::ManualClock clock;

//测试用例（traits:：name（）+“observers”）；
    testcase ("observers");

    typename Traits::template Cont <> c (clock);
    c.hash_function();
    c.key_eq();

    pass();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//矩阵
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testMaybeUnorderedMultiMap ()
{
    using Traits = TestTraits <IsUnordered, IsMulti, IsMap>;

    testConstructEmpty      <IsUnordered, IsMulti, IsMap> ();
    testConstructRange      <IsUnordered, IsMulti, IsMap> ();
    testConstructInitList   <IsUnordered, IsMulti, IsMap> ();
    testCopyMove            <IsUnordered, IsMulti, IsMap> ();
    testIterator            <IsUnordered, IsMulti, IsMap> ();
    testReverseIterator     <IsUnordered, IsMulti, IsMap> ();
    testModifiers           <IsUnordered, IsMulti, IsMap> ();
    testChronological       <IsUnordered, IsMulti, IsMap> ();
    testArrayCreate         <IsUnordered, IsMulti, IsMap> ();
    testElementErase        <IsUnordered, IsMulti, IsMap> ();
    testRangeErase          <IsUnordered, IsMulti, IsMap> ();
    testCompare             <IsUnordered, IsMulti, IsMap> ();
    testObservers           <IsUnordered, IsMulti, IsMap> ();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class aged_set_test : public aged_associative_container_test_base
{
public:
//编译时检查

    using Key = std::string;
    using T = int;

    static_assert (std::is_same <
        aged_set <Key>,
        detail::aged_ordered_container <false, false, Key, void>>::value,
            "bad alias: aged_set");

    static_assert (std::is_same <
        aged_multiset <Key>,
        detail::aged_ordered_container <true, false, Key, void>>::value,
            "bad alias: aged_multiset");

    static_assert (std::is_same <
        aged_map <Key, T>,
        detail::aged_ordered_container <false, true, Key, T>>::value,
            "bad alias: aged_map");

    static_assert (std::is_same <
        aged_multimap <Key, T>,
        detail::aged_ordered_container <true, true, Key, T>>::value,
            "bad alias: aged_multimap");

    static_assert (std::is_same <
        aged_unordered_set <Key>,
        detail::aged_unordered_container <false, false, Key, void>>::value,
            "bad alias: aged_unordered_set");

    static_assert (std::is_same <
        aged_unordered_multiset <Key>,
        detail::aged_unordered_container <true, false, Key, void>>::value,
            "bad alias: aged_unordered_multiset");

    static_assert (std::is_same <
        aged_unordered_map <Key, T>,
        detail::aged_unordered_container <false, true, Key, T>>::value,
            "bad alias: aged_unordered_map");

    static_assert (std::is_same <
        aged_unordered_multimap <Key, T>,
        detail::aged_unordered_container <true, true, Key, T>>::value,
            "bad alias: aged_unordered_multimap");

    void run () override
    {
        testMaybeUnorderedMultiMap <false, false, false>();
    }
};

class aged_map_test : public aged_associative_container_test_base
{
public:
    void run () override
    {
        testMaybeUnorderedMultiMap <false, false, true>();
    }
};

class aged_multiset_test : public aged_associative_container_test_base
{
public:
    void run () override
    {
        testMaybeUnorderedMultiMap <false, true, false>();
    }
};

class aged_multimap_test : public aged_associative_container_test_base
{
public:
    void run () override
    {
        testMaybeUnorderedMultiMap <false, true, true>();
    }
};


class aged_unordered_set_test : public aged_associative_container_test_base
{
public:
    void run () override
    {
        testMaybeUnorderedMultiMap <true, false, false>();
    }
};

class aged_unordered_map_test : public aged_associative_container_test_base
{
public:
    void run () override
    {
        testMaybeUnorderedMultiMap <true, false, true>();
    }
};

class aged_unordered_multiset_test : public aged_associative_container_test_base
{
public:
    void run () override
    {
        testMaybeUnorderedMultiMap <true, true, false>();
    }
};

class aged_unordered_multimap_test : public aged_associative_container_test_base
{
public:
    void run () override
    {
        testMaybeUnorderedMultiMap <true, true, true>();
    }
};

BEAST_DEFINE_TESTSUITE(aged_set,container,beast);
BEAST_DEFINE_TESTSUITE(aged_map,container,beast);
BEAST_DEFINE_TESTSUITE(aged_multiset,container,beast);
BEAST_DEFINE_TESTSUITE(aged_multimap,container,beast);
BEAST_DEFINE_TESTSUITE(aged_unordered_set,container,beast);
BEAST_DEFINE_TESTSUITE(aged_unordered_map,container,beast);
BEAST_DEFINE_TESTSUITE(aged_unordered_multiset,container,beast);
BEAST_DEFINE_TESTSUITE(aged_unordered_multimap,container,beast);

} //命名空间野兽
