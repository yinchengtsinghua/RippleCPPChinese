
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

#include <ripple/app/ledger/OrderBookDB.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/core/JobQueue.h>
#include <ripple/protocol/Indexes.h>

namespace ripple {

OrderBookDB::OrderBookDB (Application& app, Stoppable& parent)
    : Stoppable ("OrderBookDB", parent)
    , app_ (app)
    , mSeq (0)
    , j_ (app.journal ("OrderBookDB"))
{
}

void OrderBookDB::invalidate ()
{
    std::lock_guard <std::recursive_mutex> sl (mLock);
    mSeq = 0;
}

void OrderBookDB::setup(
    std::shared_ptr<ReadView const> const& ledger)
{
    {
        std::lock_guard <std::recursive_mutex> sl (mLock);
        auto seq = ledger->info().seq;

//每256个分类帐进行一次全面更新
        if (mSeq != 0)
        {
            if (seq == mSeq)
                return;
            if ((seq > mSeq) && ((seq - mSeq) < 256))
                return;
            if ((seq < mSeq) && ((mSeq - seq) < 16))
                return;
        }

        JLOG (j_.debug())
            << "Advancing from " << mSeq << " to " << seq;

        mSeq = seq;
    }

    if (app_.config().PATH_SEARCH_MAX == 0)
    {
//无事可做
    }
    else if (app_.config().standalone())
        update(ledger);
    else
        app_.getJobQueue().addJob(
            jtUPDATE_PF, "OrderBookDB::update",
            [this, ledger] (Job&) { update(ledger); });
}

void OrderBookDB::update(
    std::shared_ptr<ReadView const> const& ledger)
{
    hash_set< uint256 > seen;
    OrderBookDB::IssueToOrderBook destMap;
    OrderBookDB::IssueToOrderBook sourceMap;
    hash_set< Issue > XRPBooks;

    JLOG (j_.debug()) << "OrderBookDB::update>";

    if (app_.config().PATH_SEARCH_MAX == 0)
    {
//已禁用寻路功能
        return;
    }

//浏览整个分类账，查找订单簿条目
    int books = 0;

    try
    {
        for(auto& sle : ledger->sles)
        {
            if (isStopping())
            {
                JLOG (j_.info())
                    << "OrderBookDB::update exiting due to isStopping";
                std::lock_guard <std::recursive_mutex> sl (mLock);
                mSeq = 0;
                return;
            }

            if (sle->getType () == ltDIR_NODE &&
                sle->isFieldPresent (sfExchangeRate) &&
                sle->getFieldH256 (sfRootIndex) == sle->key())
            {
                Book book;
                book.in.currency.copyFrom(sle->getFieldH160(
                    sfTakerPaysCurrency));
                book.in.account.copyFrom(sle->getFieldH160 (
                    sfTakerPaysIssuer));
                book.out.account.copyFrom(sle->getFieldH160(
                    sfTakerGetsIssuer));
                book.out.currency.copyFrom (sle->getFieldH160(
                    sfTakerGetsCurrency));

                uint256 index = getBookBase (book);
                if (seen.insert (index).second)
                {
                    auto orderBook = std::make_shared<OrderBook> (index, book);
                    sourceMap[book.in].push_back (orderBook);
                    destMap[book.out].push_back (orderBook);
                    if (isXRP(book.out))
                        XRPBooks.insert(book.in);
                    ++books;
                }
            }
        }
    }
    catch (const SHAMapMissingNode&)
    {
        JLOG (j_.info())
            << "OrderBookDB::update encountered a missing node";
        std::lock_guard <std::recursive_mutex> sl (mLock);
        mSeq = 0;
        return;
    }

    JLOG (j_.debug())
        << "OrderBookDB::update< " << books << " books found";
    {
        std::lock_guard <std::recursive_mutex> sl (mLock);

        mXRPBooks.swap(XRPBooks);
        mSourceMap.swap(sourceMap);
        mDestMap.swap(destMap);
    }
    app_.getLedgerMaster().newOrderBookDB();
}

void OrderBookDB::addOrderBook(Book const& book)
{
    bool toXRP = isXRP (book.out);
    std::lock_guard <std::recursive_mutex> sl (mLock);

    if (toXRP)
    {
//我们不想搜索所有到xrp或从xrp订单
//书！
        for (auto ob: mSourceMap[book.in])
        {
if (isXRP (ob->getCurrencyOut ())) //也谈XRP
                return;
        }
    }
    else
    {
        for (auto ob: mDestMap[book.out])
        {
            if (ob->getCurrencyIn() == book.in.currency &&
                ob->getIssuerIn() == book.in.account)
            {
                return;
            }
        }
    }
    uint256 index = getBookBase(book);
    auto orderBook = std::make_shared<OrderBook> (index, book);

    mSourceMap[book.in].push_back (orderBook);
    mDestMap[book.out].push_back (orderBook);
    if (toXRP)
        mXRPBooks.insert(book.in);
}

//返回希望此issuerID和currencyID的所有医嘱簿的列表
OrderBook::List OrderBookDB::getBooksByTakerPays (Issue const& issue)
{
    std::lock_guard <std::recursive_mutex> sl (mLock);
    auto it = mSourceMap.find (issue);
    return it == mSourceMap.end () ? OrderBook::List() : it->second;
}

int OrderBookDB::getBookSize(Issue const& issue) {
    std::lock_guard <std::recursive_mutex> sl (mLock);
    auto it = mSourceMap.find (issue);
    return it == mSourceMap.end () ? 0 : it->second.size();
}

bool OrderBookDB::isBookToXRP(Issue const& issue)
{
    std::lock_guard <std::recursive_mutex> sl (mLock);
    return mXRPBooks.count(issue) > 0;
}

BookListeners::pointer OrderBookDB::makeBookListeners (Book const& book)
{
    std::lock_guard <std::recursive_mutex> sl (mLock);
    auto ret = getBookListeners (book);

    if (!ret)
    {
        ret = std::make_shared<BookListeners> ();

        mListeners [book] = ret;
        assert (getBookListeners (book) == ret);
    }

    return ret;
}

BookListeners::pointer OrderBookDB::getBookListeners (Book const& book)
{
    BookListeners::pointer ret;
    std::lock_guard <std::recursive_mutex> sl (mLock);

    auto it0 = mListeners.find (book);
    if (it0 != mListeners.end ())
        ret = it0->second;

    return ret;
}

//基于meta，将meta发送到正在侦听的流。
//我们需要确定给定元效应的流。
void OrderBookDB::processTxn (
    std::shared_ptr<ReadView const> const& ledger,
        const AcceptedLedgerTx& alTx, Json::Value const& jvObj)
{
    std::lock_guard <std::recursive_mutex> sl (mLock);
    if (alTx.getResult () == tesSUCCESS)
    {
//对于这个特定的事务，维护
//已发布的订阅。这会阻止发送
//如果事务涉及多个LTOFFER，则多次执行该事务
//同一本书的条目，或者如果它涉及多本书和
//单一客户已经订阅了这些书籍。
        hash_set<std::uint64_t> havePublished;

//检查这是一个报价还是一个报价取消还是一个付款
//接受报价。
//看看meta是什么样子的。
        for (auto& node : alTx.getMeta ()->getNodes ())
        {
            try
            {
                if (node.getFieldU16 (sfLedgerEntryType) == ltOFFER)
                {
                    SField const* field = nullptr;

//我们需要一个包含接受者所得和接受者支付的字段
//参数。
                    if (node.getFName () == sfModifiedNode)
                        field = &sfPreviousFields;
                    else if (node.getFName () == sfCreatedNode)
                        field = &sfNewFields;
                    else if (node.getFName () == sfDeletedNode)
                        field = &sfFinalFields;

                    if (field)
                    {
                        auto data = dynamic_cast<const STObject*> (
                            node.peekAtPField (*field));

                        if (data &&
                            data->isFieldPresent (sfTakerPays) &&
                            data->isFieldPresent (sfTakerGets))
                        {
//确定医嘱簿
                            Book b{data->getFieldAmount(sfTakerGets).issue(),
                                data->getFieldAmount(sfTakerPays).issue()};

                            auto listeners = getBookListeners(b);
                            if (listeners)
                            {
                                listeners->publish(jvObj, havePublished);
                            }
                        }
                    }
                }
            }
            catch (std::exception const&)
            {
                JLOG (j_.info())
                    << "Fields not found in OrderBookDB::processTxn";
            }
        }
    }
}

} //涟漪
