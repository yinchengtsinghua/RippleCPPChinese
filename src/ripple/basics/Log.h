
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

#ifndef RIPPLE_BASICS_LOG_H_INCLUDED
#define RIPPLE_BASICS_LOG_H_INCLUDED

#include <ripple/basics/UnorderedContainers.h>
#include <boost/beast/core/string.hpp>
#include <ripple/beast/utility/Journal.h>
#include <boost/filesystem.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace ripple {

//已弃用，请改用Beast:：Severities:：Severity
enum LogSeverity
{
lsINVALID   = -1,   //用于指示无效的严重性
lsTRACE     = 0,    //非常低级的进度信息，内部细节
//手术
lsDEBUG     = 1,    //功能级进度信息、操作
lsINFO      = 2,    //服务器级进度信息，主要操作
lsWARNING   = 3,    //需要人们注意的情况，可能表明
//一个问题
lsERROR     = 4,    //表示有问题的情况
lsFATAL     = 5     //表示服务器问题的严重情况
};

/*管理分区以进行日志记录。*/
class Logs
{
private:
    class Sink : public beast::Journal::Sink
    {
    private:
        Logs& logs_;
        std::string partition_;

    public:
        Sink (std::string const& partition,
            beast::severities::Severity thresh, Logs& logs);

        Sink (Sink const&) = delete;
        Sink& operator= (Sink const&) = delete;

        void
        write (beast::severities::Severity level,
               std::string const& text) override;
    };

    /*管理包含日志输出的系统文件。
        系统文件在程序执行期间保持打开状态。界面
        用于与标准日志管理进行互操作
        logrotate（8）等工具：
            http://linuxcommand.org/man_pages/logrotate8.html
        @注意，列出的接口都不是线程安全的。
    **/

    class File
    {
    public:
        /*构造时没有关联的系统文件。
            系统文件稍后可能与@ref open关联。
            见打开
        **/

        File ();

        /*摧毁物体。
            如果关联了系统文件，它将被刷新并关闭。
        **/

        ~File () = default;

        /*确定系统文件是否与日志关联。
            @return`true`如果系统文件已关联并打开
            写作。
        **/

        bool isOpen () const noexcept;

        /*将系统文件与日志关联。
            如果文件不存在，则尝试创建该文件
            打开它写作。如果文件已经存在，尝试
            使其打开以便附加。
            如果系统文件已与日志关联，则将关闭该文件。
            第一。
            @如果文件已打开，返回'true'。
        **/

//vfalc注意到该参数很不幸是boost类型，因为它
//可以是wchar或char，具体取决于平台。
//TODO替换为Beast：：文件
//
        bool open (boost::filesystem::path const& path);

        /*关闭并重新打开与日志关联的系统文件
            这有助于与外部日志管理工具进行互操作。
            @如果文件已打开，返回'true'。
        **/

        bool closeAndReopen ();

        /*如果系统文件已打开，请将其关闭。*/
        void close ();

        /*写入日志文件。
            如果没有关联的系统文件，则不执行任何操作。
        **/

        void write (char const* text);

        /*写入日志文件并附加行尾标记。
            如果没有关联的系统文件，则不执行任何操作。
        **/

        void writeln (char const* text);

        /*使用std:：string写入日志文件。*/
        /*@ {*/
        void write (std::string const& str)
        {
            write (str.c_str ());
        }

        void writeln (std::string const& str)
        {
            writeln (str.c_str ());
        }
        /*@ }*/

    private:
        std::unique_ptr <std::ofstream> m_stream;
        boost::filesystem::path m_path;
    };

    std::mutex mutable mutex_;
    std::map <std::string,
        std::unique_ptr<beast::Journal::Sink>,
            boost::beast::iless> sinks_;
    beast::severities::Severity thresh_;
    File file_;
    bool silent_ = false;

public:
    Logs(beast::severities::Severity level);

    Logs (Logs const&) = delete;
    Logs& operator= (Logs const&) = delete;

    virtual ~Logs() = default;

    bool
    open (boost::filesystem::path const& pathToLogFile);

    beast::Journal::Sink&
    get (std::string const& name);

    beast::Journal::Sink&
    operator[] (std::string const& name);

    beast::Journal
    journal (std::string const& name);

    beast::severities::Severity
    threshold() const;

    void
    threshold (beast::severities::Severity thresh);

    std::vector<std::pair<std::string, std::string>>
    partition_severities() const;

    void
    write (beast::severities::Severity level, std::string const& partition,
        std::string const& text, bool console);

    std::string
    rotate();

    /*
     *设置标志将日志写入stderr（false）或not（true）。
     *
     *@param bsilent相应地设置标志。
     **/

    void
    silent (bool bSilent)
    {
        silent_ = bSilent;
    }

    virtual
    std::unique_ptr<beast::Journal::Sink>
    makeSink(std::string const& partition,
        beast::severities::Severity startingLevel);

public:
    static
    LogSeverity
    fromSeverity (beast::severities::Severity level);

    static
    beast::severities::Severity
    toSeverity (LogSeverity level);

    static
    std::string
    toString (LogSeverity s);

    static
    LogSeverity
    fromString (std::string const& s);

private:
    enum
    {
//日志消息的最大行长度。
//如果消息超过此长度，将用省略号截断。
        maximumMessageCharacters = 12 * 1024
    };

    static
    void
    format (std::string& output, std::string const& message,
        beast::severities::Severity severity, std::string const& partition);
};

//包装日志：：流以跳过对
//如果流不是活动的，则会列出昂贵的参数。
#ifndef JLOG
#define JLOG(x) if (!x) { } else x
#endif

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//调试日志记录：

/*设置调试日志的接收器。

    @param sink对新调试接收器的唯一指针。
    @将唯一指针返回到上一个接收器。如果没有水槽，则为nullptr。
**/

std::unique_ptr<beast::Journal::Sink>
setDebugLogSink(
    std::unique_ptr<beast::Journal::Sink> sink);

/*返回调试日志。
    日志可能会排到空接收器，因此其输出
    可能永远看不见。千万不要用它来批评
    信息。
**/

beast::Journal
debugLog();

} //涟漪

#endif
