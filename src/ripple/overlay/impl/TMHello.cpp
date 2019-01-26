
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

#include <ripple/overlay/impl/TMHello.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/basics/base64.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/beast/rfc2616.h>
#include <ripple/beast/core/LexicalCast.h>
#include <ripple/protocol/digest.h>
#include <boost/regex.hpp>
#include <algorithm>

//vfalc不应该包括openssl
//ssl的头还是什么东西？完成了吗？

namespace ripple {

/*散列来自SSL流的最新完成消息
    @param ssl会话以获取消息。
    @param hash检索到的哈希的缓冲区
                        消息将被保存。缓冲区必须至少为
                        64字节长。
    @param getmessage指向要调用以检索
                        已完成消息。这要么是：
                        `ssl_get_finished`或
                        `ssl_get_peer_finished`。
    @如果成功，返回'true'，否则返回'false'。
**/

static
boost::optional<base_uint<512>>
hashLastMessage (SSL const* ssl,
    size_t (*get)(const SSL *, void *buf, size_t))
{
    enum
    {
        sslMinimumFinishedLength = 12
    };
    unsigned char buf[1024];
    size_t len = get (ssl, buf, sizeof (buf));
    if(len < sslMinimumFinishedLength)
        return boost::none;
    base_uint<512> cookie;
    SHA512 (buf, len, cookie.data());
    return cookie;
}

boost::optional<uint256>
makeSharedValue (SSL* ssl, beast::Journal journal)
{
    auto const cookie1 = hashLastMessage(ssl, SSL_get_finished);
    if (!cookie1)
    {
        JLOG (journal.error()) << "Cookie generation: local setup not complete";
        return boost::none;
    }

    auto const cookie2 = hashLastMessage(ssl, SSL_get_peer_finished);
    if (!cookie2)
    {
        JLOG (journal.error()) << "Cookie generation: peer setup not complete";
        return boost::none;
    }

    auto const result = (*cookie1 ^ *cookie2);

//两条消息哈希到相同的值和cookie
//是0。不允许这样做。
    if (result == beast::zero)
    {
        JLOG(journal.error()) << "Cookie generation: identical finished messages";
        return boost::none;
    }

    return sha512Half (Slice (result.data(), result.size()));
}

protocol::TMHello
buildHello (
    uint256 const& sharedValue,
    beast::IP::Address public_ip,
    beast::IP::Endpoint remote,
    Application& app)
{
    protocol::TMHello h;

    auto const sig = signDigest (
        app.nodeIdentity().first,
        app.nodeIdentity().second,
        sharedValue);

    h.set_protoversion (to_packed (BuildInfo::getCurrentProtocol()));
    h.set_protoversionmin (to_packed (BuildInfo::getMinimumProtocol()));
    h.set_fullversion (BuildInfo::getFullVersionString ());
    h.set_nettime (app.timeKeeper().now().time_since_epoch().count());
    h.set_nodepublic (
        toBase58 (
            TokenType::NodePublic,
            app.nodeIdentity().first));
    h.set_nodeproof (sig.data(), sig.size());
    h.set_testnet (false);

    if (beast::IP::is_public (remote))
    {
//连接到公共IP
        h.set_remote_ip_str (remote.to_string());
        if (! public_ip.is_unspecified())
            h.set_local_ip_str (public_ip.to_string());
    }

//我们总是在问候语中把自己宣传成私人的。这个
//禁止旧的对等广告代码，并允许对等查找程序
//接管功能。
    h.set_nodeprivate (true);

    auto const closedLedger = app.getLedgerMaster().getClosedLedger();

    assert(! closedLedger->open());
//vfalc应该总是有一个关闭的分类帐
    if (closedLedger)
    {
        uint256 hash = closedLedger->info().hash;
        h.set_ledgerclosed (hash.begin (), hash.size ());
        hash = closedLedger->info().parentHash;
        h.set_ledgerprevious (hash.begin (), hash.size ());
    }

    return h;
}

void
appendHello (boost::beast::http::fields& h,
    protocol::TMHello const& hello)
{
//h.append（“协议版本”……

    h.insert ("Public-Key", hello.nodepublic());

    h.insert ("Session-Signature", base64_encode (
        hello.nodeproof()));

    if (hello.has_nettime())
        h.insert ("Network-Time", std::to_string (hello.nettime()));

