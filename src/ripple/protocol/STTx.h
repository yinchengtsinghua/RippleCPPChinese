
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

#ifndef RIPPLE_PROTOCOL_STTX_H_INCLUDED
#define RIPPLE_PROTOCOL_STTX_H_INCLUDED

#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/TxFormats.h>
#include <boost/container/flat_set.hpp>
#include <boost/logic/tribool.hpp>
#include <functional>

namespace ripple {

enum TxnSql : char
{
    txnSqlNew = 'N',
    txnSqlConflict = 'C',
    txnSqlHeld = 'H',
    txnSqlValidated = 'V',
    txnSqlIncluded = 'I',
    txnSqlUnknown = 'U'
};

class STTx final
    : public STObject
    , public CountedObject <STTx>
{
public:
    static char const* getCountedObjectName () { return "STTx"; }

    static std::size_t const minMultiSigners = 1;
    static std::size_t const maxMultiSigners = 8;

public:
    STTx () = delete;
    STTx& operator= (STTx const& other) = delete;

    STTx (STTx const& other) = default;

    explicit STTx (SerialIter& sit) noexcept (false);
    explicit STTx (SerialIter&& sit) noexcept (false) : STTx(sit) {}

    explicit STTx (STObject&& object) noexcept (false);

    /*构造事务。

        返回的事务将具有指定的类型和
        回调函数添加到对象中的任何字段
        那是传进来的。
    **/

    STTx (
        TxType type,
        std::function<void(STObject&)> assembler);

    STBase*
    copy (std::size_t n, void* buf) const override
    {
        return emplace(n, buf, *this);
    }

    STBase*
    move (std::size_t n, void* buf) override
    {
        return emplace(n, buf, std::move(*this));
    }

//Stobject函数。
    SerializedTypeID getSType () const override
    {
        return STI_TRANSACTION;
    }
    std::string getFullText () const override;

//外部事务函数/签名函数。
    Blob getSignature () const;

    uint256 getSigningHash () const;

    TxType getTxnType () const
    {
        return tx_type_;
    }

    Blob getSigningPubKey () const
    {
        return getFieldVL (sfSigningPubKey);
    }

    std::uint32_t getSequence () const
    {
        return getFieldU32 (sfSequence);
    }
    void setSequence (std::uint32_t seq)
    {
        return setFieldU32 (sfSequence, seq);
    }

    boost::container::flat_set<AccountID>
    getMentionedAccounts() const;

    uint256 getTransactionID () const
    {
        return tid_;
    }

    Json::Value getJson (int options) const override;
    Json::Value getJson (int options, bool binary) const;

    void sign (
        PublicKey const& publicKey,
        SecretKey const& secretKey);

    /*检查签名。
        @return`true`如果签名有效。如果无效，则返回错误消息字符串。
    **/

    std::pair<bool, std::string>
    checkSign(bool allowMultiSign) const;

//带有元数据的SQL函数。
    static
    std::string const&
    getMetaSQLInsertReplaceHeader ();

    std::string getMetaSQL (
        std::uint32_t inLedger, std::string const& escapedMetaData) const;

    std::string getMetaSQL (
        Serializer rawTxn,
        std::uint32_t inLedger,
        char status,
        std::string const& escapedMetaData) const;

private:
    std::pair<bool, std::string> checkSingleSign () const;
    std::pair<bool, std::string> checkMultiSign () const;

    uint256 tid_;
    TxType tx_type_;
};

bool passesLocalChecks (STObject const& st, std::string&);

/*对交易进行消毒。

    事务被序列化然后反序列化，
    确保所有等价事务都处于规范状态
    形式。这还确保了程序元数据，例如
    事务摘要都是计算出来的。
**/

std::shared_ptr<STTx const>
sterilize (STTx const& stx);

/*检查事务是否为伪事务*/
bool isPseudoTx(STObject const& tx);

} //涟漪

#endif
