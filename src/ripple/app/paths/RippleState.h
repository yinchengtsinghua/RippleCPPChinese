
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

#ifndef RIPPLE_APP_PATHS_RIPPLESTATE_H_INCLUDED
#define RIPPLE_APP_PATHS_RIPPLESTATE_H_INCLUDED

#include <ripple/ledger/View.h>
#include <ripple/protocol/Rate.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <cstdint>
#include <memory> //<内存>

namespace ripple {

/*为方便起见，将信任线SLE包裹起来。
    信托关系的复杂之处在于
    “低”账户和“高”账户。这包裹了
    并从
    行上的选定帐户。
**/

//vfalc todo重命名为trustline
class RippleState
{
public:
//vfalc为什么要共享？
    using pointer = std::shared_ptr <RippleState>;

public:
    RippleState () = delete;

    virtual ~RippleState() = default;

    static RippleState::pointer makeItem(
        AccountID const& accountID,
        std::shared_ptr<SLE const> sle);

//必须公开，以便共享
    RippleState (std::shared_ptr<SLE const>&& sle,
        AccountID const& viewAccount);

    /*返回分类帐条目的状态映射键。*/
    uint256
    key() const
    {
        return sle_->key();
    }

//vfalc从每个函数名中去掉“get”

    AccountID const& getAccountID () const
    {
        return  mViewLowest ? mLowID : mHighID;
    }

    AccountID const& getAccountIDPeer () const
    {
        return !mViewLowest ? mLowID : mHighID;
    }

//是，提供了对对等的身份验证。
    bool getAuth () const
    {
        return mFlags & (mViewLowest ? lsfLowAuth : lsfHighAuth);
    }

    bool getAuthPeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowAuth : lsfHighAuth);
    }

    bool getNoRipple () const
    {
        return mFlags & (mViewLowest ? lsfLowNoRipple : lsfHighNoRipple);
    }

    bool getNoRipplePeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowNoRipple : lsfHighNoRipple);
    }

    /*我们是否在我们的同伴身上设置了冻结标志？*/
    bool getFreeze () const
    {
        return mFlags & (mViewLowest ? lsfLowFreeze : lsfHighFreeze);
    }

    /*同伴在我们身上设置了冻结标志吗？*/
    bool getFreezePeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowFreeze : lsfHighFreeze);
    }

    STAmount const& getBalance () const
    {
        return mBalance;
    }

    STAmount const& getLimit () const
    {
        return  mViewLowest ? mLowLimit : mHighLimit;
    }

    STAmount const& getLimitPeer () const
    {
        return !mViewLowest ? mLowLimit : mHighLimit;
    }

    Rate const&
    getQualityIn () const
    {
        return mViewLowest ? lowQualityIn_ : highQualityIn_;
    }

    Rate const&
    getQualityOut () const
    {
        return mViewLowest ? lowQualityOut_ : highQualityOut_;
    }

    Json::Value getJson (int);

private:
    std::shared_ptr<SLE const> sle_;

    bool                            mViewLowest;

    std::uint32_t                   mFlags;

    STAmount const&                 mLowLimit;
    STAmount const&                 mHighLimit;

    AccountID const&                  mLowID;
    AccountID const&                  mHighID;

    Rate lowQualityIn_;
    Rate lowQualityOut_;
    Rate highQualityIn_;
    Rate highQualityOut_;

    STAmount                        mBalance;
};

std::vector <RippleState::pointer>
getRippleStateItems (AccountID const& accountID,
    ReadView const& view);

} //涟漪

#endif
