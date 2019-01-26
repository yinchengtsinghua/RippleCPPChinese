
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

#ifndef RIPPLE_NODESTORE_BATCHWRITER_H_INCLUDED
#define RIPPLE_NODESTORE_BATCHWRITER_H_INCLUDED

#include <ripple/nodestore/Scheduler.h>
#include <ripple/nodestore/Task.h>
#include <ripple/nodestore/Types.h>
#include <condition_variable>
#include <mutex>

namespace ripple {
namespace NodeStore {

/*批处理写入辅助逻辑。

    批处理写入是使用计划任务执行的。使用
    类它不是必需的。后端可以实现自己的写批处理，
    或者跳过写批处理，如果这样做可以提高性能。

    参见调度程序
**/

class BatchWriter : private Task
{
public:
    /*此回调执行实际写入操作。*/
    struct Callback
    {
        virtual ~Callback () = default;
        Callback() = default;
        Callback(Callback const&) = delete;
        Callback& operator=(Callback const&) = delete;

        virtual void writeBatch (Batch const& batch) = 0;
    };

    /*创建批处理编写器。*/
    BatchWriter (Callback& callback, Scheduler& scheduler);

    /*销毁批处理编写器。

        在返回之前，批处理中的任何挂起的内容都将被写出。
    **/

    ~BatchWriter ();

    /*存储对象。

        这将添加到批处理中并启动计划任务
        写出批次。
    **/

    void store (std::shared_ptr<NodeObject> const& object);

    /*获取待处理的写入I/O量的估计值。*/
    int getWriteLoad ();

private:
    void performScheduledTask () override;
    void writeBatch ();
    void waitForWriting ();

private:
    using LockType = std::recursive_mutex;
    using CondvarType = std::condition_variable_any;

    Callback& m_callback;
    Scheduler& m_scheduler;
    LockType mWriteMutex;
    CondvarType mWriteCondition;
    int mWriteLoad;
    bool mWritePending;
    Batch mWriteSet;
};

}
}

#endif
