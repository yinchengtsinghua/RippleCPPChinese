
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

#ifndef RIPPLE_APP_MISC_TRANSACTION_H_INCLUDED
#define RIPPLE_APP_MISC_TRANSACTION_H_INCLUDED

#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/TER.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>

namespace ripple {

//
//事务应该用JSON构造。使用Stobject:：Parsejson
//获取二进制版本。
//

class Application;
class Database;
class Rules;

enum TransStatus
{
NEW         = 0, //刚收到/生成
INVALID     = 1, //签名无效，资金不足
INCLUDED    = 2, //添加到当前分类帐
CONFLICTED  = 3, //输给冲突的交易
COMMITTED   = 4, //已知在分类帐中
HELD        = 5, //现在无效，也许以后有效
REMOVED     = 6, //从分类帐中取出
OBSOLETE    = 7, //已优先处理兼容事务
INCOMPLETE  = 8  //需要更多签名
};

//此类用于构造和检查事务。
//事务是静态的，因此不需要操作函数。
class Transaction
    : public std::enable_shared_from_this<Transaction>
    , public CountedObject <Transaction>
{
public:
    static char const* getCountedObjectName () { return "Transaction"; }

    using pointer = std::shared_ptr<Transaction>;
    using ref = const pointer&;

public:
    Transaction (
        std::shared_ptr<STTx const> const&, std::string&, Application&) noexcept;

    static
    Transaction::pointer
    transactionFromSQL (
        boost::optional<std::uint64_t> const& ledgerSeq,
        boost::optional<std::string> const& status,
        Blob const& rawTxn,
        Application& app);

    static
    Transaction::pointer
    transactionFromSQLValidated (
        boost::optional<std::uint64_t> const& ledgerSeq,
        boost::optional<std::string> const& status,
        Blob const& rawTxn,
        Application& app);

    static
    TransStatus
    sqlTransactionStatus(boost::optional<std::string> const& status);

    std::shared_ptr<STTx const> const& getSTransaction ()
    {
        return mTransaction;
    }

    uint256 const& getID () const
    {
        return mTransactionID;
    }

    LedgerIndex getLedger () const
    {
        return mInLedger;
    }

    TransStatus getStatus () const
    {
        return mStatus;
    }

    TER getResult ()
    {
        return mResult;
    }

    void setResult (TER terResult)
    {
        mResult = terResult;
    }

    void setStatus (TransStatus status, std::uint32_t ledgerSeq);

    void setStatus (TransStatus status)
    {
        mStatus = status;
    }

    void setLedger (LedgerIndex ledger)
    {
        mInLedger = ledger;
    }

    /*
     *将此标志添加到批次后设置。
     **/

    void setApplying()
    {
        mApplying = true;
    }

    /*
     *检测事务是否正在批处理。
     *
     *@返回是否在批内应用事务。
     **/

    bool getApplying()
    {
        return mApplying;
    }

    /*
     *表示已尝试事务应用程序。
     **/

    void clearApplying()
    {
        mApplying = false;
    }

    Json::Value getJson (int options, bool binary = false) const;

    static Transaction::pointer load (uint256 const& id, Application& app);

private:
    uint256         mTransactionID;

    LedgerIndex     mInLedger = 0;
    TransStatus     mStatus = INVALID;
    TER             mResult = temUNCERTAIN;
    bool            mApplying = false;

    std::shared_ptr<STTx const>   mTransaction;
    Application&    mApp;
    beast::Journal  j_;
};

} //涟漪

#endif
