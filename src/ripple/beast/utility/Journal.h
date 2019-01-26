
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

#ifndef BEAST_UTILITY_JOURNAL_H_INCLUDED
#define BEAST_UTILITY_JOURNAL_H_INCLUDED

#include <cassert>
#include <sstream>

namespace beast {

/*用于轻松访问日志严重性值的命名空间。*/
namespace severities
{
    /*日志消息的严重性级别/阈值。*/
    enum Severity
    {
        kAll = 0,

        kTrace = kAll,
        kDebug,
        kInfo,
        kWarning,
        kError,
        kFatal,

        kDisabled,
        kNone = kDisabled
    };
}

/*日志消息的通用终结点。

    该杂志有几个简单的目标：

     *重量轻，按值复制。
     *允许在源代码中保留日志语句。
     *在运行时根据日志阈值控制日志记录。

    建议在格式化日志之前检查journal:：active（level）。
    文本。这样做可以避免在结果
    不会发送到日志。
**/

class Journal
{
public:
    class Sink;

private:
//日志消息的严重性级别/阈值。
    using Severity = severities::Severity;

//不变量：M_接收器始终指向有效的接收器
    Sink* m_sink;

public:
//——————————————————————————————————————————————————————————————

    /*底层消息目的地的抽象。*/
    class Sink
    {
    protected:
        Sink () = delete;
        explicit Sink(Sink const& sink) = default;
        Sink (Severity thresh, bool console);
        Sink& operator= (Sink const& lhs) = delete;

    public:
        virtual ~Sink () = 0;

        /*如果传递的严重性为的文本产生输出，则返回“true”。*/
        virtual bool active (Severity level) const;

        /*如果消息也写入输出窗口（msvc），则返回“true”。*/
        virtual bool console () const;

        /*设置消息是否也写入输出窗口（MSVC）。*/
        virtual void console (bool output);

        /*返回此接收器将报告的最低严重性级别。*/
        virtual Severity threshold() const;

        /*设置此接收器将报告的最小严重性。*/
        virtual void threshold (Severity thresh);

        /*以指定的严重性将文本写入接收器。
            如果传递了
            级别低于当前阈值（）。
        **/

        virtual void write (Severity level, std::string const& text) = 0;

    private:
        Severity thresh_;
        bool m_console;
    };

#ifndef __INTELLISENSE__
static_assert(std::is_default_constructible<Sink>::value == false, "");
static_assert(std::is_copy_constructible<Sink>::value == false, "");
static_assert(std::is_move_constructible<Sink>::value == false, "");
static_assert(std::is_copy_assignable<Sink>::value == false, "");
static_assert(std::is_move_assignable<Sink>::value == false, "");
static_assert(std::is_nothrow_destructible<Sink>::value == true, "");
#endif

    /*返回一个不执行任何操作的水槽。*/
    static Sink& getNullSink ();

//——————————————————————————————————————————————————————————————

    class Stream;

private:
    /*作用域基于ostream的容器，用于将消息写入日志。*/
    class ScopedStream
    {
    public:
        ScopedStream (ScopedStream const& other)
            : ScopedStream (other.m_sink, other.m_level)
        { }

        ScopedStream (Sink& sink, Severity level);

        template <typename T>
        ScopedStream (Stream const& stream, T const& t);

        ScopedStream (
            Stream const& stream, std::ostream& manip (std::ostream&));

        ScopedStream& operator= (ScopedStream const&) = delete;

        ~ScopedStream ();

        std::ostringstream& ostream () const
        {
            return m_ostream;
        }

        std::ostream& operator<< (
            std::ostream& manip (std::ostream&)) const;

        template <typename T>
        std::ostream& operator<< (T const& t) const;

    private:
        Sink& m_sink;
        Severity const m_level;
        std::ostringstream mutable m_ostream;
    };

#ifndef __INTELLISENSE__
static_assert(std::is_default_constructible<ScopedStream>::value == false, "");
static_assert(std::is_copy_constructible<ScopedStream>::value == true, "");
static_assert(std::is_move_constructible<ScopedStream>::value == true, "");
static_assert(std::is_copy_assignable<ScopedStream>::value == false, "");
static_assert(std::is_move_assignable<ScopedStream>::value == false, "");
static_assert(std::is_nothrow_destructible<ScopedStream>::value == true, "");
#endif

//——————————————————————————————————————————————————————————————
public:
    /*在设置字符串格式之前，提供一种轻量级的方法来检查active（）。*/
    class Stream
    {
    public:
        /*创建不产生输出的流。*/
        explicit Stream ()
            : m_sink (getNullSink())
            , m_level (severities::kDisabled)
        { }

        /*创建在给定级别写入的流。

            构造函数是内联的，因此检查active（）非常便宜。
        **/

        Stream (Sink& sink, Severity level)
            : m_sink (sink)
            , m_level (level)
        {
            assert (m_level < severities::kDisabled);
        }

        /*构造或复制另一个流。*/
        Stream (Stream const& other)
            : Stream (other.m_sink, other.m_level)
        { }

