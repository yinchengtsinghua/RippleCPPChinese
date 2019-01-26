
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

#ifndef RIPPLE_PEERFINDER_IOSFORMAT_H_INCLUDED
#define RIPPLE_PEERFINDER_IOSFORMAT_H_INCLUDED

#include <ostream>
#include <sstream>
#include <string>

namespace beast {

//一组便捷的流操作器和
//函数生成美观的日志输出。

/*左对齐指定宽度的字段。*/
struct leftw
{
    explicit leftw (int width_)
        : width (width_)
        { }
    int const width;
    template <class CharT, class Traits>
    friend std::basic_ios <CharT, Traits>& operator<< (
        std::basic_ios <CharT, Traits>& ios, leftw const& p)
    {
        ios.setf (std::ios_base::left, std::ios_base::adjustfield);
        ios.width (p.width);
        return ios;
    }
};

/*生成一个节标题，并用破折号填充行的其余部分。*/
template <class CharT, class Traits, class Allocator>
std::basic_string <CharT, Traits, Allocator> heading (
    std::basic_string <CharT, Traits, Allocator> title,
        int width = 80, CharT fill = CharT ('-'))
{
    title.reserve (width);
    title.push_back (CharT (' '));
    title.resize (width, fill);
    return title;
}

/*生成具有指定或默认大小的虚线分隔符。*/
struct divider
{
    using CharT = char;
    explicit divider (int width_ = 80, CharT fill_ = CharT ('-'))
        : width (width_)
        , fill (fill_)
        { }
    int const width;
    CharT const fill;
    template <class CharT, class Traits>
    friend std::basic_ostream <CharT, Traits>& operator<< (
        std::basic_ostream <CharT, Traits>& os, divider const& d)
    {
        os << std::basic_string <CharT, Traits> (d.width, d.fill);
        return os;
    }
};

/*创建带选项填充字符的填充字段。*/
struct fpad
{
    explicit fpad (int width_, int pad_ = 0, char fill_ = ' ')
        : width (width_ + pad_)
        , fill (fill_)
        { }
    int const width;
    char const fill;
    template <class CharT, class Traits>
    friend std::basic_ostream <CharT, Traits>& operator<< (
        std::basic_ostream <CharT, Traits>& os, fpad const& f)
    {
        os << std::basic_string <CharT, Traits> (f.width, f.fill);
        return os;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace detail {

template <typename T>
std::string to_string (T const& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

}

/*以指定宽度对正字段。*/
/*@ {*/
template <class CharT,
          class Traits = std::char_traits <CharT>,
          class Allocator = std::allocator <CharT>>
class field_t
{
public:
    using string_t = std::basic_string <CharT, Traits, Allocator>;
    field_t (string_t const& text_, int width_, int pad_, bool right_)
        : text (text_)
        , width (width_)
        , pad (pad_)
        , right (right_)
        { }
    string_t const text;
    int const width;
    int const pad;
    bool const right;
    template <class CharT2, class Traits2>
    friend std::basic_ostream <CharT2, Traits2>& operator<< (
        std::basic_ostream <CharT2, Traits2>& os,
            field_t <CharT, Traits, Allocator> const& f)
    {
        std::size_t const length (f.text.length());
        if (f.right)
        {
            if (length < f.width)
                os << std::basic_string <CharT2, Traits2> (
                    f.width - length, CharT2 (' '));
            os << f.text;
        }
        else
        {
            os << f.text;
            if (length < f.width)
                os << std::basic_string <CharT2, Traits2> (
                    f.width - length, CharT2 (' '));
        }
        if (f.pad != 0)
            os << string_t (f.pad, CharT (' '));
        return os;
    }
};

template <class CharT, class Traits, class Allocator>
field_t <CharT, Traits, Allocator> field (
    std::basic_string <CharT, Traits, Allocator> const& text,
        int width = 8, int pad = 0, bool right = false)
{
    return field_t <CharT, Traits, Allocator> (
        text, width, pad, right);
}

template <class CharT>
field_t <CharT> field (
    CharT const* text, int width = 8, int pad = 0, bool right = false)
{
    return field_t <CharT, std::char_traits <CharT>,
        std::allocator <CharT>> (std::basic_string <CharT,
            std::char_traits <CharT>, std::allocator <CharT>> (text),
                width, pad, right);
}

template <typename T>
field_t <char> field (
    T const& t, int width = 8, int pad = 0, bool right = false)
{
    std::string const text (detail::to_string (t));
    return field (text, width, pad, right);
}

template <class CharT, class Traits, class Allocator>
field_t <CharT, Traits, Allocator> rfield (
    std::basic_string <CharT, Traits, Allocator> const& text,
        int width = 8, int pad = 0)
{
    return field_t <CharT, Traits, Allocator> (
        text, width, pad, true);
}

template <class CharT>
field_t <CharT> rfield (
    CharT const* text, int width = 8, int pad = 0)
{
    return field_t <CharT, std::char_traits <CharT>,
        std::allocator <CharT>> (std::basic_string <CharT,
            std::char_traits <CharT>, std::allocator <CharT>> (text),
                width, pad, true);
}

template <typename T>
field_t <char> rfield (
    T const& t, int width = 8, int pad = 0)
{
    std::string const text (detail::to_string (t));
    return field (text, width, pad, true);
}
/*@ }*/

}

#endif
