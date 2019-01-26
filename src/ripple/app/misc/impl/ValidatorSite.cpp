
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

#include <ripple/app/misc/ValidatorList.h>
#include <ripple/app/misc/ValidatorSite.h>
#include <ripple/app/misc/detail/WorkFile.h>
#include <ripple/app/misc/detail/WorkPlain.h>
#include <ripple/app/misc/detail/WorkSSL.h>
#include <ripple/basics/base64.h>
#include <ripple/basics/Slice.h>
#include <ripple/json/json_reader.h>
#include <ripple/protocol/JsonFields.h>
#include <boost/regex.hpp>
#include <algorithm>

namespace ripple {

//默认网站查询频率-5分钟
auto constexpr DEFAULT_REFRESH_INTERVAL = std::chrono::minutes{5};
auto constexpr ERROR_RETRY_INTERVAL = std::chrono::seconds{30};
unsigned short constexpr MAX_REDIRECTS = 3;

ValidatorSite::Site::Resource::Resource (std::string uri_)
    : uri {std::move(uri_)}
{
    if (! parseUrl (pUrl, uri))
        throw std::runtime_error("URI '" + uri + "' cannot be parsed");

    if  (pUrl.scheme == "file")
    {
        if (!pUrl.domain.empty())
            throw std::runtime_error("file URI cannot contain a hostname");

#if _MSC_VER    //msvc:Windows路径需要前导/删除
        {
            if (pUrl.path[0] == '/')
                pUrl.path = pUrl.path.substr(1);

        }
#endif

        if (pUrl.path.empty())
            throw std::runtime_error("file URI must contain a path");
    }
    else if (pUrl.scheme == "http")
    {
        if (pUrl.domain.empty())
            throw std::runtime_error("http URI must contain a hostname");

        if (!pUrl.port)
            pUrl.port = 80;
    }
    else if (pUrl.scheme == "https")
    {
        if (pUrl.domain.empty())
            throw std::runtime_error("https URI must contain a hostname");

        if (!pUrl.port)
            pUrl.port = 443;
    }
    else
        throw std::runtime_error ("Unsupported scheme: '" + pUrl.scheme + "'");
}

ValidatorSite::Site::Site (std::string uri)
    : loadedResource {std::make_shared<Resource>(std::move(uri))}
    , startingResource {loadedResource}
    , redirCount {0}
    , refreshInterval {DEFAULT_REFRESH_INTERVAL}
    , nextRefresh {clock_type::now()}
{
}

ValidatorSite::ValidatorSite (
    boost::asio::io_service& ios,
    ValidatorList& validators,
    beast::Journal j)
    : ios_ (ios)
    , validators_ (validators)
    , j_ (j)
    , timer_ (ios_)
    , fetching_ (false)
    , pending_ (false)
    , stopping_ (false)
{
}

ValidatorSite::~ValidatorSite()
{
    std::unique_lock<std::mutex> lock{state_mutex_};
    if (timer_.expires_at() > clock_type::time_point{})
    {
        if (! stopping_)
        {
            lock.unlock();
            stop();
        }
        else
        {
            cv_.wait(lock, [&]{ return ! fetching_; });
        }
    }
}

bool
ValidatorSite::load (
    std::vector<std::string> const& siteURIs)
{
    JLOG (j_.debug()) <<
        "Loading configured validator list sites";

    std::lock_guard <std::mutex> lock{sites_mutex_};

    for (auto const& uri : siteURIs)
    {
        try
        {
            sites_.emplace_back (uri);
        }
        catch (std::exception const& e)
        {
            JLOG (j_.error()) <<
                "Invalid validator site uri: " << uri <<
                ": " << e.what();
            return false;
        }
    }

    JLOG (j_.debug()) <<
        "Loaded " << siteURIs.size() << " sites";

    return true;
}

void
ValidatorSite::start ()
{
    std::lock_guard <std::mutex> lock{state_mutex_};
    if (timer_.expires_at() == clock_type::time_point{})
        setTimer ();
}

void
ValidatorSite::join ()
{
    std::unique_lock<std::mutex> lock{state_mutex_};
    cv_.wait(lock, [&]{ return ! pending_; });
}

void
ValidatorSite::stop()
{
    std::unique_lock<std::mutex> lock{state_mutex_};
    stopping_ = true;
    cv_.wait(lock, [&]{ return ! fetching_; });

    if(auto sp = work_.lock())
        sp->cancel();

    error_code ec;
    timer_.cancel(ec);
    stopping_ = false;
    pending_ = false;
    cv_.notify_all();
}

void
ValidatorSite::setTimer ()
{
    std::lock_guard <std::mutex> lock{sites_mutex_};

    auto next = std::min_element(sites_.begin(), sites_.end(),
        [](Site const& a, Site const& b)
        {
            return a.nextRefresh < b.nextRefresh;
        });

    if (next != sites_.end ())
    {
        pending_ = next->nextRefresh <= clock_type::now();
        cv_.notify_all();
        timer_.expires_at (next->nextRefresh);
        timer_.async_wait (std::bind (&ValidatorSite::onTimer, this,
            std::distance (sites_.begin (), next), std::placeholders::_1));
    }
}

void
ValidatorSite::makeRequest (
    std::shared_ptr<Site::Resource> resource,
    std::size_t siteIdx,
    std::lock_guard<std::mutex>& lock)
{
    fetching_ = true;
    sites_[siteIdx].activeResource = resource;
    std::shared_ptr<detail::Work> sp;
    auto onFetch =
        [this, siteIdx] (error_code const& err, detail::response_type&& resp)
        {
            onSiteFetch (err, std::move(resp), siteIdx);
        };

    auto onFetchFile =
        [this, siteIdx] (error_code const& err, std::string const& resp)
    {
        onTextFetch (err, resp, siteIdx);
    };

    if (resource->pUrl.scheme == "https")
    {
        sp = std::make_shared<detail::WorkSSL>(
            resource->pUrl.domain,
            resource->pUrl.path,
            std::to_string(*resource->pUrl.port),
            ios_,
            j_,
            onFetch);
    }
    else if(resource->pUrl.scheme == "http")
    {
        sp = std::make_shared<detail::WorkPlain>(
            resource->pUrl.domain,
            resource->pUrl.path,
            std::to_string(*resource->pUrl.port),
            ios_,
            onFetch);
    }
    else
    {
        BOOST_ASSERT(resource->pUrl.scheme == "file");
        sp = std::make_shared<detail::WorkFile>(
            resource->pUrl.path,
            ios_,
            onFetchFile);
    }

    work_ = sp;
    sp->run ();
}

void
ValidatorSite::onTimer (
    std::size_t siteIdx,
    error_code const& ec)
{
    if (ec)
    {
//如果遇到任何错误，请重新启动计时器，除非该错误
//来自由于关机请求而中止的等待操作。
        if (ec != boost::asio::error::operation_aborted)
            onSiteFetch(ec, detail::response_type {}, siteIdx);
        return;
    }

    std::lock_guard <std::mutex> lock{sites_mutex_};
    sites_[siteIdx].nextRefresh =
        clock_type::now() + sites_[siteIdx].refreshInterval;

    assert(! fetching_);
    sites_[siteIdx].redirCount = 0;
    try
    {
//如果ssl init失败，workssl客户机可以抛出
        makeRequest(sites_[siteIdx].startingResource, siteIdx, lock);
    }
    catch (std::exception &)
    {
        onSiteFetch(
            boost::system::error_code {-1, boost::system::generic_category()},
            detail::response_type {},
            siteIdx);
    }
}

void
ValidatorSite::parseJsonResponse (
    std::string const& res,
    std::size_t siteIdx,
    std::lock_guard<std::mutex>& lock)
{
    Json::Reader r;
    Json::Value body;
    if (! r.parse(res.data(), body))
    {
        JLOG (j_.warn()) <<
            "Unable to parse JSON response from  " <<
            sites_[siteIdx].activeResource->uri;
        throw std::runtime_error{"bad json"};
    }

    if( ! body.isObject () ||
        ! body.isMember("blob")      || ! body["blob"].isString ()     ||
        ! body.isMember("manifest")  || ! body["manifest"].isString () ||
        ! body.isMember("signature") || ! body["signature"].isString() ||
        ! body.isMember("version")   || ! body["version"].isInt())
    {
        JLOG (j_.warn()) <<
            "Missing fields in JSON response from  " <<
            sites_[siteIdx].activeResource->uri;
        throw std::runtime_error{"missing fields"};
    }

    auto const disp = validators_.applyList (
        body["manifest"].asString (),
        body["blob"].asString (),
        body["signature"].asString(),
        body["version"].asUInt(),
        sites_[siteIdx].activeResource->uri);

    sites_[siteIdx].lastRefreshStatus.emplace(
        Site::Status{clock_type::now(), disp, ""});

    if (ListDisposition::accepted == disp)
    {
        JLOG (j_.debug()) <<
            "Applied new validator list from " <<
            sites_[siteIdx].activeResource->uri;
    }
    else if (ListDisposition::same_sequence == disp)
    {
        JLOG (j_.debug()) <<
            "Validator list with current sequence from " <<
            sites_[siteIdx].activeResource->uri;
    }
    else if (ListDisposition::stale == disp)
    {
        JLOG (j_.warn()) <<
            "Stale validator list from " <<
            sites_[siteIdx].activeResource->uri;
    }
    else if (ListDisposition::untrusted == disp)
    {
        JLOG (j_.warn()) <<
            "Untrusted validator list from " <<
            sites_[siteIdx].activeResource->uri;
    }
    else if (ListDisposition::invalid == disp)
    {
        JLOG (j_.warn()) <<
            "Invalid validator list from " <<
            sites_[siteIdx].activeResource->uri;
    }
    else if (ListDisposition::unsupported_version == disp)
    {
        JLOG (j_.warn()) <<
            "Unsupported version validator list from " <<
            sites_[siteIdx].activeResource->uri;
    }
    else
    {
        BOOST_ASSERT(false);
    }

    if (body.isMember ("refresh_interval") &&
        body["refresh_interval"].isNumeric ())
    {
//TODO:我们应该健全地检查/钳制这个值吗？
//为了一些合理的东西？
        sites_[siteIdx].refreshInterval =
            std::chrono::minutes{body["refresh_interval"].asUInt ()};
    }
}

std::shared_ptr<ValidatorSite::Site::Resource>
ValidatorSite::processRedirect (
    detail::response_type& res,
    std::size_t siteIdx,
    std::lock_guard<std::mutex>& lock)
{
    using namespace boost::beast::http;
    std::shared_ptr<Site::Resource> newLocation;
    if (res.find(field::location) == res.end() ||
        res[field::location].empty())
    {
        JLOG (j_.warn()) <<
            "Request for validator list at " <<
            sites_[siteIdx].activeResource->uri <<
            " returned a redirect with no Location.";
        throw std::runtime_error{"missing location"};
    }

    if (sites_[siteIdx].redirCount == MAX_REDIRECTS)
    {
        JLOG (j_.warn()) <<
            "Exceeded max redirects for validator list at " <<
            sites_[siteIdx].loadedResource->uri ;
        throw std::runtime_error{"max redirects"};
    }

    JLOG (j_.debug()) <<
        "Got redirect for validator list from " <<
        sites_[siteIdx].activeResource->uri <<
        " to new location " << res[field::location];

    try
    {
        newLocation = std::make_shared<Site::Resource>(
            std::string(res[field::location]));
        ++sites_[siteIdx].redirCount;
        if (newLocation->pUrl.scheme != "http" &&
            newLocation->pUrl.scheme != "https")
            throw std::runtime_error("invalid scheme in redirect " +
                newLocation->pUrl.scheme);
    }
    catch (std::exception &)
    {
        JLOG (j_.error()) <<
            "Invalid redirect location: " << res[field::location];
        throw;
    }
    return newLocation;
}

void
ValidatorSite::onSiteFetch(
    boost::system::error_code const& ec,
    detail::response_type&& res,
    std::size_t siteIdx)
{
    {
        std::lock_guard <std::mutex> lock_sites{sites_mutex_};
        auto onError = [&](std::string const& errMsg, bool retry)
        {
            sites_[siteIdx].lastRefreshStatus.emplace(
                Site::Status{clock_type::now(),
                            ListDisposition::invalid,
                            errMsg});
            if (retry)
                sites_[siteIdx].nextRefresh =
                        clock_type::now() + ERROR_RETRY_INTERVAL;
        };
        if (ec)
        {
            JLOG (j_.warn()) <<
                    "Problem retrieving from " <<
                    sites_[siteIdx].activeResource->uri <<
                    " " <<
                    ec.value() <<
                    ":" <<
                    ec.message();
            onError("fetch error", true);
        }
        else
        {
            try
            {
                using namespace boost::beast::http;
                switch (res.result())
                {
                case status::ok:
                    parseJsonResponse(res.body(), siteIdx, lock_sites);
                    break;
                case status::moved_permanently :
                case status::permanent_redirect :
                case status::found :
                case status::temporary_redirect :
                {
                    auto newLocation =
                        processRedirect (res, siteIdx, lock_sites);
                    assert(newLocation);
//对于perm重定向，还要更新我们的起始URI
                    if (res.result() == status::moved_permanently ||
                        res.result() == status::permanent_redirect)
                    {
                        sites_[siteIdx].startingResource = newLocation;
                    }
                    makeRequest(newLocation, siteIdx, lock_sites);
return; //我们还在抓取，所以跳过
//状态更新/通知如下
                }
                default:
                {
                    JLOG (j_.warn()) <<
                        "Request for validator list at " <<
                        sites_[siteIdx].activeResource->uri <<
                        " returned bad status: " <<
                        res.result_int();
                    onError("bad result code", true);
                }
                }
            }
            catch (std::exception& ex)
            {
                onError(ex.what(), false);
            }
        }
        sites_[siteIdx].activeResource.reset();
    }

    std::lock_guard <std::mutex> lock_state{state_mutex_};
    fetching_ = false;
    if (! stopping_)
        setTimer ();
    cv_.notify_all();
}

void
ValidatorSite::onTextFetch(
    boost::system::error_code const& ec,
    std::string const& res,
    std::size_t siteIdx)
{
    {
        std::lock_guard <std::mutex> lock_sites{sites_mutex_};
        try
        {
            if (ec)
            {
                JLOG (j_.warn()) <<
                    "Problem retrieving from " <<
                    sites_[siteIdx].activeResource->uri <<
                    " " <<
                    ec.value() <<
                    ":" <<
                    ec.message();
                throw std::runtime_error{"fetch error"};
            }

            parseJsonResponse(res, siteIdx, lock_sites);
        }
        catch (std::exception& ex)
        {
            sites_[siteIdx].lastRefreshStatus.emplace(
                Site::Status{clock_type::now(),
                ListDisposition::invalid,
                ex.what()});
        }
        sites_[siteIdx].activeResource.reset();
    }

    std::lock_guard <std::mutex> lock_state{state_mutex_};
    fetching_ = false;
    if (! stopping_)
        setTimer ();
    cv_.notify_all();
}

Json::Value
ValidatorSite::getJson() const
{
    using namespace std::chrono;
    using Int = Json::Value::Int;

    Json::Value jrr(Json::objectValue);
    Json::Value& jSites = (jrr[jss::validator_sites] = Json::arrayValue);
    {
        std::lock_guard<std::mutex> lock{sites_mutex_};
        for (Site const& site : sites_)
        {
            Json::Value& v = jSites.append(Json::objectValue);
            std::stringstream uri;
            uri << site.loadedResource->uri;
            if (site.loadedResource != site.startingResource)
                uri << " (redirects to " << site.startingResource->uri + ")";
            v[jss::uri] = uri.str();
            v[jss::next_refresh_time] = to_string(site.nextRefresh);
            if (site.lastRefreshStatus)
            {
                v[jss::last_refresh_time] =
                    to_string(site.lastRefreshStatus->refreshed);
                v[jss::last_refresh_status] =
                    to_string(site.lastRefreshStatus->disposition);
                if (! site.lastRefreshStatus->message.empty())
                    v[jss::last_refresh_message] =
                        site.lastRefreshStatus->message;
            }
            v[jss::refresh_interval_min] =
                static_cast<Int>(site.refreshInterval.count());
        }
    }
    return jrr;
}
} //涟漪
