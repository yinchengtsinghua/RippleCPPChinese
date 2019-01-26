
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

#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/contract.h>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>

namespace ripple {

Logs::Sink::Sink (std::string const& partition,
    beast::severities::Severity thresh, Logs& logs)
    : beast::Journal::Sink (thresh, false)
    , logs_(logs)
    , partition_(partition)
{
}

void
Logs::Sink::write (beast::severities::Severity level, std::string const& text)
{
    if (level < threshold())
        return;

    logs_.write (level, partition_, text, console());
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Logs::File::File()
    : m_stream (nullptr)
{
}

bool Logs::File::isOpen () const noexcept
{
    return m_stream != nullptr;
}

bool Logs::File::open (boost::filesystem::path const& path)
{
    close ();

    bool wasOpened = false;

//vfalc todo使用unicode文件路径
    std::unique_ptr <std::ofstream> stream (
        new std::ofstream (path.c_str (), std::fstream::app));

    if (stream->good ())
    {
        m_path = path;

        m_stream = std::move (stream);

        wasOpened = true;
    }

    return wasOpened;
}

bool Logs::File::closeAndReopen ()
{
    close ();

    return open (m_path);
}

void Logs::File::close ()
{
    m_stream = nullptr;
}

void Logs::File::write (char const* text)
{
    if (m_stream != nullptr)
        (*m_stream) << text;
}

void Logs::File::writeln (char const* text)
{
    if (m_stream != nullptr)
    {
        (*m_stream) << text;
        (*m_stream) << std::endl;
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Logs::Logs(beast::severities::Severity thresh)
: thresh_ (thresh) //默认严重性
{
}

bool
Logs::open (boost::filesystem::path const& pathToLogFile)
{
    return file_.open(pathToLogFile);
}

beast::Journal::Sink&
Logs::get (std::string const& name)
{
    std::lock_guard <std::mutex> lock (mutex_);
    auto const result =
        sinks_.emplace(name, makeSink(name, thresh_));
    return *result.first->second;
}

beast::Journal::Sink&
Logs::operator[] (std::string const& name)
{
    return get(name);
}

beast::Journal
Logs::journal (std::string const& name)
{
    return beast::Journal (get(name));
}

beast::severities::Severity
Logs::threshold() const
{
    return thresh_;
}

void
Logs::threshold (beast::severities::Severity thresh)
{
    std::lock_guard <std::mutex> lock (mutex_);
    thresh_ = thresh;
    for (auto& sink : sinks_)
        sink.second->threshold (thresh);
}

std::vector<std::pair<std::string, std::string>>
Logs::partition_severities() const
{
    std::vector<std::pair<std::string, std::string>> list;
    std::lock_guard <std::mutex> lock (mutex_);
    list.reserve (sinks_.size());
    for (auto const& e : sinks_)
        list.push_back(std::make_pair(e.first,
            toString(fromSeverity(e.second->threshold()))));
    return list;
}

void
Logs::write (beast::severities::Severity level, std::string const& partition,
    std::string const& text, bool console)
{
    std::string s;
    format (s, text, level, partition);
    std::lock_guard <std::mutex> lock (mutex_);
    file_.writeln (s);
    if (! silent_)
        std::cerr << s << '\n';
//vfalc todo修复控制台输出
//如果（控制台）
//输出控制台；
}

std::string
Logs::rotate()
{
    std::lock_guard <std::mutex> lock (mutex_);
    bool const wasOpened = file_.closeAndReopen ();
    if (wasOpened)
        return "The log file was closed and reopened.";
    return "The log file could not be closed and reopened.";
}

std::unique_ptr<beast::Journal::Sink>
Logs::makeSink(std::string const& name,
    beast::severities::Severity threshold)
{
    return std::make_unique<Sink>(
        name, threshold, *this);
}

LogSeverity
Logs::fromSeverity (beast::severities::Severity level)
{
    using namespace beast::severities;
    switch (level)
    {
    case kTrace:   return lsTRACE;
    case kDebug:   return lsDEBUG;
    case kInfo:    return lsINFO;
    case kWarning: return lsWARNING;
    case kError:   return lsERROR;

    default:
        assert(false);
    case kFatal:
        break;
    }

    return lsFATAL;
}

beast::severities::Severity
Logs::toSeverity (LogSeverity level)
{
    using namespace beast::severities;
    switch (level)
    {
    case lsTRACE:   return kTrace;
    case lsDEBUG:   return kDebug;
    case lsINFO:    return kInfo;
    case lsWARNING: return kWarning;
    case lsERROR:   return kError;
    default:
        assert(false);
    case lsFATAL:
        break;
    }

    return kFatal;
}

std::string
Logs::toString (LogSeverity s)
{
    switch (s)
    {
    case lsTRACE:   return "Trace";
    case lsDEBUG:   return "Debug";
    case lsINFO:    return "Info";
    case lsWARNING: return "Warning";
    case lsERROR:   return "Error";
    case lsFATAL:   return "Fatal";
    default:
        assert (false);
        return "Unknown";
    }
}

LogSeverity
Logs::fromString (std::string const& s)
{
    if (boost::iequals (s, "trace"))
        return lsTRACE;

    if (boost::iequals (s, "debug"))
        return lsDEBUG;

    if (boost::iequals (s, "info") || boost::iequals (s, "information"))
        return lsINFO;

    if (boost::iequals (s, "warn") || boost::iequals (s, "warning") || boost::iequals (s, "warnings"))
        return lsWARNING;

    if (boost::iequals (s, "error") || boost::iequals (s, "errors"))
        return lsERROR;

    if (boost::iequals (s, "fatal") || boost::iequals (s, "fatals"))
        return lsFATAL;

    return lsINVALID;
}

void
Logs::format (std::string& output, std::string const& message,
    beast::severities::Severity severity, std::string const& partition)
{
    output.reserve (message.size() + partition.size() + 100);

    output = to_string(std::chrono::system_clock::now());

    output += " ";
    if (! partition.empty ())
        output += partition + ":";

    using namespace beast::severities;
    switch (severity)
    {
    case kTrace:    output += "TRC "; break;
    case kDebug:    output += "DBG "; break;
    case kInfo:     output += "NFO "; break;
    case kWarning:  output += "WRN "; break;
    case kError:    output += "ERR "; break;
    default:
        assert(false);
    case kFatal:    output += "FTL "; break;
    }

    output += message;

//限制输出的最大长度
    if (output.size() > maximumMessageCharacters)
    {
        output.resize (maximumMessageCharacters - 3);
        output += "...";
    }

//试图阻止敏感信息出现在日志文件中
//用星号标记。
    auto scrubber = [&output](char const* token)
    {
        auto first = output.find(token);

//如果我们找到了指定的令牌，则尝试隔离
//敏感数据（用双引号括起来）并屏蔽：
        if (first != std::string::npos)
        {
            first = output.find ('\"', first + std::strlen(token));

            if (first != std::string::npos)
            {
                auto last = output.find('\"', ++first);

                if (last == std::string::npos)
                    last = output.size();

                output.replace (first, last - first, last - first, '*');
            }
        }
    };

    scrubber ("\"seed\"");
    scrubber ("\"seed_hex\"");
    scrubber ("\"secret\"");
    scrubber ("\"master_key\"");
    scrubber ("\"master_seed\"");
    scrubber ("\"master_seed_hex\"");
    scrubber ("\"passphrase\"");
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class DebugSink
{
private:
    std::reference_wrapper<beast::Journal::Sink> sink_;
    std::unique_ptr<beast::Journal::Sink> holder_;
    std::mutex m_;

public:
    DebugSink ()
        : sink_ (beast::Journal::getNullSink())
    {
    }

    DebugSink (DebugSink const&) = delete;
    DebugSink& operator=(DebugSink const&) = delete;

    DebugSink(DebugSink&&) = delete;
    DebugSink& operator=(DebugSink&&) = delete;

    std::unique_ptr<beast::Journal::Sink>
    set(std::unique_ptr<beast::Journal::Sink> sink)
    {
        std::lock_guard<std::mutex> _(m_);

        using std::swap;
        swap (holder_, sink);

        if (holder_)
            sink_ = *holder_;
        else
            sink_ = beast::Journal::getNullSink();

        return sink;
    }

    beast::Journal::Sink&
    get()
    {
        std::lock_guard<std::mutex> _(m_);
        return sink_.get();
    }
};

static
DebugSink&
debugSink()
{
    static DebugSink _;
    return _;
}

std::unique_ptr<beast::Journal::Sink>
setDebugLogSink(
    std::unique_ptr<beast::Journal::Sink> sink)
{
    return debugSink().set(std::move(sink));
}

beast::Journal
debugLog()
{
    return beast::Journal (debugSink().get());
}

} //涟漪
