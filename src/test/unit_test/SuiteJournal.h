
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
    版权所有（c）2018 Ripple Labs Inc.

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

#ifndef TEST_UNIT_TEST_SUITE_JOURNAL_H
#define TEST_UNIT_TEST_SUITE_JOURNAL_H

#include <ripple/beast/unit_test.h>
#include <ripple/beast/utility/Journal.h>

namespace ripple {
namespace test {

//用于Beast单元测试框架的journal:：sink。
class SuiteJournalSink : public beast::Journal::Sink
{
    std::string partition_;
    beast::unit_test::suite& suite_;

public:
    SuiteJournalSink(std::string const& partition,
            beast::severities::Severity threshold,
            beast::unit_test::suite& suite)
        : Sink (threshold, false)
        , partition_(partition + " ")
        , suite_ (suite)
    {
    }

//对于单元测试，总是生成日志文本。
    inline bool active(beast::severities::Severity level) const override
    {
        return true;
    }

    void
    write(beast::severities::Severity level, std::string const& text) override;
};

inline void
SuiteJournalSink::write (
    beast::severities::Severity level, std::string const& text)
{
    using namespace beast::severities;

    char const* const s = [level]()
    {
        switch(level)
        {
        case kTrace:    return "TRC:";
        case kDebug:    return "DBG:";
        case kInfo:     return "INF:";
        case kWarning:  return "WRN:";
        case kError:    return "ERR:";
        default:        break;
        case kFatal:    break;
        }
        return "FTL:";
    }();

//仅当级别至少等于阈值时才写入字符串。
    if (level >= threshold())
        suite_.log << s << partition_ << text << std::endl;
}

class SuiteJournal
{
    SuiteJournalSink sink_;
    beast::Journal journal_;

public:
    SuiteJournal(std::string const& partition,
            beast::unit_test::suite& suite,
            beast::severities::Severity threshold = beast::severities::kFatal)
        : sink_ (partition, threshold, suite)
        , journal_ (sink_)
    {
    }
    operator beast::Journal&() { return journal_; }
};

} //测试
} //涟漪

#endif
