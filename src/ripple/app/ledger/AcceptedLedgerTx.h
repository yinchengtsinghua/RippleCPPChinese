
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

#ifndef RIPPLE_APP_LEDGER_ACCEPTEDLEDGERTX_H_INCLUDED
#define RIPPLE_APP_LEDGER_ACCEPTEDLEDGERTX_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/protocol/AccountID.h>
#include <boost/container/flat_set.hpp>

namespace ripple {

class Logs;

/*
    已关闭分类帐中的交易记录。

    描述

    接受的分类帐交易记录包含以下附加信息：
    服务器需要告诉客户有关事务的信息。例如，
        -JSON形式的交易
        -哪些账户受到影响
          *InfoSub使用它向客户报告
        缓存的东西

    @代码
    @端码

    @见{uri }

    @InGroup Ripple_分类帐
**/

class AcceptedLedgerTx
{
public:
    using pointer = std::shared_ptr <AcceptedLedgerTx>;
    using ref = const pointer&;

public:
    AcceptedLedgerTx (
        std::shared_ptr<ReadView const> const& ledger,
        std::shared_ptr<STTx const> const&,
        std::shared_ptr<STObject const> const&,
        AccountIDCache const&,
        Logs&);
    AcceptedLedgerTx (
        std::shared_ptr<ReadView const> const&,
        std::shared_ptr<STTx const> const&,
        TER,
        AccountIDCache const&,
        Logs&);

    std::shared_ptr <STTx const> const& getTxn () const
    {
        return mTxn;
    }
    std::shared_ptr <TxMeta> const& getMeta () const
    {
        return mMeta;
    }

    boost::container::flat_set<AccountID> const&
    getAffected() const
    {
        return mAffected;
    }

    TxID getTransactionID () const
    {
        return mTxn->getTransactionID ();
    }
    TxType getTxnType () const
    {
        return mTxn->getTxnType ();
    }
    TER getResult () const
    {
        return mResult;
    }
    std::uint32_t getTxnSeq () const
    {
        return mMeta->getIndex ();
    }

    bool isApplied () const
    {
        return bool(mMeta);
    }
    int getIndex () const
    {
        return mMeta ? mMeta->getIndex () : 0;
    }
    std::string getEscMeta () const;
    Json::Value getJson () const
    {
        return mJson;
    }

private:
    std::shared_ptr<ReadView const> mLedger;
    std::shared_ptr<STTx const> mTxn;
    std::shared_ptr<TxMeta> mMeta;
    TER                             mResult;
    boost::container::flat_set<AccountID> mAffected;
    Blob        mRawMeta;
    Json::Value                     mJson;
    AccountIDCache const& accountCache_;
    Logs& logs_;

    void buildJson ();
};

} //涟漪

#endif
