
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

#ifndef RIPPLE_APP_TX_TRANSACTIONMETA_H_INCLUDED
#define RIPPLE_APP_TX_TRANSACTIONMETA_H_INCLUDED

#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/STArray.h>
#include <ripple/protocol/TER.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/container/flat_set.hpp>
#include <boost/optional.hpp>

namespace ripple {

//vvalco移动到ripple/app/ledger/detail，重命名为txmeta
class TxMeta
{
public:
    using pointer = std::shared_ptr<TxMeta>;
    using ref = const pointer&;

private:
    struct CtorHelper
    {
        explicit CtorHelper() = default;
    };
    template<class T>
    TxMeta (uint256 const& txID, std::uint32_t ledger, T const& data, beast::Journal j,
                        CtorHelper);
public:
    TxMeta (beast::Journal j)
        : mLedger (0)
        , mIndex (static_cast<std::uint32_t> (-1))
        , mResult (255)
        , j_ (j)
    {
    }

    TxMeta (uint256 const& txID, std::uint32_t ledger, std::uint32_t index, beast::Journal j)
        : mTransactionID (txID)
        , mLedger (ledger)
        , mIndex (static_cast<std::uint32_t> (-1))
        , mResult (255)
        , j_(j)
    {
    }

    TxMeta (uint256 const& txID, std::uint32_t ledger, Blob const&, beast::Journal j);
    TxMeta (uint256 const& txID, std::uint32_t ledger, std::string const&, beast::Journal j);
    TxMeta (uint256 const& txID, std::uint32_t ledger, STObject const&, beast::Journal j);

    void init (uint256 const& transactionID, std::uint32_t ledger);
    void clear ()
    {
        mNodes.clear ();
    }
    void swap (TxMeta&) noexcept;

    uint256 const& getTxID ()
    {
        return mTransactionID;
    }
    std::uint32_t getLgrSeq ()
    {
        return mLedger;
    }
    int getResult () const
    {
        return mResult;
    }
    TER getResultTER () const
    {
        return TER::fromInt (mResult);
    }
    std::uint32_t getIndex () const
    {
        return mIndex;
    }

    bool isNodeAffected (uint256 const& ) const;
    void setAffectedNode (uint256 const& , SField const& type,
                          std::uint16_t nodeType);
STObject& getAffectedNode (SLE::ref node, SField const& type); //必要时创建
    STObject& getAffectedNode (uint256 const& );
    const STObject& peekAffectedNode (uint256 const& ) const;

    /*返回受此交易影响的帐户列表*/
    boost::container::flat_set<AccountID>
    getAffectedAccounts() const;

    Json::Value getJson (int p) const
    {
        return getAsObject ().getJson (p);
    }
    void addRaw (Serializer&, TER, std::uint32_t index);

    STObject getAsObject () const;
    STArray& getNodes ()
    {
        return (mNodes);
    }

    void setDeliveredAmount (STAmount const& delivered)
    {
        mDelivered.reset (delivered);
    }

    STAmount getDeliveredAmount () const
    {
        assert (hasDeliveredAmount ());
        return *mDelivered;
    }

    bool hasDeliveredAmount () const
    {
        return static_cast <bool> (mDelivered);
    }

    static bool thread (STObject& node, uint256 const& prevTxID, std::uint32_t prevLgrID);

private:
    uint256       mTransactionID;
    std::uint32_t mLedger;
    std::uint32_t mIndex;
    int           mResult;

    boost::optional <STAmount> mDelivered;

    STArray mNodes;

    beast::Journal j_;
};

} //涟漪

#endif
