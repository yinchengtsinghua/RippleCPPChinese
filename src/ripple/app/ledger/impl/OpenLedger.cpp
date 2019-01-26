
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

#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/HashRouter.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/app/tx/apply.h>
#include <ripple/ledger/CachedView.h>
#include <ripple/overlay/Message.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/overlay/predicates.h>
#include <ripple/protocol/Feature.h>
#include <boost/range/adaptor/transformed.hpp>

namespace ripple {

OpenLedger::OpenLedger(std::shared_ptr<
    Ledger const> const& ledger,
        CachedSLEs& cache,
            beast::Journal journal)
    : j_ (journal)
    , cache_ (cache)
    , current_ (create(ledger->rules(), ledger))
{
}

bool
OpenLedger::empty() const
{
    std::lock_guard<
        std::mutex> lock(modify_mutex_);
    return current_->txCount() == 0;
}

std::shared_ptr<OpenView const>
OpenLedger::current() const
{
    std::lock_guard<
        std::mutex> lock(
            current_mutex_);
    return current_;
}

bool
OpenLedger::modify (modify_type const& f)
{
    std::lock_guard<
        std::mutex> lock1(modify_mutex_);
    auto next = std::make_shared<
        OpenView>(*current_);
    auto const changed = f(*next, j_);
    if (changed)
    {
        std::lock_guard<
            std::mutex> lock2(
                current_mutex_);
        current_ = std::move(next);
    }
    return changed;
}

void
OpenLedger::accept(Application& app, Rules const& rules,
    std::shared_ptr<Ledger const> const& ledger,
        OrderedTxs const& locals, bool retriesFirst,
            OrderedTxs& retries, ApplyFlags flags,
                std::string const& suffix,
                    modify_type const& f)
{
    JLOG(j_.trace()) <<
        "accept ledger " << ledger->seq() << " " << suffix;
    auto next = create(rules, ledger);
    std::map<uint256, bool> shouldRecover;
    if (retriesFirst)
    {
        for (auto const& tx : retries)
        {
            auto const txID = tx.second->getTransactionID();
            shouldRecover[txID] = app.getHashRouter().shouldRecover(txID);
        }
//处理有争议的Tx，外锁
        using empty =
            std::vector<std::shared_ptr<
                STTx const>>;
        apply (app, *next, *ledger, empty{},
            retries, flags, shouldRecover, j_);
    }
//阻止要修改的调用，否则
//新发送进入未结分类帐
//会迷路的。
    std::lock_guard<
        std::mutex> lock1(modify_mutex_);
//从当前打开的视图应用Tx
    if (! current_->txs.empty())
    {
        for (auto const& tx : current_->txs)
        {
            auto const txID = tx.first->getTransactionID();
            auto iter = shouldRecover.lower_bound(txID);
            if (iter != shouldRecover.end()
                && iter->first == txID)
//已经有了通过争议的机会
                iter->second = false;
            else
                shouldRecover.emplace_hint(iter, txID,
                    app.getHashRouter().shouldRecover(txID));
        }
        apply (app, *next, *ledger,
            boost::adaptors::transform(
                current_->txs,
            [](std::pair<std::shared_ptr<
                STTx const>, std::shared_ptr<
                    STObject const>> const& p)
            {
                return p.first;
            }),
                retries, flags, shouldRecover, j_);
    }
//调用修饰符
    if (f)
        f(*next, j_);
//应用本地TX
    for (auto const& item : locals)
        app.getTxQ().apply(app, *next,
            item.second, flags, j_);

//如果我们最近没有转发此事务，请将其转发给所有对等方
    for (auto const& txpair : next->txs)
    {
        auto const& tx = txpair.first;
        auto const txId = tx->getTransactionID();
        if (auto const toSkip = app.getHashRouter().shouldRelay(txId))
        {
            JLOG(j_.debug()) << "Relaying recovered tx " << txId;
            protocol::TMTransaction msg;
            Serializer s;

            tx->add(s);
            msg.set_rawtransaction(s.data(), s.size());
            msg.set_status(protocol::tsNEW);
            msg.set_receivetimestamp(
                app.timeKeeper().now().time_since_epoch().count());
            app.overlay().foreach(send_if_not(
                std::make_shared<Message>(msg, protocol::mtTRANSACTION),
                peer_in_set(*toSkip)));
        }
    }

//切换到新的打开视图
    std::lock_guard<
        std::mutex> lock2(current_mutex_);
    current_ = std::move(next);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::shared_ptr<OpenView>
OpenLedger::create (Rules const& rules,
    std::shared_ptr<Ledger const> const& ledger)
{
    return std::make_shared<OpenView>(
        open_ledger, rules, std::make_shared<
            CachedLedger const>(ledger,
                cache_));
}

auto
OpenLedger::apply_one (Application& app, OpenView& view,
    std::shared_ptr<STTx const> const& tx,
        bool retry, ApplyFlags flags, bool shouldRecover,
            beast::Journal j) -> Result
{
    if (retry)
        flags = flags | tapRETRY;
    auto const result = [&]
    {
        auto const queueResult = app.getTxQ().apply(
            app, view, tx, flags | tapPREFER_QUEUE, j);
//如果事务不能进入队列
//原因，它仍然可以恢复，试着把它
//直接放入未结分类帐，否则将其放下。
        if (queueResult.first == telCAN_NOT_QUEUE && shouldRecover)
            return ripple::apply(app, view, *tx, flags, j);
        return queueResult;
    }();
    if (result.second ||
            result.first == terQUEUED)
        return Result::success;
    if (isTefFailure (result.first) ||
        isTemMalformed (result.first) ||
            isTelLocal (result.first))
        return Result::failure;
    return Result::retry;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::string
debugTxstr (std::shared_ptr<STTx const> const& tx)
{
    std::stringstream ss;
    ss << tx->getTransactionID();
    return ss.str().substr(0, 4);
}

std::string
debugTostr (OrderedTxs const& set)
{
    std::stringstream ss;
    for(auto const& item : set)
        ss << debugTxstr(item.second) << ", ";
    return ss.str();
}

std::string
debugTostr (SHAMap const& set)
{
    std::stringstream ss;
    for (auto const& item : set)
    {
        try
        {
            SerialIter sit(item.slice());
            auto const tx = std::make_shared<
                STTx const>(sit);
            ss << debugTxstr(tx) << ", ";
        }
        catch(std::exception const&)
        {
            ss << "THRO, ";
        }
    }
    return ss.str();
}

std::string
debugTostr (std::shared_ptr<ReadView const> const& view)
{
    std::stringstream ss;
    for(auto const& item : view->txs)
        ss << debugTxstr(item.first) << ", ";
    return ss.str();
}

} //涟漪
