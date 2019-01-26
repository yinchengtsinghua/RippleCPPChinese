
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

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/ledger/LocalTxs.h>
#include <ripple/app/main/Application.h>
#include <ripple/protocol/Indexes.h>

/*
 此代码防止出现以下情况：
1）客户提交交易。
2）该事务进入该服务器的分类帐
   相信将是共识分类账。
3）服务器在没有
   交易（因为它在上一个分类账中）。
4）本地共识分类账不是多数分类账。
   （由于网络条件、拜占庭断层等原因）
   多数分类账不包括交易。
5）服务器创建一个新的未结分类账，其中不包括
   或将其存在以前的分类帐中。
6）客户提交另一个交易并获得一个解释
   初步结果。
7）服务器不中继第二个事务，至少
   还没有。

使用此代码，当步骤5发生时，第一个事务将
应用于未结分类帐，以便第二个交易将
第6步正常成功。交易保持跟踪
测试适用于所有新的开放式分类账，直到在完全-
已验证的分类帐
**/


namespace ripple {

//此类将指向事务的指针与
//它的过期分类帐。它还缓存发行帐户。
class LocalTx
{
public:

//持有交易的分类账的数量基本上是
//任意的允许交易
//进入一个完全有效的分类帐。
    static int const holdLedgers = 5;

    LocalTx (LedgerIndex index, std::shared_ptr<STTx const> const& txn)
        : m_txn (txn)
        , m_expire (index + holdLedgers)
        , m_id (txn->getTransactionID ())
        , m_account (txn->getAccountID(sfAccount))
        , m_seq (txn->getSequence())
    {
        if (txn->isFieldPresent (sfLastLedgerSequence))
            m_expire = std::min (m_expire, txn->getFieldU32 (sfLastLedgerSequence) + 1);
    }

    uint256 const& getID () const
    {
        return m_id;
    }

    std::uint32_t getSeq () const
    {
        return m_seq;
    }

    bool isExpired (LedgerIndex i) const
    {
        return i > m_expire;
    }

    std::shared_ptr<STTx const> const& getTX () const
    {
        return m_txn;
    }

    AccountID const& getAccount () const
    {
        return m_account;
    }

private:

    std::shared_ptr<STTx const> m_txn;
    LedgerIndex                    m_expire;
    uint256                        m_id;
    AccountID                      m_account;
    std::uint32_t                  m_seq;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class LocalTxsImp
    : public LocalTxs
{
public:
    LocalTxsImp() = default;

//向本地事务集添加新事务
    void push_back (LedgerIndex index, std::shared_ptr<STTx const> const& txn) override
    {
        std::lock_guard <std::mutex> lock (m_lock);

        m_txns.emplace_back (index, txn);
    }

    CanonicalTXSet
    getTxSet () override
    {
        CanonicalTXSet tset (uint256 {});

//获取本地事务集作为规范
//设置（以便按有效顺序应用）
        {
            std::lock_guard <std::mutex> lock (m_lock);

            for (auto const& it : m_txns)
                tset.insert (it.getTX());
        }

        return tset;
    }

//删除已接受的交易
//到一个完全有效的分类帐，是（现在）不可能的，
//或者已经过期
    void sweep (ReadView const& view) override
    {
        std::lock_guard <std::mutex> lock (m_lock);

        m_txns.remove_if ([&view](auto const& txn)
        {
            if (txn.isExpired (view.info().seq))
                return true;
            if (view.txExists(txn.getID()))
                return true;

            std::shared_ptr<SLE const> sle = view.read(
                 keylet::account(txn.getAccount()));
            if (! sle)
                return false;
            return sle->getFieldU32 (sfSequence) > txn.getSeq ();
        });
    }

    std::size_t size () override
    {
        std::lock_guard <std::mutex> lock (m_lock);

        return m_txns.size ();
    }

private:
    std::mutex m_lock;
    std::list <LocalTx> m_txns;
};

std::unique_ptr<LocalTxs>
make_LocalTxs ()
{
    return std::make_unique<LocalTxsImp> ();
}

} //涟漪
