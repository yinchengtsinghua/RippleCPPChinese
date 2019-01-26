
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

#include <ripple/beast/utility/Journal.h>
#include <cassert>

namespace beast {

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//一个什么都不做的水槽。
class NullJournalSink : public Journal::Sink
{
public:
    NullJournalSink ()
    : Sink (severities::kDisabled, false)
    { }

    ~NullJournalSink() override = default;

    bool active (severities::Severity) const override
    {
        return false;
    }

    bool console() const override
    {
        return false;
    }

    void console (bool) override
    {
    }

    severities::Severity threshold() const override
    {
        return severities::kDisabled;
    }

    void threshold (severities::Severity) override
    {
    }

    void write (severities::Severity, std::string const&) override
    {
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Journal::Sink& Journal::getNullSink ()
{
    static NullJournalSink sink;
    return sink;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Journal::Sink::Sink (Severity thresh, bool console)
    : thresh_ (thresh)
    , m_console (console)
{
}

Journal::Sink::~Sink() = default;

bool Journal::Sink::active (Severity level) const
{
    return level >= thresh_;
}

bool Journal::Sink::console () const
{
    return m_console;
}

void Journal::Sink::console (bool output)
{
    m_console = output;
}

severities::Severity Journal::Sink::threshold () const
{
    return thresh_;
}

void Journal::Sink::threshold (Severity thresh)
{
    thresh_ = thresh;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Journal::ScopedStream::ScopedStream (Sink& sink, Severity level)
    : m_sink (sink)
    , m_level (level)
{
//从所有目录应用的修饰符
    m_ostream
        << std::boolalpha
        << std::showbase;
}

Journal::ScopedStream::ScopedStream (
    Stream const& stream, std::ostream& manip (std::ostream&))
    : ScopedStream (stream.sink(), stream.level())
{
    m_ostream << manip;
}

Journal::ScopedStream::~ScopedStream ()
{
    std::string const& s (m_ostream.str());
    if (! s.empty ())
    {
        if (s == "\n")
            m_sink.write (m_level, "");
        else
            m_sink.write (m_level, s);
    }
}

std::ostream& Journal::ScopedStream::operator<< (std::ostream& manip (std::ostream&)) const
{
    return m_ostream << manip;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Journal::ScopedStream Journal::Stream::operator<< (
    std::ostream& manip (std::ostream&)) const
{
    return ScopedStream (*this, manip);
}

} //野兽

