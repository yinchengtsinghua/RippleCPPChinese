
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
    版权所有（c）2012=2014 Ripple Labs Inc.

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

#ifndef RIPPLE_RPC_RPCHELPERS_H_INCLUDED
#define RIPPLE_RPC_RPCHELPERS_H_INCLUDED

#include <ripple/beast/core/SemanticVersion.h>
#include <ripple/ledger/TxMeta.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/rpc/impl/Tuning.h>
#include <ripple/rpc/Status.h>
#include <boost/optional.hpp>

namespace Json {
class Value;
}

namespace ripple {

class ReadView;
class Transaction;

namespace RPC {

struct Context;

/*从帐户ID或公钥获取帐户ID。*/
boost::optional<AccountID>
accountFromStringStrict (std::string const&);

//-->sternt:公钥、帐户ID或常规种子。
//-->bstrict:仅允许帐户ID或公钥。
//
//返回一个json:：objectValue，其中包含错误信息（如果有）。
Json::Value
accountFromString (AccountID& result, std::string const& strIdent,
    bool bStrict = false);

/*收集分类帐中帐户的所有对象。
    @param ledger ledger用于搜索帐户对象。
    @param account accountid查找对象。
    @param type收集此类型的对象。ltinvalid收集所有类型。
    @param dirindex开始从此目录收集帐户对象。
    @param entryindex开始从此目录节点收集对象。
    @param限制要查找的最大对象数。
    @param jvresult保存请求对象的JSON结果。
**/

bool
getAccountObjects (ReadView const& ledger, AccountID const& account,
    LedgerEntryType const type, uint256 dirIndex, uint256 const& entryIndex,
    std::uint32_t const limit, Json::Value& jvResult);

/*从请求中查找分类帐，并将JSON：：result填入
    错误，或表示分类帐的数据。

    如果返回值没有错误，则分类帐指针将
    已经填满了。
**/

Json::Value
lookupLedger (std::shared_ptr<ReadView const>&, Context&);

/*从请求中查找分类帐，并用数据填充json:：result
    表示分类帐。

    如果返回状态为“正常”，则分类帐指针将被填充。
**/

Status
lookupLedger (std::shared_ptr<ReadView const>&, Context&, Json::Value& result);

hash_set <AccountID>
parseAccountIds(Json::Value const& jvArray);

void
addPaymentDeliveredAmount(Json::Value&, Context&,
    std::shared_ptr<Transaction>, TxMeta::pointer);

/*注入描述分类账分录的JSON

    影响：
        将'sle'的JSON描述添加到'jv'。

        如果'sle'拥有帐户根目录，还将添加
        如果存在sfemailhash，则返回urlgravatar字段json。
**/

void
injectSLE(Json::Value& jv, SLE const& sle);

/*从上下文中检索限制值，或设置默认值-
    如果不是管理请求，则将限制限制限制为max和min。

    如果有错误，请将其作为JSON返回。
**/

boost::optional<Json::Value>
readLimitField(unsigned int& limit, Tuning::LimitRange const&, Context const&);

boost::optional<Seed>
getSeedFromRPC(Json::Value const& params, Json::Value& error);

boost::optional<Seed>
parseRippleLibSeed(Json::Value const& params);

std::pair<PublicKey, SecretKey>
keypairForSignature(Json::Value const& params, Json::Value& error);

extern beast::SemanticVersion const firstVersion;
extern beast::SemanticVersion const goodVersion;
extern beast::SemanticVersion const lastVersion;

template <class Object>
void
setVersion(Object& parent)
{
    auto&& object = addObject (parent, jss::version);
    object[jss::first] = firstVersion.print();
    object[jss::good] = goodVersion.print();
    object[jss::last] = lastVersion.print();
}

std::pair<RPC::Status, LedgerEntryType>
    chooseLedgerEntryType(Json::Value const& params);

} //RPC
} //涟漪

#endif
