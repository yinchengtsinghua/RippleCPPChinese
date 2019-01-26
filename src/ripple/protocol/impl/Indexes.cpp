
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

#include <ripple/protocol/digest.h>
#include <ripple/protocol/Indexes.h>
#include <boost/endian/conversion.hpp>
#include <cassert>

namespace ripple {

//获取保存最后256个分类帐的节点的索引
uint256
getLedgerHashIndex ()
{
    return sha512Half(std::uint16_t(spaceSkipList));
}

//获取包含256个分类帐的节点的索引，其中包括
//此分类帐的哈希（如果不是多个分类帐，则是它后面的第一个分类帐）
//256）。
uint256
getLedgerHashIndex (std::uint32_t desiredLedgerIndex)
{
    return sha512Half(
        std::uint16_t(spaceSkipList),
        std::uint32_t(desiredLedgerIndex >> 16));
}
//获取保存已启用修订的节点的索引
uint256
getLedgerAmendmentIndex ()
{
    return sha512Half(std::uint16_t(spaceAmendment));
}

//获取保存费用计划的节点的索引
uint256
getLedgerFeeIndex ()
{
    return sha512Half(std::uint16_t(spaceFee));
}

uint256
getAccountRootIndex (AccountID const& account)
{
    return sha512Half(
        std::uint16_t(spaceAccount),
        account);
}

uint256
getGeneratorIndex (AccountID const& uGeneratorID)
{
    return sha512Half(
        std::uint16_t(spaceGenerator),
        uGeneratorID);
}

uint256
getBookBase (Book const& book)
{
    assert (isConsistent (book));
//返回质量为0。
    return getQualityIndex(sha512Half(
        std::uint16_t(spaceBookDir),
        book.in.currency,
        book.out.currency,
        book.in.account,
        book.out.account));
}

uint256
getOfferIndex (AccountID const& account, std::uint32_t uSequence)
{
    return sha512Half(
        std::uint16_t(spaceOffer),
        account,
        std::uint32_t(uSequence));
}

uint256
getOwnerDirIndex (AccountID const& account)
{
    return sha512Half(
        std::uint16_t(spaceOwnerDir),
        account);
}


uint256
getDirNodeIndex (uint256 const& uDirRoot, const std::uint64_t uNodeIndex)
{
    if (uNodeIndex == 0)
        return uDirRoot;

    return sha512Half(
        std::uint16_t(spaceDirNode),
        uDirRoot,
        std::uint64_t(uNodeIndex));
}

uint256
getQualityIndex (uint256 const& uBase, const std::uint64_t uNodeDir)
{
//索引以big endian格式存储：它们以存储的十六进制打印。
//最重要的字节是第一个。最低有效字节表示
//相邻条目。我们把unoder放在8个最右边的字节中
//相邻的。希望unodeir为big endian格式，以便++转到下一个
//索引项。
    uint256 uNode (uBase);

//托多（汤姆）：一定有更好的办法。
//vfalc[base_int]这假定某种存储格式
    ((std::uint64_t*) uNode.end ())[-1] = boost::endian::native_to_big (uNodeDir);

    return uNode;
}

uint256
getQualityNext (uint256 const& uBase)
{
    static uint256 const uNext (
        from_hex_text<uint256>("10000000000000000"));
    return uBase + uNext;
}

std::uint64_t
getQuality (uint256 const& uBase)
{
//vfalc[base_int]这假定某种存储格式
    return boost::endian::big_to_native (((std::uint64_t*) uBase.end ())[-1]);
}

uint256
getTicketIndex (AccountID const& account, std::uint32_t uSequence)
{
    return sha512Half(
        std::uint16_t(spaceTicket),
        account,
        std::uint32_t(uSequence));
}

uint256
getRippleStateIndex (AccountID const& a, AccountID const& b, Currency const& currency)
{
    if (a < b)
        return sha512Half(
            std::uint16_t(spaceRipple),
            a,
            b,
            currency);
    return sha512Half(
        std::uint16_t(spaceRipple),
        b,
        a,
        currency);
}

uint256
getRippleStateIndex (AccountID const& a, Issue const& issue)
{
    return getRippleStateIndex (a, issue.account, issue.currency);
}

uint256
getSignerListIndex (AccountID const& account)
{
//我们准备在未来有多个签名者，
//但我们还没有。预期有多个签名者
//我们提供一个32位ID来定位签名者列表。直到我们
//*有*多个签名者列表，我们可以将该ID默认为零。
    return sha512Half(
        std::uint16_t(spaceSignerList),
        account,
std::uint32_t (0));  //0==默认签名者列表ID。
}

uint256
getCheckIndex (AccountID const& account, std::uint32_t uSequence)
{
    return sha512Half(
        std::uint16_t(spaceCheck),
        account,
        std::uint32_t(uSequence));
}

uint256
getDepositPreauthIndex (AccountID const& owner, AccountID const& preauthorized)
{
    return sha512Half(
        std::uint16_t(spaceDepositPreauth),
        owner,
        preauthorized);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace keylet {

Keylet account_t::operator()(
    AccountID const& id) const
{
    return { ltACCOUNT_ROOT,
        getAccountRootIndex(id) };
}

Keylet child (uint256 const& key)
{
    return { ltCHILD, key };
}

Keylet skip_t::operator()() const
{
    return { ltLEDGER_HASHES,
        getLedgerHashIndex() };
}

Keylet skip_t::operator()(LedgerIndex ledger) const
{
    return { ltLEDGER_HASHES,
        getLedgerHashIndex(ledger) };
}

Keylet amendments_t::operator()() const
{
    return { ltAMENDMENTS,
        getLedgerAmendmentIndex() };
}

Keylet fees_t::operator()() const
{
    return { ltFEE_SETTINGS,
        getLedgerFeeIndex() };
}

Keylet book_t::operator()(Book const& b) const
{
    return { ltDIR_NODE,
        getBookBase(b) };
}

Keylet line_t::operator()(AccountID const& id0,
    AccountID const& id1, Currency const& currency) const
{
    return { ltRIPPLE_STATE,
        getRippleStateIndex(id0, id1, currency) };
}

Keylet line_t::operator()(AccountID const& id,
    Issue const& issue) const
{
    return { ltRIPPLE_STATE,
        getRippleStateIndex(id, issue) };
}

Keylet offer_t::operator()(AccountID const& id,
    std::uint32_t seq) const
{
    return { ltOFFER,
        getOfferIndex(id, seq) };
}

Keylet quality_t::operator()(Keylet const& k,
    std::uint64_t q) const
{
    assert(k.type == ltDIR_NODE);
    return { ltDIR_NODE,
        getQualityIndex(k.key, q) };
}

Keylet next_t::operator()(Keylet const& k) const
{
    assert(k.type == ltDIR_NODE);
    return { ltDIR_NODE,
        getQualityNext(k.key) };
}

Keylet ticket_t::operator()(AccountID const& id,
    std::uint32_t seq) const
{
    return { ltTICKET,
        getTicketIndex(id, seq) };
}

Keylet signers_t::operator()(AccountID const& id) const
{
    return { ltSIGNER_LIST,
        getSignerListIndex(id) };
}

Keylet check_t::operator()(AccountID const& id,
    std::uint32_t seq) const
{
    return { ltCHECK,
        getCheckIndex(id, seq) };
}

Keylet depositPreauth_t::operator()(AccountID const& owner,
    AccountID const& preauthorized) const
{
    return { ltDEPOSIT_PREAUTH,
        getDepositPreauthIndex(owner, preauthorized) };
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Keylet unchecked (uint256 const& key)
{
    return { ltANY, key };
}

Keylet ownerDir(AccountID const& id)
{
    return { ltDIR_NODE,
        getOwnerDirIndex(id) };
}

Keylet page(uint256 const& key,
    std::uint64_t index)
{
    return { ltDIR_NODE,
        getDirNodeIndex(key, index) };
}

Keylet page(Keylet const& root,
    std::uint64_t index)
{
    assert(root.type == ltDIR_NODE);
    return page(root.key, index);
}

Keylet
escrow (AccountID const& source, std::uint32_t seq)
{
    sha512_half_hasher h;
    using beast::hash_append;
    hash_append(h, std::uint16_t(spaceEscrow));
    hash_append(h, source);
    hash_append(h, seq);
    return { ltESCROW, static_cast<uint256>(h) };
}

Keylet
payChan (AccountID const& source, AccountID const& dst, std::uint32_t seq)
{
    sha512_half_hasher h;
    using beast::hash_append;
    hash_append(h, std::uint16_t(spaceXRPUChannel));
    hash_append(h, source);
    hash_append(h, dst);
    hash_append(h, seq);
    return { ltPAYCHAN, static_cast<uint256>(h) };
}

} //小键盘

} //涟漪
