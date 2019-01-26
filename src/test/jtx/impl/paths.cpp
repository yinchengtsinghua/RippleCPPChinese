
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

#include <ripple/app/paths/Pathfinder.h>
#include <test/jtx/paths.h>
#include <ripple/protocol/JsonFields.h>

namespace ripple {
namespace test {
namespace jtx {

void
paths::operator()(Env& env, JTx& jt) const
{
    auto& jv = jt.jv;
    auto const from = env.lookup(
        jv[jss::Account].asString());
    auto const to = env.lookup(
        jv[jss::Destination].asString());
    auto const amount = amountFromJson(
        sfAmount, jv[jss::Amount]);
    Pathfinder pf (
        std::make_shared<RippleLineCache>(env.current()),
            from, to, in_.currency, in_.account,
                amount, boost::none, env.app());
    if (! pf.findPaths(depth_))
        return;

    STPath fp;
    pf.computePathRanks (limit_);
    auto const found = pf.getBestPaths(
        limit_, fp, {}, in_.account);

//vfalc todo api允许调用者检查stpathset
//vfalc isdefault应重命名为empty（）。
    if (! found.isDefault())
        jv[jss::Paths] = found.getJson(0);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Json::Value&
path::create()
{
    return jv_.append(Json::objectValue);
}

void
path::append_one(Account const& account)
{
    auto& jv = create();
    jv["account"] = toBase58(account.id());
}

void
path::append_one(IOU const& iou)
{
    auto& jv = create();
    jv["currency"] = to_string(iou.issue().currency);
    jv["account"] = toBase58(iou.issue().account);
}

void
path::append_one(BookSpec const& book)
{
    auto& jv = create();
    jv["currency"] = to_string(book.currency);
    jv["issuer"] = toBase58(book.account);
}

void
path::operator()(Env& env, JTx& jt) const
{
    jt.jv["Paths"].append(jv_);
}

} //JTX
} //测试
} //涟漪
