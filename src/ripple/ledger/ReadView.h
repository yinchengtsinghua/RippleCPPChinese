
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

#ifndef RIPPLE_LEDGER_READVIEW_H_INCLUDED
#define RIPPLE_LEDGER_READVIEW_H_INCLUDED

#include <ripple/ledger/detail/ReadViewFwdRange.h>
#include <ripple/basics/chrono.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/XRPAmount.h>
#include <ripple/beast/hash/uhash.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>
#include <cassert>
#include <cstdint>
#include <memory>
#include <unordered_set>

namespace ripple {

/*反映特定分类帐的费用设置。

    对于任何应用的交易，费用总是相同的
    分类帐费用的变动发生在分类账之间。
**/

struct Fees
{
std::uint64_t base = 0;         //参考Tx成本（下降）
std::uint32_t units = 0;        //参考费用单位
std::uint32_t reserve = 0;      //储备基数（下降）
std::uint32_t increment = 0;    //储备增量（下降）

    explicit Fees() = default;
    Fees (Fees const&) = default;
    Fees& operator= (Fees const&) = default;

    /*返回给定所有者计数的帐户准备金，以滴为单位。

        准备金按准备金基数加上
        保留增量乘以增量数。
    **/

    XRPAmount
    accountReserve (std::size_t ownerCount) const
    {
        return { reserve + ownerCount * increment };
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*有关支持该视图的名义分类帐的信息。*/
struct LedgerInfo
{
    explicit LedgerInfo() = default;

//
//适用于所有分类帐
//

    LedgerIndex seq = 0;
    NetClock::time_point parentCloseTime = {};

//
//对于封闭式分类账
//

//关闭表示“Tx设置已确定”
    uint256 hash = beast::zero;
    uint256 txHash = beast::zero;
    uint256 accountHash = beast::zero;
    uint256 parentHash = beast::zero;

    XRPAmount drops = beast::zero;

//如果验证为假，则表示“尚未验证”。
//一旦验证为真，以后将永远不会设置为假。
//vfalc todo使其不可变
    bool mutable validated = false;
    bool accepted = false;

//指示此分类帐结算方式的标志
    int closeFlags = 0;

//此分类帐关闭时间的解决方案（2-120秒）
    NetClock::duration closeTimeResolution = {};

//对于已关闭的分类帐，分类帐的时间
//关闭。对于未结分类帐，分类帐的时间
//如果没有交易将关闭。
//
    NetClock::time_point closeTime = {};
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class DigestAwareReadView;

/*控制协议行为的规则。*/
class Rules
{
private:
    class Impl;

    std::shared_ptr<Impl const> impl_;

public:
    Rules (Rules const&) = default;
    Rules& operator= (Rules const&) = default;

    Rules() = delete;

    /*构造空规则集。

        这些规则反映在
        创世纪分类帐。
    **/

    explicit Rules(std::unordered_set<uint256, beast::uhash<>> const& presets);

    /*从分类帐构造规则。

        对分类帐内容进行规则分析
        以及对目标的修正和摘录。
    **/

    explicit Rules(
        DigestAwareReadView const& ledger,
            std::unordered_set<uint256, beast::uhash<>> const& presets);

    /*如果功能已启用，则返回“true”。*/
    bool
    enabled (uint256 const& id) const;

    /*如果这些规则与分类帐不匹配，则返回“true”。*/
    bool
    changed (DigestAwareReadView const& ledger) const;

    /*如果两个规则集相同，则返回“true”。

        @注意，这是为了诊断。确定是否新
        应构造规则，请先调用changed（）。
    **/

    bool
    operator== (Rules const&) const;

    bool
    operator!= (Rules const& other) const
    {
        return ! (*this == other);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*分类帐中的视图。

    此接口提供对状态的读取访问
    以及交易项目。没有检查站
    或者元数据的计算。
**/

class ReadView
{
public:
    using tx_type =
        std::pair<std::shared_ptr<STTx const>,
            std::shared_ptr<STObject const>>;

    using key_type = uint256;

    using mapped_type =
        std::shared_ptr<SLE const>;

    struct sles_type : detail::ReadViewFwdRange<
        std::shared_ptr<SLE const>>
    {
        explicit sles_type (ReadView const& view);
        iterator begin() const;
        iterator const& end() const;
        iterator upper_bound(key_type const& key) const;
    };

    struct txs_type
        : detail::ReadViewFwdRange<tx_type>
    {
        explicit txs_type (ReadView const& view);
        bool empty() const;
        iterator begin() const;
        iterator const& end() const;
    };

    virtual ~ReadView() = default;

    ReadView& operator= (ReadView&& other) = delete;
    ReadView& operator= (ReadView const& other) = delete;

    ReadView ()
        : sles(*this)
        , txs(*this)
    {
    }

    ReadView (ReadView const& other)
        : sles(*this)
        , txs(*this)
    {
    }

    ReadView (ReadView&& other)
        : sles(*this)
        , txs(*this)
    {
    }

    /*返回有关分类帐的信息。*/
    virtual
    LedgerInfo const&
    info() const = 0;

    /*如果这反映未结分类帐，则返回true。*/
    virtual
    bool
    open() const = 0;

    /*返回上一个分类帐的关闭时间。*/
    NetClock::time_point
    parentCloseTime() const
    {
        return info().parentCloseTime;
    }

    /*返回基本分类帐的序列号。*/
    LedgerIndex
    seq() const
    {
        return info().seq;
    }

    /*返回基本分类帐的费用。*/
    virtual
    Fees const&
    fees() const = 0;

    /*返回Tx处理规则。*/
    virtual
    Rules const&
    rules() const = 0;

    /*确定状态项是否存在。

        @注意，这比调用read更有效。

        @return`true`如果SLE与
                指定键。
    **/

    virtual
    bool
    exists (Keylet const& k) const = 0;

    /*返回下一个状态项的键。

        返回第一个状态项的键
        其键大于指定键。如果
        不存在这样的键，将返回boost：：none。

        如果启用了“last”，则在以下情况下返回boost:：none
        返回的钥匙在打开的外面
        间隔（键，最后一个）。
    **/

    virtual
    boost::optional<key_type>
    succ (key_type const& key, boost::optional<
        key_type> const& last = boost::none) const = 0;

    /*返回与键关联的状态项。

        影响：
            如果密钥存在，则为调用方提供所有权
            不可修改的对应SLE。

        @注意，当返回的SLE是来自
              调用者的视角，可以更改
              其他呼叫者通过原始操作。

        @return`nullptr`如果键不存在或
                如果类型不匹配。
    **/

    virtual
    std::shared_ptr<SLE const>
    read (Keylet const& k) const = 0;

//付款中的账户不允许使用在此期间获得的资产。
//付款。PaymentSandbox跟踪借项、贷项和所有者计数
//付款期间帐户所做的更改。`Balancehook`调整余额
//因此，新收购的资产不计入余额。
//这是支持PaymentSandbox所必需的。
    virtual
    STAmount
    balanceHook (AccountID const& account,
        AccountID const& issuer,
            STAmount const& amount) const
    {
        return amount;
    }

//付款中的账户不允许使用在此期间获得的资产。
//付款。PaymentSandbox跟踪借项、贷项和所有者计数
//付款期间帐户所做的更改。`ownerCounthook`调整
//所以它返回到目前为止所有者计数的最大值。
//这是支持PaymentSandbox所必需的。
    virtual
    std::uint32_t
    ownerCountHook (AccountID const& account,
        std::uint32_t count) const
    {
        return count;
    }

//由实现使用
    virtual
    std::unique_ptr<sles_type::iter_base>
    slesBegin() const = 0;

//由实现使用
    virtual
    std::unique_ptr<sles_type::iter_base>
    slesEnd() const = 0;

//由实现使用
    virtual
    std::unique_ptr<sles_type::iter_base>
    slesUpperBound(key_type const& key) const = 0;

//由实现使用
    virtual
    std::unique_ptr<txs_type::iter_base>
    txsBegin() const = 0;

//由实现使用
    virtual
    std::unique_ptr<txs_type::iter_base>
    txsEnd() const = 0;

    /*如果Tx映射中存在Tx，则返回“true”。

        如果Tx是
        基本分类帐，或者如果是新插入的TX。
    **/

    virtual
    bool
    txExists (key_type const& key) const = 0;

    /*从Tx映射读取事务。

        如果视图表示打开的分类帐，
        元数据对象将为空。

        @返回一对nullptr，如果
                在Tx映射中找不到密钥。
    **/

    virtual
    tx_type
    txRead (key_type const& key) const = 0;

//
//成员空间
//

    /*分类帐状态项目的可重复范围。

        @note访问分类帐中的每个州条目可以
              随着账本的增长变得相当昂贵。
    **/

    sles_type sles;

//交易范围
    txs_type txs;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*将键与摘要关联的readview。*/
class DigestAwareReadView
    : public ReadView
{
public:
    using digest_type = uint256;

    DigestAwareReadView () = default;
    DigestAwareReadView (const DigestAwareReadView&) = default;

    /*返回与键关联的摘要。

        @return boost：：如果该项不存在，则为无。
    **/

    virtual
    boost::optional<digest_type>
    digest (key_type const& key) const = 0;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//分类帐关闭标志
static
std::uint32_t const sLCF_NoConsensusTime = 0x01;

static
std::uint32_t const sLCF_SHAMapV2 = 0x02;

inline
bool getCloseAgree (LedgerInfo const& info)
{
    return (info.closeFlags & sLCF_NoConsensusTime) == 0;
}

inline
bool getSHAMapV2 (LedgerInfo const& info)
{
    return (info.closeFlags & sLCF_SHAMapV2) != 0;
}

void addRaw (LedgerInfo const&, Serializer&);

} //涟漪

#include <ripple/ledger/detail/ReadViewFwdRange.ipp>

#endif
