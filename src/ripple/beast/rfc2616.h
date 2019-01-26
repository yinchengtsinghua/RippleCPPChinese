
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
    版权所有2014，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_RFC2616_HPP
#define BEAST_RFC2616_HPP

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/rfc7230.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/utility/string_ref.hpp>
#include <algorithm>
#include <cctype>
#include <string>
#include <iterator>
#include <tuple> //对于std:：tie，尽快移除
#include <utility>
#include <vector>

namespace beast {
namespace rfc2616 {

namespace detail {

/*执行RFC2616遵从性的例程。
    RCF2616：
        超文本传输协议——HTTP/1.1
        http://www.w3.org/protocols/rfc2616/rfc2616
**/


struct ci_equal_pred
{
    explicit ci_equal_pred() = default;

    bool operator()(char c1, char c2)
    {
//vfalc todo在此处使用表查找
        return std::tolower(static_cast<unsigned char>(c1)) ==
               std::tolower(static_cast<unsigned char>(c2));
    }
};

} //细节

/*如果“c”是线性空白，则返回“true”。

    这排除了行连续性允许的CRLF序列。
**/

inline
bool
is_lws(char c)
{
    return c == ' ' || c == '\t';
}

/*如果'c'是任何空白字符，则返回'true'。*/
inline
bool
is_white(char c)
{
    switch (c)
    {
    case ' ':  case '\f': case '\n':
    case '\r': case '\t': case '\v':
        return true;
    };
    return false;
}

/*如果“c”是控制字符，则返回“true”。*/
inline
bool
is_control(char c)
{
    return c <= 31 || c >= 127;
}

/*如果“c”是分隔符，则返回“true”。*/
inline
bool
is_separator(char c)
{
//vfalc可以使用静态表
    switch (c)
    {
    case '(': case ')': case '<': case '>':  case '@':
    case ',': case ';': case ':': case '\\': case '"':
    case '{': case '}': case ' ': case '\t':
        return true;
    };
    return false;
}

/*如果“c”是字符，则返回“true”。*/
inline
bool
is_char(char c)
{
    return c >= 0 && c <= 127;
}

template <class FwdIter>
FwdIter
trim_left (FwdIter first, FwdIter last)
{
    return std::find_if_not (first, last,
        is_white);
}

template <class FwdIter>
FwdIter
trim_right (FwdIter first, FwdIter last)
{
    if (first == last)
        return last;
    do
    {
        --last;
        if (! is_white (*last))
            return ++last;
    }
    while (last != first);
    return first;
}

template <class CharT, class Traits, class Allocator>
void
trim_right_in_place (std::basic_string <
    CharT, Traits, Allocator>& s)
{
    s.resize (std::distance (s.begin(),
        trim_right (s.begin(), s.end())));
}

template <class FwdIter>
std::pair <FwdIter, FwdIter>
trim (FwdIter first, FwdIter last)
{
    first = trim_left (first, last);
    last = trim_right (first, last);
    return std::make_pair (first, last);
}

template <class String>
String
trim (String const& s)
{
    using std::begin;
    using std::end;
    auto first = begin(s);
    auto last = end(s);
    std::tie (first, last) = trim (first, last);
    return { first, last };
}

template <class String>
String
trim_right (String const& s)
{
    using std::begin;
    using std::end;
    auto first (begin(s));
    auto last (end(s));
    last = trim_right (first, last);
    return { first, last };
}

inline
std::string
trim (std::string const& s)
{
    return trim <std::string> (s);
}

/*解析由逗号分隔的值的字符序列。
    将转换双引号和转义序列。过剩白色
    不复制空格、逗号、双引号和空元素。
    格式：
       （标记带引号的字符串）
    参考文献：
        http://www.w3.org/protocols/rfc2616/rfc2616-sec2.html sec2
**/

template <class FwdIt,
    class Result = std::vector<
        std::basic_string<typename
            std::iterator_traits<FwdIt>::value_type>>,
                class Char>
Result
split(FwdIt first, FwdIt last, Char delim)
{
    Result result;
    using string = typename Result::value_type;
    FwdIt iter = first;
    string e;
    while (iter != last)
    {
        if (*iter == '"')
        {
//引文串
            ++iter;
            while (iter != last)
            {
                if (*iter == '"')
                {
                    ++iter;
                    break;
                }

                if (*iter == '\\')
                {
//引号对
                    ++iter;
                    if (iter != last)
                        e.append (1, *iter++);
                }
                else
                {
//QDTEX
                    e.append (1, *iter++);
                }
            }
            if (! e.empty())
            {
                result.emplace_back(std::move(e));
                e.clear();
            }
        }
        else if (*iter == delim)
        {
            e = trim_right (e);
            if (! e.empty())
            {
                result.emplace_back(std::move(e));
                e.clear();
            }
            ++iter;
        }
        else if (is_lws (*iter))
        {
            ++iter;
        }
        else
        {
            e.append (1, *iter++);
        }
    }

    if (! e.empty())
    {
        e = trim_right (e);
        if (! e.empty())
            result.emplace_back(std::move(e));
    }
    return result;
}

template <class FwdIt,
    class Result = std::vector<
        std::basic_string<typename std::iterator_traits<
            FwdIt>::value_type>>>
Result
split_commas(FwdIt first, FwdIt last)
{
    return split(first, last, ',');
}

template <class Result = std::vector<std::string>>
Result
split_commas(boost::beast::string_view const& s)
{
    return split_commas(s.begin(), s.end());
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*循环访问逗号分隔的列表。

    满足ForwardIterator的要求。

    RFC2616 2.1中定义的列表。

    @note返回的值可能包含反斜杠转义。
**/

class list_iterator
{
    using iter_type = boost::string_ref::const_iterator;

    iter_type it_;
    iter_type end_;
    boost::string_ref value_;

public:
    using value_type = boost::string_ref;
    using pointer = value_type const*;
    using reference = value_type const&;
    using difference_type = std::ptrdiff_t;
    using iterator_category =
        std::forward_iterator_tag;

    list_iterator(iter_type begin, iter_type end)
        : it_(begin)
        , end_(end)
    {
        if(it_ != end_)
            increment();
    }

    bool
    operator==(list_iterator const& other) const
    {
        return other.it_ == it_ && other.end_ == end_
            && other.value_.size() == value_.size();
    }

    bool
    operator!=(list_iterator const& other) const
    {
        return !(*this == other);
    }

    reference
    operator*() const
    {
        return value_;
    }

    pointer
    operator->() const
    {
        return &*(*this);
    }

    list_iterator&
    operator++()
    {
        increment();
        return *this;
    }

    list_iterator
    operator++(int)
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

private:
    template<class = void>
    void
    increment();
};

template<class>
void
list_iterator::increment()
{
    value_.clear();
    while(it_ != end_)
    {
        if(*it_ == '"')
        {
//引文串
            ++it_;
            if(it_ == end_)
                return;
            if(*it_ != '"')
            {
                auto start = it_;
                for(;;)
                {
                    ++it_;
                    if(it_ == end_)
                    {
                        value_ = boost::string_ref(
                            &*start, std::distance(start, it_));
                        return;
                    }
                    if(*it_ == '"')
                    {
                        value_ = boost::string_ref(
                            &*start, std::distance(start, it_));
                        ++it_;
                        return;
                    }
                }
            }
            ++it_;
        }
        else if(*it_ == ',')
        {
            it_++;
            continue;
        }
        else if(is_lws(*it_))
        {
            ++it_;
            continue;
        }
        else
        {
            auto start = it_;
            for(;;)
            {
                ++it_;
                if(it_ == end_ ||
                    *it_ == ',' ||
                        is_lws(*it_))
                {
                    value_ = boost::string_ref(
                        &*start, std::distance(start, it_));
                    return;
                }
            }
        }
    }
}

/*如果两个字符串相等，则返回true。

    使用不区分大小写的比较。
**/

inline
bool
ci_equal(boost::string_ref s1, boost::string_ref s2)
{
    return boost::range::equal(s1, s2,
        detail::ci_equal_pred{});
}

/*返回表示列表的范围。*/
inline
boost::iterator_range<list_iterator>
make_list(boost::string_ref const& field)
{
    return boost::iterator_range<list_iterator>{
        list_iterator{field.begin(), field.end()},
            list_iterator{field.end(), field.end()}};
}

/*如果列表中存在指定的标记，则返回true。

    使用不区分大小写的比较。
**/

template<class = void>
bool
token_in_list(boost::string_ref const& value,
    boost::string_ref const& token)
{
    for(auto const& item : make_list(value))
        if(ci_equal(item, token))
            return true;
    return false;
}

template<bool isRequest, class Body, class Fields>
bool
is_keep_alive(boost::beast::http::message<isRequest, Body, Fields> const& m)
{
    if(m.version() <= 10)
        return boost::beast::http::token_list{
            m[boost::beast::http::field::connection]}.exists("keep-alive");
    return ! boost::beast::http::token_list{
        m[boost::beast::http::field::connection]}.exists("close");
}

} //RCF2616
} //野兽

#endif