        Stream& operator= (Stream const& other) = delete;

        /*返回此流写入的接收器。*/
        Sink& sink() const
        {
            return m_sink;
        }

        /*返回此流报告的消息的严重性级别。*/
        Severity level() const
        {
            return m_level;
        }

        /*如果接收器在此流级别记录任何内容，则返回“true”。*/
        /*@ {*/
        bool active() const
        {
            return m_sink.active (m_level);
        }

        explicit
        operator bool() const
        {
            return active();
        }
        /*@ }*/

        /*输出流支持。*/
        /*@ {*/
        ScopedStream operator<< (std::ostream& manip (std::ostream&)) const;

        template <typename T>
        ScopedStream operator<< (T const& t) const;
        /*@ }*/

    private:
        Sink& m_sink;
        Severity m_level;
    };

#ifndef __INTELLISENSE__
static_assert(std::is_default_constructible<Stream>::value == true, "");
static_assert(std::is_copy_constructible<Stream>::value == true, "");
static_assert(std::is_move_constructible<Stream>::value == true, "");
static_assert(std::is_copy_assignable<Stream>::value == false, "");
static_assert(std::is_move_assignable<Stream>::value == false, "");
static_assert(std::is_nothrow_destructible<Stream>::value == true, "");
#endif

//——————————————————————————————————————————————————————————————

    /*日记没有默认构造函数。*/
    Journal () = delete;

    /*创建写入指定接收器的日志。*/
    explicit Journal (Sink& sink)
    : m_sink (&sink)
    { }

    /*返回与此日志关联的接收器。*/
    Sink& sink() const
    {
        return *m_sink;
    }

    /*返回具有指定严重性级别的此接收器的流。*/
    Stream stream (Severity level) const
    {
        return Stream (*m_sink, level);
    }

    /*如果将在此严重级别记录任何消息，则返回“true”。
        对于要记录的消息，严重性必须等于或高于
        接收器的严重性阈值。
    **/

    bool active (Severity level) const
    {
        return m_sink->active (level);
    }

    /*严重性流访问功能。*/
    /*@ {*/
    Stream trace() const
    {
        return { *m_sink, severities::kTrace };
    }

    Stream debug() const
    {
        return { *m_sink, severities::kDebug };
    }

    Stream info() const
    {
        return { *m_sink, severities::kInfo };
    }

    Stream warn() const
    {
        return { *m_sink, severities::kWarning };
    }

    Stream error() const
    {
        return { *m_sink, severities::kError };
    }

    Stream fatal() const
    {
        return { *m_sink, severities::kFatal };
    }
    /*@ }*/
};

#ifndef __INTELLISENSE__
static_assert(std::is_default_constructible<Journal>::value == false, "");
static_assert(std::is_copy_constructible<Journal>::value == true, "");
static_assert(std::is_move_constructible<Journal>::value == true, "");
static_assert(std::is_copy_assignable<Journal>::value == true, "");
static_assert(std::is_move_assignable<Journal>::value == true, "");
static_assert(std::is_nothrow_destructible<Journal>::value == true, "");
#endif

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <typename T>
Journal::ScopedStream::ScopedStream (Journal::Stream const& stream, T const& t)
   : ScopedStream (stream.sink(), stream.level())
{
    m_ostream << t;
}

template <typename T>
std::ostream&
Journal::ScopedStream::operator<< (T const& t) const
{
    m_ostream << t;
    return m_ostream;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <typename T>
Journal::ScopedStream
Journal::Stream::operator<< (T const& t) const
{
    return ScopedStream (*this, t);
}

namespace detail {

template<class CharT, class Traits = std::char_traits<CharT>>
class logstream_buf
    : public std::basic_stringbuf<CharT, Traits>
{
    beast::Journal::Stream strm_;

    template<class T>
    void write(T const*) = delete;

    void write(char const* s)
    {
        if(strm_)
            strm_ << s;
    }

    void write(wchar_t const* s)
    {
        if(strm_)
            strm_ << s;
    }

public:
    explicit
    logstream_buf(beast::Journal::Stream const& strm)
        : strm_(strm)
    {
    }

    ~logstream_buf()
    {
        sync();
    }

    int
    sync() override
    {
        write(this->str().c_str());
        this->str("");
        return 0;
    }
};

} //细节

template<
    class CharT,
    class Traits = std::char_traits<CharT>
>
class basic_logstream
    : public std::basic_ostream<CharT, Traits>
{
    typedef CharT                          char_type;
    typedef Traits                         traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::pos_type pos_type;
    typedef typename traits_type::off_type off_type;

    detail::logstream_buf<CharT, Traits> buf_;
public:
    explicit
    basic_logstream(beast::Journal::Stream const& strm)
        : std::basic_ostream<CharT, Traits>(&buf_)
        , buf_(strm)
    {
    }
};

using logstream = basic_logstream<char>;
using logwstream = basic_logstream<wchar_t>;

} //野兽

#endif
