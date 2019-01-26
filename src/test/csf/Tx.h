
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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
#ifndef RIPPLE_TEST_CSF_TX_H_INCLUDED
#define RIPPLE_TEST_CSF_TX_H_INCLUDED
#include <ripple/beast/hash/uhash.h>
#include <ripple/beast/hash/hash_append.h>
#include <boost/function_output_iterator.hpp>
#include <boost/container/flat_set.hpp>
#include <map>
#include <ostream>
#include <string>

namespace ripple {
namespace test {
namespace csf {

//！单笔交易
class Tx
{
public:
    using ID = std::uint32_t;

    Tx(ID i) : id_{i}
    {
    }

    ID
    id() const
    {
        return id_;
    }

    bool
    operator<(Tx const& o) const
    {
        return id_ < o.id_;
    }

    bool
    operator==(Tx const& o) const
    {
        return id_ == o.id_;
    }

private:
    ID id_;
};

//！————————————————————————————————————————————————————————————————
//！所有Tx的集合都表示为性能的扁平集合。
using TxSetType = boost::container::flat_set<Tx>;

//！txset是一组要考虑包括在分类帐中的交易记录。
class TxSet
{
public:
    using ID = beast::uhash<>::result_type;
    using Tx = csf::Tx;

    static ID calcID(TxSetType const & txs)
    {
        return beast::uhash<>{}(txs);
    }

    class MutableTxSet
    {
        friend class TxSet;

        TxSetType txs_;

    public:
        MutableTxSet(TxSet const& s) : txs_{s.txs_}
        {
        }

        bool
        insert(Tx const& t)
        {
            return txs_.insert(t).second;
        }

        bool
        erase(Tx::ID const& txId)
        {
            return txs_.erase(Tx{txId}) > 0;
        }
    };

    TxSet() = default;
    TxSet(TxSetType const& s) : txs_{s}, id_{calcID(txs_)}
    {
    }

    TxSet(MutableTxSet && m)
        : txs_{std::move(m.txs_)}, id_{calcID(txs_)}
    {
    }

    bool
    exists(Tx::ID const txId) const
    {
        auto it = txs_.find(Tx{txId});
        return it != txs_.end();
    }

    Tx const*
    find(Tx::ID const& txId) const
    {
        auto it = txs_.find(Tx{txId});
        if (it != txs_.end())
            return &(*it);
        return nullptr;
    }

    TxSetType const &
    txs() const
    {
        return txs_;
    }

    ID
    id() const
    {
        return id_;
    }

    /*@缺少tx:：id的返回映射。真实手段
                    它就在这一套里，而不是另一套。虚假手段
                    是在另一组，不是这个
    **/

    std::map<Tx::ID, bool>
    compare(TxSet const& other) const
    {
        std::map<Tx::ID, bool> res;

        auto populate_diffs = [&res](auto const& a, auto const& b, bool s) {
            auto populator = [&](auto const& tx) { res[tx.id()] = s; };
            std::set_difference(
                a.begin(),
                a.end(),
                b.begin(),
                b.end(),
                boost::make_function_output_iterator(std::ref(populator)));
        };

        populate_diffs(txs_, other.txs_, true);
        populate_diffs(other.txs_, txs_, false);
        return res;
    }

private:
//！集合包含实际事务
    TxSetType txs_;

//！此Tx集的唯一ID
    ID id_;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//用于调试打印的帮助程序函数

inline std::ostream&
operator<<(std::ostream& o, const Tx& t)
{
    return o << t.id();
}

template <class T>
inline std::ostream&
operator<<(std::ostream& o, boost::container::flat_set<T> const& ts)
{
    o << "{ ";
    bool do_comma = false;
    for (auto const& t : ts)
    {
        if (do_comma)
            o << ", ";
        else
            do_comma = true;
        o << t;
    }
    o << " }";
    return o;
}

inline std::string
to_string(TxSetType const& txs)
{
    std::stringstream ss;
    ss << txs;
    return ss.str();
}

template <class Hasher>
inline void
hash_append(Hasher& h, Tx const& tx)
{
    using beast::hash_append;
    hash_append(h, tx.id());
}

}  //脑脊液
}  //测试
}  //涟漪

#endif
