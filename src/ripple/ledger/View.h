
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

#ifndef RIPPLE_LEDGER_VIEW_H_INCLUDED
#define RIPPLE_LEDGER_VIEW_H_INCLUDED

#include <ripple/ledger/ApplyView.h>
#include <ripple/ledger/OpenView.h>
#include <ripple/ledger/RawView.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/Rate.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/TER.h>
#include <ripple/core/Config.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>
#include <functional>
#include <map>
#include <memory>
#include <utility>

#include <vector>

namespace ripple {

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//观察员
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*控制冻结账户余额的处理*/
enum FreezeHandling
{
    fhIGNORE_FREEZE,
    fhZERO_IF_FROZEN
};

bool
isGlobalFrozen (ReadView const& view,
    AccountID const& issuer);

bool
isFrozen (ReadView const& view, AccountID const& account,
    Currency const& currency, AccountID const& issuer);

//返回一个帐户可以在不负债的情况下花费的金额。
//
//<--saamount：账户持有的货币金额。可能是否定的。
STAmount
accountHolds (ReadView const& view,
    AccountID const& account, Currency const& currency,
        AccountID const& issuer, FreezeHandling zeroIfFrozen,
              beast::Journal j);

STAmount
accountFunds (ReadView const& view, AccountID const& id,
    STAmount const& saDefault, FreezeHandling freezeHandling,
        beast::Journal j);

//返回帐户的流动性（未保留）xrp。一般比较喜欢
//在此接口上调用accountholds（）。但是这个接口
//允许调用方临时调整所有者计数
//必要的。
//
//@param ownercountadj正数加计数，负数减计数。
XRPAmount
xrpLiquid (ReadView const& view, AccountID const& id,
    std::int32_t ownerCountAdj, beast::Journal j);

/*循环访问帐户所有者目录中的所有项。*/
void
forEachItem (ReadView const& view, AccountID const& id,
    std::function<void (std::shared_ptr<SLE const> const&)> f);

/*在所有者目录中的一个项目之后迭代所有项目。
    @param在要开始的项的键之后
    @param提示目录页包含“after”
    @param限制返回的最大项目数
    
**/

bool
forEachItemAfter (ReadView const& view, AccountID const& id,
    uint256 const& after, std::uint64_t const hint,
        unsigned int limit, std::function<
            bool (std::shared_ptr<SLE const> const&)> f);

Rate
transferRate (ReadView const& view,
    AccountID const& issuer);

/*如果目录为空，则返回“true”
    @param key目录的键
**/

bool
dirIsEmpty (ReadView const& view,
    Keylet const& k);

//返回第一个条目并前进udirentry。
//<--是的，如果有下一个条目。
//vfalc使用迭代器修复这些笨拙的例程
bool
cdirFirst (ReadView const& view,
uint256 const& uRootIndex,  //>目录的根目录。
std::shared_ptr<SLE const>& sleNode,      //<->当前节点
unsigned int& uDirEntry,    //<下一个条目
uint256& uEntryIndex,       //<--条目（如果可用）。否则，为零。
    beast::Journal j);

//返回当前条目并前进udirentry。
//<--是的，如果有下一个条目。
//vfalc使用迭代器修复这些笨拙的例程
bool
cdirNext (ReadView const& view,
uint256 const& uRootIndex,  //-->目录根目录
std::shared_ptr<SLE const>& sleNode,      //<->当前节点
unsigned int& uDirEntry,    //<>下一个条目
uint256& uEntryIndex,       //<--条目（如果可用）。否则，为零。
    beast::Journal j);

//返回已启用修订的列表
std::set <uint256>
getEnabledAmendments (ReadView const& view);

//返回已获得多数通过的修正案地图
using majorityAmendments_t = std::map <uint256, NetClock::time_point>;
majorityAmendments_t
getMajorityAmendments (ReadView const& view);

/*按顺序返回分类帐的哈希。
    通过查找“跳过列表”来检索哈希。
    在通过的分类帐中。因为跳过列表是有限的
    如果请求的分类帐序列号为
    超出了跳过中表示的分类帐范围
    列表，然后返回boost：：none。
    @返回分类帐的哈希值
            给定序列号或boost:：none。
**/

boost::optional<uint256>
hashOfSeq (ReadView const& ledger, LedgerIndex seq,
    beast::Journal journal);

/*找到一个分类帐索引，我们可以很容易地从中获得所需的分类帐

    我们返回的索引应满足两个要求：
        1）必须是具有分类账哈希的分类账的索引。
            我们正在寻找。这意味着它的序列必须等于
            大于我们想要的序列，但不大于256
            因为每个分类帐都包含前256个分类帐的散列。

        2）它的哈希值必须便于我们找到。这意味着它必须是0 mod 256
            因为每一本这样的分类账都永久地被记录在分类账上。
            我们可以通过跳过列表轻松检索的页面。
**/

inline
LedgerIndex
getCandidateLedger (LedgerIndex requested)
{
    return (requested + 255) & (~255);
}

/*如果测试分类帐可证明不兼容，则返回false
    使用有效的分类账，也就是说，他们不可能
    两者都有效。如果您有两个分类账，请使用第一个表格，
    如果您还没有获得有效的分类帐，请使用第二张表格
**/

bool areCompatible (ReadView const& validLedger, ReadView const& testLedger,
    beast::Journal::Stream& s, const char* reason);

bool areCompatible (uint256 const& validHash, LedgerIndex validIndex,
    ReadView const& testLedger, beast::Journal::Stream& s, const char* reason);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//修饰语
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*向上或向下调整所有者计数。*/
void
adjustOwnerCount (ApplyView& view,
    std::shared_ptr<SLE> const& sle,
        std::int32_t amount, beast::Journal j);

//返回第一个条目并前进udirentry。
//<--是的，如果有下一个条目。
//vfalc使用迭代器修复这些笨拙的例程
bool
dirFirst (ApplyView& view,
uint256 const& uRootIndex,  //>目录的根目录。
std::shared_ptr<SLE>& sleNode,      //<->当前节点
unsigned int& uDirEntry,    //<下一个条目
uint256& uEntryIndex,       //<--条目（如果可用）。否则，为零。
    beast::Journal j);

//返回当前条目并前进udirentry。
//<--是的，如果有下一个条目。
//vfalc使用迭代器修复这些笨拙的例程
bool
dirNext (ApplyView& view,
uint256 const& uRootIndex,  //-->目录根目录
std::shared_ptr<SLE>& sleNode,      //<->当前节点
unsigned int& uDirEntry,    //<>下一个条目
uint256& uEntryIndex,       //<--条目（如果可用）。否则，为零。
    beast::Journal j);

std::function<void (SLE::ref)>
describeOwnerDir(AccountID const& account);

//贬低
boost::optional<std::uint64_t>
dirAdd (ApplyView& view,
    Keylet const&                       uRootIndex,
    uint256 const&                      uLedgerIndex,
    bool                                strictOrder,
    std::function<void (SLE::ref)>      fDescriber,
    beast::Journal j);

//vfalc注意两个stamount参数应该
//是“数量”，一个单位减去数字。
//
/*创建信任行

    这可以设置初始余额。
**/

TER
trustCreate (ApplyView& view,
    const bool      bSrcHigh,
    AccountID const&  uSrcAccountID,
    AccountID const&  uDstAccountID,
uint256 const&  uIndex,             //-->Ripple状态条目
SLE::ref        sleAccount,         //-->正在设置的帐户。
const bool      bAuth,              //-->授权帐户。
const bool      bNoRipple,          //-->其他人无法通过
const bool      bFreeze,            //>资金不能离开
STAmount const& saBalance,          //-->正在设置的帐户余额。
//颁发者应为noaccount（）。
STAmount const& saLimit,            //>正在设置的帐户限制。
//颁发者应该是正在设置的帐户。
    std::uint32_t uSrcQualityIn,
    std::uint32_t uSrcQualityOut,
    beast::Journal j);

TER
trustDelete (ApplyView& view,
    std::shared_ptr<SLE> const& sleRippleState,
        AccountID const& uLowAccountID,
            AccountID const& uHighAccountID,
                beast::Journal j);

/*删除报价。

    要求：
        传递的“sle”是从以前的
        调用View.Peek（）。
**/

TER
offerDelete (ApplyView& view,
    std::shared_ptr<SLE> const& sle,
        beast::Journal j);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//
//货币转移
//

//直接发送，不收取费用：
//-偿还欠条和/或发送发送方自己的欠条。
//-创建所需的信任线。
//-->BCheckIssuer：通常要求涉及到颁发者。
TER
rippleCredit (ApplyView& view,
    AccountID const& uSenderID, AccountID const& uReceiverID,
    const STAmount & saAmount, bool bCheckIssuer,
    beast::Journal j);

TER
accountSend (ApplyView& view,
    AccountID const& from,
        AccountID const& to,
            const STAmount & saAmount,
                 beast::Journal j);

TER
issueIOU (ApplyView& view,
    AccountID const& account,
        STAmount const& amount,
            Issue const& issue,
                beast::Journal j);

TER
redeemIOU (ApplyView& view,
    AccountID const& account,
        STAmount const& amount,
            Issue const& issue,
                beast::Journal j);

TER
transferXRP (ApplyView& view,
    AccountID const& from,
        AccountID const& to,
            STAmount const& amount,
                beast::Journal j);

NetClock::time_point const& fix1141Time ();
bool fix1141 (NetClock::time_point const closeTime);

NetClock::time_point const& fix1274Time ();
bool fix1274 (NetClock::time_point const closeTime);

NetClock::time_point const& fix1298Time ();
bool fix1298 (NetClock::time_point const closeTime);

NetClock::time_point const& fix1443Time ();
bool fix1443 (NetClock::time_point const closeTime);

NetClock::time_point const& fix1449Time ();
bool fix1449 (NetClock::time_point const closeTime);

} //涟漪

#endif
