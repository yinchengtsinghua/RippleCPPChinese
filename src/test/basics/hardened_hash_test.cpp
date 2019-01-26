
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

#include <ripple/basics/hardened_hash.h>
#include <ripple/beast/unit_test.h>
#include <boost/functional/hash.hpp>
#include <array>
#include <cstdint>
#include <iomanip>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace ripple {
namespace detail {

template <class T>
class test_user_type_member
{
private:
    T t;

public:
    explicit test_user_type_member (T const& t_ = T())
        : t (t_)
    {
    }

    template <class Hasher>
    friend void hash_append (Hasher& h, test_user_type_member const& a) noexcept
    {
        using beast::hash_append;
        hash_append (h, a.t);
    }
};

template <class T>
class test_user_type_free
{
private:
    T t;

public:
    explicit test_user_type_free (T const& t_ = T())
        : t (t_)
    {
    }

    template <class Hasher>
    friend void hash_append (Hasher& h, test_user_type_free const& a) noexcept
    {
        using beast::hash_append;
        hash_append (h, a.t);
    }
};

} //细节
} //涟漪

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace ripple {

namespace detail {

template <class T>
using test_hardened_unordered_set =
    std::unordered_set <T, hardened_hash <>>;

template <class T>
using test_hardened_unordered_map =
    std::unordered_map <T, int, hardened_hash <>>;

template <class T>
using test_hardened_unordered_multiset =
    std::unordered_multiset <T, hardened_hash <>>;

template <class T>
using test_hardened_unordered_multimap =
    std::unordered_multimap <T, int, hardened_hash <>>;

} //细节

template <std::size_t Bits, class UInt = std::uint64_t>
class unsigned_integer
{
private:
    static_assert (std::is_integral<UInt>::value &&
        std::is_unsigned <UInt>::value,
            "UInt must be an unsigned integral type");

    static_assert (Bits%(8*sizeof(UInt))==0,
        "Bits must be a multiple of 8*sizeof(UInt)");

    static_assert (Bits >= (8*sizeof(UInt)),
        "Bits must be at least 8*sizeof(UInt)");

    static std::size_t const size = Bits/(8*sizeof(UInt));

    std::array <UInt, size> m_vec;

public:
    using value_type = UInt;

    static std::size_t const bits = Bits;
    static std::size_t const bytes = bits / 8;

    template <class Int>
    static
    unsigned_integer
    from_number (Int v)
    {
        unsigned_integer result;
        for (std::size_t i (1); i < size; ++i)
            result.m_vec [i] = 0;
        result.m_vec[0] = v;
        return result;
    }

    void*
    data() noexcept
    {
        return &m_vec[0];
    }

    void const*
    data() const noexcept
    {
        return &m_vec[0];
    }

    template <class Hasher>
    friend void hash_append(Hasher& h, unsigned_integer const& a) noexcept
    {
        using beast::hash_append;
        hash_append (h, a.m_vec);
    }

    friend
    std::ostream&
    operator<< (std::ostream& s, unsigned_integer const& v)
    {
        for (std::size_t i (0); i < size; ++i)
            s <<
                std::hex <<
                std::setfill ('0') <<
                std::setw (2*sizeof(UInt)) <<
                v.m_vec[i]
                ;
        return s;
    }
};

using sha256_t = unsigned_integer <256, std::size_t>;

#ifndef __INTELLISENSE__
static_assert (sha256_t::bits == 256,
    "sha256_t must have 256 bits");
#endif

} //涟漪

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace ripple {

class hardened_hash_test
    : public beast::unit_test::suite
{
public:
    template <class T>
    void
    check ()
    {
        T t{};
        hardened_hash <>() (t);
        pass();
    }

    template <template <class T> class U>
    void
    check_user_type()
    {
        check <U <bool>> ();
        check <U <char>> ();
        check <U <signed char>> ();
        check <U <unsigned char>> ();
//这些会给助推系统带来麻烦
//检查<u<char16_t>>（）；
//检查<u<char32_t>>（）；
        check <U <wchar_t>> ();
        check <U <short>> ();
        check <U <unsigned short>> ();
        check <U <int>> ();
        check <U <unsigned int>> ();
        check <U <long>> ();
        check <U <long long>> ();
        check <U <unsigned long>> ();
        check <U <unsigned long long>> ();
        check <U <float>> ();
        check <U <double>> ();
        check <U <long double>> ();
    }

    template <template <class T> class C >
    void
    check_container()
    {
        {
            C <detail::test_user_type_member <std::string>> c;
        }

        pass();

        {
            C <detail::test_user_type_free <std::string>> c;
        }

        pass();
    }

    void
    test_user_types()
    {
        testcase ("user types");
        check_user_type <detail::test_user_type_member> ();
        check_user_type <detail::test_user_type_free> ();
    }

    void
    test_containers()
    {
        testcase ("containers");
        check_container <detail::test_hardened_unordered_set>();
        check_container <detail::test_hardened_unordered_map>();
        check_container <detail::test_hardened_unordered_multiset>();
        check_container <detail::test_hardened_unordered_multimap>();
    }

    void
    run () override
    {
        test_user_types();
        test_containers();
    }
};

BEAST_DEFINE_TESTSUITE(hardened_hash,basics,ripple);

} //涟漪
