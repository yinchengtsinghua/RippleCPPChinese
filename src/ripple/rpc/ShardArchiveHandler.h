
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

#ifndef RIPPLE_RPC_SHARDARCHIVEHANDLER_H_INCLUDED
#define RIPPLE_RPC_SHARDARCHIVEHANDLER_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/net/SSLHTTPDownloader.h>

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/filesystem.hpp>

namespace ripple {
namespace RPC {

/*处理下载并导入一个或多个碎片存档。*/
class ShardArchiveHandler
    : public std::enable_shared_from_this <ShardArchiveHandler>
{
public:
    ShardArchiveHandler() = delete;
    ShardArchiveHandler(ShardArchiveHandler const&) = delete;
    ShardArchiveHandler& operator= (ShardArchiveHandler&&) = delete;
    ShardArchiveHandler& operator= (ShardArchiveHandler const&) = delete;

    /*@param验证是否应使用网络验证碎片数据。*/
    ShardArchiveHandler(Application& app, bool validate);

    ~ShardArchiveHandler();

    /*初始化处理程序。
        @return`true`如果初始化成功。
    **/

    bool
    init();

    /*对要下载和导入的存档进行排队。
        @param shardindindex要导入的碎片的索引。
        @param url存档的位置。
        @return`true`如果添加成功。
    **/

    bool
    add(std::uint32_t shardIndex, parsedURL&& url);

    /*开始下载和导入排队的存档。*/
    void
    next();

    /*返回排队存档的索引。
        @返回排队存档的索引。
    **/

    std::string
    toString() const;

private:
//下载程序用来通知下载完成的回调。
    void
    complete(boost::filesystem::path dstPath);

//提取存档并导入碎片的作业。
    void
    process(boost::filesystem::path const& dstPath);

    void
    remove(std::uint32_t shardIndex);

    Application& app_;
    std::shared_ptr<SSLHTTPDownloader> downloader_;
    std::map<std::uint32_t, parsedURL> archives_;
    bool const validate_;
    boost::filesystem::path const downloadDir_;
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
    beast::Journal j_;
};

} //RPC
} //涟漪

#endif
