
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

#ifndef RIPPLE_APP_MISC_MANIFEST_H_INCLUDED
#define RIPPLE_APP_MISC_MANIFEST_H_INCLUDED

#include <ripple/basics/UnorderedContainers.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>
#include <string>

namespace ripple {

/*
    验证程序密钥清单
    -----------------

    假设安装在Ripple验证器上的密钥被破坏。不是
    只需要在每个验证器上生成和安装新的密钥对，
    每个涟漪需要用新的公钥更新其配置，以及
    在完成之前容易受到伪造的验证签名的攻击。这个
    解决方案是一个新的间接层：主密钥
    限制性访问控制用于签署“清单”：本质上，是指
    证书，包括主公钥，用于
    验证验证（将由其秘密副本签名），a
    序列号和数字签名。

    清单有两种序列化形式：一种包括数字
    签名和不存在的签名之间存在明显的因果关系
    无签名的（后一个）表单之间的关系，签名
    包括签名的（以前的）表格。在
    换句话说，消息不能包含自身的签名。代码
    下面存储一个包含签名的序列化清单，以及
    当需要验证时，动态生成无签名表单
    签名。

    manifestcache存储的实例，用于每个受信任的验证器，（a）其
    掌握公共密钥，以及（b）它拥有的所有有效清单中最高级的
    为那个验证器看到，如果有的话。启动时，[validator_token]配置
    条目（包含此验证器的清单）被解码并
    添加到清单缓存。其他舱单加上“八卦”
    从波纹状的同龄人那里收到。

    当临时密钥被破坏时，会创建一个新的签名密钥对，
    以及一份新的舱单（序列号更高）。
    由主密钥签名。当一个有波纹的对等机收到新的清单时，
    它用主密钥验证它并（假设它有效）丢弃
    旧的临时钥匙和存储新的钥匙。如果主密钥本身
    已损坏，序列号为0xffffff的清单将取代
    先前的清单并丢弃任何现有的临时密钥，而不存储
    新的。这些吊销清单是从
    [validator_key_revocation]配置条目以及作为八卦信息从
    同龄人。因为不会接受此主密钥的进一步清单
    （因为不可能有更高的序列号），并且没有打开签名密钥
    记录，不接受来自受损验证器的验证。
**/


//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

struct Manifest
{
    std::string serialized;
    PublicKey masterKey;
    PublicKey signingKey;
    std::uint32_t sequence;

    Manifest(std::string s, PublicKey pk, PublicKey spk, std::uint32_t seq);

    Manifest(Manifest&& other) = default;
    Manifest& operator=(Manifest&& other) = default;

    inline bool
    operator==(Manifest const& rhs) const
    {
        return sequence == rhs.sequence && masterKey == rhs.masterKey &&
               signingKey == rhs.signingKey && serialized == rhs.serialized;
    }

    inline bool
    operator!=(Manifest const& rhs) const
    {
        return !(*this == rhs);
    }

    /*来自序列化字符串的构造清单

        @param s序列化清单字符串

        @return`boost:：none`如果字符串无效

        @注意，这不会验证清单签名。
              应在构造清单之后调用“manifest:：verify”。
    **/

    static boost::optional<Manifest> make_Manifest(std::string s);

///returns`true`如果清单签名有效
    bool verify () const;

///返回序列化清单数据的哈希
    uint256 hash () const;

///returns`true`如果清单撤消主密钥
    bool revoked () const;

///返回清单签名
    Blob getSignature () const;

///返回清单主密钥签名
    Blob getMasterSignature () const;
};

struct ValidatorToken
{
    std::string manifest;
    SecretKey validationSecret;

private:
    ValidatorToken(std::string const& m, SecretKey const& valSecret);

public:
    ValidatorToken(ValidatorToken const&) = delete;
    ValidatorToken(ValidatorToken&& other) = default;

