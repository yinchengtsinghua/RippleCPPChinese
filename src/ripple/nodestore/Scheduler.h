
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

#ifndef RIPPLE_NODESTORE_SCHEDULER_H_INCLUDED
#define RIPPLE_NODESTORE_SCHEDULER_H_INCLUDED

#include <ripple/nodestore/Task.h>
#include <chrono>

namespace ripple {
namespace NodeStore {

/*包含有关提取操作的信息。*/
struct FetchReport
{
    explicit FetchReport() = default;

    std::chrono::milliseconds elapsed;
    bool isAsync;
    bool wentToDisk;
    bool wasFound;
};

/*包含有关批写入操作的信息。*/
struct BatchWriteReport
{
    explicit BatchWriteReport() = default;

    std::chrono::milliseconds elapsed;
    int writeCount;
};

/*异步后端活动的调度

    为了提高性能，后端可以选择执行写入操作
    成批地可以使用提供的计划程序计划这些写入操作
    对象。

    @参见批处理编写器
**/

class Scheduler
{
public:
    virtual ~Scheduler() = default;

    /*安排任务。
        根据实现的不同，可以在
        当前执行线程或定义的未指定实现
        外螺纹。
    **/

    virtual void scheduleTask (Task& task) = 0;

    /*报告获取完成
        允许计划程序监视节点存储的性能
    **/

    virtual void onFetch (FetchReport const& report) = 0;

    /*报告批写入的完成情况
        允许计划程序监视节点存储的性能
    **/

    virtual void onBatchWrite (BatchWriteReport const& report) = 0;
};

}
}

#endif
