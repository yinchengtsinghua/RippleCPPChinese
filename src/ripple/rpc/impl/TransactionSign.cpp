
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
    版权所有（c）2012-2014 Ripple Labs Inc.

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

#include <ripple/rpc/impl/TransactionSign.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/app/paths/Pathfinder.h>
#include <ripple/app/tx/apply.h>              //有效期：有效
#include <ripple/basics/Log.h>
#include <ripple/basics/mulDiv.h>
#include <ripple/json/json_writer.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/Sign.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/protocol/STParsedJSON.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/rpc/impl/LegacyPathFind.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/impl/Tuning.h>
#include <algorithm>
#include <iterator>

namespace ripple {
namespace RPC {
namespace detail {

//用于传递返回时使用的额外参数
//对象的签名。
class SigningForParams
{
private:
    AccountID const* const multiSigningAcctID_;
    PublicKey* const multiSignPublicKey_;
    Buffer* const multiSignature_;

public:
    explicit SigningForParams ()
    : multiSigningAcctID_ (nullptr)
    , multiSignPublicKey_ (nullptr)
    , multiSignature_ (nullptr)
    { }

    SigningForParams (SigningForParams const& rhs) = delete;

    SigningForParams (
        AccountID const& multiSigningAcctID,
        PublicKey& multiSignPublicKey,
        Buffer& multiSignature)
    : multiSigningAcctID_ (&multiSigningAcctID)
    , multiSignPublicKey_ (&multiSignPublicKey)
    , multiSignature_ (&multiSignature)
    { }

    bool isMultiSigning () const
    {
        return ((multiSigningAcctID_ != nullptr) &&
                (multiSignPublicKey_ != nullptr) &&
                (multiSignature_ != nullptr));
    }

//当多重签名时，我们不应该编辑tx_json字段。
    bool editFields () const
    {
        return !isMultiSigning();
    }

//除非ismultisign（）返回true，否则不要调用此方法。
    AccountID const& getSigner ()
    {
        return *multiSigningAcctID_;
    }

    void setPublicKey (PublicKey const& multiSignPublicKey)
    {
        *multiSignPublicKey_ = multiSignPublicKey;
    }

