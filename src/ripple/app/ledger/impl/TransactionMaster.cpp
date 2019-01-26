
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

#include <ripple/app/ledger/TransactionMaster.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/app/main/Application.h>
#include <ripple/protocol/STTx.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/chrono.h>

namespace ripple {

TransactionMaster::TransactionMaster (Application& app)
    : mApp (app)
    , mCache ("TransactionCache", 65536, std::chrono::minutes {30}, stopwatch(),
        mApp.journal("TaggedCache"))
{
}

bool TransactionMaster::inLedger (uint256 const& hash, std::uint32_t ledger)
{
    auto txn = mCache.fetch (hash);

    if (!txn)
        return false;

    txn->setStatus (COMMITTED, ledger);
    return true;
}

std::shared_ptr<Transaction>
TransactionMaster::fetch (uint256 const& txnID, bool checkDisk)
{
    auto txn = mCache.fetch (txnID);

    if (!checkDisk || txn)
        return txn;

    txn = Transaction::load (txnID, mApp);

    if (!txn)
        return txn;

    mCache.canonicalize (txnID, txn);

    return txn;
}

std::shared_ptr<STTx const>
TransactionMaster::fetch (std::shared_ptr<SHAMapItem> const& item,
    SHAMapTreeNode::TNType type,
        bool checkDisk, std::uint32_t uCommitLedger)
{
    std::shared_ptr<STTx const>  txn;
    auto iTx = fetch (item->key(), false);

    if (!iTx)
    {

        if (type == SHAMapTreeNode::tnTRANSACTION_NM)
        {
            SerialIter sit (item->slice());
            txn = std::make_shared<STTx const> (std::ref (sit));
        }
        else if (type == SHAMapTreeNode::tnTRANSACTION_MD)
        {
            auto blob = SerialIter{item->data(), item->size()}.getVL();
            txn = std::make_shared<STTx const>(SerialIter{blob.data(), blob.size()});
        }
    }
    else
    {
        if (uCommitLedger)
            iTx->setStatus (COMMITTED, uCommitLedger);

        txn = iTx->getSTransaction ();
    }

    return txn;
}

void
TransactionMaster::canonicalize(std::shared_ptr<Transaction>* pTransaction)
{
    uint256 const tid = (*pTransaction)->getID();
    if (tid != beast::zero)
    {
        auto txn = *pTransaction;
//vvalco note规范化可以更改txn的值！
        mCache.canonicalize(tid, txn);
        *pTransaction = txn;
    }
}

void TransactionMaster::sweep (void)
{
    mCache.sweep ();
}

TaggedCache <uint256, Transaction>& TransactionMaster::getCache()
{
    return mCache;
}

} //涟漪
