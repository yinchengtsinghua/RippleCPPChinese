
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

#ifndef RIPPLE_PROTOCOL_INDEXES_H_INCLUDED
#define RIPPLE_PROTOCOL_INDEXES_H_INCLUDED

#include <ripple/protocol/Keylet.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/basics/base_uint.h>
#include <ripple/protocol/Book.h>
#include <cstdint>

namespace ripple {

//获取保存最后256个分类帐的节点的索引
uint256
getLedgerHashIndex ();

//获取包含256个分类帐的节点的索引，其中包括
//此分类帐的哈希（如果不是多个分类帐，则是它后面的第一个分类帐）
//256）。
uint256
getLedgerHashIndex (std::uint32_t desiredLedgerIndex);

//获取保存已启用修订的节点的索引
uint256
getLedgerAmendmentIndex ();

//获取保存费用计划的节点的索引
uint256
getLedgerFeeIndex ();

uint256
getAccountRootIndex (AccountID const& account);

uint256
getGeneratorIndex (AccountID const& uGeneratorID);

uint256
getBookBase (Book const& book);

uint256
getOfferIndex (AccountID const& account, std::uint32_t uSequence);

uint256
getOwnerDirIndex (AccountID const& account);

uint256
getDirNodeIndex (uint256 const& uDirRoot, const std::uint64_t uNodeIndex);

uint256
getQualityIndex (uint256 const& uBase, const std::uint64_t uNodeDir = 0);

uint256
getQualityNext (uint256 const& uBase);

//vvalco这个名字可能更好
std::uint64_t
getQuality (uint256 const& uBase);

uint256
getTicketIndex (AccountID const& account, std::uint32_t uSequence);

uint256
getRippleStateIndex (AccountID const& a, AccountID const& b, Currency const& currency);

uint256
getRippleStateIndex (AccountID const& a, Issue const& issue);

uint256
getSignerListIndex (AccountID const& account);

uint256
getCheckIndex (AccountID const& account, std::uint32_t uSequence);

uint256
getDepositPreauthIndex (AccountID const& owner, AccountID const& preauthorized);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*沃尔法科托多
    对于只接受uin256和
    只附上LedgerEntryType，我们可以评论一下
    接线员查看什么是中断，那些呼叫站点是
    将密钥作为
    参数，或具有存储keylet的数据成员。
**/


/*keylet计算功能。*/
namespace keylet {

/*会计根*/
struct account_t
{
    explicit account_t() = default;

    Keylet operator()(AccountID const& id) const;
};
static account_t const account {};

/*修正表*/
struct amendments_t
{
    explicit amendments_t() = default;

    Keylet operator()() const;
};
static amendments_t const amendments {};

/*可以在owner目录中的任何项目。*/
Keylet child (uint256 const& key);

/*跳表*/
struct skip_t
{
    explicit skip_t() = default;

    Keylet operator()() const;

    Keylet operator()(LedgerIndex ledger) const;
};
static skip_t const skip {};

/*分类帐费用*/
struct fees_t
{
    explicit fees_t() = default;

//vfalc这可能是constexpr
    Keylet operator()() const;
};
static fees_t const fees {};

/*订单簿的开头*/
struct book_t
{
    explicit book_t() = default;

    Keylet operator()(Book const& b) const;
};
static book_t const book {};

/*信任线*/
struct line_t
{
    explicit line_t() = default;

    Keylet operator()(AccountID const& id0,
        AccountID const& id1, Currency const& currency) const;

    Keylet operator()(AccountID const& id,
        Issue const& issue) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltRIPPLE_STATE, key };
    }
};
static line_t const line {};

/*客户的报价*/
struct offer_t
{
    explicit offer_t() = default;

    Keylet operator()(AccountID const& id,
        std::uint32_t seq) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltOFFER, key };
    }
};
static offer_t const offer {};

/*特定质量的初始目录页*/
struct quality_t
{
    explicit quality_t() = default;

    Keylet operator()(Keylet const& k,
        std::uint64_t q) const;
};
static quality_t const quality {};

/*下一个低质量的方向*/
struct next_t
{
    explicit next_t() = default;

    Keylet operator()(Keylet const& k) const;
};
static next_t const next {};

/*属于帐户的票据*/
struct ticket_t
{
    explicit ticket_t() = default;

    Keylet operator()(AccountID const& id,
        std::uint32_t seq) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltTICKET, key };
    }
};
static ticket_t const ticket {};

/*签字人名单*/
struct signers_t
{
    explicit signers_t() = default;

    Keylet operator()(AccountID const& id) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltSIGNER_LIST, key };
    }
};
static signers_t const signers {};

/*支票*/
struct check_t
{
    explicit check_t() = default;

    Keylet operator()(AccountID const& id,
        std::uint32_t seq) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltCHECK, key };
    }
};
static check_t const check {};

/*存款人*/
struct depositPreauth_t
{
    explicit depositPreauth_t() = default;

    Keylet operator()(AccountID const& owner,
        AccountID const& preauthorized) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltDEPOSIT_PREAUTH, key };
    }
};
static depositPreauth_t const depositPreauth {};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*任何分类账分录*/
Keylet unchecked(uint256 const& key);

/*帐户目录的根页*/
Keylet ownerDir (AccountID const& id);

/*目录中的页面*/
/*@ {*/
Keylet page (uint256 const& root, std::uint64_t index);
Keylet page (Keylet const& root, std::uint64_t index);
/*@ }*/

//贬低
inline
Keylet page (uint256 const& key)
{
    return { ltDIR_NODE, key };
}

/*托管条目*/
Keylet
escrow (AccountID const& source, std::uint32_t seq);

/*支付渠道*/
Keylet
payChan (AccountID const& source, AccountID const& dst, std::uint32_t seq);

} //小键盘

}

#endif
