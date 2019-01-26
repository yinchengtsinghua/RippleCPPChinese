
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

#ifndef RIPPLE_CORE_JOBTYPEINFO_H_INCLUDED
#define RIPPLE_CORE_JOBTYPEINFO_H_INCLUDED

namespace ripple
{

/*保存有关作业的所有“静态”信息，这些信息不会更改*/
class JobTypeInfo
{
private:
    JobType const m_type;
    std::string const m_name;

    /*此作业类型的运行作业数限制。*/
    int const m_limit;

    /*特殊作业不通过作业队列调度*/
    bool const m_special;

    /*此作业类型的平均和峰值延迟。未指定0*/
    std::chrono::milliseconds const m_avgLatency;
    std::chrono::milliseconds const m_peakLatency;

public:
//不可默认构造
    JobTypeInfo () = delete;

    JobTypeInfo (JobType type, std::string name, int limit,
            bool special, std::chrono::milliseconds avgLatency,
            std::chrono::milliseconds peakLatency)
        : m_type (type)
        , m_name (std::move(name))
        , m_limit (limit)
        , m_special (special)
        , m_avgLatency (avgLatency)
        , m_peakLatency (peakLatency)
    {

    }

    JobType type () const
    {
        return m_type;
    }

    std::string const& name () const
    {
        return m_name;
    }

    int limit () const
    {
        return m_limit;
    }

    bool special () const
    {
        return m_special;
    }

    std::chrono::milliseconds getAverageLatency () const
    {
        return m_avgLatency;
    }

    std::chrono::milliseconds getPeakLatency () const
    {
        return m_peakLatency;
    }
};

}

#endif
