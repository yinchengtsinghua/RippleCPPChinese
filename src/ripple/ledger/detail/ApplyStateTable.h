
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

#ifndef RIPPLE_LEDGER_APPLYSTATETABLE_H_INCLUDED
#define RIPPLE_LEDGER_APPLYSTATETABLE_H_INCLUDED

#include <ripple/ledger/OpenView.h>
#include <ripple/ledger/RawView.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/ledger/TxMeta.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/XRPAmount.h>
#include <ripple/beast/utility/Journal.h>
#include <memory>

namespace ripple {
namespace detail {

//缓冲修改的帮助程序类
class ApplyStateTable
{
public:
    using key_type = ReadView::key_type;

private:
    enum class Action
    {
        cache,
        erase,
        insert,
        modify,
    };

    using items_t = std::map<key_type,
        std::pair<Action, std::shared_ptr<SLE>>>;

    items_t items_;
    XRPAmount dropsDestroyed_ = 0;

public:
    ApplyStateTable() = default;
    ApplyStateTable (ApplyStateTable&&) = default;

    ApplyStateTable (ApplyStateTable const&) = delete;
    ApplyStateTable& operator= (ApplyStateTable&&) = delete;
    ApplyStateTable& operator= (ApplyStateTable const&) = delete;

    void
    apply (RawView& to) const;

    void
    apply (OpenView& to, STTx const& tx,
        TER ter, boost::optional<
            STAmount> const& deliver,
                beast::Journal j);

    bool
    exists (ReadView const& base,
        Keylet const& k) const;

    boost::optional<key_type>
    succ (ReadView const& base,
        key_type const& key, boost::optional<
            key_type> const& last) const;

    std::shared_ptr<SLE const>
    read (ReadView const& base,
        Keylet const& k) const;

    std::shared_ptr<SLE>
    peek (ReadView const& base,
        Keylet const& k);

    std::size_t
    size () const;

    void
    visit (ReadView const& base,
        std::function <void (
            uint256 const& key,
            bool isDelete,
            std::shared_ptr <SLE const> const& before,
            std::shared_ptr <SLE const> const& after)> const& func) const;

    void
    erase (ReadView const& base,
        std::shared_ptr<SLE> const& sle);

    void
    rawErase (ReadView const& base,
        std::shared_ptr<SLE> const& sle);

    void
    insert(ReadView const& base,
        std::shared_ptr<SLE> const& sle);

    void
    update(ReadView const& base,
        std::shared_ptr<SLE> const& sle);

    void
    replace(ReadView const& base,
        std::shared_ptr<SLE> const& sle);

    void
    destroyXRP (XRPAmount const& fee);

//用于调试
    XRPAmount const& dropsDestroyed () const
    {
        return dropsDestroyed_;
    }

private:
    using Mods = hash_map<key_type,
        std::shared_ptr<SLE>>;

    static
    void
    threadItem (TxMeta& meta,
        std::shared_ptr<SLE> const& to);

    std::shared_ptr<SLE>
    getForMod (ReadView const& base,
        key_type const& key, Mods& mods,
            beast::Journal j);

    void
    threadTx (ReadView const& base, TxMeta& meta,
        AccountID const& to, Mods& mods,
            beast::Journal j);

    void
    threadOwners (ReadView const& base,
        TxMeta& meta, std::shared_ptr<
            SLE const> const& sle, Mods& mods,
                beast::Journal j);
};

} //细节
} //涟漪

#endif
