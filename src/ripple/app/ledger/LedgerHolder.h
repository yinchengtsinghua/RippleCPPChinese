
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

#ifndef RIPPLE_APP_LEDGER_LEDGERHOLDER_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERHOLDER_H_INCLUDED

#include <ripple/basics/contract.h>
#include <mutex>

namespace ripple {

//std:：atomic<std:：shared_ptr>>是否可以释放此锁？

//vfalc注：这个类可以替换为atomic<shared&ptr<…>>

/*以线程安全的方式保存分类帐。

    vfalc todo构造函数应该需要一个有效的分类帐，这是
                对象始终保持值的方式。我们可以使用
                所有情况下的创世纪分类账。
**/

class LedgerHolder
{
public:
//更新保留的分类帐
    void set (std::shared_ptr<Ledger const> ledger)
    {
        if(! ledger)
            LogicError("LedgerHolder::set with nullptr");
        if(! ledger->isImmutable())
            LogicError("LedgerHolder::set with mutable Ledger");
        std::lock_guard <std::mutex> sl (m_lock);
        m_heldLedger = std::move(ledger);
    }

//返回（不可变）保留的分类帐
    std::shared_ptr<Ledger const> get ()
    {
        std::lock_guard <std::mutex> sl (m_lock);
        return m_heldLedger;
    }

    bool empty ()
    {
        std::lock_guard <std::mutex> sl (m_lock);
        return m_heldLedger == nullptr;
    }

private:
    std::mutex m_lock;
    std::shared_ptr<Ledger const> m_heldLedger;
};

} //涟漪

#endif
