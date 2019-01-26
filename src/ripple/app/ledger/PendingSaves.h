
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
  版权所有（c）2012-2015 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_PENDINGSAVES_H_INCLUDED
#define RIPPLE_APP_PENDINGSAVES_H_INCLUDED

#include <ripple/protocol/Protocol.h>
#include <map>
#include <mutex>
#include <condition_variable>

namespace ripple {

/*跟踪哪些分类帐尚未完全保存。

    在分类帐生成过程中，此集合将保留
    跟踪那些正在建造但尚未建造的分类账
    完全写下来了。
**/

class PendingSaves
{
private:
    std::mutex mutable mutex_;
    std::map <LedgerIndex, bool> map_;
    std::condition_variable await_;

public:

    /*开始使用分类帐

        这是在更新sqlite索引之前调用的。

        @如果需要完成工作，返回“真”
    **/

    bool
    startWork (LedgerIndex seq)
    {
        std::lock_guard <std::mutex> lock(mutex_);

        auto it = map_.find (seq);

        if ((it == map_.end()) || it->second)
        {
//工作完成或其他线程正在执行
            return false;
        }

        it->second = true;
        return true;
    }

    /*完成分类帐的处理

        这是在更新sqlite索引之后调用的。
        正在进行的工作的跟踪被删除，并且
        将通知等待完成的线程。
    **/

    void
    finishWork (LedgerIndex seq)
    {
        std::lock_guard <std::mutex> lock(mutex_);

        map_.erase (seq);
        await_.notify_all();
    }

    /*如果正在保存分类帐，则返回“true”。*/
    bool
    pending (LedgerIndex seq)
    {
        std::lock_guard <std::mutex> lock(mutex_);
        return map_.find(seq) != map_.end();
    }

    /*检查是否应发送分类帐

        调用以确定是否应完成工作或
        派遣。如果工作已经在进行，
        调用是同步的，请等待工作完成。

        @如果要完成或分派工作，返回“真”
    **/

    bool
    shouldWork (LedgerIndex seq, bool isSynchronous)
    {
        std::unique_lock <std::mutex> lock(mutex_);
        do
        {
            auto it = map_.find (seq);

            if (it == map_.end())
            {
                map_.emplace(seq, false);
                return true;
            }

            if (! isSynchronous)
            {
//已发送
                return false;
            }

            if (! it->second)
            {
//已调度，但未调度
                return true;
            }

//已经在进行中，只需要等待
            await_.wait (lock);

        } while (true);
    }

    /*获取挂起保存的快照

        返回的映射中的每个条目都对应于分类帐
        正在进行或已调度的。布尔值表示
        当前是否正在进行工作。
    **/

    std::map <LedgerIndex, bool>
    getSnapshot () const
    {
        std::lock_guard <std::mutex> lock(mutex_);

        return map_;
    }

};

}

#endif
