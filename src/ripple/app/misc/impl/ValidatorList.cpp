
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
    版权所有（c）2015 Ripple Labs Inc.

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

#include <ripple/app/misc/ValidatorList.h>
#include <ripple/basics/base64.h>
#include <ripple/basics/date.h>
#include <ripple/basics/Slice.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/json/json_reader.h>
#include <ripple/protocol/JsonFields.h>
#include <boost/regex.hpp>

#include <mutex>
#include <shared_mutex>

namespace ripple {

std::string
to_string(ListDisposition disposition)
{
    switch (disposition)
    {
        case ListDisposition::accepted:
            return "accepted";
        case ListDisposition::same_sequence:
            return "same_sequence";
        case ListDisposition::unsupported_version:
            return "unsupported_version";
        case ListDisposition::untrusted:
            return "untrusted";
        case ListDisposition::stale:
            return "stale";
        case ListDisposition::invalid:
            return "invalid";
    }
    return "unknown";
}

ValidatorList::ValidatorList (
    ManifestCache& validatorManifests,
    ManifestCache& publisherManifests,
    TimeKeeper& timeKeeper,
    beast::Journal j,
    boost::optional<std::size_t> minimumQuorum)
    : validatorManifests_ (validatorManifests)
    , publisherManifests_ (publisherManifests)
    , timeKeeper_ (timeKeeper)
    , j_ (j)
, quorum_ (minimumQuorum.value_or(1)) //创世纪分类帐法定人数
    , minimumQuorum_ (minimumQuorum)
{
}

bool
ValidatorList::load (
    PublicKey const& localSigningKey,
    std::vector<std::string> const& configKeys,
    std::vector<std::string> const& publisherKeys)
{
    static boost::regex const re (
"[[:space:]]*"            //跳过前导空格
"([[:alnum:]]+)"          //节点恒等式
"(?:"                     //开始可选注释块
"[[:space:]]+"            //（跳过所有前导空格）
"(?:"                     //开始可选注释
"(.*[^[:space:]]+)"       //评论
"[[:space:]]*"            //（跳过所有尾随空格）
")?"                      //结束可选注释
")?"                      //结束可选注释块
    );

    std::unique_lock<std::shared_timed_mutex> read_lock{mutex_};

    JLOG (j_.debug()) <<
        "Loading configured trusted validator list publisher keys";

    std::size_t count = 0;
    for (auto key : publisherKeys)
    {
        JLOG (j_.trace()) <<
            "Processing '" << key << "'";

        auto const ret = strUnHex (key);

        if (! ret.second || ! publicKeyType(makeSlice(ret.first)))
        {
            JLOG (j_.error()) <<
                "Invalid validator list publisher key: " << key;
            return false;
        }

        auto id = PublicKey(makeSlice(ret.first));

        if (publisherManifests_.revoked (id))
        {
            JLOG (j_.warn()) <<
                "Configured validator list publisher key is revoked: " << key;
            continue;
        }

        if (publisherLists_.count(id))
        {
            JLOG (j_.warn()) <<
                "Duplicate validator list publisher key: " << key;
            continue;
        }

        publisherLists_[id].available = false;
        ++count;
    }

    JLOG (j_.debug()) <<
        "Loaded " << count << " keys";

    localPubKey_ = validatorManifests_.getMasterKey (localSigningKey);

//将本地验证程序密钥视为已在配置中列出
    if (localPubKey_.size())
        keyListings_.insert ({ localPubKey_, 1 });

    JLOG (j_.debug()) <<
        "Loading configured validator keys";

    count = 0;
    PublicKey local;
    for (auto const& n : configKeys)
    {
        JLOG (j_.trace()) <<
            "Processing '" << n << "'";

        boost::smatch match;

        if (!boost::regex_match (n, match, re))
        {
            JLOG (j_.error()) <<
                "Malformed entry: '" << n << "'";
            return false;
        }

        auto const id = parseBase58<PublicKey>(
            TokenType::NodePublic, match[1]);

        if (!id)
        {
            JLOG (j_.error()) << "Invalid node identity: " << match[1];
            return false;
        }

//跳过已添加的本地密钥
        if (*id == localPubKey_ || *id == localSigningKey)
            continue;

        auto ret = keyListings_.insert ({*id, 1});
        if (! ret.second)
        {
            JLOG (j_.warn()) << "Duplicate node identity: " << match[1];
            continue;
        }
        auto it = publisherLists_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(local),
            std::forward_as_tuple());
//配置列出的密钥永不过期
        if (it.second)
            it.first->second.expiration = TimeKeeper::time_point::max();
        it.first->second.list.emplace_back(std::move(*id));
        it.first->second.available = true;
        ++count;
    }

