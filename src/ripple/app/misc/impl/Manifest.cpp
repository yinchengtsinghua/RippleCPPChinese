
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

#include <ripple/app/misc/Manifest.h>
#include <ripple/basics/base64.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/beast/rfc2616.h>
#include <ripple/core/DatabaseCon.h>
#include <ripple/json/json_reader.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/Sign.h>
#include <boost/regex.hpp>
#include <numeric>
#include <stdexcept>

namespace ripple {

boost::optional<Manifest>
Manifest::make_Manifest (std::string s)
{
    try
    {
        STObject st (sfGeneric);
        SerialIter sit (s.data (), s.size ());
        st.set (sit);
        auto const pk = st.getFieldVL (sfPublicKey);
        if (! publicKeyType (makeSlice(pk)))
            return boost::none;

        auto const opt_seq = get (st, sfSequence);
        auto const opt_msig = get (st, sfMasterSignature);
        if (!opt_seq || !opt_msig)
            return boost::none;

//不需要签名密钥和签名
//主密钥吊销
        if (*opt_seq != std::numeric_limits<std::uint32_t>::max ())
        {
            auto const spk = st.getFieldVL (sfSigningPubKey);
            if (! publicKeyType (makeSlice(spk)))
                return boost::none;
            if (! get (st, sfSignature))
                return boost::none;
            return Manifest (std::move (s), PublicKey (makeSlice(pk)),
                PublicKey (makeSlice(spk)), *opt_seq);
        }

        return Manifest (std::move (s), PublicKey (makeSlice(pk)),
            PublicKey(), *opt_seq);
    }
    catch (std::exception const&)
    {
        return boost::none;
    }
}

template<class Stream>
Stream&
logMftAct (
    Stream& s,
    std::string const& action,
    PublicKey const& pk,
    std::uint32_t seq)
{
    s << "Manifest: " << action <<
         ";Pk: " << toBase58 (TokenType::NodePublic, pk) <<
         ";Seq: " << seq << ";";
    return s;
}

template<class Stream>
Stream& logMftAct (
    Stream& s,
    std::string const& action,
    PublicKey const& pk,
    std::uint32_t seq,
    std::uint32_t oldSeq)
{
    s << "Manifest: " << action <<
         ";Pk: " << toBase58 (TokenType::NodePublic, pk) <<
         ";Seq: " << seq <<
         ";OldSeq: " << oldSeq << ";";
    return s;
}

Manifest::Manifest (std::string s,
                    PublicKey pk,
                    PublicKey spk,
                    std::uint32_t seq)
    : serialized (std::move (s))
    , masterKey (std::move (pk))
    , signingKey (std::move (spk))
    , sequence (seq)
{
}

bool Manifest::verify () const
{
    STObject st (sfGeneric);
    SerialIter sit (serialized.data (), serialized.size ());
    st.set (sit);

//不需要签名密钥和签名
//主密钥吊销
    if (! revoked () && ! ripple::verify (st, HashPrefix::manifest, signingKey))
        return false;

    return ripple::verify (
        st, HashPrefix::manifest, masterKey, sfMasterSignature);
}

uint256 Manifest::hash () const
{
    STObject st (sfGeneric);
    SerialIter sit (serialized.data (), serialized.size ());
    st.set (sit);
    return st.getHash (HashPrefix::manifest);
}

bool Manifest::revoked () const
{
    /*
        最大可能的序列号意味着主密钥
        已被吊销。
    **/

    return sequence == std::numeric_limits<std::uint32_t>::max ();
}

Blob Manifest::getSignature () const
{
    STObject st (sfGeneric);
    SerialIter sit (serialized.data (), serialized.size ());
    st.set (sit);
    return st.getFieldVL (sfSignature);
}

Blob Manifest::getMasterSignature () const
{
    STObject st (sfGeneric);
    SerialIter sit (serialized.data (), serialized.size ());
    st.set (sit);
    return st.getFieldVL (sfMasterSignature);
}

ValidatorToken::ValidatorToken(std::string const& m, SecretKey const& valSecret)
    : manifest(m)
    , validationSecret(valSecret)
{
}

boost::optional<ValidatorToken>
ValidatorToken::make_ValidatorToken(std::vector<std::string> const& tokenBlob)
{
    try
    {
        std::string tokenStr;
        tokenStr.reserve (
            std::accumulate (tokenBlob.cbegin(), tokenBlob.cend(), std::size_t(0),
                [] (std::size_t init, std::string const& s)
                {
                    return init + s.size();
                }));

        for (auto const& line : tokenBlob)
            tokenStr += beast::rfc2616::trim(line);

        tokenStr = base64_decode(tokenStr);

        Json::Reader r;
        Json::Value token;
        if (! r.parse (tokenStr, token))
            return boost::none;

        if (token.isMember("manifest") && token["manifest"].isString() &&
            token.isMember("validation_secret_key") &&
            token["validation_secret_key"].isString())
        {
            auto const ret = strUnHex (token["validation_secret_key"].asString());
            if (! ret.second || ! ret.first.size ())
                return boost::none;

            return ValidatorToken(
                token["manifest"].asString(),
                SecretKey(Slice{ret.first.data(), ret.first.size()}));
        }
        else
        {
            return boost::none;
        }
    }
    catch (std::exception const&)
    {
        return boost::none;
    }
}

PublicKey
ManifestCache::getSigningKey (PublicKey const& pk) const
{
    std::lock_guard<std::mutex> lock{read_mutex_};
    auto const iter = map_.find (pk);

    if (iter != map_.end () && !iter->second.revoked ())
        return iter->second.signingKey;

    return pk;
}

PublicKey
ManifestCache::getMasterKey (PublicKey const& pk) const
{
    std::lock_guard<std::mutex> lock{read_mutex_};
    auto const iter = signingToMasterKeys_.find (pk);

    if (iter != signingToMasterKeys_.end ())
        return iter->second;

    return pk;
}

bool
ManifestCache::revoked (PublicKey const& pk) const
{
    std::lock_guard<std::mutex> lock{read_mutex_};
    auto const iter = map_.find (pk);

    if (iter != map_.end ())
        return iter->second.revoked ();

    return false;
}

ManifestDisposition
ManifestCache::applyManifest (Manifest m)
{
    std::lock_guard<std::mutex> applyLock{apply_mutex_};

    /*
        在我们花时间检查签名之前，确保
        序列号比我们现有的任何序列号都要新。
    **/

    auto const iter = map_.find(m.masterKey);

    if (iter != map_.end() &&
        m.sequence <= iter->second.sequence)
    {
        /*
            我们正在跟踪的验证器收到了清单，但是
            其序列号不高于已存储的序列号。
            这种情况通常发生在没有最新八卦的同龄人身上。
            连接。
        **/

        if (auto stream = j_.debug())
            logMftAct(stream, "Stale", m.masterKey, m.sequence, iter->second.sequence);
return ManifestDisposition::stale;  //不是更新的清单，忽略
    }

    if (! m.verify())
    {
        /*
          清单的签名无效。
          这不应该正常发生。
        **/

        if (auto stream = j_.warn())
            logMftAct(stream, "Invalid", m.masterKey, m.sequence);
        return ManifestDisposition::invalid;
    }

    std::lock_guard<std::mutex> readLock{read_mutex_};

    bool const revoked = m.revoked();

    if (revoked)
    {
        /*
            验证程序主密钥已被泄露，因此其清单
            现在是不可信的。为了阻止我们接受
            由泄露的主密钥store签名的伪造清单
            此清单具有最高的序列号
            因此不能被伪造的替代。
        **/

        if (auto stream = j_.warn())
            logMftAct(stream, "Revoked", m.masterKey, m.sequence);
    }

    if (iter == map_.end ())
    {
        /*
            这是第一个收到的受信任主密钥清单
            （可能是我们自己的）。这只在每个验证器上发生一次
            跑。
        **/

        if (auto stream = j_.info())
            logMftAct(stream, "AcceptedNew", m.masterKey, m.sequence);

        if (! revoked)
            signingToMasterKeys_[m.signingKey] = m.masterKey;

        auto masterKey = m.masterKey;
        map_.emplace(std::move(masterKey), std::move(m));
    }
    else
    {
        /*
            临时密钥被吊销并被新密钥取代。
            这是预期的，但应该很少发生。
        **/

        if (auto stream = j_.info())
            logMftAct(stream, "AcceptedUpdate",
                      m.masterKey, m.sequence, iter->second.sequence);

        signingToMasterKeys_.erase (iter->second.signingKey);

        if (! revoked)
            signingToMasterKeys_[m.signingKey] = m.masterKey;

        iter->second = std::move (m);
    }

    return ManifestDisposition::accepted;
}

void
ManifestCache::load (
    DatabaseCon& dbCon, std::string const& dbTable)
{
//加载存储在数据库中的清单
    std::string const sql =
        "SELECT RawData FROM " + dbTable + ";";
    auto db = dbCon.checkoutDb ();
    soci::blob sociRawData (*db);
    soci::statement st =
        (db->prepare << sql,
             soci::into (sociRawData));
    st.execute ();
    while (st.fetch ())
    {
        std::string serialized;
        convert (sociRawData, serialized);
        if (auto mo = Manifest::make_Manifest (std::move (serialized)))
        {
            if (!mo->verify())
            {
                JLOG(j_.warn())
                    << "Unverifiable manifest in db";
                continue;
            }

            applyManifest (std::move(*mo));
        }
        else
        {
            JLOG(j_.warn())
                << "Malformed manifest in database";
        }
    }
}

bool
ManifestCache::load (
    DatabaseCon& dbCon, std::string const& dbTable,
    std::string const& configManifest,
    std::vector<std::string> const& configRevocation)
{
    load (dbCon, dbTable);

    if (! configManifest.empty())
    {
        auto mo = Manifest::make_Manifest (
            base64_decode(configManifest));
        if (! mo)
        {
            JLOG (j_.error()) << "Malformed validator_token in config";
            return false;
        }

        if (mo->revoked())
        {
            JLOG (j_.warn()) <<
                "Configured manifest revokes public key";
        }

        if (applyManifest (std::move(*mo)) ==
            ManifestDisposition::invalid)
        {
            JLOG (j_.error()) << "Manifest in config was rejected";
            return false;
        }
    }

    if (! configRevocation.empty())
    {
        std::string revocationStr;
        revocationStr.reserve (
            std::accumulate (configRevocation.cbegin(), configRevocation.cend(), std::size_t(0),
                [] (std::size_t init, std::string const& s)
                {
                    return init + s.size();
                }));

        for (auto const& line : configRevocation)
            revocationStr += beast::rfc2616::trim(line);

        auto mo = Manifest::make_Manifest (
            base64_decode(revocationStr));

        if (! mo || ! mo->revoked() ||
            applyManifest (std::move(*mo)) == ManifestDisposition::invalid)
        {
            JLOG (j_.error()) << "Invalid validator key revocation in config";
            return false;
        }
    }

    return true;
}

void ManifestCache::save (
    DatabaseCon& dbCon, std::string const& dbTable,
    std::function <bool (PublicKey const&)> isTrusted)
{
    std::lock_guard<std::mutex> lock{apply_mutex_};

    auto db = dbCon.checkoutDb ();

    soci::transaction tr(*db);
    *db << "DELETE FROM " << dbTable;
    std::string const sql =
        "INSERT INTO " + dbTable + " (RawData) VALUES (:rawData);";
    for (auto const& v : map_)
    {
//保存所有吊销清单，
//但只保存受信任的非吊销清单。
        if (! v.second.revoked() && ! isTrusted (v.second.masterKey))
        {

            JLOG(j_.info())
               << "Untrusted manifest in cache not saved to db";
            continue;
        }

//Soci不支持大容量插入blob数据
//不要重用BLOB，因为清单ECDSA签名的长度不同
//但blob写入长度应大于等于最后一次写入
        soci::blob rawData(*db);
        convert (v.second.serialized, rawData);
        *db << sql,
            soci::use (rawData);
    }
    tr.commit ();
}
}
