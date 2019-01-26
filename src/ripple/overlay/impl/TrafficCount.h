
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

#ifndef RIPPLE_OVERLAY_TRAFFIC_H_INCLUDED
#define RIPPLE_OVERLAY_TRAFFIC_H_INCLUDED

#include <ripple/basics/safe_cast.h>
#include <ripple/protocol/messages.h>

#include <atomic>
#include <map>

namespace ripple {

class TrafficCount
{

public:

    using count_t = std::atomic <unsigned long>;

    class TrafficStats
    {
        public:

        count_t bytesIn;
        count_t bytesOut;
        count_t messagesIn;
        count_t messagesOut;

        TrafficStats() : bytesIn(0), bytesOut(0),
            messagesIn(0), messagesOut(0)
        { ; }

        TrafficStats(const TrafficStats& ts)
            : bytesIn (ts.bytesIn.load())
            , bytesOut (ts.bytesOut.load())
            , messagesIn (ts.messagesIn.load())
            , messagesOut (ts.messagesOut.load())
        { ; }

        operator bool () const
        {
            return messagesIn || messagesOut;
        }
    };


    enum class category
    {
CT_base,           //基本对等开销，必须首先
CT_overlay,        //覆盖管理
        CT_transaction,
        CT_proposal,
        CT_validation,
CT_get_ledger,     //我们试图得到的账本
CT_share_ledger,   //我们共享的分类帐
CT_get_trans,      //我们试图得到的事务集
CT_share_trans,    //我们得到的事务集
CT_unknown         //必须是最后的
    };

    static const char* getName (category c);

    static category categorize (
        ::google::protobuf::Message const& message,
        int type, bool inbound);

    void addCount (category cat, bool inbound, int number)
    {
        if (inbound)
        {
            counts_[cat].bytesIn += number;
            ++counts_[cat].messagesIn;
        }
        else
        {
            counts_[cat].bytesOut += number;
            ++counts_[cat].messagesOut;
        }
    }

    TrafficCount()
    {
        for (category i = category::CT_base;
            i <= category::CT_unknown;
            i = safe_cast<category>(safe_cast<std::underlying_type_t<category>>(i) + 1))
        {
            counts_[i];
        }
    }

    std::map <std::string, TrafficStats>
    getCounts () const
    {
        std::map <std::string, TrafficStats> ret;

        for (auto& i : counts_)
        {
            if (i.second)
                ret.emplace (std::piecewise_construct,
                    std::forward_as_tuple (getName (i.first)),
                    std::forward_as_tuple (i.second));
        }

        return ret;
    }

    protected:

    std::map <category, TrafficStats> counts_;
};

}
#endif
