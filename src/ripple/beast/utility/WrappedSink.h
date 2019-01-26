
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

#ifndef BEAST_UTILITY_WRAPPEDSINK_H_INCLUDED
#define BEAST_UTILITY_WRAPPEDSINK_H_INCLUDED

#include <ripple/beast/utility/Journal.h>

namespace beast {

/*包装journal：：sink以在其输出前面加上字符串。*/

//包装水槽既有水槽，也有水槽：
//o它从接收器继承，因此具有正确的接口。
//o它有一个接收器（引用），因此它保留传递的write（）行为。
//从基类继承的数据将被忽略。
class WrappedSink : public beast::Journal::Sink
{
private:
    beast::Journal::Sink& sink_;
    std::string prefix_;

public:
    explicit
    WrappedSink (beast::Journal::Sink& sink, std::string const& prefix = "")
        : Sink (sink)
        , sink_(sink)
        , prefix_(prefix)
    {
    }

    explicit
    WrappedSink (beast::Journal const& journal, std::string const& prefix = "")
        : WrappedSink (journal.sink(), prefix)
    {
    }

    void prefix (std::string const& s)
    {
        prefix_ = s;
    }

    bool
    active (beast::severities::Severity level) const override
    {
        return sink_.active (level);
    }

    bool
    console () const override
    {
        return sink_.console ();
    }

    void console (bool output) override
    {
        sink_.console (output);
    }

    beast::severities::Severity
    threshold() const override
    {
        return sink_.threshold();
    }

    void threshold (beast::severities::Severity thresh) override
    {
        sink_.threshold (thresh);
    }

    void write (beast::severities::Severity level, std::string const& text) override
    {
        using beast::Journal;
        sink_.write (level, prefix_ + text);
    }
};

}

#endif
