
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

#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/impl/Tuning.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/Role.h>

namespace ripple {

//从分类帐获取状态节点
//输入：
//限制：整数，最大条目数
//标记：不透明，恢复点
//二进制：布尔值，格式
//类型：字符串//可选，默认为所有分类帐节点类型
//输出：
//分类帐哈希：所选分类帐的哈希
//分类帐索引：所选分类帐索引
//状态：状态节点数组
//标记：恢复点（如果有）
Json::Value doLedgerData (RPC::Context& context)
{
    std::shared_ptr<ReadView const> lpLedger;
    auto const& params = context.params;

    auto jvResult = RPC::lookupLedger(lpLedger, context);
    if (!lpLedger)
        return jvResult;

    bool const isMarker = params.isMember (jss::marker);
    ReadView::key_type key = ReadView::key_type();
    if (isMarker)
    {
        Json::Value const& jMarker = params[jss::marker];
        if (! (jMarker.isString () && key.SetHex (jMarker.asString ())))
            return RPC::expected_field_error (jss::marker, "valid");
    }

    bool const isBinary = params[jss::binary].asBool();

    int limit = -1;
    if (params.isMember (jss::limit))
    {
        Json::Value const& jLimit = params[jss::limit];
        if (!jLimit.isIntegral ())
            return RPC::expected_field_error (jss::limit, "integer");

        limit = jLimit.asInt ();
    }

    auto maxLimit = RPC::Tuning::pageLength(isBinary);
    if ((limit < 0) || ((limit > maxLimit) && (! isUnlimited (context.role))))
        limit = maxLimit;

    jvResult[jss::ledger_hash] = to_string (lpLedger->info().hash);
    jvResult[jss::ledger_index] = lpLedger->info().seq;

    if (! isMarker)
    {
//第一次查询时返回基本分类帐数据
        jvResult[jss::ledger] = getJson (
            LedgerFill (*lpLedger, isBinary ?
                LedgerFill::Options::binary : 0));
    }

    auto type = RPC::chooseLedgerEntryType(params);
    if (type.first)
    {
        jvResult.clear();
        type.first.inject(jvResult);
        return jvResult;
    }
    Json::Value& nodes = jvResult[jss::state];

    auto e = lpLedger->sles.end();
    for (auto i = lpLedger->sles.upper_bound(key); i != e; ++i)
    {
        auto sle = lpLedger->read(keylet::unchecked((*i)->key()));
        if (limit-- <= 0)
        {
//在当前键之前停止处理。
            auto k = sle->key();
            jvResult[jss::marker] = to_string(--k);
            break;
        }

        if (type.second == ltINVALID || sle->getType () == type.second)
        {
            if (isBinary)
            {
                Json::Value& entry = nodes.append (Json::objectValue);
                entry[jss::data] = serializeHex(*sle);
                entry[jss::index] = to_string(sle->key());
            }
            else
            {
                Json::Value& entry = nodes.append (sle->getJson (0));
                entry[jss::index] = to_string(sle->key());
            }
        }
    }

    return jvResult;
}

} //涟漪
