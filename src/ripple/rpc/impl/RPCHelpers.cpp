
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

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/ledger/View.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/Feature.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <boost/algorithm/string/case_conv.hpp>

namespace ripple {
namespace RPC {

boost::optional<AccountID>
accountFromStringStrict(std::string const& account)
{
    boost::optional <AccountID> result;

    auto const publicKey = parseBase58<PublicKey> (
        TokenType::AccountPublic,
        account);

    if (publicKey)
        result = calcAccountID (*publicKey);
    else
        result = parseBase58<AccountID> (account);

    return result;
}

Json::Value
accountFromString(
    AccountID& result, std::string const& strIdent, bool bStrict)
{
    if (auto accountID = accountFromStringStrict (strIdent))
    {
        result = *accountID;
        return Json::objectValue;
    }

    if (bStrict)
    {
        auto id = deprecatedParseBitcoinAccountID (strIdent);
        return rpcError (id ? rpcACT_BITCOIN : rpcACT_MALFORMED);
    }

//我们允许使用不好实践的种子
//只是为了调试方便。
    auto const seed = parseGenericSeed (strIdent);

    if (!seed)
        return rpcError (rpcBAD_SEED);

    auto const keypair = generateKeyPair (
        KeyType::secp256k1,
        *seed);

    result = calcAccountID (keypair.first);
    return Json::objectValue;
}

bool
getAccountObjects(ReadView const& ledger, AccountID const& account,
    LedgerEntryType const type, uint256 dirIndex, uint256 const& entryIndex,
    std::uint32_t const limit, Json::Value& jvResult)
{
    auto const rootDirIndex = getOwnerDirIndex (account);
    auto found = false;

    if (dirIndex.isZero ())
    {
        dirIndex = rootDirIndex;
        found = true;
    }

    auto dir = ledger.read({ltDIR_NODE, dirIndex});
    if (! dir)
        return false;

    std::uint32_t i = 0;
    auto& jvObjects = (jvResult[jss::account_objects] = Json::arrayValue);
    for (;;)
    {
        auto const& entries = dir->getFieldV256 (sfIndexes);
        auto iter = entries.begin ();

        if (! found)
        {
            iter = std::find (iter, entries.end (), entryIndex);
            if (iter == entries.end ())
                return false;

            found = true;
        }

        for (; iter != entries.end (); ++iter)
        {
            auto const sleNode = ledger.read(keylet::child(*iter));
            if (type == ltINVALID || sleNode->getType () == type)
            {
                jvObjects.append (sleNode->getJson (0));

                if (++i == limit)
                {
                    if (++iter != entries.end ())
                    {
                        jvResult[jss::limit] = limit;
                        jvResult[jss::marker] = to_string (dirIndex) + ',' +
                            to_string (*iter);
                        return true;
                    }

                    break;
                }
            }
        }

        auto const nodeIndex = dir->getFieldU64 (sfIndexNext);
        if (nodeIndex == 0)
            return true;

        dirIndex = getDirNodeIndex (rootDirIndex, nodeIndex);
        dir = ledger.read({ltDIR_NODE, dirIndex});
        if (! dir)
            return true;

        if (i == limit)
        {
            auto const& e = dir->getFieldV256 (sfIndexes);
            if (! e.empty ())
            {
                jvResult[jss::limit] = limit;
                jvResult[jss::marker] = to_string (dirIndex) + ',' +
                    to_string (*e.begin ());
            }

            return true;
        }
    }
}

namespace {

bool
isValidatedOld(LedgerMaster& ledgerMaster, bool standalone)
{
    if (standalone)
        return false;

    return ledgerMaster.getValidatedLedgerAge () >
        Tuning::maxValidatedLedgerAge;
}

template <class T>
Status
ledgerFromRequest(T& ledger, Context& context)
{
    static auto const minSequenceGap = 10;

    ledger.reset();

    auto& params = context.params;
    auto& ledgerMaster = context.ledgerMaster;

    auto indexValue = params[jss::ledger_index];
    auto hashValue = params[jss::ledger_hash];

//我们需要支持传统的“分类账”字段。
    auto& legacyLedger = params[jss::ledger];
    if (legacyLedger)
    {
        if (legacyLedger.asString().size () > 12)
            hashValue = legacyLedger;
        else
            indexValue = legacyLedger;
    }

    if (hashValue)
    {
        if (! hashValue.isString ())
            return {rpcINVALID_PARAMS, "ledgerHashNotString"};

        uint256 ledgerHash;
        if (! ledgerHash.SetHex (hashValue.asString ()))
            return {rpcINVALID_PARAMS, "ledgerHashMalformed"};

        ledger = ledgerMaster.getLedgerByHash (ledgerHash);
        if (ledger == nullptr)
            return {rpcLGR_NOT_FOUND, "ledgerNotFound"};
    }
    else if (indexValue.isNumeric())
    {
        ledger = ledgerMaster.getLedgerBySeq (indexValue.asInt ());

        if (ledger == nullptr)
        {
            auto cur = ledgerMaster.getCurrentLedger();
            if (cur->info().seq == indexValue.asInt())
                ledger = cur;
        }

        if (ledger == nullptr)
            return {rpcLGR_NOT_FOUND, "ledgerNotFound"};

        if (ledger->info().seq > ledgerMaster.getValidLedgerIndex() &&
            isValidatedOld(ledgerMaster, context.app.config().standalone()))
        {
            ledger.reset();
            return {rpcNO_NETWORK, "InsufficientNetworkMode"};
        }
    }
    else
    {
        if (isValidatedOld (ledgerMaster, context.app.config().standalone()))
            return {rpcNO_NETWORK, "InsufficientNetworkMode"};

        auto const index = indexValue.asString ();
        if (index == "validated")
        {
            ledger = ledgerMaster.getValidatedLedger ();
            if (ledger == nullptr)
                return {rpcNO_NETWORK, "InsufficientNetworkMode"};

            assert (! ledger->open());
        }
        else
        {
            if (index.empty () || index == "current")
            {
                ledger = ledgerMaster.getCurrentLedger ();
                assert (ledger->open());
            }
            else if (index == "closed")
            {
                ledger = ledgerMaster.getClosedLedger ();
                assert (! ledger->open());
            }
            else
            {
                return {rpcINVALID_PARAMS, "ledgerIndexMalformed"};
            }

            if (ledger == nullptr)
                return {rpcNO_NETWORK, "InsufficientNetworkMode"};

            if (ledger->info().seq + minSequenceGap <
                ledgerMaster.getValidLedgerIndex ())
            {
                ledger.reset ();
                return {rpcNO_NETWORK, "InsufficientNetworkMode"};
            }
        }
    }

    return Status::OK;
}

bool
isValidated(LedgerMaster& ledgerMaster, ReadView const& ledger,
    Application& app)
{
    if (ledger.open())
        return false;

    if (ledger.info().validated)
        return true;

    auto seq = ledger.info().seq;
    try
    {
//使用上次验证的分类帐中的跳过列表查看分类帐
//出现在最后一个已验证的分类账之前（因此
//验证。
        auto hash = ledgerMaster.walkHashBySeq (seq);

        if (!hash || ledger.info().hash != *hash)
        {
//此分类帐的哈希不是已验证分类帐的哈希
            if (hash)
            {
                assert(hash->isNonZero());
                uint256 valHash = getHashByIndex (seq, app);
                if (valHash == ledger.info().hash)
                {
//SQL数据库与分类帐链不匹配
                    ledgerMaster.clearLedger (seq);
                }
            }
            return false;
        }
    }
    catch (SHAMapMissingNode const&)
    {
        auto stream = app.journal ("RPCHandler").warn();
        JLOG (stream)
            << "Missing SHANode " << std::to_string (seq);
        return false;
    }

//将分类帐标记为“已验证”，以便在再次看到时节省时间。
    ledger.info().validated = true;
    return true;
}

} //命名空间

//lookupledger命令的早期版本将接受
//“ledger_index”参数作为字符串，并静默地将其视为对
//返回当前分类帐，虽然没有严重错误，但可能导致很多
//混乱的
//
//现在，代码会有力地验证输入，并确保
//“分类帐索引”参数的值是作为
//整数或字符串“current”、“closed”或“validated”之一。
//此外，该代码确保在“分类帐哈希”中传递的值是
//字符串和有效哈希。无效值将返回适当的错误
//代码。
//
//在没有“分类账哈希”或“分类账索引”参数的情况下，代码
//假设“分类帐索引”的值为“当前”。
//
//返回json:：objectValue。如果有错误，那就是
//返回值。否则，对象包含字段“validated”和
//可选字段“分类账哈希”、“分类账索引”和
//如果定义了“分类帐当前索引”。
Status
lookupLedger(std::shared_ptr<ReadView const>& ledger, Context& context,
    Json::Value& result)
{
    if (auto status = ledgerFromRequest (ledger, context))
        return status;

    auto& info = ledger->info();

    if (!ledger->open())
    {
        result[jss::ledger_hash] = to_string (info.hash);
        result[jss::ledger_index] = info.seq;
    }
    else
    {
        result[jss::ledger_current_index] = info.seq;
    }

    result[jss::validated] = isValidated (context.ledgerMaster, *ledger, context.app);
    return Status::OK;
}

Json::Value
lookupLedger(std::shared_ptr<ReadView const>& ledger, Context& context)
{
    Json::Value result;
    if (auto status = lookupLedger (ledger, context, result))
        status.inject (result);

    return result;
}

hash_set<AccountID>
parseAccountIds(Json::Value const& jvArray)
{
    hash_set<AccountID> result;
    for (auto const& jv: jvArray)
    {
        if (! jv.isString())
            return hash_set<AccountID>();
        auto const id =
            parseBase58<AccountID>(jv.asString());
        if (! id)
            return hash_set<AccountID>();
        result.insert(*id);
    }
    return result;
}

void
addPaymentDeliveredAmount(Json::Value& meta, RPC::Context& context,
    std::shared_ptr<Transaction> transaction, TxMeta::pointer transactionMeta)
{
//如果交易
//成功-否则无法传递任何内容。
    if (! transaction)
        return;

    auto const serializedTx = transaction->getSTransaction ();
    if (! serializedTx)
        return;

    {
//仅包括付款和支票现金交易的此字段。
        TxType const tt {serializedTx->getTxnType()};
        if ((tt != ttPAYMENT) && (tt != ttCHECK_CASH))
            return;

//如果修复
//启用。
        if (tt == ttCHECK_CASH)
        {
            auto const view = context.app.openLedger().current();
            if (!view || !view->rules().enabled (fix1623))
                return;
        }
    }

    if (transactionMeta)
    {
        if (transactionMeta->getResultTER() != tesSUCCESS)
            return;

//如果事务在
//然后我们使用元数据。
        if (transactionMeta->hasDeliveredAmount ())
        {
            meta[jss::delivered_amount] =
                transactionMeta->getDeliveredAmount ().getJson (1);
            return;
        }
    }
    else if (transaction->getResult() != tesSUCCESS)
    {
        return;
    }

    if (serializedTx->isFieldPresent (sfAmount))
    {
//分类帐4594095是DeliveredAmount字段所在的第一个分类帐。
//在部分付款时出现，其缺席表明
//交货金额列在“金额”字段中。
        if (transaction->getLedger () >= 4594095)
        {
            meta[jss::delivered_amount] =
                serializedTx->getFieldAmount (sfAmount).getJson (1);
            return;
        }

//如果在部署DeliveredAmount代码后很长时间分类帐关闭
//则其缺席表示交付的金额列在
//数量字段。DeliveredAmount于2014年1月24日上线。
        using namespace std::chrono_literals;
        auto const ct =
            context.ledgerMaster.getCloseTimeBySeq (transaction->getLedger ());
        if (ct && (*ct > NetClock::time_point{446000000s}))
        {
//44.60万人在2014年2月，在交付后很长一段时间。
            meta[jss::delivered_amount] =
                serializedTx->getFieldAmount (sfAmount).getJson (1);
            return;
        }
    }

//否则，我们报告“不可用”，无法将其解析为
//合理数量。
    meta[jss::delivered_amount] = Json::Value ("unavailable");
}

void
injectSLE(Json::Value& jv, SLE const& sle)
{
    jv = sle.getJson(0);
    if (sle.getType() == ltACCOUNT_ROOT)
    {
        if (sle.isFieldPresent(sfEmailHash))
        {
            auto const& hash =
                sle.getFieldH128(sfEmailHash);
            Blob const b (hash.begin(), hash.end());
            std::string md5 = strHex(makeSlice(b));
            boost::to_lower(md5);
//vfalc todo给出一个名称并移动这个常量
//到一个更可见的位置。阿尔索
//这不是HTTPS吗？
            jv[jss::urlgravatar] = str(boost::format(
"http://www.gravatar.com/avatar/%s”）%md5）；
        }
    }
    else
    {
        jv[jss::Invalid] = true;
    }
}

boost::optional<Json::Value>
readLimitField(unsigned int& limit, Tuning::LimitRange const& range,
    Context const& context)
{
    limit = range.rdefault;
    if (auto const& jvLimit = context.params[jss::limit])
    {
        if (! (jvLimit.isUInt() || (jvLimit.isInt() && jvLimit.asInt() >= 0)))
            return RPC::expected_field_error (jss::limit, "unsigned integer");

        limit = jvLimit.asUInt();
        if (! isUnlimited (context.role))
            limit = std::max(range.rmin, std::min(range.rmax, limit));
    }
    return boost::none;
}

boost::optional<Seed>
parseRippleLibSeed(Json::Value const& value)
{
//Ripple lib编码用于生成ED25519钱包的种子
//非标准方式。虽然涟漪从来没有这样编码种子，但我们
//尝试检测这些密钥以避免用户混淆。
    if (!value.isString())
        return boost::none;

    auto const result = decodeBase58Token(value.asString(), TokenType::None);

    if (result.size() == 18 &&
            static_cast<std::uint8_t>(result[0]) == std::uint8_t(0xE1) &&
            static_cast<std::uint8_t>(result[1]) == std::uint8_t(0x4B))
        return Seed(makeSlice(result.substr(2)));

    return boost::none;
}

boost::optional<Seed>
getSeedFromRPC(Json::Value const& params, Json::Value& error)
{
//数组应该是constexpr，但这会使Visual Studio不高兴。
    static char const* const seedTypes[]
    {
        jss::passphrase.c_str(),
        jss::seed.c_str(),
        jss::seed_hex.c_str()
    };

//标识正在使用的种子类型。
    char const* seedType = nullptr;
    int count = 0;
    for (auto t : seedTypes)
    {
        if (params.isMember (t))
        {
            ++count;
            seedType = t;
        }
    }

    if (count != 1)
    {
        error = RPC::make_param_error (
            "Exactly one of the following must be specified: " +
            std::string(jss::passphrase) + ", " +
            std::string(jss::seed) + " or " +
            std::string(jss::seed_hex));
        return boost::none;
    }

//确保存在字符串
    if (! params[seedType].isString())
    {
        error = RPC::expected_field_error (seedType, "string");
        return boost::none;
    }

    auto const fieldContents = params[seedType].asString();

//将字符串转换为种子。
    boost::optional<Seed> seed;

    if (seedType == jss::seed.c_str())
        seed = parseBase58<Seed> (fieldContents);
    else if (seedType == jss::passphrase.c_str())
        seed = parseGenericSeed (fieldContents);
    else if (seedType == jss::seed_hex.c_str())
    {
        uint128 s;

        if (s.SetHexExact (fieldContents))
            seed.emplace (Slice(s.data(), s.size()));
    }

    if (!seed)
        error = rpcError (rpcBAD_SEED);

    return seed;
}

std::pair<PublicKey, SecretKey>
keypairForSignature(Json::Value const& params, Json::Value& error)
{
    bool const has_key_type  = params.isMember (jss::key_type);

//我们允许的所有秘密类型，但一次只能有一个。
//数组应该是constexpr，但这会使Visual Studio不高兴。
    static char const* const secretTypes[]
    {
        jss::passphrase.c_str(),
        jss::secret.c_str(),
        jss::seed.c_str(),
        jss::seed_hex.c_str()
    };

//标识正在使用的机密类型。
    char const* secretType = nullptr;
    int count = 0;
    for (auto t : secretTypes)
    {
        if (params.isMember (t))
        {
            ++count;
            secretType = t;
        }
    }

    if (count == 0 || secretType == nullptr)
    {
        error = RPC::missing_field_error (jss::secret);
        return { };
    }

    if (count > 1)
    {
        error = RPC::make_param_error (
            "Exactly one of the following must be specified: " +
            std::string(jss::passphrase) + ", " +
            std::string(jss::secret) + ", " +
            std::string(jss::seed) + " or " +
            std::string(jss::seed_hex));
        return { };
    }

    boost::optional<KeyType> keyType;
    boost::optional<Seed> seed;

    if (has_key_type)
    {
        if (! params[jss::key_type].isString())
        {
            error = RPC::expected_field_error (
                jss::key_type, "string");
            return { };
        }

        keyType = keyTypeFromString(params[jss::key_type].asString());

        if (!keyType)
        {
            error = RPC::invalid_field_error(jss::key_type);
            return { };
        }

        if (secretType == jss::secret.c_str())
        {
            error = RPC::make_param_error (
                "The secret field is not allowed if " +
                std::string(jss::key_type) + " is used.");
            return { };
        }
    }

//Ripple lib编码用于生成ED25519钱包的种子
//非标准方式。虽然我们从不这样编码种子，但我们尝试
//检测这些密钥以避免用户混淆。
    if (secretType != jss::seed_hex.c_str())
    {
        seed = RPC::parseRippleLibSeed(params[secretType]);

        if (seed)
        {
//如果用户传入ED25519种子，但*显式*
//请求了另一个密钥类型，返回一个错误。
            if (keyType.value_or(KeyType::ed25519) != KeyType::ed25519)
            {
                error = RPC::make_error (rpcBAD_SEED,
                    "Specified seed is for an Ed25519 wallet.");
                return { };
            }

            keyType = KeyType::ed25519;
        }
    }

    if (!keyType)
        keyType = KeyType::secp256k1;

    if (!seed)
    {
        if (has_key_type)
            seed = getSeedFromRPC(params, error);
        else
        {
            if (!params[jss::secret].isString())
            {
                error = RPC::expected_field_error(jss::secret, "string");
                return {};
            }

            seed = parseGenericSeed(params[jss::secret].asString());
        }
    }

    if (!seed)
    {
        if (!contains_error (error))
        {
            error = RPC::make_error (rpcBAD_SEED,
                RPC::invalid_field_message (secretType));
        }

        return { };
    }

    if (keyType != KeyType::secp256k1 && keyType != KeyType::ed25519)
        LogicError ("keypairForSignature: invalid key type");

    return generateKeyPair (*keyType, *seed);
}

std::pair<RPC::Status, LedgerEntryType>
chooseLedgerEntryType(Json::Value const& params)
{
    std::pair<RPC::Status, LedgerEntryType> result{ RPC::Status::OK, ltINVALID };
    if (params.isMember(jss::type))
    {
        static
            std::array<std::pair<char const *, LedgerEntryType>, 13> const types
        { {
            { jss::account,         ltACCOUNT_ROOT },
            { jss::amendments,      ltAMENDMENTS },
            { jss::check,           ltCHECK },
            { jss::deposit_preauth, ltDEPOSIT_PREAUTH },
            { jss::directory,       ltDIR_NODE },
            { jss::escrow,          ltESCROW },
            { jss::fee,             ltFEE_SETTINGS },
            { jss::hashes,          ltLEDGER_HASHES },
            { jss::offer,           ltOFFER },
            { jss::payment_channel, ltPAYCHAN },
            { jss::signer_list,     ltSIGNER_LIST },
            { jss::state,           ltRIPPLE_STATE },
            { jss::ticket,          ltTICKET }
            } };

        auto const& p = params[jss::type];
        if (!p.isString())
        {
            result.first = RPC::Status{ rpcINVALID_PARAMS,
                "Invalid field 'type', not string." };
            assert(result.first.type() == RPC::Status::Type::error_code_i);
            return result;
        }

        auto const filter = p.asString();
        auto iter = std::find_if(types.begin(), types.end(),
            [&filter](decltype (types.front())& t)
        {
            return t.first == filter;
        });
        if (iter == types.end())
        {
            result.first = RPC::Status{ rpcINVALID_PARAMS,
                "Invalid field 'type'." };
            assert(result.first.type() == RPC::Status::Type::error_code_i);
            return result;
        }
        result.second = iter->second;
    }
    return result;
}

beast::SemanticVersion const firstVersion("1.0.0");
beast::SemanticVersion const goodVersion("1.0.0");
beast::SemanticVersion const lastVersion("1.0.0");

} //RPC
} //涟漪
