
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
    版权所有（c）2014 Ripple Labs Inc.

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

#ifndef RIPPLE_TX_SETSIGNERLIST_H_INCLUDED
#define RIPPLE_TX_SETSIGNERLIST_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/app/tx/impl/SignerEntries.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/STArray.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/basics/Log.h>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace ripple {

/*
有关setSignerList事务的概述，请参见readme.md。
此类实现。
**/

class SetSignerList : public Transactor
{
private:
//在预计算期间确定的值供以后使用。
    enum Operation {unknown, set, destroy};
    Operation do_ {unknown};
    std::uint32_t quorum_ {0};
    std::vector<SignerEntries::SignerEntry> signers_;

public:
    explicit SetSignerList (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    bool
    affectsSubsequentTransactionAuth(STTx const& tx)
    {
        return true;
    }

    static
    NotTEC
    preflight (PreflightContext const& ctx);

    TER doApply () override;
    void preCompute() override;

private:
    static
    std::tuple<NotTEC, std::uint32_t,
        std::vector<SignerEntries::SignerEntry>,
            Operation>
    determineOperation(STTx const& tx,
        ApplyFlags flags, beast::Journal j);

    static
    NotTEC validateQuorumAndSignerEntries (
        std::uint32_t quorum,
            std::vector<SignerEntries::SignerEntry> const& signers,
                AccountID const& account,
                    beast::Journal j);

    TER replaceSignerList ();
    TER destroySignerList ();

    TER removeSignersFromLedger (Keylet const& accountKeylet,
        Keylet const& ownerDirKeylet, Keylet const& signerListKeylet);
    void writeSignersToSLE (
        SLE::pointer const& ledgerEntry, std::uint32_t flags) const;

//计算特征多重信号保留前所有者计数的方法。
//这需要保留在代码库中，直到没有签名者保留为止
//在接受之前创建的分类帐中
//功能多信号保留…永远有效。
    static int signerCountBasedOwnerCountDelta (std::size_t entryCount);
};

} //涟漪

#endif
