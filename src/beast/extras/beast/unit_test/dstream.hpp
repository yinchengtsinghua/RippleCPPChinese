
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//
//版权所有（c）2013-2017 Vinnie Falco（Gmail.com上的Vinnie Dot Falco）
//
//在Boost软件许可证1.0版下分发。（见附件
//文件许可证_1_0.txt或复制到http://www.boost.org/license_1_0.txt）
//

#ifndef BEAST_UNIT_TEST_DSTREAM_HPP
#define BEAST_UNIT_TEST_DSTREAM_HPP

#include <boost/config.hpp>
#include <ios>
#include <memory>
#include <ostream>
#include <streambuf>
#include <string>

#ifdef BOOST_WINDOWS
#include <boost/detail/winapi/basic_types.hpp>
//包括<boost/detail/winapi/debugapi.hpp>
#endif

namespace beast {
namespace unit_test {

#ifdef BOOST_WINDOWS

namespace detail {

template<class CharT, class Traits, class Allocator>
class dstream_buf
    : public std::basic_stringbuf<CharT, Traits, Allocator>
{
    using ostream = std::basic_ostream<CharT, Traits>;

    bool dbg_;
    ostream& os_;

    template<class T>
    void write(T const*) = delete;

    void write(char const* s)
    {
        if(dbg_)
            /*oost：：detail：：winapi*/：：outputDebugStringa（s）；
        奥斯<
    }

    无效写入（wchar_t const*s）
    {
        If（dBGz）
            /*增强：：详细信息：：winap*/::OutputDebugStringW(s);

        os_ << s;
    }

public:
    explicit
    dstream_buf(ostream& os)
        : os_(os)
        /*bg_uu（/*boost:：detail:：winapi*/：：isDebuggerPresent（）！= 0）
    {
    }

    ~dStudioBuff.（）
    {
        同步（）；
    }

    int
    同步（）超驰
    {
        写入（this->str（）.c_str（））；
        这个-> STR（“”）；
        返回0；
    }
}；

} / /细节

/**std:：ostream，带有Visual Studio IDE重定向。

    此流的实例包装指定的“std:：ostream”
    （例如“std:：cout”或“std:：cerr”）。如果IDE调试器
    在创建流时附加，输出将
    另外复制到Visual Studio输出窗口。
**/

template<
    class CharT,
    class Traits = std::char_traits<CharT>,
    class Allocator = std::allocator<CharT>
>
class basic_dstream
    : public std::basic_ostream<CharT, Traits>
{
    detail::dstream_buf<
        CharT, Traits, Allocator> buf_;

public:
    /*构建流。

        @param操作要包装的输出流。
    **/

    explicit
    basic_dstream(std::ostream& os)
        : std::basic_ostream<CharT, Traits>(&buf_)
        , buf_(os)
    {
        if(os.flags() & std::ios::unitbuf)
            std::unitbuf(*this);
    }
};

using dstream = basic_dstream<char>;
using dwstream = basic_dstream<wchar_t>;

#else

using dstream = std::ostream&;
using dwstream = std::wostream&;

#endif

} //单位试验
} //野兽

#endif
