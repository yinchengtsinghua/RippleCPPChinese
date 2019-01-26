
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

#include <ripple/nodestore/impl/BatchWriter.h>

namespace ripple {
namespace NodeStore {

BatchWriter::BatchWriter (Callback& callback, Scheduler& scheduler)
    : m_callback (callback)
    , m_scheduler (scheduler)
    , mWriteLoad (0)
    , mWritePending (false)
{
    mWriteSet.reserve (batchWritePreallocationSize);
}

BatchWriter::~BatchWriter ()
{
    waitForWriting ();
}

void
BatchWriter::store (std::shared_ptr<NodeObject> const& object)
{
    std::unique_lock<decltype(mWriteMutex)> sl (mWriteMutex);

//如果批量已达到极限，我们等待
//直到批处理编写器完成
    while (mWriteSet.size() >= batchWriteLimitSize)
        mWriteCondition.wait (sl);

    mWriteSet.push_back (object);

    if (! mWritePending)
    {
        mWritePending = true;

        m_scheduler.scheduleTask (*this);
    }
}

int
BatchWriter::getWriteLoad ()
{
    std::lock_guard<decltype(mWriteMutex)> sl (mWriteMutex);

    return std::max (mWriteLoad, static_cast<int> (mWriteSet.size ()));
}

void
BatchWriter::performScheduledTask ()
{
    writeBatch ();
}

void
BatchWriter::writeBatch ()
{
    for (;;)
    {
        std::vector< std::shared_ptr<NodeObject> > set;

        set.reserve (batchWritePreallocationSize);

        {
            std::lock_guard<decltype(mWriteMutex)> sl (mWriteMutex);

            mWriteSet.swap (set);
            assert (mWriteSet.empty ());
            mWriteLoad = set.size ();

            if (set.empty ())
            {
                mWritePending = false;
                mWriteCondition.notify_all ();

//vfalc note将此函数修复为不从中间返回
                return;
            }

        }

        BatchWriteReport report;
        report.writeCount = set.size();
        auto const before = std::chrono::steady_clock::now();

        m_callback.writeBatch (set);

        report.elapsed = std::chrono::duration_cast <std::chrono::milliseconds>
            (std::chrono::steady_clock::now() - before);

        m_scheduler.onBatchWrite (report);
    }
}

void
BatchWriter::waitForWriting ()
{
    std::unique_lock <decltype(mWriteMutex)> sl (mWriteMutex);

    while (mWritePending)
        mWriteCondition.wait (sl);
}

}
}
