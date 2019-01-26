
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

#ifndef RIPPLE_APP_MISC_VALIDATORLIST_H_INCLUDED
#define RIPPLE_APP_MISC_VALIDATORLIST_H_INCLUDED

#include <ripple/app/misc/Manifest.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/core/TimeKeeper.h>
#include <ripple/crypto/csprng.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/PublicKey.h>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/range/adaptors.hpp>
#include <mutex>
#include <shared_mutex>
#include <numeric>

namespace ripple {

enum class ListDisposition
{
//列表有效
    accepted = 0,

///与当前列表的顺序相同
    same_sequence,

//不支持/list版本
    unsupported_version,

///list由不受信任的发布者密钥签名
    untrusted,

///trusted publisher key，但seq太旧
    stale,

///格式或签名无效
    invalid
};

std::string
to_string(ListDisposition disposition);

/*更新验证程序列表后受信任节点中的更改
**/

struct TrustChanges
{
    explicit TrustChanges() = default;

    hash_set<NodeID> added;
    hash_set<NodeID> removed;
};

/*
    可信验证程序列表
    -----------------

    Rippled接受来自可信验证器的分类帐建议和验证
    节点。一旦收到的分类账数量
    分类帐的可信验证满足或超过仲裁值。

    此类管理本地波纹节点的验证公钥集
    信托基金。使用中列出的密钥填充受信任密钥列表。
    配置文件以及由受信任发布者签名的列表。这个
    在配置中指定了受信任的发布者公钥。

    新列表应包括以下数据：

    @li@c“blob”：base64编码的包含@c“sequence”的json字符串，@c
        “expiration”和@c“validators”字段。@c“expiration”包含
        Ripple时间戳（2000年1月1日起的秒数（00:00 UTC）），用于
        列表过期。@c“validators”包含一个对象数组，其中
        @c“验证\u公钥”和可选的@c“清单”字段。
        @c“validation_public_key”应该是十六进制编码的主公钥。
        @c“manifest”应该是base64编码的验证器清单。

    @li@c“manifest”：包含
        发布者的主密钥和签名公钥。

    @li@c“signature”：使用发布者的
        签名密钥。

    @li@c“版本”：1

    各个验证器列表由发布者单独存储。数
    还跟踪验证程序公钥出现的列表。

    可信验证公钥列表将在每个公钥的开头重置
    考虑到最新已知名单以及
    接收验证的验证程序集。上市的
    验证公钥将被无序排列，然后按列表数排序
    它们出现在。（洗牌通过
    相同数量的列表不具有确定性。）为计算仲裁值
    新的可信验证程序列表。如果只有一个列表，则所有列出的键
    是值得信赖的。否则，可信列表大小设置为仲裁的125%。
**/

class ValidatorList
{
    struct PublisherList
    {
        explicit PublisherList() = default;

        bool available;
        std::vector<PublicKey> list;
        std::size_t sequence;
        TimeKeeper::time_point expiration;
        std::string siteUri;
    };

    ManifestCache& validatorManifests_;
    ManifestCache& publisherManifests_;
    TimeKeeper& timeKeeper_;
    beast::Journal j_;
    std::shared_timed_mutex mutable mutex_;

    std::atomic<std::size_t> quorum_;
    boost::optional<std::size_t> minimumQuorum_;

//发布服务器主公钥存储的已发布列表
    hash_map<PublicKey, PublisherList> publisherLists_;

//列出的主公钥及其出现的列表数
    hash_map<PublicKey, std::size_t> keyListings_;

//受信任的主密钥的当前列表
    hash_set<PublicKey> trustedKeys_;

    PublicKey localPubKey_;

//当前支持的发布者列表格式版本
    static constexpr std::uint32_t requiredListVersion = 1;

public:
    ValidatorList (
        ManifestCache& validatorManifests,
        ManifestCache& publisherManifests,
        TimeKeeper& timeKeeper,
        beast::Journal j,
        boost::optional<std::size_t> minimumQuorum = boost::none);
    ~ValidatorList () = default;

    /*加载配置的受信任密钥。

        @param localsigningkey此节点的验证公钥

        @param config keys来自config的受信任密钥列表。每个条目包括
        一个base58编码的验证公钥，可选后跟一个
        评论。

        @param publisher keys可信发布者公钥列表。每个条目
        包含base58编码的帐户公钥。

        @标准螺纹安全

        可以同时调用

        @return`false`如果条目无效或不可分析
    **/

    bool
    load (
        PublicKey const& localSigningKey,
        std::vector<std::string> const& configKeys,
        std::vector<std::string> const& publisherKeys);