    if (hello.has_ledgerindex())
        h.insert ("Ledger", std::to_string (hello.ledgerindex()));

    if (hello.has_ledgerclosed())
        h.insert ("Closed-Ledger", base64_encode (
            hello.ledgerclosed()));

    if (hello.has_ledgerprevious())
        h.insert ("Previous-Ledger", base64_encode (
            hello.ledgerprevious()));

    if (hello.has_local_ip_str())
        h.insert ("Local-IP", hello.local_ip_str());

    if (hello.has_remote_ip())
        h.insert ("Remote-IP", hello.remote_ip_str());
}

std::vector<ProtocolVersion>
parse_ProtocolVersions(boost::beast::string_view const& value)
{
    static boost::regex re (
"^"                  //起点线
"RTXP/"              //字符串“rtxp/”
"([1-9][0-9]*)"      //数字（非零且无前导零）
"\\."                //一段时间
"(0|[1-9][0-9]*)"    //一个数字（除非正好是零，否则没有前导零）
"$"                  //字符串的结尾
        , boost::regex_constants::optimize);

    auto const list = beast::rfc2616::split_commas(value);
    std::vector<ProtocolVersion> result;
    for (auto const& s : list)
    {
        boost::smatch m;
        if (! boost::regex_match (s, m, re))
            continue;
        int major;
        int minor;
        if (! beast::lexicalCastChecked (
                major, std::string (m[1])))
            continue;
        if (! beast::lexicalCastChecked (
                minor, std::string (m[2])))
            continue;
        result.push_back (std::make_pair (major, minor));
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique (result.begin(), result.end()), result.end());
    return result;
}

boost::optional<protocol::TMHello>
parseHello (bool request, boost::beast::http::fields const& h, beast::Journal journal)
{
//tmhello中的协议版本已过时，
//它被标题中的值取代。
    protocol::TMHello hello;

    {
//要求的
        auto const iter = h.find ("Upgrade");
        if (iter == h.end())
            return boost::none;
        auto const versions = parse_ProtocolVersions(iter->value().to_string());
        if (versions.empty())
            return boost::none;
        hello.set_protoversion(
            (safe_cast<std::uint32_t>(versions.back().first) << 16) |
            (safe_cast<std::uint32_t>(versions.back().second)));
        hello.set_protoversionmin(
            (safe_cast<std::uint32_t>(versions.front().first) << 16) |
            (safe_cast<std::uint32_t>(versions.front().second)));
    }

    {
//要求的
        auto const iter = h.find ("Public-Key");
        if (iter == h.end())
            return boost::none;
        auto const pk = parseBase58<PublicKey>(
            TokenType::NodePublic, iter->value().to_string());
        if (!pk)
            return boost::none;
        hello.set_nodepublic (iter->value().to_string());
    }

    {
//要求的
        auto const iter = h.find ("Session-Signature");
        if (iter == h.end())
            return boost::none;
//TODO安全审查
        hello.set_nodeproof (base64_decode (iter->value().to_string()));
    }

    {
        auto const iter = h.find (request ?
            "User-Agent" : "Server");
        if (iter != h.end())
            hello.set_fullversion (iter->value().to_string());
    }

    {
        auto const iter = h.find ("Network-Time");
        if (iter != h.end())
        {
            std::uint64_t nettime;
            if (! beast::lexicalCastChecked(nettime, iter->value().to_string()))
                return boost::none;
            hello.set_nettime (nettime);
        }
    }

    {
        auto const iter = h.find ("Ledger");
        if (iter != h.end())
        {
            LedgerIndex ledgerIndex;
            if (! beast::lexicalCastChecked(ledgerIndex, iter->value().to_string()))
                return boost::none;
            hello.set_ledgerindex (ledgerIndex);
        }
    }

    {
        auto const iter = h.find ("Closed-Ledger");
        if (iter != h.end())
            hello.set_ledgerclosed (base64_decode (iter->value().to_string()));
    }

    {
        auto const iter = h.find ("Previous-Ledger");
        if (iter != h.end())
            hello.set_ledgerprevious (base64_decode (iter->value().to_string()));
    }

    {
        auto const iter = h.find ("Local-IP");
        if (iter != h.end())
        {
            boost::system::error_code ec;
            auto address =
                beast::IP::Address::from_string (iter->value().to_string(), ec);
            if (ec)
            {
                JLOG(journal.warn()) << "invalid Local-IP: "
                    << iter->value().to_string();
                return boost::none;
            }
            hello.set_local_ip_str(address.to_string());
        }
    }

    {
        auto const iter = h.find ("Remote-IP");
        if (iter != h.end())
        {
            boost::system::error_code ec;
            auto address =
                beast::IP::Address::from_string (iter->value().to_string(), ec);
            if (ec)
            {
                JLOG(journal.warn()) << "invalid Remote-IP: "
                    << iter->value().to_string();
                return boost::none;
            }
            hello.set_remote_ip_str(address.to_string());
        }
    }

    return hello;
}

boost::optional<PublicKey>
verifyHello (protocol::TMHello const& h,
    uint256 const& sharedValue,
    beast::IP::Address public_ip,
    beast::IP::Endpoint remote,
    beast::Journal journal,
    Application& app)
{
    if (h.has_nettime ())
    {
        auto const ourTime = app.timeKeeper().now().time_since_epoch().count();
        auto const minTime = ourTime - clockToleranceDeltaSeconds;
        auto const maxTime = ourTime + clockToleranceDeltaSeconds;

        if (h.nettime () > maxTime)
        {
            JLOG(journal.info()) <<
                "Clock for is off by +" << h.nettime() - ourTime;
            return boost::none;
        }

        if (h.nettime () < minTime)
        {
            JLOG(journal.info()) <<
                "Clock is off by -" << ourTime - h.nettime();
            return boost::none;
        }

        JLOG(journal.trace()) <<
            "Connect: time offset " <<
            safe_cast<std::int64_t>(ourTime) - h.nettime();
    }

    if (h.protoversionmin () > to_packed (BuildInfo::getCurrentProtocol()))
    {
        JLOG(journal.info()) <<
            "Hello: Disconnect: Protocol mismatch [" <<
            "Peer expects " << to_string (
                BuildInfo::make_protocol(h.protoversion())) <<
            " and we run " << to_string (
                BuildInfo::getCurrentProtocol()) << "]";
        return boost::none;
    }

    auto const publicKey = parseBase58<PublicKey>(
        TokenType::NodePublic, h.nodepublic());

    if (! publicKey)
    {
        JLOG(journal.info()) <<
            "Hello: Disconnect: Bad node public key.";
        return boost::none;
    }

    if (publicKeyType(*publicKey) != KeyType::secp256k1)
    {
        JLOG(journal.info()) <<
            "Hello: Disconnect: Unsupported public key type.";
        return boost::none;
    }

    if (*publicKey == app.nodeIdentity().first)
    {
        JLOG(journal.info()) <<
            "Hello: Disconnect: Self connection.";
        return boost::none;
    }

    if (! verifyDigest (*publicKey, sharedValue,
        makeSlice (h.nodeproof()), false))
    {
//无法验证它们是否具有所声明公钥的私钥。
        JLOG(journal.info()) <<
            "Hello: Disconnect: Failed to verify session.";
        return boost::none;
    }

    if (h.has_local_ip_str () &&
        is_public (remote))
    {
        boost::system::error_code ec;
        auto local_ip =
            beast::IP::Address::from_string (h.local_ip_str(), ec);
        if (ec)
        {
            JLOG(journal.warn()) << "invalid local-ip: " << h.local_ip_str();
            return boost::none;
        }

        if (remote.address() != local_ip)
        {
//遥控器要求我们确认连接来自正确的IP
            JLOG(journal.info()) <<
                "Hello: Disconnect: Peer IP is " << remote.address().to_string()
                << " not " << local_ip.to_string();
            return boost::none;
        }
    }

    if (h.has_remote_ip_str () &&
        is_public (remote) &&
        (! beast::IP::is_unspecified(public_ip)))
    {
        boost::system::error_code ec;
        auto remote_ip =
            beast::IP::Address::from_string (h.remote_ip_str(), ec);
        if (ec)
        {
            JLOG(journal.warn()) << "invalid remote-ip: " << h.remote_ip_str();
            return boost::none;
        }

        if (remote_ip != public_ip)
        {
//我们知道我们的公共IP和对等端从一些
//其他知识产权
            JLOG(journal.info()) <<
                "Hello: Disconnect: Our IP is " << public_ip.to_string()
                << " not " << remote_ip.to_string();
            return boost::none;
        }
    }

    return publicKey;
}

}
