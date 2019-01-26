
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

#include <ripple/app/tx/impl/SetRegularKey.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {

std::uint64_t
SetRegularKey::calculateBaseFee (
    ReadView const& view,
    STTx const& tx)
{
    auto const id = tx.getAccountID(sfAccount);
    auto const spk = tx.getSigningPubKey();

    if (publicKeyType (makeSlice (spk)))
    {
        if (calcAccountID(PublicKey (makeSlice(spk))) == id)
        {
            auto const sle = view.read(keylet::account(id));

            if (sle && (! (sle->getFlags () & lsfPasswordSpent)))
            {
//旗子是武装的，他们用正确的帐户签名
                return 0;
            }
        }
    }

    return Transactor::calculateBaseFee (view, tx);
}

NotTEC
SetRegularKey::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    std::uint32_t const uTxFlags = ctx.tx.getFlags ();

    if (uTxFlags & tfUniversalMask)
    {
        JLOG(ctx.j.trace()) <<
            "Malformed transaction: Invalid flags set.";

        return temINVALID_FLAG;
    }

    return preflight2(ctx);
}

TER
SetRegularKey::doApply ()
{
    auto const sle = view ().peek (
        keylet::account (account_));

    if (!minimumFee (ctx_.app, ctx_.baseFee, view ().fees (), view ().flags ()))
        sle->setFlag (lsfPasswordSpent);

    if (ctx_.tx.isFieldPresent (sfRegularKey))
    {
        sle->setAccountID (sfRegularKey,
            ctx_.tx.getAccountID (sfRegularKey));
    }
    else
    {
        if (sle->isFlag (lsfDisableMaster) &&
            !view().peek (keylet::signers (account_)))
//帐户已禁用主密钥，没有多签名者签名者列表。
            return tecNO_ALTERNATIVE_KEY;

        sle->makeFieldAbsent (sfRegularKey);
    }

    return tesSUCCESS;
}

}
