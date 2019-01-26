
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

#ifndef RIPPLE_CORE_JOB_H_INCLUDED
#define RIPPLE_CORE_JOB_H_INCLUDED

#include <ripple/core/LoadMonitor.h>
#include <functional>

#include <functional>

namespace ripple {

//请注意，此队列应仅用于CPU绑定的作业
//主要用于签名检查

enum JobType
{
//表示无效作业的特殊类型-将很快消失。
    jtINVALID = -1,

//作业类型-此枚举中的位置指示作业优先级
//优先级较低的早期作业。如果你愿意
//以特定优先级插入作业，只需将其添加到正确的位置。

jtPACK,          //为对等机生成获取包
jtPUBOLDLEDGER,  //已接受旧分类帐
jtVALIDATION_ut, //来自不可信源的验证
jtTRANSACTION_l, //本地事务
jtLEDGER_REQ,    //对等请求分类账/txset数据
jtPROPOSAL_ut,   //来自不可信来源的建议
jtLEDGER_DATA,   //我们正在获取的分类帐的接收数据
jtCLIENT,        //来自客户端的websocket命令
jtRPC,           //来自客户端的websocket命令
jtUPDATE_PF,     //更新寻路请求
jtTRANSACTION,   //从网络接收的事务
jtBATCH,         //应用批处理事务
jtADVANCE,       //预先确认/获得的分类账
jtPUBLEDGER,     //发布完全接受的分类帐
jtTXN_DATA,      //获取建议集
jtWAL,           //提前写入日志
jtVALIDATION_t,  //来自可信来源的验证
jtWRITE,         //写出哈希对象
jtACCEPT,        //接受共识分类账
jtPROPOSAL_t,    //来自可信来源的建议
jtSWEEP,         //扫描陈旧结构
jtNETOP_CLUSTER, //NetworkOps群集对等报告
jtNETOP_TIMER,   //网络操作网络计时器处理
jtADMIN,         //管理操作

//不由作业池调度的特殊作业类型
    jtPEER          ,
    jtDISK          ,
    jtTXN_PROC      ,
    jtOB_SETUP      ,
    jtPATH_FIND     ,
    jtHO_READ       ,
    jtHO_WRITE      ,
jtGENERIC       ,   //用来测量时间

//节点存储监视
    jtNS_SYNC_READ  ,
    jtNS_ASYNC_READ ,
    jtNS_WRITE      ,
};

class Job
{
public:
    using clock_type = std::chrono::steady_clock;

    /*默认构造函数。

        允许作业用作容器类型。

        这用于允许jobmap[key]=value等操作。
    **/

//vfalc注意，我不希望有一个默认的构造对象。
//没有关联的作业的语义是什么？
//功能？具有不变量“所有作业对象引用
//一份工作“将减少各州的数量。
//
    Job ();

//工作（工作常数和其他）；

    Job (JobType type, std::uint64_t index);

    /*用于检查是否取消作业的回调。*/
    using CancelCallback = std::function <bool(void)>;

//vvalco todo尝试移除对LoadMonitor的依赖。
    Job (JobType type,
         std::string const& name,
         std::uint64_t index,
         LoadMonitor& lm,
         std::function <void (Job&)> const& job,
         CancelCallback cancelCallback);

//作业和操作员=（作业常数和其他）；

    JobType getType () const;

    CancelCallback getCancelCallback () const;

    /*返回作业排队的时间。*/
    clock_type::time_point const& queue_time () const;

    /*如果正在运行的作业应尽最大努力取消，则返回“true”。*/
    bool shouldCancel () const;

    void doJob ();

    void rename (std::string const& n);

//这些比较运算符使作业按优先级排序
//在作业集中
    bool operator< (const Job& j) const;
    bool operator> (const Job& j) const;
    bool operator<= (const Job& j) const;
    bool operator>= (const Job& j) const;

private:
    CancelCallback m_cancelCallback;
    JobType                     mType;
    std::uint64_t               mJobIndex;
    std::function <void (Job&)> mJob;
    std::shared_ptr<LoadEvent>  m_loadEvent;
    std::string                 mName;
    clock_type::time_point m_queue_time;
};

}

#endif
