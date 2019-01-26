
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

#ifndef RIPPLE_LEDGER_OPENVIEW_H_INCLUDED
#define RIPPLE_LEDGER_OPENVIEW_H_INCLUDED

#include <ripple/ledger/RawView.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/ledger/detail/RawStateTable.h>
#include <ripple/basics/qalloc.h>
#include <ripple/protocol/XRPAmount.h>
#include <functional>
#include <utility>

namespace ripple {

/*打开分类帐结构标记。

    使用此标记构建的视图将具有
    交易中应用的未结分类账规则
    处理。
**/

struct open_ledger_t
{
    explicit open_ledger_t() = default;
};

extern open_ledger_t const open_ledger;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*累积状态和Tx更改的可写分类帐视图。

    @note作为readview呈现给客户。
**/

class OpenView final
    : public ReadView
    , public TxsRawView
{
private:
    class txs_iter_impl;

//Tx列表，关键订单
    using txs_map = std::map<key_type,
        std::pair<std::shared_ptr<Serializer const>,
        std::shared_ptr<Serializer const>>,
        std::less<key_type>, qalloc_type<std::pair<key_type const,
        std::pair<std::shared_ptr<Serializer const>,
        std::shared_ptr<Serializer const>>>, false>>;

    Rules rules_;
    txs_map txs_;
    LedgerInfo info_;
    ReadView const* base_;
    detail::RawStateTable items_;
    std::shared_ptr<void const> hold_;
    bool open_ = true;

public:
    OpenView() = delete;
    OpenView& operator= (OpenView&&) = delete;
    OpenView& operator= (OpenView const&) = delete;

    OpenView (OpenView&&) = default;

    /*构造一个浅显的副本。

        影响：

            使用的副本创建新对象
            修改状态表。

        共享指针管理的对象是
        不重复，但在实例之间共享。
        因为SLE是不可变的，所以调用
        rawview接口不能破坏不变量。
    **/

    OpenView (OpenView const&) = default;

    /*构造打开的分类帐视图。

        影响：

            序列号设置为
            父级加一的序列号。

            ParentCloseTime设置为
            家长关闭时间。

            如果“hold”不是nullptr，则保留
            “保留”副本的所有权到
            元视图被破坏。

            调用rules（）将返回
            施工规定。

        Tx列表开始为空，将包含
        所有新插入的Tx。
    **/

    /*@ {*/
    OpenView (open_ledger_t,
        ReadView const* base, Rules const& rules,
            std::shared_ptr<void const> hold = nullptr);

    OpenView (open_ledger_t, Rules const& rules,
            std::shared_ptr<ReadView const> const& base)
        : OpenView (open_ledger, &*base, rules, base)
    {
    }
    /*@ }*/

    /*构建新的上次关闭的分类帐。

        影响：

            从底部复制LedgerInfo。

            规则是从基础继承的。

        Tx列表开始为空，将包含
        所有新插入的Tx。
    **/

    OpenView (ReadView const* base,
        std::shared_ptr<void const> hold = nullptr);

    /*如果这反映未结分类帐，则返回true。*/
    bool
    open() const override
    {
        return open_;
    }

    /*返回自创建以来插入的Tx数。

        用于设置“应用顺序”
        计算事务元数据时。
    **/

    std::size_t
    txCount() const;

    /*应用更改。*/
    void
    apply (TxsRawView& to) const;

//读取视图

    LedgerInfo const&
    info() const override;

    Fees const&
    fees() const override;

    Rules const&
    rules() const override;

    bool
    exists (Keylet const& k) const override;

    boost::optional<key_type>
    succ (key_type const& key, boost::optional<
        key_type> const& last = boost::none) const override;

    std::shared_ptr<SLE const>
    read (Keylet const& k) const override;

    std::unique_ptr<sles_type::iter_base>
    slesBegin() const override;

    std::unique_ptr<sles_type::iter_base>
    slesEnd() const override;

    std::unique_ptr<sles_type::iter_base>
    slesUpperBound(uint256 const& key) const override;

    std::unique_ptr<txs_type::iter_base>
    txsBegin() const override;

    std::unique_ptr<txs_type::iter_base>
    txsEnd() const override;

    bool
    txExists (key_type const& key) const override;

    tx_type
    txRead (key_type const& key) const override;

//罗维尤

    void
    rawErase (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawInsert (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawReplace (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawDestroyXRP(
        XRPAmount const& fee) override;

//TXSRAWVIEW

    void
    rawTxInsert (key_type const& key,
        std::shared_ptr<Serializer const>
            const& txn, std::shared_ptr<
                Serializer const>
                    const& metaData) override;
};

} //涟漪

#endif
