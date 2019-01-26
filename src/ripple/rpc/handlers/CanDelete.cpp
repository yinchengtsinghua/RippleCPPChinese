
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

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/SHAMapStore.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/rpc/Context.h>
#include <ripple/beast/core/LexicalCast.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/format.hpp>

namespace ripple {

//可以删除[<ledgerid><ledgerhash>now always never]
Json::Value doCanDelete (RPC::Context& context)
{
    if (! context.app.getSHAMapStore().advisoryDelete())
        return RPC::make_error(rpcNOT_ENABLED);

    Json::Value ret (Json::objectValue);

    if (context.params.isMember(jss::can_delete))
    {
        Json::Value canDelete  = context.params.get(jss::can_delete, 0);
        std::uint32_t canDeleteSeq = 0;

        if (canDelete.isUInt())
        {
            canDeleteSeq = canDelete.asUInt();
        }
        else
        {
            std::string canDeleteStr = canDelete.asString();
            boost::to_lower (canDeleteStr);

            if (canDeleteStr.find_first_not_of ("0123456789") ==
                std::string::npos)
            {
                canDeleteSeq =
                        beast::lexicalCast <std::uint32_t>(canDeleteStr);
            }
            else if (canDeleteStr == "never")
            {
                canDeleteSeq = 0;
            }
            else if (canDeleteStr == "always")
            {
                canDeleteSeq = std::numeric_limits <std::uint32_t>::max();
            }
            else if (canDeleteStr == "now")
            {
                canDeleteSeq = context.app.getSHAMapStore().getLastRotated();
                if (!canDeleteSeq)
                    return RPC::make_error (rpcNOT_READY);
            }
            else if (canDeleteStr.size() == 64 &&
                canDeleteStr.find_first_not_of("0123456789abcdef") ==
                std::string::npos)
            {
                auto ledger = context.ledgerMaster.getLedgerByHash (
                    from_hex_text<uint256>(canDeleteStr));

                if (!ledger)
                    return RPC::make_error(rpcLGR_NOT_FOUND, "ledgerNotFound");

                canDeleteSeq = ledger->info().seq;
            }
            else
            {
                return RPC::make_error (rpcINVALID_PARAMS);
            }
        }

        ret[jss::can_delete] =
                context.app.getSHAMapStore().setCanDelete (canDeleteSeq);
    }
    else
    {
        ret[jss::can_delete] = context.app.getSHAMapStore().getCanDelete();
    }

    return ret;
}

} //涟漪
