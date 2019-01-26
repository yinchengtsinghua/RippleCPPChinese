
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
#include <ripple/basics/strHex.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {

//{
//分类帐哈希：<ledger>
//分类帐索引：<ledger索引>
//}
Json::Value doLedgerHeader (RPC::Context& context)
{
    std::shared_ptr<ReadView const> lpLedger;
    auto jvResult = RPC::lookupLedger (lpLedger, context);

    if (!lpLedger)
        return jvResult;

    Serializer  s;
    addRaw (lpLedger->info(), s);
    jvResult[jss::ledger_data] = strHex (s.peekData ());

//此信息未经验证：仅当他们信任时才应使用它
//我们。
    addJson (jvResult, {*lpLedger, 0});

    return jvResult;
}


} //涟漪