    static boost::optional<ValidatorToken>
    make_ValidatorToken(std::vector<std::string> const& tokenBlob);
};

enum class ManifestDisposition
{
///manifest有效
    accepted = 0,

///序列太旧
    stale,

///及时，但签名无效
    invalid
};

inline std::string
to_string(ManifestDisposition m)
{
    switch (m)
    {
        case ManifestDisposition::accepted:
            return "accepted";
        case ManifestDisposition::stale:
            return "stale";
        case ManifestDisposition::invalid:
            return "invalid";
        default:
            return "unknown";
    }
}

class DatabaseCon;

/*记住序列号最高的清单。*/
class ManifestCache
{
private:
    beast::Journal mutable j_;
    std::mutex apply_mutex_;
    std::mutex mutable read_mutex_;

    /*主公钥存储的活动清单。*/
    hash_map <PublicKey, Manifest> map_;

    /*主公钥由当前临时公钥存储。*/
    hash_map <PublicKey, PublicKey> signingToMasterKeys_;

public:
    explicit
    ManifestCache (beast::Journal j =
        beast::Journal(beast::Journal::getNullSink()))
        : j_ (j)
    {
    }

    /*返回主密钥的当前签名密钥。

        @param pk主公钥

        @return pk如果清单中没有已知的签名密钥

        @标准螺纹安全

        可以同时调用
    **/

    PublicKey
    getSigningKey (PublicKey const& pk) const;

    /*返回临时签名密钥的主公钥。

        @param pk临时签名公钥

        @return pk如果签名密钥不在有效清单中

        @标准螺纹安全

        可以同时调用
    **/

    PublicKey
    getMasterKey (PublicKey const& pk) const;

    /*如果主密钥已在清单中撤消，则返回“true”。

        @param pk主公钥

        @标准螺纹安全

        可以同时调用
    **/

    bool
    revoked (PublicKey const& pk) const;

    /*将清单添加到缓存。

        @param m要添加的清单

        @return`manifestDisposition:：accepted`如果成功，或者
                `stale`或'invalid`否则

        @标准螺纹安全

        可以同时调用
    **/

    ManifestDisposition
    applyManifest (
        Manifest m);

    /*使用数据库和配置中的清单填充清单缓存。

        @param dbcon数据库与dbtable的连接

        @param dbtable数据库表

        @param configmanifest base64本地节点的编码清单
            验证键

        @param configrevocation base64编码的验证程序密钥吊销
            从配置

        @标准螺纹安全

        可以同时调用
    **/

    bool load (
        DatabaseCon& dbCon, std::string const& dbTable,
        std::string const& configManifest,
        std::vector<std::string> const& configRevocation);

    /*用数据库中的清单填充清单缓存。

        @param dbcon数据库与dbtable的连接

        @param dbtable数据库表

        @标准螺纹安全

        可以同时调用
    **/

    void load (
        DatabaseCon& dbCon, std::string const& dbTable);

    /*将缓存的清单保存到数据库。

        @param dbcon数据库连接与'validatomanifests'表

        @param is trusted函数，如果清单可信，则返回true

        @标准螺纹安全

        可以同时调用
    **/

    void save (
        DatabaseCon& dbCon, std::string const& dbTable,
        std::function <bool (PublicKey const&)> isTrusted);

    /*对每个填充的清单调用一次回调。

        @注意从调用manifestcache成员时未定义的行为结果
        在回调中

        为每个清单调用了@param f函数

        @标准螺纹安全

        可以同时调用
    **/

    template <class Function>
    void
    for_each_manifest(Function&& f) const
    {
        std::lock_guard<std::mutex> lock{read_mutex_};
        for (auto const& m : map_)
        {
            f(m.second);
        }
    }

    /*对每个填充的清单调用一次回调。

        @注意从调用manifestcache成员时未定义的行为结果
        在回调中

        以最大次数f调用的@param pf pre函数
            调用（用于内存分配）

        为每个清单调用了@param f函数

        @标准螺纹安全

        可以同时调用
    **/

    template <class PreFun, class EachFun>
    void
    for_each_manifest(PreFun&& pf, EachFun&& f) const
    {
        std::lock_guard<std::mutex> lock{read_mutex_};
        pf(map_.size ());
        for (auto const& m : map_)
        {
            f(m.second);
        }
    }
};

} //涟漪

#endif
