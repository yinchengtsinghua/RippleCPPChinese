
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

#ifndef RIPPLE_APP_LEDGER_ACCEPTEDLEDGER_H_INCLUDED
#define RIPPLE_APP_LEDGER_ACCEPTEDLEDGER_H_INCLUDED

#include <ripple/app/ledger/AcceptedLedgerTx.h>
#include <ripple/protocol/AccountID.h>

namespace ripple {

/*不可撤销的分类账。

    接受的分类帐是一个有足够数量的
    验证以使本地服务器相信它是不可撤销的。

    接受的分类账的存在意味着所有之前的分类账
    被接受。
**/

/*vvalco todo消化本术语澄清：
    已关闭和已接受指未通过
    验证阈值。一旦他们通过门槛，他们就会
    “验证”。关闭只是意味着关闭时间已经过去，没有
    新的交易可以进入。”“接受”意味着我们相信
    共识过程的结果（尽管尚未验证
    还没有。
**/

class AcceptedLedger
{
public:
    using pointer        = std::shared_ptr<AcceptedLedger>;
    using ret            = const pointer&;
    using map_t          = std::map<int, AcceptedLedgerTx::pointer>;
//地图必须是订购的地图！
    using value_type     = map_t::value_type;
    using const_iterator = map_t::const_iterator;

public:
    std::shared_ptr<ReadView const> const& getLedger () const
    {
        return mLedger;
    }
    const map_t& getMap () const
    {
        return mMap;
    }

    int getTxnCount () const
    {
        return mMap.size ();
    }

    AcceptedLedgerTx::pointer getTxn (int) const;

    AcceptedLedger (
        std::shared_ptr<ReadView const> const& ledger,
        AccountIDCache const& accountCache, Logs& logs);

private:
    void insert (AcceptedLedgerTx::ref);

    std::shared_ptr<ReadView const> mLedger;
    map_t mMap;
};

} //涟漪

#endif
