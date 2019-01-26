
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

#ifndef RIPPLE_APP_MISC_CANONICALTXSET_H_INCLUDED
#define RIPPLE_APP_MISC_CANONICALTXSET_H_INCLUDED

#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/protocol/STTx.h>

namespace ripple {

/*持有推迟到下一次协商一致通过的交易。

    “规范”是指应用事务的顺序。

    -按顺序排列来自同一帐户的交易记录

**/

//vfalc todo重命名为sortedtxset
class CanonicalTXSet
{
private:
    class Key
    {
    public:
        Key (uint256 const& account, std::uint32_t seq, uint256 const& id)
            : mAccount (account)
            , mTXid (id)
            , mSeq (seq)
        {
        }

        bool operator<  (Key const& rhs) const;
        bool operator>  (Key const& rhs) const;
        bool operator<= (Key const& rhs) const;
        bool operator>= (Key const& rhs) const;

        bool operator== (Key const& rhs) const
        {
            return mTXid == rhs.mTXid;
        }
        bool operator!= (Key const& rhs) const
        {
            return mTXid != rhs.mTXid;
        }

        uint256 const& getTXID () const
        {
            return mTXid;
        }

    private:
        uint256 mAccount;
        uint256 mTXid;
        std::uint32_t mSeq;
    };

//计算给定帐户的salted密钥
    uint256 accountKey (AccountID const& account);

public:
    using const_iterator = std::map <Key, std::shared_ptr<STTx const>>::const_iterator;

public:
    explicit CanonicalTXSet (LedgerHash const& saltHash)
        : salt_ (saltHash)
    {
    }

    void insert (std::shared_ptr<STTx const> const& txn);

    std::vector<std::shared_ptr<STTx const>>
    prune(AccountID const& account, std::uint32_t const seq);

//vfalc todo删除此函数
    void reset (LedgerHash const& salt)
    {
        salt_ = salt;
        map_.clear ();
    }

    const_iterator erase (const_iterator const& it)
    {
        return map_.erase(it);
    }

    const_iterator begin () const
    {
        return map_.begin();
    }

    const_iterator end() const
    {
        return map_.end();
    }

    size_t size () const
    {
        return map_.size ();
    }
    bool empty () const
    {
        return map_.empty ();
    }

    uint256 const& key() const
    {
        return salt_;
    }

private:
    std::map <Key, std::shared_ptr<STTx const>> map_;

//过去常常在账户上撒盐，这样人们就不能因为账户号码太低而挖矿。
    uint256 salt_;
};

} //涟漪

#endif