    JLOG (j_.debug()) <<
        "Loaded " << count << " entries";

    return true;
}

ListDisposition
ValidatorList::applyList (
    std::string const& manifest,
    std::string const& blob,
    std::string const& signature,
    std::uint32_t version,
    std::string siteUri)
{
    if (version != requiredListVersion)
        return ListDisposition::unsupported_version;

    std::unique_lock<std::shared_timed_mutex> lock{mutex_};

    Json::Value list;
    PublicKey pubKey;
    auto const result = verify (list, pubKey, manifest, blob, signature);
    if (result != ListDisposition::accepted)
        return result;

//更新发布者列表
    Json::Value const& newList = list["validators"];
    publisherLists_[pubKey].available = true;
    publisherLists_[pubKey].sequence = list["sequence"].asUInt ();
    publisherLists_[pubKey].expiration = TimeKeeper::time_point{
        TimeKeeper::duration{list["expiration"].asUInt()}};
    publisherLists_[pubKey].siteUri = std::move(siteUri);
    std::vector<PublicKey>& publisherList = publisherLists_[pubKey].list;

    std::vector<PublicKey> oldList = publisherList;
    publisherList.clear ();
    publisherList.reserve (newList.size ());
    std::vector<std::string> manifests;
    for (auto const& val : newList)
    {
        if (val.isObject() &&
            val.isMember ("validation_public_key") &&
            val["validation_public_key"].isString ())
        {
            std::pair<Blob, bool> ret (strUnHex (
                val["validation_public_key"].asString ()));

            if (! ret.second || ! publicKeyType(makeSlice(ret.first)))
            {
                JLOG (j_.error()) <<
                    "Invalid node identity: " <<
                    val["validation_public_key"].asString ();
            }
            else
            {
                publisherList.push_back (
                    PublicKey(Slice{ ret.first.data (), ret.first.size() }));
            }

            if (val.isMember ("manifest") && val["manifest"].isString ())
                manifests.push_back(val["manifest"].asString ());
        }
    }

//为添加和删除的密钥更新密钥列表
    std::sort (
        publisherList.begin (),
        publisherList.end ());

    auto iNew = publisherList.begin ();
    auto iOld = oldList.begin ();
    while (iNew != publisherList.end () ||
        iOld != oldList.end ())
    {
        if (iOld == oldList.end () ||
            (iNew != publisherList.end () &&
            *iNew < *iOld))
        {
//为添加的键增加列表计数
            ++keyListings_[*iNew];
            ++iNew;
        }
        else if (iNew == publisherList.end () ||
            (iOld != oldList.end () && *iOld < *iNew))
        {
//已删除键的递减列表计数
            if (keyListings_[*iOld] <= 1)
                keyListings_.erase (*iOld);
            else
                --keyListings_[*iOld];
            ++iOld;
        }
        else
        {
            ++iNew;
            ++iOld;
        }
    }

    if (publisherList.empty())
    {
        JLOG (j_.warn()) <<
            "No validator keys included in valid list";
    }

    for (auto const& valManifest : manifests)
    {
        auto m = Manifest::make_Manifest (
            base64_decode(valManifest));

        if (! m || ! keyListings_.count (m->masterKey))
        {
            JLOG (j_.warn()) <<
                "List for " << strHex(pubKey) <<
                " contained untrusted validator manifest";
            continue;
        }

        auto const result = validatorManifests_.applyManifest (std::move(*m));
        if (result == ManifestDisposition::invalid)
        {
            JLOG (j_.warn()) <<
                "List for " << strHex(pubKey) <<
                " contained invalid validator manifest";
        }
    }

    return ListDisposition::accepted;
}

ListDisposition
ValidatorList::verify (
    Json::Value& list,
    PublicKey& pubKey,
    std::string const& manifest,
    std::string const& blob,
    std::string const& signature)
{
    auto m = Manifest::make_Manifest (base64_decode(manifest));

    if (! m || ! publisherLists_.count (m->masterKey))
        return ListDisposition::untrusted;

    pubKey = m->masterKey;
    auto const revoked = m->revoked();

    auto const result = publisherManifests_.applyManifest (
        std::move(*m));

    if (revoked && result == ManifestDisposition::accepted)
    {
        removePublisherList (pubKey);
        publisherLists_.erase (pubKey);
    }

    if (revoked || result == ManifestDisposition::invalid)
        return ListDisposition::untrusted;

    auto const sig = strUnHex(signature);
    auto const data = base64_decode (blob);
    if (! sig.second ||
        ! ripple::verify (
            publisherManifests_.getSigningKey(pubKey),
            makeSlice(data),
            makeSlice(sig.first)))
        return ListDisposition::invalid;

    Json::Reader r;
    if (! r.parse (data, list))
        return ListDisposition::invalid;

    if (list.isMember("sequence") && list["sequence"].isInt() &&
        list.isMember("expiration") && list["expiration"].isInt() &&
        list.isMember("validators") && list["validators"].isArray())
    {
        auto const sequence = list["sequence"].asUInt();
        auto const expiration = TimeKeeper::time_point{
            TimeKeeper::duration{list["expiration"].asUInt()}};
        if (sequence < publisherLists_[pubKey].sequence ||
            expiration <= timeKeeper_.now())
            return ListDisposition::stale;
        else if (sequence == publisherLists_[pubKey].sequence)
            return ListDisposition::same_sequence;
    }
    else
    {
        return ListDisposition::invalid;
    }

    return ListDisposition::accepted;
}

bool
ValidatorList::listed (
    PublicKey const& identity) const
{
    std::shared_lock<std::shared_timed_mutex> read_lock{mutex_};

    auto const pubKey = validatorManifests_.getMasterKey (identity);
    return keyListings_.find (pubKey) != keyListings_.end ();
}

bool
ValidatorList::trusted (PublicKey const& identity) const
{
    std::shared_lock<std::shared_timed_mutex> read_lock{mutex_};

    auto const pubKey = validatorManifests_.getMasterKey (identity);
    return trustedKeys_.find (pubKey) != trustedKeys_.end();
}

boost::optional<PublicKey>
ValidatorList::getListedKey (
    PublicKey const& identity) const
{
    std::shared_lock<std::shared_timed_mutex> read_lock{mutex_};

    auto const pubKey = validatorManifests_.getMasterKey (identity);
    if (keyListings_.find (pubKey) != keyListings_.end ())
        return pubKey;
    return boost::none;
}

boost::optional<PublicKey>
ValidatorList::getTrustedKey (PublicKey const& identity) const
{
    std::shared_lock<std::shared_timed_mutex> read_lock{mutex_};

    auto const pubKey = validatorManifests_.getMasterKey (identity);
    if (trustedKeys_.find (pubKey) != trustedKeys_.end())
        return pubKey;
    return boost::none;
}

bool
ValidatorList::trustedPublisher (PublicKey const& identity) const
{
    std::shared_lock<std::shared_timed_mutex> read_lock{mutex_};
    return identity.size() && publisherLists_.count (identity);
}

PublicKey
ValidatorList::localPublicKey () const
{
    std::shared_lock<std::shared_timed_mutex> read_lock{mutex_};
    return localPubKey_;
}

bool
ValidatorList::removePublisherList (PublicKey const& publisherKey)
{
    auto const iList = publisherLists_.find (publisherKey);
    if (iList == publisherLists_.end ())
        return false;

    JLOG (j_.debug()) <<
        "Removing validator list for publisher " << strHex(publisherKey);

    for (auto const& val : iList->second.list)
    {
        auto const& iVal = keyListings_.find (val);
        if (iVal == keyListings_.end())
            continue;

        if (iVal->second <= 1)
            keyListings_.erase (iVal);
        else
            --iVal->second;
    }

    iList->second.list.clear();
    iList->second.available = false;

    return true;
}

std::size_t
ValidatorList::count() const
{
    std::shared_lock<std::shared_timed_mutex> read_lock{mutex_};
    return publisherLists_.size();
}

boost::optional<TimeKeeper::time_point>
ValidatorList::expires() const
{
    std::shared_lock<std::shared_timed_mutex> read_lock{mutex_};
    boost::optional<TimeKeeper::time_point> res{boost::none};
    for (auto const& p : publisherLists_)
    {
//未取出的
        if (p.second.expiration == TimeKeeper::time_point{})
            return boost::none;

//最早
        if (!res || p.second.expiration < *res)
            res = p.second.expiration;
    }
    return res;
}

Json::Value
ValidatorList::getJson() const
{
    Json::Value res(Json::objectValue);

    std::shared_lock<std::shared_timed_mutex> read_lock{mutex_};

    res[jss::validation_quorum] = static_cast<Json::UInt>(quorum());

    {
        auto& x = (res[jss::validator_list] = Json::objectValue);

        x[jss::count] = static_cast<Json::UInt>(count());

        if (auto when = expires())
        {
            if (*when == TimeKeeper::time_point::max())
            {
                x[jss::expiration] = "never";
                x[jss::status] = "active";
            }
            else
            {
                x[jss::expiration] = to_string(*when);

                if (*when > timeKeeper_.now())
                    x[jss::status] = "active";
                else
                    x[jss::status] = "expired";
            }
        }
        else
        {
            x[jss::status] = "unknown";
            x[jss::expiration] = "unknown";
        }
    }

//本地静态密钥
    PublicKey local;
    Json::Value& jLocalStaticKeys =
        (res[jss::local_static_keys] = Json::arrayValue);
    auto it = publisherLists_.find(local);
    if (it != publisherLists_.end())
    {
        for (auto const& key : it->second.list)
            jLocalStaticKeys.append(
                toBase58(TokenType::NodePublic, key));
    }

//出版商名单
    Json::Value& jPublisherLists =
        (res[jss::publisher_lists] = Json::arrayValue);
    for (auto const& p : publisherLists_)
    {
        if(local == p.first)
            continue;
        Json::Value& curr = jPublisherLists.append(Json::objectValue);
        curr[jss::pubkey_publisher] = strHex(p.first);
        curr[jss::available] = p.second.available;
        curr[jss::uri] = p.second.siteUri;
        if(p.second.expiration != TimeKeeper::time_point{})
        {
            curr[jss::seq] = static_cast<Json::UInt>(p.second.sequence);
            curr[jss::expiration] = to_string(p.second.expiration);
            curr[jss::version] = requiredListVersion;
        }
        Json::Value& keys = (curr[jss::list] = Json::arrayValue);
        for (auto const& key : p.second.list)
        {
            keys.append(toBase58(TokenType::NodePublic, key));
        }
    }

//受信任的验证程序密钥
    Json::Value& jValidatorKeys =
        (res[jss::trusted_validator_keys] = Json::arrayValue);
    for (auto const& k : trustedKeys_)
    {
        jValidatorKeys.append(toBase58(TokenType::NodePublic, k));
    }

//签名密钥
    Json::Value& jSigningKeys = (res[jss::signing_keys] = Json::objectValue);
    validatorManifests_.for_each_manifest(
        [&jSigningKeys, this](Manifest const& manifest) {

            auto it = keyListings_.find(manifest.masterKey);
            if (it != keyListings_.end())
            {
                jSigningKeys[toBase58(
                    TokenType::NodePublic, manifest.masterKey)] =
                    toBase58(TokenType::NodePublic, manifest.signingKey);
            }
        });

    return res;
}

void
ValidatorList::for_each_listed (
    std::function<void(PublicKey const&, bool)> func) const
{
    std::shared_lock<std::shared_timed_mutex> read_lock{mutex_};

    for (auto const& v : keyListings_)
        func (v.first, trusted(v.first));
}

std::size_t
ValidatorList::calculateQuorum (
    std::size_t trusted, std::size_t seen)
{
//在所有配置的列表之前，不要使用可实现的仲裁
//发布服务器可用
    for (auto const& list : publisherLists_)
    {
        if (! list.second.available)
            return std::numeric_limits<std::size_t>::max();
    }

//使用80%的法定人数来平衡叉的安全性、活力和所需的UNL。
//重叠。
//
//xrp分类账共识协议分析定理8
//（https://arxiv.org/abs/1802.07242）说：
//如果oi，j>nj/2+ni−qi+ti，j用于
//每对节点pi，pj。
//
//ni:pi's unl的大小
//NJ：PJ的UNL尺寸
//oi，j：两个UNL中的验证器数量
//qi:pi's unl的验证法定人数
//ti，tj：pi和pj的unls中允许的拜占庭式错误的最大数量
//ti，j：最小ti，tj，oi，j
//
//假设ni<nj，意思和ti，j=ti
//
//对于qi=0.8*ni，我们使ti<=0.2*ni
//（我们可以使ti降低，并且允许较少的un重叠。不过，按顺序
//为了将安全优先于生存，我们需要Ti>=Ni-qi）
//
//80%的仲裁允许两个unl安全地拥有<.2*ni唯一验证器
//它们之间：
//
//Pi＝Ni-Oi，J
//PJ= NJ-OI，J
//
//oi，j>nj/2+ni−qi+ti，j
//ni-pi>（ni-pi+pj）/2+ni−.8*ni+.2*ni
//Pi+PJ＜2×Ni
    auto quorum = static_cast<std::size_t>(std::ceil(trusted * 0.8f));

//如果出现正常仲裁，则使用通过命令行指定的较低仲裁
//基于最近收到的验证数，无法访问。
    if (minimumQuorum_ && *minimumQuorum_ < quorum && seen < quorum)
    {
        quorum = *minimumQuorum_;

        JLOG (j_.warn())
            << "Using unsafe quorum of "
            << quorum
            << " as specified in the command line";
    }

    return quorum;
}

TrustChanges
ValidatorList::updateTrusted(hash_set<NodeID> const& seenValidators)
{
    std::unique_lock<std::shared_timed_mutex> lock{mutex_};

//删除所有过期的已发布列表
    for (auto const& list : publisherLists_)
    {
        if (list.second.available &&
            list.second.expiration <= timeKeeper_.now())
            removePublisherList(list.first);
    }

    TrustChanges trustChanges;

    auto it = trustedKeys_.cbegin();
    while (it != trustedKeys_.cend())
    {
        if (! keyListings_.count(*it) ||
            validatorManifests_.revoked(*it))
        {
            trustChanges.removed.insert(calcNodeID(*it));
            it = trustedKeys_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto const& val : keyListings_)
    {
        if (! validatorManifests_.revoked(val.first) &&
                trustedKeys_.emplace(val.first).second)
            trustChanges.added.insert(calcNodeID(val.first));
    }

    JLOG (j_.debug()) <<
        trustedKeys_.size() << "  of " << keyListings_.size() <<
        " listed validators eligible for inclusion in the trusted set";

    quorum_ = calculateQuorum (trustedKeys_.size(), seenValidators.size());

    JLOG(j_.debug()) << "Using quorum of " << quorum_ << " for new set of "
                     << trustedKeys_.size() << " trusted validators ("
                     << trustChanges.added.size() << " added, "
                     << trustChanges.removed.size() << " removed)";

    if (trustedKeys_.size() < quorum_)
    {
        JLOG (j_.warn()) <<
            "New quorum of " << quorum_ <<
            " exceeds the number of trusted validators (" <<
            trustedKeys_.size() << ")";
    }

    return trustChanges;
}

} //涟漪
