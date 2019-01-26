
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

#include <ripple/app/main/Application.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/net/RPCErr.h>
#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/Handler.h>
#include <ripple/rpc/ShardArchiveHandler.h>

#include <boost/algorithm/string.hpp>

namespace ripple {

/*下载和导入碎片存档的rpc命令。
    {
      shards：[index:<integer>，url:<string>]
      validate:<bool>>/可选，默认为true
    }

    例子：
    {
      “command”：“下载碎片”，
      “碎片”：
        “index”：1，“url”：“https://domain.com/1.tar.lz4”，
        “index”：5，“url”：“https://domain.com/5.tar.lz4”
      ]
    }
**/

Json::Value
doDownloadShard(RPC::Context& context)
{
    if (context.role != Role::ADMIN)
        return rpcError(rpcNO_PERMISSION);

//必须配置碎片存储
    auto shardStore {context.app.getShardStore()};
    if (!shardStore)
        return rpcError(rpcNOT_ENABLED);

//拒绝已下载的请求
    if (shardStore->getNumPreShard())
        return rpcError(rpcTOO_BUSY);

    if (!context.params.isMember(jss::shards))
        return RPC::missing_field_error(jss::shards);
    if (!context.params[jss::shards].isArray() ||
        context.params[jss::shards].size() == 0)
    {
        return RPC::expected_field_error(
            std::string(jss::shards), "an array");
    }

//验证碎片
    static const std::string ext {".tar.lz4"};
    std::map<std::uint32_t, parsedURL> archives;
    for (auto& it : context.params[jss::shards])
    {
//验证索引
        if (!it.isMember(jss::index))
            return RPC::missing_field_error(jss::index);
        auto& jv {it[jss::index]};
        if (!(jv.isUInt() || (jv.isInt() && jv.asInt() >= 0)))
        {
            return RPC::expected_field_error(
                std::string(jss::index), "an unsigned integer");
        }

//验证URL
        if (!it.isMember(jss::url))
            return RPC::missing_field_error(jss::url);
        parsedURL url;
        if (!parseUrl(url, it[jss::url].asString()) ||
            url.domain.empty() || url.path.empty())
        {
            return RPC::invalid_field_error(jss::url);
        }
        if (url.scheme != "https")
            return RPC::expected_field_error(std::string(jss::url), "HTTPS");

//url必须指向lz4压缩tar存档“.tar.lz4”
        auto archiveName {url.path.substr(url.path.find_last_of("/\\") + 1)};
        if (archiveName.empty() || archiveName.size() <= ext.size())
        {
            return RPC::make_param_error("Invalid field '" +
                std::string(jss::url) + "', invalid archive name");
        }
        if (!boost::iends_with(archiveName, ext))
        {
            return RPC::make_param_error("Invalid field '" +
                std::string(jss::url) + "', invalid archive extension");
        }

//检查重复索引
        if (!archives.emplace(jv.asUInt(), std::move(url)).second)
        {
            return RPC::make_param_error("Invalid field '" +
                std::string(jss::index) + "', duplicate shard ids.");
        }
    }

    bool validate {true};
    if (context.params.isMember(jss::validate))
    {
        if (!context.params[jss::validate].isBool())
        {
            return RPC::expected_field_error(
                std::string(jss::validate), "a bool");
        }
        validate = context.params[jss::validate].asBool();
    }

//开始下载。处理程序在下载时保持自身活动。
    auto handler {
        std::make_shared<RPC::ShardArchiveHandler>(context.app, validate)};
    if (!handler->init())
        return rpcError(rpcINTERNAL);
    for (auto& ar : archives)
    {
        if (!handler->add(ar.first, std::move(ar.second)))
        {
            return RPC::make_param_error("Invalid field '" +
                std::string(jss::index) + "', shard id " +
                std::to_string(ar.first) + " exists or being acquired");
        }
    }
    handler->next();

    return RPC::makeObjectValue("downloading shards " + handler->toString());
}

} //涟漪
