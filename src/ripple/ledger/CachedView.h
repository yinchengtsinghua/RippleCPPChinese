
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

#ifndef RIPPLE_LEDGER_CACHEDVIEW_H_INCLUDED
#define RIPPLE_LEDGER_CACHEDVIEW_H_INCLUDED

#include <ripple/ledger/CachedSLEs.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/basics/hardened_hash.h>
#include <map>
#include <memory>
#include <mutex>
#include <type_traits>

namespace ripple {

namespace detail {

class CachedViewImpl
    : public DigestAwareReadView
{
private:
    DigestAwareReadView const& base_;
    CachedSLEs& cache_;
    std::mutex mutable mutex_;
    std::unordered_map<key_type,
        std::shared_ptr<SLE const>,
            hardened_hash<>> mutable map_;

public:
    CachedViewImpl() = delete;
    CachedViewImpl (CachedViewImpl const&) = delete;
    CachedViewImpl& operator= (CachedViewImpl const&) = delete;

    CachedViewImpl (DigestAwareReadView const* base,
            CachedSLEs& cache)
        : base_ (*base)
        , cache_ (cache)
    {
    }

//
//读取视图
//

    bool
    exists (Keylet const& k) const override;

    std::shared_ptr<SLE const>
    read (Keylet const& k) const override;

    bool
    open() const override
    {
        return base_.open();
    }

    LedgerInfo const&
    info() const override
    {
        return base_.info();
    }

    Fees const&
    fees() const override
    {
        return base_.fees();
    }

    Rules const&
    rules() const override
    {
        return base_.rules();
    }

    boost::optional<key_type>
    succ (key_type const& key, boost::optional<
        key_type> const& last = boost::none) const override
    {
        return base_.succ(key, last);
    }

    std::unique_ptr<sles_type::iter_base>
    slesBegin() const override
    {
        return base_.slesBegin();
    }

    std::unique_ptr<sles_type::iter_base>
    slesEnd() const override
    {
        return base_.slesEnd();
    }

    std::unique_ptr<sles_type::iter_base>
    slesUpperBound(uint256 const& key) const override
    {
        return base_.slesUpperBound(key);
    }

    std::unique_ptr<txs_type::iter_base>
    txsBegin() const override
    {
        return base_.txsBegin();
    }

    std::unique_ptr<txs_type::iter_base>
    txsEnd() const override
    {
        return base_.txsEnd();
    }

    bool
    txExists(key_type const& key) const override
    {
        return base_.txExists(key);
    }

    tx_type
    txRead (key_type const& key) const override
    {
        return base_.txRead(key);
    }

//
//摘要警告视图
//

    boost::optional<digest_type>
    digest (key_type const& key) const override
    {
        return base_.digest(key);
    }

};

} //细节

/*包装DigesAwareReadView以提供缓存。

    @tparam base a digestawareadview的子类
**/

template <class Base>
class CachedView
    : public detail::CachedViewImpl
{
private:
    static_assert(std::is_base_of<
        DigestAwareReadView, Base>::value, "");

    std::shared_ptr<Base const> sp_;

public:
    using base_type = Base;

    CachedView() = delete;
    CachedView (CachedView const&) = delete;
    CachedView& operator= (CachedView const&) = delete;

    CachedView (std::shared_ptr<
        Base const> const& base, CachedSLEs& cache)
        : CachedViewImpl (base.get(), cache)
        , sp_ (base)
    {
    }

    /*返回基类型。

        @注意，这会破坏封装并绕过缓存。
    **/

    std::shared_ptr<Base const> const&
    base() const
    {
        return sp_;
    }
};

} //涟漪

#endif
