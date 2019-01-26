
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

#ifndef RIPPLE_APP_LEDGER_LOCALTXS_H_INCLUDED
#define RIPPLE_APP_LEDGER_LOCALTXS_H_INCLUDED

#include <ripple/app/misc/CanonicalTXSet.h>
#include <ripple/ledger/ReadView.h>
#include <memory>

namespace ripple {

//跟踪本地客户发出的交易
//确保我们总是将它们应用于未结分类账
//保留它们直到我们在完全验证的分类账中看到它们

class LocalTxs
{
public:
    virtual ~LocalTxs () = default;

//添加新的本地事务
    virtual void push_back (LedgerIndex index, std::shared_ptr<STTx const> const& txn) = 0;

//将本地交易记录集返回到新的未结分类帐
    virtual CanonicalTXSet getTxSet () = 0;

//基于新的完全有效的分类帐删除过时的交易记录
    virtual void sweep (ReadView const& view) = 0;

    virtual std::size_t size () = 0;
};

std::unique_ptr<LocalTxs>
make_LocalTxs ();

} //涟漪

#endif