    /*应用已发布的公钥列表

        @param manifest base64编码的发布者密钥清单

        @param blob base64编码的包含已发布验证程序列表的json

        解码blob的@param签名

        已发布列表格式的@param版本

        @return`listposition:：accepted`如果成功应用了列表

        @标准螺纹安全

        可以同时调用
    **/

    ListDisposition
    applyList (
        std::string const& manifest,
        std::string const& blob,
        std::string const& signature,
        std::uint32_t version,
        std::string siteUri);

    /*更新受信任的节点

        根据最新清单重置受信任的节点，收到验证，
        和列表。

        @param seenvalidators已签名的验证程序的nodeid集
        最近收到的验证

        @return trustedkeychanges实例新信任或不信任
        节点标识。

        @标准螺纹安全

        可以同时调用
    **/

    TrustChanges
    updateTrusted (hash_set<NodeID> const& seenValidators);

    /*获取当前受信任密钥集的仲裁值

        法定人数是一个分类帐需要的最小验证数
        完全验证。当受信任的验证集
        更新密钥（在每轮共识开始时），主要是
        取决于受信任密钥的数量。

        @标准螺纹安全

        可以同时调用

        @返回仲裁值
    **/

    std::size_t
    quorum () const
    {
        return quorum_;
    }

    /*如果公钥受信任，则返回“true”

        @param标识验证公钥

        @标准螺纹安全

        可以同时调用
    **/

    bool
    trusted (
        PublicKey const& identity) const;

    /*如果公钥包含在任何列表中，则返回“true”

        @param标识验证公钥

        @标准螺纹安全

        可以同时调用
    **/

    bool
    listed (
        PublicKey const& identity) const;

    /*如果公钥受信任，则返回主公钥

        @param标识验证公钥

        @return`boost:：none`如果密钥不可信

        @标准螺纹安全

        可以同时调用
    **/

    boost::optional<PublicKey>
    getTrustedKey (
        PublicKey const& identity) const;

    /*如果公钥包含在任何列表中，则返回列出的主公钥

        @param标识验证公钥

        @return`boost:：none`如果未列出键

        @标准螺纹安全

        可以同时调用
    **/

    boost::optional<PublicKey>
    getListedKey (
        PublicKey const& identity) const;

    /*如果公钥是可信发布者，则返回“true”

        @param identity publisher公钥

        @标准螺纹安全

        可以同时调用
    **/

    bool
    trustedPublisher (
        PublicKey const& identity) const;

    /*返回本地验证程序公钥

        @标准螺纹安全

        可以同时调用
    **/

    PublicKey
    localPublicKey () const;

    /*对列出的每个验证公钥调用一次回调。

        @注意从调用validatorlist成员时未定义的行为结果
        在回调中

        传递到lambda的参数是：

        @li验证公钥

        @li一个布尔值，指示这是否是可信密钥

        @标准螺纹安全

        可以同时调用
    **/

    void
    for_each_listed (
        std::function<void(PublicKey const&, bool)> func) const;

    /*返回已配置的验证程序列表站点数。*/
    std::size_t
    count() const;

    /*返回验证程序列表将过期的时间

        @注意，如果发布的列表没有
        自过期后更新。它将是Boost：：如果有的话，没有。
        尚未提取配置的已发布列表。

        @标准螺纹安全
        可以同时调用
    **/

    boost::optional<TimeKeeper::time_point>
    expires() const;

    /*返回验证程序列表状态的JSON表示形式

        @标准螺纹安全
        可以同时调用
    **/

    Json::Value
    getJson() const;

private:
    /*检查受信任的有效已发布列表的响应

        @return`listposition:：accepted`如果可以应用列表

        @标准螺纹安全

        调用公共成员函数应锁定互斥体
    **/

    ListDisposition
    verify (
        Json::Value& list,
        PublicKey& pubKey,
        std::string const& manifest,
        std::string const& blob,
        std::string const& signature);

    /*停止信任发布者的密钥列表。

        @param publisher key发布者公钥

        @return`false`如果密钥不可信

        @标准螺纹安全

        调用公共成员函数应锁定互斥体
    **/

    bool
    removePublisherList (PublicKey const& publisherKey);

    /*返回可信验证程序集的仲裁

        @param可信的可信验证器密钥数

        @param看到已签名的可信验证器的数目
        最近收到的验证*/

    std::size_t
    calculateQuorum (
        std::size_t trusted, std::size_t seen);
};
} //涟漪

#endif
