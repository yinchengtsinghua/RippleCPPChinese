
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

//模块：../impl/ipendpoint.cpp../impl/ipaddressv4.cpp../impl/ipaddressv6.cpp

#include <ripple/beast/net/IPEndpoint.h>
#include <ripple/beast/unit_test.h>
#include <ripple/basics/random.h>
#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/predef.h>
#include <test/beast/IPEndpointCommon.h>
#include <typeinfo>

namespace beast {
namespace IP {

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class IPEndpoint_test : public unit_test::suite
{
public:
    void shouldParseAddrV4 (
        std::string const& s,
        std::uint32_t value,
        std::string const& normal = "")
    {
        boost::system::error_code ec;
        Address const result {Address::from_string (s, ec)};
        if (! BEAST_EXPECTS(! ec, ec.message()))
            return;
        if (! BEAST_EXPECTS(result.is_v4(), s + " not v4"))
            return;
        if (! BEAST_EXPECTS(result.to_v4().to_ulong() == value,
                    s + " value mismatch"))
            return;
        BEAST_EXPECTS(result.to_string () == (normal.empty() ? s : normal),
                s + " as string");
    }

    void failParseAddr (std::string const& s)
    {
        boost::system::error_code ec;
        auto a = Address::from_string (s, ec);
        BEAST_EXPECTS(ec, s + " parses as " + a.to_string());
    }

    void testAddressV4 ()
    {
        testcase ("AddressV4");

        BEAST_EXPECT(AddressV4{}.to_ulong() == 0);
        BEAST_EXPECT(is_unspecified (AddressV4{}));
        BEAST_EXPECT(AddressV4{0x01020304}.to_ulong() == 0x01020304);
        AddressV4::bytes_type d = {{1,2,3,4}};
        BEAST_EXPECT(AddressV4{d}.to_ulong() == 0x01020304);

        unexpected (is_unspecified (AddressV4{d}));

        AddressV4 const v1 {1};
        BEAST_EXPECT(AddressV4{v1}.to_ulong() == 1);

        {
            AddressV4 v;
            v = v1;
            BEAST_EXPECT(v.to_ulong() == v1.to_ulong());
        }

        {
            AddressV4 v;
            auto d = v.to_bytes();
            d[0] = 1;
            d[1] = 2;
            d[2] = 3;
            d[3] = 4;
            v = AddressV4{d};
            BEAST_EXPECT(v.to_ulong() == 0x01020304);
        }

        BEAST_EXPECT(AddressV4(0x01020304).to_string() == "1.2.3.4");

        shouldParseAddrV4 ("1.2.3.4", 0x01020304);
        shouldParseAddrV4 ("255.255.255.255", 0xffffffff);
        shouldParseAddrV4 ("0.0.0.0", 0);

        failParseAddr (".");
        failParseAddr ("..");
        failParseAddr ("...");
        failParseAddr ("....");
#if BOOST_OS_WINDOWS
//ASIO中的Windows错误-我认为这些不应该分析
//实际上，它们不在Mac/Linux上
        shouldParseAddrV4 ("1", 0x00000001, "0.0.0.1");
        shouldParseAddrV4 ("1.2", 0x01000002, "1.0.0.2");
        shouldParseAddrV4 ("1.2.3", 0x01020003, "1.2.0.3");
#else
        failParseAddr ("1");
        failParseAddr ("1.2");
        failParseAddr ("1.2.3");
#endif
        failParseAddr ("1.");
        failParseAddr ("1.2.");
        failParseAddr ("1.2.3.");
        failParseAddr ("256.0.0.0");
        failParseAddr ("-1.2.3.4");
    }

    void testAddressV4Proxy ()
    {
      testcase ("AddressV4::Bytes");

      AddressV4::bytes_type d1 = {{10,0,0,1}};
      AddressV4 v4 {d1};
      BEAST_EXPECT(v4.to_bytes()[0]==10);
      BEAST_EXPECT(v4.to_bytes()[1]==0);
      BEAST_EXPECT(v4.to_bytes()[2]==0);
      BEAST_EXPECT(v4.to_bytes()[3]==1);

      BEAST_EXPECT((~((0xff)<<16)) == 0xff00ffff);

      auto d2 = v4.to_bytes();
      d2[1] = 10;
      v4 = AddressV4{d2};
      BEAST_EXPECT(v4.to_bytes()[0]==10);
      BEAST_EXPECT(v4.to_bytes()[1]==10);
      BEAST_EXPECT(v4.to_bytes()[2]==0);
      BEAST_EXPECT(v4.to_bytes()[3]==1);
    }

//——————————————————————————————————————————————————————————————

    void testAddress ()
    {
        testcase ("Address");

        boost::system::error_code ec;
        Address result {Address::from_string ("1.2.3.4", ec)};
        AddressV4::bytes_type d = {{1,2,3,4}};
        BEAST_EXPECT(! ec);
        BEAST_EXPECT(
            result.is_v4 () &&
            result.to_v4() == AddressV4{d});
    }

//——————————————————————————————————————————————————————————————

    void shouldParseEPV4 (
        std::string const& s,
        AddressV4::bytes_type const& value,
        std::uint16_t p,
        std::string const& normal = "")
    {
        auto result {Endpoint::from_string_checked (s)};
        if (! BEAST_EXPECT(result.second))
            return;
        if (! BEAST_EXPECT(result.first.address().is_v4 ()))
            return;
        if (! BEAST_EXPECT(result.first.address().to_v4() == AddressV4 {value}))
            return;

        BEAST_EXPECT(result.first.port() == p);
        BEAST_EXPECT(to_string (result.first) == (normal.empty() ? s : normal));
    }

    void shouldParseEPV6 (
        std::string const& s,
        AddressV6::bytes_type const& value,
        std::uint16_t p,
        std::string const& normal = "")
    {
        auto result {Endpoint::from_string_checked (s)};
        if (! BEAST_EXPECT(result.second))
            return;
        if (! BEAST_EXPECT(result.first.address().is_v6 ()))
            return;
        if (! BEAST_EXPECT(result.first.address().to_v6() == AddressV6 {value}))
            return;

        BEAST_EXPECT(result.first.port() == p);
        BEAST_EXPECT(to_string (result.first) == (normal.empty() ? s : normal));
    }

    void failParseEP (std::string s)
    {
        auto a1 = Endpoint::from_string(s);
        BEAST_EXPECTS(is_unspecified (a1), s + " parses as " + a1.to_string());

        auto a2 = Endpoint::from_string(s);
        BEAST_EXPECTS(is_unspecified (a2), s + " parses as " + a2.to_string());

        boost::replace_last(s, ":", " ");
        auto a3 = Endpoint::from_string(s);
        BEAST_EXPECTS(is_unspecified (a3), s + " parses as " + a3.to_string());
    }

    void testEndpoint ()
    {
        testcase ("Endpoint");

        shouldParseEPV4("1.2.3.4", {{1,2,3,4}}, 0);
        shouldParseEPV4("1.2.3.4:5", {{1,2,3,4}}, 5);
        shouldParseEPV4("1.2.3.4 5", {{1,2,3,4}}, 5, "1.2.3.4:5");
//前导、尾随空格
        shouldParseEPV4("   1.2.3.4:5", {{1,2,3,4}}, 5, "1.2.3.4:5");
        shouldParseEPV4("1.2.3.4:5    ", {{1,2,3,4}}, 5, "1.2.3.4:5");
        shouldParseEPV4("1.2.3.4   ", {{1,2,3,4}}, 0, "1.2.3.4");
        shouldParseEPV4("  1.2.3.4", {{1,2,3,4}}, 0, "1.2.3.4");
        shouldParseEPV6(
            "2001:db8:a0b:12f0::1",
            {{32, 01, 13, 184, 10, 11, 18, 240, 0, 0, 0, 0, 0, 0, 0, 1}},
            0);
        shouldParseEPV6(
            "[2001:db8:a0b:12f0::1]:8",
            {{32, 01, 13, 184, 10, 11, 18, 240, 0, 0, 0, 0, 0, 0, 0, 1}},
            8);
        shouldParseEPV6(
            "[2001:2002:2003:2004:2005:2006:2007:2008]:65535",
            {{32, 1, 32, 2, 32, 3, 32, 4, 32, 5, 32, 6, 32, 7, 32, 8}},
            65535);
        shouldParseEPV6(
            "2001:2002:2003:2004:2005:2006:2007:2008 65535",
            {{32, 1, 32, 2, 32, 3, 32, 4, 32, 5, 32, 6, 32, 7, 32, 8}},
            65535,
            "[2001:2002:2003:2004:2005:2006:2007:2008]:65535");

        Endpoint ep;

        AddressV4::bytes_type d = {{127,0,0,1}};
        ep = Endpoint (AddressV4 {d}, 80);
        BEAST_EXPECT(! is_unspecified (ep));
        BEAST_EXPECT(! is_public (ep));
        BEAST_EXPECT(  is_private (ep));
        BEAST_EXPECT(! is_multicast (ep));
        BEAST_EXPECT(  is_loopback (ep));
        BEAST_EXPECT(to_string (ep) == "127.0.0.1:80");
//与IPv6中映射的v4地址相同
        ep = Endpoint (AddressV6::v4_mapped(AddressV4 {d}), 80);
        BEAST_EXPECT(! is_unspecified (ep));
        BEAST_EXPECT(! is_public (ep));
        BEAST_EXPECT(  is_private (ep));
        BEAST_EXPECT(! is_multicast (ep));
BEAST_EXPECT(! is_loopback (ep)); //映射的环回不是环回
        BEAST_EXPECTS(to_string (ep) == "[::ffff:127.0.0.1]:80", to_string (ep));

        d = {{10,0,0,1}};
        ep = Endpoint (AddressV4 {d});
        BEAST_EXPECT(get_class (ep.to_v4()) == 'A');
        BEAST_EXPECT(! is_unspecified (ep));
        BEAST_EXPECT(! is_public (ep));
        BEAST_EXPECT(  is_private (ep));
        BEAST_EXPECT(! is_multicast (ep));
        BEAST_EXPECT(! is_loopback (ep));
        BEAST_EXPECT(to_string (ep) == "10.0.0.1");
//与IPv6中映射的v4地址相同
        ep = Endpoint (AddressV6::v4_mapped(AddressV4 {d}));
        BEAST_EXPECT(get_class (ep.to_v6().to_v4()) == 'A');
        BEAST_EXPECT(! is_unspecified (ep));
        BEAST_EXPECT(! is_public (ep));
        BEAST_EXPECT(  is_private (ep));
        BEAST_EXPECT(! is_multicast (ep));
        BEAST_EXPECT(! is_loopback (ep));
        BEAST_EXPECTS(to_string (ep) == "::ffff:10.0.0.1", to_string(ep));

        d = {{166,78,151,147}};
        ep = Endpoint (AddressV4 {d});
        BEAST_EXPECT(! is_unspecified (ep));
        BEAST_EXPECT(  is_public (ep));
        BEAST_EXPECT(! is_private (ep));
        BEAST_EXPECT(! is_multicast (ep));
        BEAST_EXPECT(! is_loopback (ep));
        BEAST_EXPECT(to_string (ep) == "166.78.151.147");
//与IPv6中映射的v4地址相同
        ep = Endpoint (AddressV6::v4_mapped(AddressV4 {d}));
        BEAST_EXPECT(! is_unspecified (ep));
        BEAST_EXPECT(  is_public (ep));
        BEAST_EXPECT(! is_private (ep));
        BEAST_EXPECT(! is_multicast (ep));
        BEAST_EXPECT(! is_loopback (ep));
        BEAST_EXPECTS(to_string (ep) == "::ffff:166.78.151.147", to_string(ep));

//专用IPv6
        AddressV6::bytes_type d2 = {{253,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}};
        ep = Endpoint (AddressV6 {d2});
        BEAST_EXPECT(! is_unspecified (ep));
        BEAST_EXPECT(! is_public (ep));
        BEAST_EXPECT(  is_private (ep));
        BEAST_EXPECT(! is_multicast (ep));
        BEAST_EXPECT(! is_loopback (ep));
        BEAST_EXPECTS(to_string (ep) == "fd00::1", to_string(ep));

        {
            ep = Endpoint::from_string ("192.0.2.112");
            BEAST_EXPECT(! is_unspecified (ep));
            BEAST_EXPECT(ep == Endpoint::from_string ("192.0.2.112"));

            auto const ep1 = Endpoint::from_string ("192.0.2.112:2016");
            BEAST_EXPECT(! is_unspecified (ep1));
            BEAST_EXPECT(ep.address() == ep1.address());
            BEAST_EXPECT(ep1.port() == 2016);

            auto const ep2 =
                Endpoint::from_string ("192.0.2.112:2016");
            BEAST_EXPECT(! is_unspecified (ep2));
            BEAST_EXPECT(ep.address() == ep2.address());
            BEAST_EXPECT(ep2.port() == 2016);
            BEAST_EXPECT(ep1 == ep2);

            auto const ep3 =
                Endpoint::from_string ("192.0.2.112 2016");
            BEAST_EXPECT(! is_unspecified (ep3));
            BEAST_EXPECT(ep.address() == ep3.address());
            BEAST_EXPECT(ep3.port() == 2016);
            BEAST_EXPECT(ep2 == ep3);

            auto const ep4 =
                Endpoint::from_string ("192.0.2.112     2016");
            BEAST_EXPECT(! is_unspecified (ep4));
            BEAST_EXPECT(ep.address() == ep4.address());
            BEAST_EXPECT(ep4.port() == 2016);
            BEAST_EXPECT(ep3 == ep4);

            BEAST_EXPECT(to_string(ep1) == to_string(ep2));
            BEAST_EXPECT(to_string(ep1) == to_string(ep3));
            BEAST_EXPECT(to_string(ep1) == to_string(ep4));
        }

        {
            ep = Endpoint::from_string("[::]:2017");
            BEAST_EXPECT(is_unspecified (ep));
            BEAST_EXPECT(ep.port() == 2017);
            BEAST_EXPECT(ep.address() == AddressV6{});
        }

//失败：
        failParseEP ("192.0.2.112:port");
        failParseEP ("ip:port");
        failParseEP ("");
        failParseEP ("1.2.3.256");

#if BOOST_OS_WINDOWS
//Windows ASIO错误…误报
        shouldParseEPV4 ("255", {{0,0,0,255}}, 0, "0.0.0.255");
        shouldParseEPV4 ("512", {{0,0,2,0}}, 0, "0.0.2.0");
        shouldParseEPV4 ("1.2.3:80", {{1,2,0,3}}, 80, "1.2.0.3:80");
#else
        failParseEP ("255");
        failParseEP ("512");
        failParseEP ("1.2.3:80");
#endif

        failParseEP ("1.2.3.4:65536");
        failParseEP ("1.2.3.4:89119");
        failParseEP ("1.2.3:89119");
        failParseEP ("[::1]:89119");
        failParseEP ("[::az]:1");
        failParseEP ("[1234:5678:90ab:cdef:1234:5678:90ab:cdef:1111]:1");
        failParseEP ("[1234:5678:90ab:cdef:1234:5678:90ab:cdef:1111]:12345");
        failParseEP ("abcdef:12345");
        failParseEP ("[abcdef]:12345");
        failParseEP ("foo.org 12345");

//用散列容器测试
        std::unordered_set<Endpoint> eps;
        constexpr auto items {100};
        float max_lf {0};
        for (auto i = 0; i < items; ++i)
        {
            eps.insert(randomEP(ripple::rand_int(0,1) == 1));
            max_lf = std::max(max_lf, eps.load_factor());
        }
        BEAST_EXPECT(eps.bucket_count() >= items);
        BEAST_EXPECT(max_lf > 0.90);
    }

//——————————————————————————————————————————————————————————————

    template <typename T>
    bool parse (std::string const& text, T& t)
    {
        std::istringstream stream {text};
        stream >> t;
        return !stream.fail();
    }

    template <typename T>
    void shouldPass (std::string const& text, std::string const& normal="")
    {
        using namespace std::literals;
        T t;
        BEAST_EXPECT(parse (text, t));
        BEAST_EXPECTS(to_string (t) == (normal.empty() ? text : normal),
                "string mismatch for "s + text);
    }

    template <typename T>
    void shouldFail (std::string const& text)
    {
        T t;
        unexpected (parse (text, t), text + " should not parse");
    }

    template <typename T>
    void testParse (char const* name)
    {
        testcase (name);

        shouldPass <T> ("0.0.0.0");
        shouldPass <T> ("192.168.0.1");
        shouldPass <T> ("168.127.149.132");
        shouldPass <T> ("168.127.149.132:80");
        shouldPass <T> ("168.127.149.132:54321");
        shouldPass <T> ("2001:db8:a0b:12f0::1");
        shouldPass <T> ("[2001:db8:a0b:12f0::1]:8");
        shouldPass <T> ("2001:db8:a0b:12f0::1 8", "[2001:db8:a0b:12f0::1]:8");
        shouldPass <T> ("[::1]:8");
        shouldPass <T> ("[2001:2002:2003:2004:2005:2006:2007:2008]:65535");

        shouldFail <T> ("1.2.3.256");
        shouldFail <T> ("");
#if BOOST_OS_WINDOWS
//Windows ASIO错误…误报
        shouldPass <T> ("512", "0.0.2.0");
        shouldPass <T> ("255", "0.0.0.255");
        shouldPass <T> ("1.2.3:80", "1.2.0.3:80");
#else
        shouldFail <T> ("512");
        shouldFail <T> ("255");
        shouldFail <T> ("1.2.3:80");
#endif
        shouldFail <T> ("1.2.3:65536");
        shouldFail <T> ("1.2.3:72131");
        shouldFail <T> ("[::1]:89119");
        shouldFail <T> ("[::az]:1");
        shouldFail <T> ("[1234:5678:90ab:cdef:1234:5678:90ab:cdef:1111]:1");
        shouldFail <T> ("[1234:5678:90ab:cdef:1234:5678:90ab:cdef:1111]:12345");
    }

    void run () override
    {
        testAddressV4 ();
        testAddressV4Proxy();
        testAddress ();
        testEndpoint ();
        testParse <Endpoint> ("Parse Endpoint");
    }
};

BEAST_DEFINE_TESTSUITE(IPEndpoint,net,beast);

}
}
