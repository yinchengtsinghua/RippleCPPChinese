
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

#ifndef RIPPLE_APP_LEDGER_OPENLEDGER_H_INCLUDED
#define RIPPLE_APP_LEDGER_OPENLEDGER_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/ledger/CachedSLEs.h>
#include <ripple/ledger/OpenView.h>
#include <ripple/app/misc/CanonicalTXSet.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/core/Config.h>
#include <ripple/beast/utility/Journal.h>
#include <cassert>
#include <mutex>

namespace ripple {

//我们总共多传球多少次
//我们必须确保至少有一个不可再审的通行证
#define LEDGER_TOTAL_PASSES 3

//我们还有多少次重试
//如果上一次重试通过进行了更改，则执行
#define LEDGER_RETRY_PASSES 1

using OrderedTxs = CanonicalTXSet;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*表示未结分类帐。*/
class OpenLedger
{
private:
    beast::Journal j_;
    CachedSLEs& cache_;
    std::mutex mutable modify_mutex_;
    std::mutex mutable current_mutex_;
    std::shared_ptr<OpenView const> current_;

public:
    /*修改函数的签名。

        修改函数在
        使用OpenView应用和修改以累积
        要用于日志记录的更改和日志。

        返回值“true”将通知OpenLedger
        做出了改变。总是返回
        “对”不会造成伤害，但可能
        次优的
    **/

    using modify_type = std::function<
        bool(OpenView&, beast::Journal)>;

    OpenLedger() = delete;
    OpenLedger (OpenLedger const&) = delete;
    OpenLedger& operator= (OpenLedger const&) = delete;

    /*创建新的未结分类帐对象。

        @param ledger已关闭的分类帐
    **/

    explicit
    OpenLedger(std::shared_ptr<
        Ledger const> const& ledger,
            CachedSLEs& cache,
                beast::Journal journal);

    /*如果没有事务，则返回“true”。

        分类帐结算的行为可能不同
        取决于交易是否存在
        在未结分类帐中。

        @注意，返回的值仅对
              那个特定的瞬间。开放的，
              空分类帐可以从变为非空
              后续修改。来电者
              负责同步
              返回值。
    **/

    bool
    empty() const;

    /*返回当前未结分类帐的视图。

        线程安全：
            可以从任何线程并发调用。

        影响：
            调用方被授予
            未结分类帐的不可修改快照
            打电话的时候。
    **/

    std::shared_ptr<OpenView const>
    current() const;

    /*修改未结分类帐

        线程安全：
            可以从任何线程并发调用。

        如果“f”返回“true”，则在
        OpenView将发布到打开的分类帐。

        @如果打开的视图被更改，返回'true'
    **/

    bool
    modify (modify_type const& f);

    /*接受新的分类帐。

        线程安全：
            可以从任何线程并发调用。

        影响：

            基于已接受分类帐的新打开视图
            已创建，并且可检索的列表
            可以先应用事务
            取决于“retriessfirst”的值。

            当前打开视图中的交易记录
            应用于新的打开视图。

            应用本地事务列表
            到新的打开视图。

            调用可选的修改函数f
            进一步修改
            原子地打开视图。变化在
            修改函数对不可见
            在accept（）返回之前调用方。

            任何失败的可检索事务都将保留
            在呼叫方的“重试”中。

            当前视图自动设置为
            新的开放视图。

        @param管理未结分类帐的规则
        @param ledger新关闭的分类帐
    **/

    void
    accept (Application& app, Rules const& rules,
        std::shared_ptr<Ledger const> const& ledger,
            OrderedTxs const& locals, bool retriesFirst,
                OrderedTxs& retries, ApplyFlags flags,
                    std::string const& suffix = "",
                        modify_type const& f = {});

private:
    /*应用事务的算法。

        它具有重试逻辑和排序语义
        用于达成共识并建立未结分类账。
    **/

    template <class FwdRange>
    static
    void
    apply (Application& app, OpenView& view,
        ReadView const& check, FwdRange const& txs,
            OrderedTxs& retries, ApplyFlags flags,
                std::map<uint256, bool>& shouldRecover,
                    beast::Journal j);

    enum Result
    {
        success,
        failure,
        retry
    };

    std::shared_ptr<OpenView>
    create (Rules const& rules,
        std::shared_ptr<Ledger const> const& ledger);

    static
    Result
    apply_one (Application& app, OpenView& view,
        std::shared_ptr< STTx const> const& tx,
            bool retry, ApplyFlags flags,
                bool shouldRecover, beast::Journal j);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class FwdRange>
void
OpenLedger::apply (Application& app, OpenView& view,
    ReadView const& check, FwdRange const& txs,
        OrderedTxs& retries, ApplyFlags flags,
            std::map<uint256, bool>& shouldRecover,
                beast::Journal j)
{
    for (auto iter = txs.begin();
        iter != txs.end(); ++iter)
    {
        try
        {
//取消对迭代器can的引用
//因为它可能会被改变。
            auto const tx = *iter;
            auto const txId = tx->getTransactionID();
            if (check.txExists(txId))
                continue;
            auto const result = apply_one(app, view,
                tx, true, flags, shouldRecover[txId], j);
            if (result == Result::retry)
                retries.insert(tx);
        }
        catch(std::exception const&)
        {
            JLOG(j.error()) <<
                "Caught exception";
        }
    }
    bool retry = true;
    for (int pass = 0;
        pass < LEDGER_TOTAL_PASSES;
            ++pass)
    {
        int changes = 0;
        auto iter = retries.begin();
        while (iter != retries.end())
        {
            switch (apply_one(app, view,
                iter->second, retry, flags,
                    shouldRecover[iter->second->getTransactionID()], j))
            {
            case Result::success:
                ++changes;
            case Result::failure:
                iter = retries.erase (iter);
                break;
            case Result::retry:
                ++iter;
            }
        }
//非重试通行证未做任何更改
        if (! changes && ! retry)
            return;
//停止可重审通行证
        if (! changes || (pass >= LEDGER_RETRY_PASSES))
            retry = false;
    }

//如果还有交易，我们必须
//至少在最后一次传球中尝试过
    assert (retries.empty() || ! retry);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//用于调试日志记录

std::string
debugTxstr (std::shared_ptr<STTx const> const& tx);

std::string
debugTostr (OrderedTxs const& set);

std::string
debugTostr (SHAMap const& set);

std::string
debugTostr (std::shared_ptr<ReadView const> const& view);

} //涟漪

#endif
