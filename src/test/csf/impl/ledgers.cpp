
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
#include <test/csf/ledgers.h>
#include <algorithm>

#include <sstream>

namespace ripple {
namespace test {
namespace csf {

Ledger::Instance const Ledger::genesis;

Json::Value
Ledger::getJson() const
{
    Json::Value res(Json::objectValue);
    res["id"] = static_cast<ID::value_type>(id());
    res["seq"] = static_cast<Seq::value_type>(seq());
    return res;
}

bool
Ledger::isAncestor(Ledger const& ancestor) const
{
    if (ancestor.seq() < seq())
        return operator[](ancestor.seq()) == ancestor.id();
    return false;
}

Ledger::ID
Ledger::operator[](Seq s) const
{
    if(s > seq())
        return {};
    if(s== seq())
        return id();
    return instance_->ancestors[static_cast<Seq::value_type>(s)];

}

Ledger::Seq
mismatch(Ledger const& a, Ledger const& b)
{
    using Seq = Ledger::Seq;

//结束是范围结束后的1
    Seq start{0};
    Seq end = std::min(a.seq() + Seq{1}, b.seq() + Seq{1});

//在[开始，结束]中查找不匹配项
//二元搜索
    Seq count = end - start;
    while(count > Seq{0})
    {
        Seq step = count/Seq{2};
        Seq curr = start + step;
        if(a[curr] == b[curr])
        {
//转到下半场
            start = ++curr;
            count -= step + Seq{1};
        }
        else
            count = step;
    }
    return start;
}

LedgerOracle::LedgerOracle()
{
    instances_.insert(InstanceEntry{Ledger::genesis, nextID()});
}

Ledger::ID
LedgerOracle::nextID() const
{
    return Ledger::ID{static_cast<Ledger::ID::value_type>(instances_.size())};
}

Ledger
LedgerOracle::accept(
    Ledger const& parent,
    TxSetType const& txs,
    NetClock::duration closeTimeResolution,
    NetClock::time_point const& consensusCloseTime)
{
    using namespace std::chrono_literals;
    Ledger::Instance next(*parent.instance_);
    next.txs.insert(txs.begin(), txs.end());
    next.seq = parent.seq() + Ledger::Seq{1};
    next.closeTimeResolution = closeTimeResolution;
    next.closeTimeAgree = consensusCloseTime != NetClock::time_point{};
    if(next.closeTimeAgree)
        next.closeTime = effCloseTime(
            consensusCloseTime, closeTimeResolution, parent.closeTime());
    else
        next.closeTime = parent.closeTime() + 1s;

    next.parentCloseTime = parent.closeTime();
    next.parentID = parent.id();
    next.ancestors.push_back(parent.id());

    auto it = instances_.left.find(next);
    if (it == instances_.left.end())
    {
        using Entry = InstanceMap::left_value_type;
        it = instances_.left.insert(Entry{next, nextID()}).first;
    }
    return Ledger(it->second, &(it->first));
}

boost::optional<Ledger>
LedgerOracle::lookup(Ledger::ID const & id) const
{
    auto const it = instances_.right.find(id);
    if(it != instances_.right.end())
    {
        return Ledger(it->first, &(it->second));
    }
    return boost::none;
}


std::size_t
LedgerOracle::branches(std::set<Ledger> const & ledgers) const
{
//提示始终维护序列号最大的分类帐
//沿着所有已知的链条。
    std::vector<Ledger> tips;
    tips.reserve(ledgers.size());

    for (Ledger const & ledger : ledgers)
    {
//三种选择，
//1。分类帐在新的分支机构上
//2。分类帐在我们看到小费的一家分公司。
//三。分类帐是分行的新提示
        bool found = false;
        for (auto idx = 0; idx < tips.size() && !found; ++idx)
        {
            bool const idxEarlier = tips[idx].seq() < ledger.seq();
            Ledger const & earlier = idxEarlier ? tips[idx] : ledger;
            Ledger const & later = idxEarlier ? ledger : tips[idx] ;
            if (later.isAncestor(earlier))
            {
                tips[idx] = later;
                found = true;
            }
        }

        if(!found)
            tips.push_back(ledger);

    }
//提示的大小是分支数
    return tips.size();
}
}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹
