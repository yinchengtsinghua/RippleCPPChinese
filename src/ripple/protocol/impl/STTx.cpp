
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

#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/Sign.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/protocol/STArray.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/json/to_string.h>
#include <boost/format.hpp>
#include <array>
#include <memory>
#include <type_traits>
#include <utility>

namespace ripple {

static
auto getTxFormat (TxType type)
{
    auto format = TxFormats::getInstance().findByType (type);

    if (format == nullptr)
    {
        Throw<std::runtime_error> (
            "Invalid transaction type " +
            std::to_string (
                safe_cast<std::underlying_type_t<TxType>>(type)));
    }

    return format;
}

STTx::STTx (STObject&& object) noexcept (false)
    : STObject (std::move (object))
{
    tx_type_ = safe_cast<TxType> (getFieldU16 (sfTransactionType));
applyTemplate (getTxFormat (tx_type_)->elements);  //可能扔
    tid_ = getHash(HashPrefix::transactionID);
}

STTx::STTx (SerialIter& sit) noexcept (false)
    : STObject (sfTransaction)
{
    int length = sit.getBytesLeft ();

    if ((length < txMinSizeBytes) || (length > txMaxSizeBytes))
        Throw<std::runtime_error> ("Transaction length invalid");

    if (set (sit))
        Throw<std::runtime_error> ("Transaction contains an object terminator");

    tx_type_ = safe_cast<TxType> (getFieldU16 (sfTransactionType));

applyTemplate (getTxFormat (tx_type_)->elements);  //可能扔
    tid_ = getHash(HashPrefix::transactionID);
}

STTx::STTx (
        TxType type,
        std::function<void(STObject&)> assembler)
    : STObject (sfTransaction)
{
    auto format = getTxFormat (type);

    set (format->elements);
    setFieldU16 (sfTransactionType, format->getType ());

    assembler (*this);

    tx_type_ = safe_cast<TxType>(getFieldU16 (sfTransactionType));

    if (tx_type_ != type)
        LogicError ("Transaction type was mutated during assembly");

    tid_ = getHash(HashPrefix::transactionID);
}

std::string
STTx::getFullText () const
{
    std::string ret = "\"";
    ret += to_string (getTransactionID ());
    ret += "\" = {";
    ret += STObject::getFullText ();
    ret += "}";
    return ret;
}

boost::container::flat_set<AccountID>
STTx::getMentionedAccounts () const
{
    boost::container::flat_set<AccountID> list;

    for (auto const& it : *this)
    {
        if (auto sa = dynamic_cast<STAccount const*> (&it))
        {
            assert(! sa->isDefault());
            if (! sa->isDefault())
                list.insert(sa->value());
        }
        else if (auto sa = dynamic_cast<STAmount const*> (&it))
        {
            auto const& issuer = sa->getIssuer ();
            if (! isXRP (issuer))
                list.insert(issuer);
        }
    }

    return list;
}

static Blob getSigningData (STTx const& that)
{
    Serializer s;
    s.add32 (HashPrefix::txSign);
    that.addWithoutSigningFields (s);
    return s.getData();
}

uint256
STTx::getSigningHash () const
{
    return STObject::getSigningHash (HashPrefix::txSign);
}

Blob STTx::getSignature () const
{
    try
    {
        return getFieldVL (sfTxnSignature);
    }
    catch (std::exception const&)
    {
        return Blob ();
    }
}

void STTx::sign (
    PublicKey const& publicKey,
    SecretKey const& secretKey)
{
    auto const data = getSigningData (*this);

    auto const sig = ripple::sign (
        publicKey,
        secretKey,
        makeSlice(data));

    setFieldVL (sfTxnSignature, sig);
    tid_ = getHash(HashPrefix::transactionID);
}

std::pair<bool, std::string> STTx::checkSign(bool allowMultiSign) const
{
    std::pair<bool, std::string> ret {false, ""};
    try
    {
        if (allowMultiSign)
        {
//通过查看确定我们是单签名还是多签名
//在签名处。它是空的，我们一定是
//多重签署。否则我们是单笔签字。
            Blob const& signingPubKey = getFieldVL (sfSigningPubKey);
            ret = signingPubKey.empty () ?
                checkMultiSign () : checkSingleSign ();
        }
        else
        {
            ret = checkSingleSign ();
        }
    }
    catch (std::exception const&)
    {
        ret = {false, "Internal signature check failure."};
    }
    return ret;
}

Json::Value STTx::getJson (int) const
{
    Json::Value ret = STObject::getJson (0);
    ret[jss::hash] = to_string (getTransactionID ());
    return ret;
}

Json::Value STTx::getJson (int options, bool binary) const
{
    if (binary)
    {
        Json::Value ret;
        Serializer s = STObject::getSerializer ();
        ret[jss::tx] = strHex (s.peekData ());
        ret[jss::hash] = to_string (getTransactionID ());
        return ret;
    }
    return getJson(options);
}

std::string const&
STTx::getMetaSQLInsertReplaceHeader ()
{
    static std::string const sql = "INSERT OR REPLACE INTO Transactions "
        "(TransID, TransType, FromAcct, FromSeq, LedgerSeq, Status, RawTxn, TxnMeta)"
        " VALUES ";

    return sql;
}

std::string STTx::getMetaSQL (std::uint32_t inLedger,
                                               std::string const& escapedMetaData) const
{
    Serializer s;
    add (s);
    return getMetaSQL (s, inLedger, txnSqlValidated, escapedMetaData);
}

//vfalc这可能是其他地方的免费功能
std::string
STTx::getMetaSQL (Serializer rawTxn,
    std::uint32_t inLedger, char status, std::string const& escapedMetaData) const
{
    static boost::format bfTrans ("('%s', '%s', '%s', '%d', '%d', '%c', %s, %s)");
    std::string rTxn = sqlEscape (rawTxn.peekData ());

    auto format = TxFormats::getInstance().findByType (tx_type_);
    assert (format != nullptr);

    return str (boost::format (bfTrans)
                % to_string (getTransactionID ()) % format->getName ()
                % toBase58(getAccountID(sfAccount))
                % getSequence () % inLedger % status % rTxn % escapedMetaData);
}

std::pair<bool, std::string> STTx::checkSingleSign () const
{
//我们不允许非空的sfsigningpubkey和sfsigner。
//这将允许以两种方式签署事务。所以如果两者都
//字段存在。签名无效。
    if (isFieldPresent (sfSigners))
        return {false, "Cannot both single- and multi-sign."};

    bool validSig = false;
    try
    {
        bool const fullyCanonical = (getFlags() & tfFullyCanonicalSig);
        auto const spk = getFieldVL (sfSigningPubKey);

        if (publicKeyType (makeSlice(spk)))
        {
            Blob const signature = getFieldVL (sfTxnSignature);
            Blob const data = getSigningData (*this);

            validSig = verify (
                PublicKey (makeSlice(spk)),
                makeSlice(data),
                makeSlice(signature),
                fullyCanonical);
        }
    }
    catch (std::exception const&)
    {
//假设是签名失败。
        validSig = false;
    }
    if (validSig == false)
        return {false, "Invalid signature."};

    return {true, ""};
}

std::pair<bool, std::string> STTx::checkMultiSign () const
{
//确保有多个点火器。否则他们不是
//尝试多重签名，但我们的SigningPubKey不正确。
    if (!isFieldPresent (sfSigners))
        return {false, "Empty SigningPubKey."};

//我们不允许同时使用sfsigner和sftxsignature。两领域
//如果存在，则表示事务是双向签名的。
    if (isFieldPresent (sfTxnSignature))
        return {false, "Cannot both single- and multi-sign."};

    STArray const& signers {getFieldArray (sfSigners)};

//签名者的数量必须在已知的范围内。
    if (signers.size() < minMultiSigners || signers.size() > maxMultiSigners)
        return {false, "Invalid Signers array size."};

//我们可以通过
//预先构造我们散列的部分数据。填充序列化程序
//从签名到签名都保持不变。
    Serializer const dataStart {startMultiSigningData (*this)};

//我们还在循环中使用sfaccount字段。得到一次。
    auto const txnAccountID = getAccountID (sfAccount);

//确定签名是否必须是完全规范的。
    bool const fullyCanonical = (getFlags() & tfFullyCanonicalSig);

//签名者必须按accountID排序。
    AccountID lastAccountID (beast::zero);

    for (auto const& signer : signers)
    {
        auto const accountID = signer.getAccountID (sfAccount);

//帐户所有者不能为自己进行多重签名。
        if (accountID == txnAccountID)
            return {false, "Invalid multisigner."};

//不允许重复的签名者。
        if (lastAccountID == accountID)
            return {false, "Duplicate Signers not allowed."};

//帐户必须按帐户ID排列。不允许重复。
        if (lastAccountID > accountID)
            return {false, "Unsorted Signers array."};

//下一个签名必须大于此签名。
        lastAccountID = accountID;

//验证签名。
        bool validSig = false;
        try
        {
            Serializer s = dataStart;
            finishMultiSigningData (accountID, s);

            auto spk = signer.getFieldVL (sfSigningPubKey);

            if (publicKeyType (makeSlice(spk)))
            {
                Blob const signature =
                    signer.getFieldVL (sfTxnSignature);

                validSig = verify (
                    PublicKey (makeSlice(spk)),
                    s.slice(),
                    makeSlice(signature),
                    fullyCanonical);
            }
        }
        catch (std::exception const&)
        {
//我们假设任何问题都与签名有关。
            validSig = false;
        }
        if (!validSig)
            return {false, std::string("Invalid signature on account ") +
                toBase58(accountID)  + "."};
    }

//所有签名均已验证。
    return {true, ""};
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

static
bool
isMemoOkay (STObject const& st, std::string& reason)
{
    if (!st.isFieldPresent (sfMemos))
        return true;

    auto const& memos = st.getFieldArray (sfMemos);

//2048号是预分配提示，不是硬限制
//避免分配/复制/免费
    Serializer s (2048);
    memos.add (s);

//Fixme将备忘录限制移动到可调配置中
    if (s.getDataLength () > 1024)
    {
        reason = "The memo exceeds the maximum allowed size.";
        return false;
    }

    for (auto const& memo : memos)
    {
        auto memoObj = dynamic_cast <STObject const*> (&memo);

        if (!memoObj || (memoObj->getFName() != sfMemo))
        {
            reason = "A memo array may contain only Memo objects.";
            return false;
        }

        for (auto const& memoElement : *memoObj)
        {
            auto const& name = memoElement.getFName();

            if (name != sfMemoType &&
                name != sfMemoData &&
                name != sfMemoFormat)
            {
                reason = "A memo may contain only MemoType, MemoData or "
                         "MemoFormat fields.";
                return false;
            }

//原始数据存储为十六进制八位字节，我们希望对其进行解码。
            auto data = strUnHex (memoElement.getText ());

            if (!data.second)
            {
                reason = "The MemoType, MemoData and MemoFormat fields may "
                         "only contain hex-encoded data.";
                return false;
            }

            if (name == sfMemoData)
                continue;

//memotype和memoformat只允许使用以下字符：
//根据RFC 3986，URL中允许的字符：字母数字和
//以下符号：-.\：/？[][]！$（&）*+；
            static std::array<char, 256> const allowedSymbols = []
            {
                std::array<char, 256> a;
                a.fill(0);

                std::string symbols (
                    "0123456789"
                    "-._~:/?#[]@!$&'()*+,;=%"
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz");

                for(char c : symbols)
                    a[c] = 1;
                return a;
            }();

            for (auto c : data.first)
            {
                if (!allowedSymbols[c])
                {
                    reason = "The MemoType and MemoFormat fields may only "
                             "contain characters that are allowed in URLs "
                             "under RFC 3986.";
                    return false;
                }
            }
        }
    }

    return true;
}

//确保所有帐户字段都是160位
static
bool
isAccountFieldOkay (STObject const& st)
{
    for (int i = 0; i < st.getCount(); ++i)
    {
        auto t = dynamic_cast<STAccount const*>(st.peekAtPIndex (i));
        if (t && t->isDefault ())
            return false;
    }

    return true;
}

bool passesLocalChecks (STObject const& st, std::string& reason)
{
    if (!isMemoOkay (st, reason))
        return false;

    if (!isAccountFieldOkay (st))
    {
        reason = "An account field is invalid.";
        return false;
    }

    if (isPseudoTx(st))
    {
        reason = "Cannot submit pseudo transactions.";
        return false;
    }
    return true;
}

std::shared_ptr<STTx const>
sterilize (STTx const& stx)
{
    Serializer s;
    stx.add(s);
    SerialIter sit(s.slice());
    return std::make_shared<STTx const>(std::ref(sit));
}

bool
isPseudoTx(STObject const& tx)
{
    auto t = tx[~sfTransactionType];
    if (!t)
        return false;
    auto tt = safe_cast<TxType>(*t);
    return tt == ttAMENDMENT || tt == ttFEE;
}

} //涟漪
