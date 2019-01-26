
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
    版权所有（c）2016 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_MISC_VALIDATORSITE_H_INCLUDED
#define RIPPLE_APP_MISC_VALIDATORSITE_H_INCLUDED

#include <ripple/app/misc/ValidatorList.h>
#include <ripple/app/misc/detail/Work.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/json/json_value.h>
#include <boost/asio.hpp>
#include <mutex>
#include <memory>

namespace ripple {

/*
    验证站点
    ----------------

    此类管理用于获取
    最新发布的推荐验证程序列表。

    列表是定期获取的。
    提取的列表应为JSON格式，并包含以下内容
    领域：

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

    @li@c“刷新间隔”（可选）
**/

class ValidatorSite
{
    friend class Work;

private:
    using error_code = boost::system::error_code;
    using clock_type = std::chrono::system_clock;

    struct Site
    {
        struct Status
        {
            clock_type::time_point refreshed;
            ListDisposition disposition;
            std::string message;
        };

        struct Resource
        {
            explicit Resource(std::string uri_);
            const std::string uri;
            parsedURL pUrl;
        };

        explicit Site(std::string uri);

///config加载的原始URI
        std::shared_ptr<Resource> loadedResource;

///the resource to request at<timer>
///间隔。与加载的资源相同
///永久性再贴现除外。
        std::shared_ptr<Resource> startingResource;

///正在请求的活动资源。
///与StartingResource相同，但
///当我们得到临时重定向时
        std::shared_ptr<Resource> activeResource;

        unsigned short redirCount;
        std::chrono::minutes refreshInterval;
        clock_type::time_point nextRefresh;
        boost::optional<Status> lastRefreshStatus;
    };

    boost::asio::io_service& ios_;
    ValidatorList& validators_;
    beast::Journal j_;
    std::mutex mutable sites_mutex_;
    std::mutex mutable state_mutex_;

    std::condition_variable cv_;
    std::weak_ptr<detail::Work> work_;
    boost::asio::basic_waitable_timer<clock_type> timer_;

//当前正在从网站提取列表
    std::atomic<bool> fetching_;

//应提取一个或多个列表
    std::atomic<bool> pending_;
    std::atomic<bool> stopping_;

//为获取列表配置的URI列表
    std::vector<Site> sites_;

public:
    ValidatorSite (
        boost::asio::io_service& ios,
        ValidatorList& validators,
        beast::Journal j);
    ~ValidatorSite ();

    /*加载配置的站点URI。

        @param siteuris要获取已发布验证程序列表的uris列表

        @标准螺纹安全

        可以同时调用

        @return`false`如果条目无效或不可分析
    **/

    bool
    load (
        std::vector<std::string> const& siteURIs);

    /*开始从网站获取列表

        如果列表提取已经开始，则此操作不起作用。

        @标准螺纹安全

        可以同时调用
    **/

    void
    start ();

    /*等待当前从站点提取完成

        @标准螺纹安全

        可以同时调用
    **/

    void
    join ();

    /*停止从网站获取列表

        在列表提取停止之前，此块一直阻止

        @标准螺纹安全

        可以同时调用
    **/

    void
    stop ();

    /*返回已配置验证程序站点的JSON表示形式
     **/

    Json::Value
    getJson() const;

private:
///queue下一个要提取的站点
    void
    setTimer ();

///获取时间已到的站点
    void
    onTimer (
        std::size_t siteIdx,
        error_code const& ec);

///store从站点获取的最新列表
    void
    onSiteFetch (
        boost::system::error_code const& ec,
        detail::response_type&& res,
        std::size_t siteIdx);

///store从任何位置提取的最新列表
    void
    onTextFetch(
        boost::system::error_code const& ec,
        std::string const& res,
        std::size_t siteIdx);

///initiate对给定资源的请求。
///需要锁定站点
    void
    makeRequest (
        std::shared_ptr<Site::Resource> resource,
        std::size_t siteIdx,
        std::lock_guard<std::mutex>& lock);

///parse来自验证程序列表站点的json响应。
///需要锁定站点
    void
    parseJsonResponse (
        std::string const& res,
        std::size_t siteIdx,
        std::lock_guard<std::mutex>& lock);

///解释重定向响应。
///需要锁定站点
    std::shared_ptr<Site::Resource>
    processRedirect (
        detail::response_type& res,
        std::size_t siteIdx,
        std::lock_guard<std::mutex>& lock);
};

} //涟漪

#endif
