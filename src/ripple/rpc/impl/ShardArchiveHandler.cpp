
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

#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/basics/Archive.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/rpc/ShardArchiveHandler.h>

#include <memory>

namespace ripple {
namespace RPC {

using namespace boost::filesystem;
using namespace std::chrono_literals;

ShardArchiveHandler::ShardArchiveHandler(Application& app, bool validate)
    : app_(app)
    , validate_(validate)
    , downloadDir_(get(app_.config().section(
        ConfigSection::shardDatabase()), "path", "") + "/download")
    , timer_(app_.getIOService())
    , j_(app.journal("ShardArchiveHandler"))
{
    assert(app_.getShardStore());
}

ShardArchiveHandler::~ShardArchiveHandler()
{
    timer_.cancel();
    for (auto const& ar : archives_)
        app_.getShardStore()->removePreShard(ar.first);

//删除临时根目录下载目录
    try
    {
        remove_all(downloadDir_);
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "exception: " << e.what();
    }
}

bool
ShardArchiveHandler::init()
{
    if (!app_.getShardStore())
    {
        JLOG(j_.error()) <<
            "No shard store available";
        return false;
    }
    if (downloader_)
    {
        JLOG(j_.error()) <<
            "Already initialized";
        return false;
    }

    try
    {
//从碰撞中清除残余物
        remove_all(downloadDir_);

//创建临时根下载目录
        create_directory(downloadDir_);
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "exception: " << e.what();
        return false;
    }

    downloader_ = std::make_shared<SSLHTTPDownloader>(
        app_.getIOService(), j_);
    return downloader_->init(app_.config());
}

bool
ShardArchiveHandler::add(std::uint32_t shardIndex, parsedURL&& url)
{
    assert(downloader_);
    auto const it {archives_.find(shardIndex)};
    if (it != archives_.end())
        return url == it->second;
    if (!app_.getShardStore()->prepareShard(shardIndex))
        return false;
    archives_.emplace(shardIndex, std::move(url));
    return true;
}

void
ShardArchiveHandler::next()
{
    assert(downloader_);

//检查所有档案是否完成
    if (archives_.empty())
        return;

//在根目录下创建临时存档目录
    auto const dstDir {
        downloadDir_ / std::to_string(archives_.begin()->first)};
    try
    {
        create_directory(dstDir);
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "exception: " << e.what();
        remove(archives_.begin()->first);
        return next();
    }

//下载存档文件
    auto const& url {archives_.begin()->second};
    downloader_->download(
        url.domain,
        std::to_string(url.port.get_value_or(443)),
        url.path,
        11,
        dstDir / "archive.tar.lz4",
        std::bind(&ShardArchiveHandler::complete,
            shared_from_this(), std::placeholders::_1));
}

std::string
ShardArchiveHandler::toString() const
{
    assert(downloader_);
    RangeSet<std::uint32_t> rs;
    for (auto const& ar : archives_)
        rs.insert(ar.first);
    return to_string(rs);
};

void
ShardArchiveHandler::complete(path dstPath)
{
    try
    {
        if (!is_regular_file(dstPath))
        {
            auto ar {archives_.begin()};
            JLOG(j_.error()) <<
                "Downloading shard id " << ar->first <<
                " URL " << ar->second.domain << ar->second.path;
            remove(ar->first);
            return next();
        }
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "exception: " << e.what();
        remove(archives_.begin()->first);
        return next();
    }

//在另一个线程中处理以不保留IO服务
    app_.getJobQueue().addJob(
        jtCLIENT, "ShardArchiveHandler",
        [=, dstPath = std::move(dstPath), ptr = shared_from_this()](Job&)
        {
//如果正在验证且未同步，则推迟并重试
            auto const mode {ptr->app_.getOPs().getOperatingMode()};
            if (ptr->validate_ && mode != NetworkOPs::omFULL)
            {
                timer_.expires_from_now(std::chrono::seconds{
                    (NetworkOPs::omFULL - mode) * 10});
                timer_.async_wait(
                    [=, dstPath = std::move(dstPath), ptr = std::move(ptr)]
                    (boost::system::error_code const& ec)
                    {
                        if (ec != boost::asio::error::operation_aborted)
                            ptr->complete(std::move(dstPath));
                    });
                return;
            }
            ptr->process(dstPath);
            ptr->next();
        });
}

void
ShardArchiveHandler::process(path const& dstPath)
{
    auto const shardIndex {archives_.begin()->first};
    auto const shardDir {dstPath.parent_path() / std::to_string(shardIndex)};
    try
    {
//解压缩并提取下载的文件
        extractTarLz4(dstPath, dstPath.parent_path());

//提取的根目录名必须与碎片索引匹配
        if (!is_directory(shardDir))
        {
            JLOG(j_.error()) <<
                "Shard " << shardIndex <<
                " mismatches archive shard directory";
            return remove(shardIndex);
        }
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "exception: " << e.what();
        return remove(shardIndex);
    }

//将碎片导入碎片存储
    if (!app_.getShardStore()->importShard(shardIndex, shardDir, validate_))
    {
        JLOG(j_.error()) <<
            "Importing shard " << shardIndex;
    }
    else
    {
        JLOG(j_.debug()) <<
            "Shard " << shardIndex << " downloaded and imported";
    }
    remove(shardIndex);
}

void
ShardArchiveHandler::remove(std::uint32_t shardIndex)
{
    app_.getShardStore()->removePreShard(shardIndex);
    archives_.erase(shardIndex);

    auto const dstDir {downloadDir_ / std::to_string(shardIndex)};
    try
    {
        remove_all(dstDir);
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "exception: " << e.what();
    }
}

} //RPC
} //涟漪
