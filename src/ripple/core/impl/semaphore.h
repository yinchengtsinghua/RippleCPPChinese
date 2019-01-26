
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

#ifndef RIPPLE_CORE_SEMAPHORE_H_INCLUDED
#define RIPPLE_CORE_SEMAPHORE_H_INCLUDED

#include <condition_variable>
#include <mutex>

namespace ripple {

template <class Mutex, class CondVar>
class basic_semaphore
{
private:
    using scoped_lock = std::unique_lock <Mutex>;

    Mutex m_mutex;
    CondVar m_cond;
    std::size_t m_count;

public:
    using size_type = std::size_t;

    /*使用可选的初始计数创建信号量。
        如果未指定，则初始计数为零。
    **/

    explicit basic_semaphore (size_type count = 0)
        : m_count (count)
    {
    }

    /*增加计数并取消阻止一个等待线程。*/
    void notify ()
    {
        scoped_lock lock (m_mutex);
        ++m_count;
        m_cond.notify_one ();
    }

    /*在调用notify之前阻止。*/
    void wait ()
    {
        scoped_lock lock (m_mutex);
        while (m_count == 0)
            m_cond.wait (lock);
        --m_count;
    }

    /*执行非阻塞等待。
        @如果等待满意，返回'true'。
    **/

    bool try_wait ()
    {
        scoped_lock lock (m_mutex);
        if (m_count == 0)
            return false;
        --m_count;
        return true;
    }
};

using semaphore = basic_semaphore <std::mutex, std::condition_variable>;

} //涟漪

#endif