    void moveMultiSignature (Buffer&& multiSignature)
    {
        *multiSignature_ = std::move (multiSignature);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

static error_code_i acctMatchesPubKey (
    std::shared_ptr<SLE const> accountState,
    AccountID const& accountID,
    PublicKey const& publicKey)
{
    auto const publicKeyAcctID = calcAccountID(publicKey);
    bool const isMasterKey = publicKeyAcctID == accountID;

//如果我们找不到accountroot，但accountID匹配，那么
//够好了。
    if (!accountState)
    {
        if (isMasterKey)
            return rpcSUCCESS;
        return rpcBAD_SECRET;
    }

//如果我们*能*到accountroot，检查master_是否禁用。
    auto const& sle = *accountState;
    if (isMasterKey)
    {
        if (sle.isFlag(lsfDisableMaster))
            return rpcMASTER_DISABLED;
        return rpcSUCCESS;
    }

//最后的喘息是我们有公共的普通钥匙。
    if ((sle.isFieldPresent (sfRegularKey)) &&
        (publicKeyAcctID == sle.getAccountID (sfRegularKey)))
    {
        return rpcSUCCESS;
    }
    return rpcBAD_SECRET;
}

static Json::Value checkPayment(
    Json::Value const& params,
    Json::Value& tx_json,
    AccountID const& srcAddressID,
    Role const role,
    Application& app,
    std::shared_ptr<ReadView const> const& ledger,
    bool doPath)
{
//只有付款路径。
    if (tx_json[jss::TransactionType].asString () != "Payment")
        return Json::Value();

    if (!tx_json.isMember (jss::Amount))
        return RPC::missing_field_error ("tx_json.Amount");

    STAmount amount;

    if (! amountFromJsonNoThrow (amount, tx_json [jss::Amount]))
        return RPC::invalid_field_error ("tx_json.Amount");

    if (!tx_json.isMember (jss::Destination))
        return RPC::missing_field_error ("tx_json.Destination");

    auto const dstAccountID = parseBase58<AccountID>(
        tx_json[jss::Destination].asString());
    if (! dstAccountID)
        return RPC::invalid_field_error ("tx_json.Destination");

    if ((doPath == false) && params.isMember (jss::build_path))
        return RPC::make_error (rpcINVALID_PARAMS,
            "Field 'build_path' not allowed in this context.");

    if (tx_json.isMember (jss::Paths) && params.isMember (jss::build_path))
        return RPC::make_error (rpcINVALID_PARAMS,
            "Cannot specify both 'tx_json.Paths' and 'build_path'");

    if (!tx_json.isMember (jss::Paths) && params.isMember (jss::build_path))
    {
        STAmount    sendMax;

        if (tx_json.isMember (jss::SendMax))
        {
            if (! amountFromJsonNoThrow (sendMax, tx_json [jss::SendMax]))
                return RPC::invalid_field_error ("tx_json.SendMax");
        }
        else
        {
//如果没有sendmax，则默认为发送方为发卡方的金额。
            sendMax = amount;
            sendMax.setIssuer (srcAddressID);
        }

        if (sendMax.native () && amount.native ())
            return RPC::make_error (rpcINVALID_PARAMS,
                "Cannot build XRP to XRP paths.");

        {
            LegacyPathFind lpf (isUnlimited (role), app);
            if (!lpf.isOk ())
                return rpcError (rpcTOO_BUSY);

            STPathSet result;
            if (ledger)
            {
                Pathfinder pf(std::make_shared<RippleLineCache>(ledger),
                    srcAddressID, *dstAccountID, sendMax.issue().currency,
                        sendMax.issue().account, amount, boost::none, app);
                if (pf.findPaths(app.config().PATH_SEARCH_OLD))
                {
//4是最大路径
                    pf.computePathRanks(4);
                    STPath fullLiquidityPath;
                    STPathSet paths;
                    result = pf.getBestPaths(4, fullLiquidityPath, paths,
                        sendMax.issue().account);
                }
            }

            auto j = app.journal ("RPCHandler");
            JLOG (j.debug())
                << "transactionSign: build_path: "
                << result.getJson (0);

            if (! result.empty ())
                tx_json[jss::Paths] = result.getJson (0);
        }
    }
    return Json::Value();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//验证（但不要修改）txjson的内容。
//
//返回一对<json:：value，accountID>。json：：值将包含错误
//信息，如果有错误。成功后，返回帐户ID
//json：：值将为空。
//
//此代码不检查“sequence”字段，因为
//因为该字段对上下文特别敏感。
static std::pair<Json::Value, AccountID>
checkTxJsonFields (
    Json::Value const& tx_json,
    Role const role,
    bool const verify,
    std::chrono::seconds validatedLedgerAge,
    Config const& config,
    LoadFeeTrack const& feeTrack)
{
    std::pair<Json::Value, AccountID> ret;

    if (!tx_json.isObject())
    {
        ret.first = RPC::object_field_error (jss::tx_json);
        return ret;
    }

    if (! tx_json.isMember (jss::TransactionType))
    {
        ret.first = RPC::missing_field_error ("tx_json.TransactionType");
        return ret;
    }

    if (! tx_json.isMember (jss::Account))
    {
        ret.first = RPC::make_error (rpcSRC_ACT_MISSING,
            RPC::missing_field_message ("tx_json.Account"));
        return ret;
    }

    auto const srcAddressID = parseBase58<AccountID>(
        tx_json[jss::Account].asString());

    if (! srcAddressID)
    {
        ret.first = RPC::make_error (rpcSRC_ACT_MALFORMED,
            RPC::invalid_field_message ("tx_json.Account"));
        return ret;
    }

//检查当前分类帐。
    if (verify && !config.standalone() &&
        (validatedLedgerAge > Tuning::maxValidatedLedgerAge))
    {
        ret.first = rpcError (rpcNO_CURRENT);
        return ret;
    }

//检查是否有负载。
    if (feeTrack.isLoadedCluster() && !isUnlimited (role))
    {
        ret.first = rpcError (rpcTOO_BUSY);
        return ret;
    }

//一切都很好。返回accountID。
    ret.second = *srcAddressID;
    return ret;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//一个只移动的结构，使返回json:：value或
//std:：shared_ptr<sttx const>from transactionPreprocessImpl（）。
struct transactionPreProcessResult
{
    Json::Value const first;
    std::shared_ptr<STTx> const second;

    transactionPreProcessResult () = delete;
    transactionPreProcessResult (transactionPreProcessResult const&) = delete;
    transactionPreProcessResult (transactionPreProcessResult&& rhs)
: first (std::move (rhs.first))    //VS2013不会默认这一点。
    , second (std::move (rhs.second))
    { }

    transactionPreProcessResult& operator=
        (transactionPreProcessResult const&) = delete;
     transactionPreProcessResult& operator=
        (transactionPreProcessResult&&) = delete;

    transactionPreProcessResult (Json::Value&& json)
    : first (std::move (json))
    , second ()
    { }

    explicit transactionPreProcessResult (std::shared_ptr<STTx>&& st)
    : first ()
    , second (std::move (st))
    { }
};

static
transactionPreProcessResult
transactionPreProcessImpl (
    Json::Value& params,
    Role role,
    SigningForParams& signingArgs,
    std::chrono::seconds validatedLedgerAge,
    Application& app,
    std::shared_ptr<OpenView const> const& ledger)
{
    auto j = app.journal ("RPCHandler");

    Json::Value jvResult;
    auto const keypair = keypairForSignature (params, jvResult);
    if (contains_error (jvResult))
        return std::move (jvResult);

    bool const verify = !(params.isMember (jss::offline)
                          && params[jss::offline].asBool());

    if (! params.isMember (jss::tx_json))
        return RPC::missing_field_error (jss::tx_json);

    Json::Value& tx_json (params [jss::tx_json]);

//检查tx_json字段，但不要添加任何字段。
    auto txJsonResult = checkTxJsonFields (
        tx_json, role, verify, validatedLedgerAge,
        app.config(), app.getFeeTrack());

    if (RPC::contains_error (txJsonResult.first))
        return std::move (txJsonResult.first);

    auto const srcAddressID = txJsonResult.second;

//这个测试覆盖了我们离线的情况，所以序列号
//无法在本地确定。如果我们处于离线状态，则呼叫者必须
//提供序列号。
    if (!verify && !tx_json.isMember (jss::Sequence))
        return RPC::missing_field_error ("tx_json.Sequence");

    std::shared_ptr<SLE const> sle = ledger->read(
            keylet::account(srcAddressID));

    if (verify && !sle)
    {
//如果没有脱机并且找不到帐户，则出错。
        JLOG (j.debug())
            << "transactionSign: Failed to find source account "
            << "in current ledger: "
            << toBase58(srcAddressID);

        return rpcError (rpcSRC_ACT_NOT_FOUND);
    }

    {
        Json::Value err = checkFee (
            params,
            role,
            verify && signingArgs.editFields(),
            app.config(),
            app.getFeeTrack(),
            app.getTxQ(),
            ledger);

        if (RPC::contains_error (err))
            return std::move (err);

        err = checkPayment (
            params,
            tx_json,
            srcAddressID,
            role,
            app,
            ledger,
            verify && signingArgs.editFields());

        if (RPC::contains_error(err))
            return std::move (err);
    }

    if (signingArgs.editFields())
    {
        if (!tx_json.isMember (jss::Sequence))
        {
            if (! sle)
            {
                JLOG (j.debug())
                << "transactionSign: Failed to find source account "
                << "in current ledger: "
                << toBase58(srcAddressID);

                return rpcError (rpcSRC_ACT_NOT_FOUND);
            }

            auto seq = (*sle)[sfSequence];
            auto const queued = app.getTxQ().getAccountTxs(srcAddressID,
                *ledger);
//如果帐户在txq中有任何txs，跳过这些序列
//数字（考虑可能的差距）。
            for(auto const& tx : queued)
            {
                if (tx.first == seq)
                    ++seq;
                else if (tx.first > seq)
                    break;
            }
            tx_json[jss::Sequence] = seq;
        }

        if (!tx_json.isMember (jss::Flags))
            tx_json[jss::Flags] = tfFullyCanonicalSig;
    }

//如果是多重点火，那么我们需要返回公钥。
    if (signingArgs.isMultiSigning())
        signingArgs.setPublicKey (keypair.first);

    if (verify)
    {
        if (! sle)
//XXX忽略未创建账户的交易。
            return rpcError (rpcSRC_ACT_NOT_FOUND);

        JLOG (j.trace())
            << "verify: " << toBase58(calcAccountID(keypair.first))
            << " : " << toBase58(srcAddressID);

//如果帐户和机密有多个签名，则不要执行此测试
//在那种情况下可能不属于一起。
        if (!signingArgs.isMultiSigning())
        {
//确保帐户和秘密属于一起。
            auto const err = acctMatchesPubKey (
                sle, srcAddressID, keypair.first);

            if (err != rpcSUCCESS)
                return rpcError (err);
        }
    }

    STParsedJSONObject parsed (std::string (jss::tx_json), tx_json);
    if (parsed.object == boost::none)
    {
        Json::Value err;
        err [jss::error] = parsed.error [jss::error];
        err [jss::error_code] = parsed.error [jss::error_code];
        err [jss::error_message] = parsed.error [jss::error_message];
        return std::move (err);
    }

    std::shared_ptr<STTx> stpTrans;
    try
    {
//如果要生成多签名，signingpubkey必须
//为空，否则它必须是主帐户的公钥。
        parsed.object->setFieldVL (sfSigningPubKey,
            signingArgs.isMultiSigning()
                ? Slice (nullptr, 0)
                : keypair.first.slice());

        stpTrans = std::make_shared<STTx> (
            std::move (parsed.object.get()));
    }
    catch (STObject::FieldErr& err)
    {
        return RPC::make_error (rpcINVALID_PARAMS, err.what());
    }
    catch (std::exception&)
    {
        return RPC::make_error (rpcINTERNAL,
            "Exception occurred constructing serialized transaction");
    }

    std::string reason;
    if (!passesLocalChecks (*stpTrans, reason))
        return RPC::make_error (rpcINVALID_PARAMS, reason);

//如果是multisign，则返回multisignature，否则设置txsignature字段。
    if (signingArgs.isMultiSigning ())
    {
        Serializer s = buildMultiSigningData (*stpTrans,
            signingArgs.getSigner ());

        auto multisig = ripple::sign (
            keypair.first,
            keypair.second,
            s.slice());

        signingArgs.moveMultiSignature (std::move (multisig));
    }
    else
    {
        stpTrans->sign (keypair.first, keypair.second);
    }

    return transactionPreProcessResult {std::move (stpTrans)};
}

static
std::pair <Json::Value, Transaction::pointer>
transactionConstructImpl (std::shared_ptr<STTx const> const& stpTrans,
    Rules const& rules, Application& app)
{
    std::pair <Json::Value, Transaction::pointer> ret;

//将传入的sttx转换为事务。
    Transaction::pointer tpTrans;
    {
        std::string reason;
        tpTrans = std::make_shared<Transaction>(
            stpTrans, reason, app);
        if (tpTrans->getStatus () != NEW)
        {
            ret.first = RPC::make_error (rpcINTERNAL,
                "Unable to construct transaction: " + reason);
            return ret;
        }
    }
    try
    {
//确保我们刚建立的事务通过序列化合法化
//然后将其反序列化。如果结果不相等
//对于初始事务，那么
//在STTx通过。
        {
            Serializer s;
            tpTrans->getSTransaction ()->add (s);
            Blob transBlob = s.getData ();
            SerialIter sit {makeSlice(transBlob)};

//如果需要，请检查签名。
            auto sttxNew = std::make_shared<STTx const> (sit);
            if (!app.checkSigs())
                forceValidity(app.getHashRouter(),
                    sttxNew->getTransactionID(), Validity::SigGoodOnly);
            if (checkValidity(app.getHashRouter(),
                *sttxNew, rules, app.config()).first != Validity::Valid)
            {
                ret.first = RPC::make_error (rpcINTERNAL,
                    "Invalid signature.");
                return ret;
            }

            std::string reason;
            auto tpTransNew =
                std::make_shared<Transaction> (sttxNew, reason, app);

            if (tpTransNew)
            {
                if (!tpTransNew->getSTransaction()->isEquivalent (
                        *tpTrans->getSTransaction()))
                {
                    tpTransNew.reset ();
                }
                tpTrans = std::move (tpTransNew);
            }
        }
    }
    catch (std::exception&)
    {
//假设任何异常都与交易灭菌有关。
        tpTrans.reset ();
    }

    if (!tpTrans)
    {
        ret.first = RPC::make_error (rpcINTERNAL,
            "Unable to sterilize transaction.");
        return ret;
    }
    ret.second = std::move (tpTrans);
    return ret;
}

static Json::Value transactionFormatResultImpl (Transaction::pointer tpTrans)
{
    Json::Value jvResult;
    try
    {
        jvResult[jss::tx_json] = tpTrans->getJson (0);
        jvResult[jss::tx_blob] = strHex (
            tpTrans->getSTransaction ()->getSerializer ().peekData ());

        if (temUNCERTAIN != tpTrans->getResult ())
        {
            std::string sToken;
            std::string sHuman;

            transResultInfo (tpTrans->getResult (), sToken, sHuman);

            jvResult[jss::engine_result]           = sToken;
            jvResult[jss::engine_result_code]      = tpTrans->getResult ();
            jvResult[jss::engine_result_message]   = sHuman;
        }
    }
    catch (std::exception&)
    {
        jvResult = RPC::make_error (rpcINTERNAL,
            "Exception occurred during JSON handling.");
    }
    return jvResult;
}

} //细节

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Json::Value checkFee (
    Json::Value& request,
    Role const role,
    bool doAutoFill,
    Config const& config,
    LoadFeeTrack const& feeTrack,
    TxQ const& txQ,
    std::shared_ptr<OpenView const> const& ledger)
{
    Json::Value& tx (request[jss::tx_json]);
    if (tx.isMember (jss::Fee))
        return Json::Value();

    if (! doAutoFill)
        return RPC::missing_field_error ("tx_json.Fee");

    int mult = Tuning::defaultAutoFillFeeMultiplier;
    int div = Tuning::defaultAutoFillFeeDivisor;
    if (request.isMember (jss::fee_mult_max))
    {
        if (request[jss::fee_mult_max].isInt())
        {
            mult = request[jss::fee_mult_max].asInt();
            if (mult < 0)
                return RPC::make_error(rpcINVALID_PARAMS,
                    RPC::expected_field_message(jss::fee_mult_max,
                        "a positive integer"));
        }
        else
        {
            return RPC::make_error (rpcHIGH_FEE,
                RPC::expected_field_message (jss::fee_mult_max,
                    "a positive integer"));
        }
    }
    if (request.isMember(jss::fee_div_max))
    {
        if (request[jss::fee_div_max].isInt())
        {
            div = request[jss::fee_div_max].asInt();
            if (div <= 0)
                return RPC::make_error(rpcINVALID_PARAMS,
                    RPC::expected_field_message(jss::fee_div_max,
                        "a positive integer"));
        }
        else
        {
            return RPC::make_error(rpcHIGH_FEE,
                RPC::expected_field_message(jss::fee_div_max,
                    "a positive integer"));
        }
    }

//以费用单位表示的默认费用。
    std::uint64_t const feeDefault = config.TRANSACTION_FEE_BASE;

//行政和已确定的端点免收当地费用。
    std::uint64_t const loadFee =
        scaleFeeLoad (feeDefault, feeTrack,
            ledger->fees(), isUnlimited (role));
    std::uint64_t fee = loadFee;
    {
        auto const metrics = txQ.getMetrics(*ledger);
        auto const baseFee = ledger->fees().base;
        auto escalatedFee = mulDiv(
            metrics.openLedgerFeeLevel, baseFee,
                metrics.referenceFeeLevel).second;
        if (mulDiv(escalatedFee, metrics.referenceFeeLevel,
                baseFee).second < metrics.openLedgerFeeLevel)
            ++escalatedFee;
        fee = std::max(fee, escalatedFee);
    }

    auto const limit = [&]()
    {
//将费用单位缩放为下降：
        auto const drops = mulDiv (feeDefault,
            ledger->fees().base, ledger->fees().units);
        if (!drops.first)
            Throw<std::overflow_error>("mulDiv");
        auto const result = mulDiv (drops.second, mult, div);
        if (!result.first)
            Throw<std::overflow_error>("mulDiv");
        return result.second;
    }();

    if (fee > limit)
    {
        std::stringstream ss;
        ss << "Fee of " << fee
            << " exceeds the requested tx limit of " << limit;
        return RPC::make_error (rpcHIGH_FEE, ss.str());
    }

    tx [jss::Fee] = static_cast<unsigned int>(fee);
    return Json::Value();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*返回json:：objectValue。*/
Json::Value transactionSign (
    Json::Value jvRequest,
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app)
{
    using namespace detail;

    auto const& ledger = app.openLedger().current();
    auto j = app.journal ("RPCHandler");
    JLOG (j.debug()) << "transactionSign: " << jvRequest;

//根据交易类型添加和修改字段。
    SigningForParams signForParams;
    transactionPreProcessResult preprocResult = transactionPreProcessImpl (
        jvRequest, role, signForParams,
        validatedLedgerAge, app, ledger);

    if (!preprocResult.second)
        return preprocResult.first;

//确保STTX进行合法交易。
    std::pair <Json::Value, Transaction::pointer> txn =
        transactionConstructImpl (
            preprocResult.second, ledger->rules(), app);

    if (!txn.second)
        return txn.first;

    return transactionFormatResultImpl (txn.second);
}

/*返回json:：objectValue。*/
Json::Value transactionSubmit (
    Json::Value jvRequest,
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app,
    ProcessTransactionFn const& processTransaction)
{
    using namespace detail;

    auto const& ledger = app.openLedger().current();
    auto j = app.journal ("RPCHandler");
    JLOG (j.debug()) << "transactionSubmit: " << jvRequest;


//根据交易类型添加和修改字段。
    SigningForParams signForParams;
    transactionPreProcessResult preprocResult = transactionPreProcessImpl (
        jvRequest, role, signForParams, validatedLedgerAge, app, ledger);

    if (!preprocResult.second)
        return preprocResult.first;

//确保STTX进行合法交易。
    std::pair <Json::Value, Transaction::pointer> txn =
        transactionConstructImpl (
            preprocResult.second, ledger->rules(), app);

    if (!txn.second)
        return txn.first;

//最后，提交事务。
    try
    {
//Fixme:为了提高性能，应该使用异步接口
        processTransaction (
            txn.second, isUnlimited (role), true, failType);
    }
    catch (std::exception&)
    {
        return RPC::make_error (rpcINTERNAL,
            "Exception occurred during transaction submission.");
    }

    return transactionFormatResultImpl (txn.second);
}

namespace detail
{
//Transactionsignfor共享了一些字段检查
//和交易提交多个签名。把他们聚集在这里。
static Json::Value checkMultiSignFields (Json::Value const& jvRequest)
{
   if (! jvRequest.isMember (jss::tx_json))
        return RPC::missing_field_error (jss::tx_json);

    Json::Value const& tx_json (jvRequest [jss::tx_json]);

    if (!tx_json.isObject())
        return RPC::invalid_field_message (jss::tx_json);

//我们之前需要检查一些附加字段
//我们连载。如果我们先序列化，然后生成不太有用的
//错误消息。
    if (!tx_json.isMember (jss::Sequence))
        return RPC::missing_field_error ("tx_json.Sequence");

    if (!tx_json.isMember (sfSigningPubKey.getJsonName()))
        return RPC::missing_field_error ("tx_json.SigningPubKey");

    if (!tx_json[sfSigningPubKey.getJsonName()].asString().empty())
        return RPC::make_error (rpcINVALID_PARAMS,
            "When multi-signing 'tx_json.SigningPubKey' must be empty.");

    return Json::Value ();
}

//排序并验证stsigners数组。
//
//如果没有错误，则返回空的json：：值。
static Json::Value sortAndValidateSigners (
    STArray& signers, AccountID const& signingForID)
{
    if (signers.empty ())
        return RPC::make_param_error ("Signers array may not be empty.");

//签名者必须按帐户排序。
    std::sort (signers.begin(), signers.end(),
        [](STObject const& a, STObject const& b)
    {
        return (a[sfAccount] < b[sfAccount]);
    });

//签名者不能包含任何重复项。
    auto const dupIter = std::adjacent_find (
        signers.begin(), signers.end(),
        [] (STObject const& a, STObject const& b)
        {
            return (a[sfAccount] == b[sfAccount]);
        });

    if (dupIter != signers.end())
    {
        std::ostringstream err;
        err << "Duplicate Signers:Signer:Account entries ("
            << toBase58((*dupIter)[sfAccount])
            << ") are not allowed.";
        return RPC::make_param_error(err.str ());
    }

//帐户不能自行签名。
    if (signers.end() != std::find_if (signers.begin(), signers.end(),
        [&signingForID](STObject const& elem)
        {
            return elem[sfAccount] == signingForID;
        }))
    {
        std::ostringstream err;
        err << "A Signer may not be the transaction's Account ("
            << toBase58(signingForID) << ").";
        return RPC::make_param_error(err.str ());
    }
    return {};
}

} //细节

/*返回json:：objectValue。*/
Json::Value transactionSignFor (
    Json::Value jvRequest,
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app)
{
    auto const& ledger = app.openLedger().current();
    auto j = app.journal ("RPCHandler");
    JLOG (j.debug()) << "transactionSignFor: " << jvRequest;

//验证签名者的帐户字段是否存在。
    const char accountField[] = "account";

    if (! jvRequest.isMember (accountField))
        return RPC::missing_field_error (accountField);

//将签名者的帐户转换为用于多重签名的帐户ID。
    auto const signerAccountID = parseBase58<AccountID>(
        jvRequest[accountField].asString());
    if (! signerAccountID)
    {
        return RPC::make_error (rpcSRC_ACT_MALFORMED,
            RPC::invalid_field_message (accountField));
    }

   if (! jvRequest.isMember (jss::tx_json))
        return RPC::missing_field_error (jss::tx_json);

    {
        Json::Value& tx_json (jvRequest [jss::tx_json]);

        if (!tx_json.isObject())
            return RPC::object_field_error (jss::tx_json);

//如果缺少txjson.signingpubkey字段，
//插入一个空的。
        if (!tx_json.isMember (sfSigningPubKey.getJsonName()))
            tx_json[sfSigningPubKey.getJsonName()] = "";
    }

//多签名时，“序列”和“签名PubKey”字段必须
//由呼叫方传入。
    using namespace detail;
    {
        Json::Value err = checkMultiSignFields (jvRequest);
        if (RPC::contains_error (err))
            return err;
    }

//根据交易类型添加和修改字段。
    Buffer multiSignature;
    PublicKey multiSignPubKey;
    SigningForParams signForParams(
        *signerAccountID, multiSignPubKey, multiSignature);

    transactionPreProcessResult preprocResult = transactionPreProcessImpl (
        jvRequest,
        role,
        signForParams,
        validatedLedgerAge,
        app,
        ledger);

    if (!preprocResult.second)
        return preprocResult.first;

    {
        std::shared_ptr<SLE const> account_state = ledger->read(
                keylet::account(*signerAccountID));
//确保帐户和秘密属于一起。
        auto const err = acctMatchesPubKey (
            account_state,
            *signerAccountID,
            multiSignPubKey);

        if (err != rpcSUCCESS)
            return rpcError (err);
    }

//将新生成的签名注入txjson.signers。
    auto& sttx = preprocResult.second;
    {
//生成要注入的Signer对象。
        STObject signer (sfSigner);
        signer[sfAccount] = *signerAccountID;
        signer.setFieldVL (sfTxnSignature, multiSignature);
        signer.setFieldVL (sfSigningPubKey, multiSignPubKey.slice());

//如果还没有签名者数组，请创建一个。
        if (!sttx->isFieldPresent (sfSigners))
            sttx->setFieldArray (sfSigners, {});

        auto& signers = sttx->peekFieldArray (sfSigners);
        signers.emplace_back (std::move (signer));

//必须对数组进行排序和验证。
        auto err = sortAndValidateSigners (signers, (*sttx)[sfAccount]);
        if (RPC::contains_error (err))
            return err;
    }

//确保STTX进行合法交易。
    std::pair <Json::Value, Transaction::pointer> txn =
        transactionConstructImpl (sttx, ledger->rules(), app);

    if (!txn.second)
        return txn.first;

    return transactionFormatResultImpl (txn.second);
}

/*返回json:：objectValue。*/
Json::Value transactionSubmitMultiSigned (
    Json::Value jvRequest,
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app,
    ProcessTransactionFn const& processTransaction)
{
    auto const& ledger = app.openLedger().current();
    auto j = app.journal ("RPCHandler");
    JLOG (j.debug())
        << "transactionSubmitMultiSigned: " << jvRequest;

//多签名时，“序列”和“签名PubKey”字段必须
//由呼叫方传入。
    using namespace detail;
    {
        Json::Value err = checkMultiSignFields (jvRequest);
        if (RPC::contains_error (err))
            return err;
    }

    Json::Value& tx_json (jvRequest ["tx_json"]);

    auto const txJsonResult = checkTxJsonFields (
        tx_json, role, true, validatedLedgerAge,
        app.config(), app.getFeeTrack());

    if (RPC::contains_error (txJsonResult.first))
        return std::move (txJsonResult.first);

    auto const srcAddressID = txJsonResult.second;

    std::shared_ptr<SLE const> sle = ledger->read(
            keylet::account(srcAddressID));

    if (!sle)
    {
//如果找不到帐户，则出错。
        JLOG (j.debug())
            << "transactionSubmitMultiSigned: Failed to find source account "
            << "in current ledger: "
            << toBase58(srcAddressID);

        return rpcError (rpcSRC_ACT_NOT_FOUND);
    }

    {
        Json::Value err = checkFee (
            jvRequest, role, false, app.config(), app.getFeeTrack(),
                app.getTxQ(), ledger);

        if (RPC::contains_error(err))
            return err;

        err = checkPayment (
            jvRequest,
            tx_json,
            srcAddressID,
            role,
            app,
            ledger,
            false);

        if (RPC::contains_error(err))
            return err;
    }

//研磨Tx中的JSON，生成STTx。
    std::shared_ptr<STTx> stpTrans;
    {
        STParsedJSONObject parsedTx_json ("tx_json", tx_json);
        if (!parsedTx_json.object)
        {
            Json::Value jvResult;
            jvResult ["error"] = parsedTx_json.error ["error"];
            jvResult ["error_code"] = parsedTx_json.error ["error_code"];
            jvResult ["error_message"] = parsedTx_json.error ["error_message"];
            return jvResult;
        }
        try
        {
            stpTrans = std::make_shared<STTx>(
                std::move(parsedTx_json.object.get()));
        }
        catch (STObject::FieldErr& err)
        {
            return RPC::make_error (rpcINVALID_PARAMS, err.what());
        }
        catch (std::exception& ex)
        {
            std::string reason (ex.what ());
            return RPC::make_error (rpcINTERNAL,
                "Exception while serializing transaction: " + reason);
        }
        std::string reason;
        if (!passesLocalChecks (*stpTrans, reason))
            return RPC::make_error (rpcINVALID_PARAMS, reason);
    }

//验证序列化事务中的字段。
    {
//我们现在已经将事务文本序列化为正确的格式。
//验证选择字段的值。
//
//SigningPubKey必须存在，但为空。
        if (!stpTrans->getFieldVL (sfSigningPubKey).empty ())
        {
            std::ostringstream err;
            err << "Invalid  " << sfSigningPubKey.fieldName
                << " field.  Field must be empty when multi-signing.";
            return RPC::make_error (rpcINVALID_PARAMS, err.str ());
        }

//费用字段必须在xrp中且大于零。
        auto const fee = stpTrans->getFieldAmount (sfFee);

        if (!isLegalNet (fee))
        {
            std::ostringstream err;
            err << "Invalid " << sfFee.fieldName
                << " field.  Fees must be specified in XRP.";
            return RPC::make_error (rpcINVALID_PARAMS, err.str ());
        }
        if (fee <= 0)
        {
            std::ostringstream err;
            err << "Invalid " << sfFee.fieldName
                << " field.  Fees must be greater than zero.";
            return RPC::make_error (rpcINVALID_PARAMS, err.str ());
        }
    }

//验证签名者字段是否存在。
    if (! stpTrans->isFieldPresent (sfSigners))
        return RPC::missing_field_error ("tx_json.Signers");

//如果存在signers字段，则sfield保证它是一个数组。
//获取对签名者数组的引用，以便我们可以对其进行验证和排序。
    auto& signers = stpTrans->peekFieldArray (sfSigners);

    if (signers.empty ())
        return RPC::make_param_error("tx_json.Signers array may not be empty.");

//签名者数组只能包含签名者对象。
    if (std::find_if_not(signers.begin(), signers.end(), [](STObject const& obj)
        {
            return (
//签名者对象总是包含这些字段，而不包含其他字段。
                obj.isFieldPresent (sfAccount) &&
                obj.isFieldPresent (sfSigningPubKey) &&
                obj.isFieldPresent (sfTxnSignature) &&
                obj.getCount() == 3);
        }) != signers.end())
    {
        return RPC::make_param_error (
            "Signers array may only contain Signer entries.");
    }

//必须对数组进行排序和验证。
    auto err = sortAndValidateSigners (signers, srcAddressID);
    if (RPC::contains_error (err))
        return err;

//确保SerializedTransaction进行合法事务。
    std::pair <Json::Value, Transaction::pointer> txn =
        transactionConstructImpl (stpTrans, ledger->rules(), app);

    if (!txn.second)
        return txn.first;

//最后，提交事务。
    try
    {
//Fixme:为了提高性能，应该使用异步接口
        processTransaction (
            txn.second, isUnlimited (role), true, failType);
    }
    catch (std::exception&)
    {
        return RPC::make_error (rpcINTERNAL,
            "Exception occurred during transaction submission.");
    }

    return transactionFormatResultImpl (txn.second);
}

} //RPC
} //涟漪
