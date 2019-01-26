
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

#ifndef RIPPLE_APP_LEDGER_ORDERBOOKDB_H_INCLUDED
#define RIPPLE_APP_LEDGER_ORDERBOOKDB_H_INCLUDED

#include <ripple/app/ledger/AcceptedLedgerTx.h>
#include <ripple/app/ledger/BookListeners.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/OrderBook.h>
#include <mutex>

namespace ripple {

class OrderBookDB
    : public Stoppable
{
public:
    OrderBookDB (Application& app, Stoppable& parent);

    void setup (std::shared_ptr<ReadView const> const& ledger);
    void update (std::shared_ptr<ReadView const> const& ledger);
    void invalidate ();

    void addOrderBook(Book const&);

    /*@返回需要此issuerid和currencyid的所有医嘱簿的列表。
     **/

    OrderBook::List getBooksByTakerPays (Issue const&);

    /*@返回需要此问题的所有订单的计数，以及
        CurryCyID。*/

    int getBookSize(Issue const&);

    bool isBookToXRP (Issue const&);

    BookListeners::pointer getBookListeners (Book const&);
    BookListeners::pointer makeBookListeners (Book const&);

//查看此txn是否影响任何订单
    void processTxn (
        std::shared_ptr<ReadView const> const& ledger,
        const AcceptedLedgerTx& alTx, Json::Value const& jvObj);

    using IssueToOrderBook = hash_map <Issue, OrderBook::List>;

private:
    void rawAddBook(Book const&);

    Application& app_;

//CI/II
    IssueToOrderBook mSourceMap;

//通过CO/IO
    IssueToOrderBook mDestMap;

//是否存在到XRP的订单簿？
    hash_set <Issue> mXRPBooks;

    std::recursive_mutex mLock;

    using BookToListenersMap = hash_map <Book, BookListeners::pointer>;

    BookToListenersMap mListeners;

    std::uint32_t mSeq;

    beast::Journal j_;
};

} //涟漪

#endif
