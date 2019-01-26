
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

#include <ripple/app/tx/impl/SetAccount.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/st.h>
#include <ripple/ledger/View.h>

namespace ripple {

bool
SetAccount::affectsSubsequentTransactionAuth(STTx const& tx)
{
    auto const uTxFlags = tx.getFlags();
    if(uTxFlags & (tfRequireAuth | tfOptionalAuth))
        return true;

    auto const uSetFlag = tx[~sfSetFlag];
    if(uSetFlag && (*uSetFlag == asfRequireAuth ||
        *uSetFlag == asfDisableMaster ||
            *uSetFlag == asfAccountTxnID))
                return true;

    auto const uClearFlag = tx[~sfClearFlag];
    return uClearFlag && (*uClearFlag == asfRequireAuth ||
        *uClearFlag == asfDisableMaster ||
            *uClearFlag == asfAccountTxnID);
}

NotTEC
SetAccount::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto& tx = ctx.tx;
    auto& j = ctx.j;

    std::uint32_t const uTxFlags = tx.getFlags ();

    if (uTxFlags & tfAccountSetMask)
    {
        JLOG(j.trace()) << "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

    std::uint32_t const uSetFlag = tx.getFieldU32 (sfSetFlag);
    std::uint32_t const uClearFlag = tx.getFieldU32 (sfClearFlag);

    if ((uSetFlag != 0) && (uSetFlag == uClearFlag))
    {
        JLOG(j.trace()) << "Malformed transaction: Set and clear same flag.";
        return temINVALID_FLAG;
    }

//
//需要量
//
    bool bSetRequireAuth   = (uTxFlags & tfRequireAuth) || (uSetFlag == asfRequireAuth);
    bool bClearRequireAuth = (uTxFlags & tfOptionalAuth) || (uClearFlag == asfRequireAuth);

    if (bSetRequireAuth && bClearRequireAuth)
    {
        JLOG(j.trace()) << "Malformed transaction: Contradictory flags set.";
        return temINVALID_FLAG;
    }

//
//需求标签
//
    bool bSetRequireDest   = (uTxFlags & TxFlag::requireDestTag) || (uSetFlag == asfRequireDest);
    bool bClearRequireDest = (uTxFlags & tfOptionalDestTag) || (uClearFlag == asfRequireDest);

    if (bSetRequireDest && bClearRequireDest)
    {
        JLOG(j.trace()) << "Malformed transaction: Contradictory flags set.";
        return temINVALID_FLAG;
    }

//
//不允许XRP
//
    bool bSetDisallowXRP   = (uTxFlags & tfDisallowXRP) || (uSetFlag == asfDisallowXRP);
    bool bClearDisallowXRP = (uTxFlags & tfAllowXRP) || (uClearFlag == asfDisallowXRP);

    if (bSetDisallowXRP && bClearDisallowXRP)
    {
        JLOG(j.trace()) << "Malformed transaction: Contradictory flags set.";
        return temINVALID_FLAG;
    }

//转移率
    if (tx.isFieldPresent (sfTransferRate))
    {
        std::uint32_t uRate = tx.getFieldU32 (sfTransferRate);

        if (uRate && (uRate < QUALITY_ONE))
        {
            JLOG(j.trace()) << "Malformed transaction: Transfer rate too small.";
            return temBAD_TRANSFER_RATE;
        }

        if (ctx.rules.enabled(fix1201) && (uRate > 2 * QUALITY_ONE))
        {
            JLOG(j.trace()) << "Malformed transaction: Transfer rate too large.";
            return temBAD_TRANSFER_RATE;
        }
    }

//尺寸标注
    if (tx.isFieldPresent (sfTickSize))
    {
        if (!ctx.rules.enabled(featureTickSize))
            return temDISABLED;

        auto uTickSize = tx[sfTickSize];
        if (uTickSize &&
            ((uTickSize < Quality::minTickSize) ||
            (uTickSize > Quality::maxTickSize)))
        {
            JLOG(j.trace()) << "Malformed transaction: Bad tick size.";
            return temBAD_TICK_SIZE;
        }
    }

    if (auto const mk = tx[~sfMessageKey])
    {
        if (mk->size() && ! publicKeyType ({mk->data(), mk->size()}))
        {
            JLOG(j.trace()) << "Invalid message key specified.";
            return telBAD_PUBLIC_KEY;
        }
    }

    auto const domain = tx[~sfDomain];
    if (domain && domain->size() > DOMAIN_BYTES_MAX)
    {
        JLOG(j.trace()) << "domain too long";
        return telBAD_DOMAIN;
    }

    return preflight2(ctx);
}

TER
SetAccount::preclaim(PreclaimContext const& ctx)
{
    auto const id = ctx.tx[sfAccount];

    std::uint32_t const uTxFlags = ctx.tx.getFlags();

    auto const sle = ctx.view.read(
        keylet::account(id));

    std::uint32_t const uFlagsIn = sle->getFieldU32(sfFlags);

    std::uint32_t const uSetFlag = ctx.tx.getFieldU32(sfSetFlag);

//旧帐户集标志
    bool bSetRequireAuth = (uTxFlags & tfRequireAuth) || (uSetFlag == asfRequireAuth);

//
//需要量
//
    if (bSetRequireAuth && !(uFlagsIn & lsfRequireAuth))
    {
        if (!dirIsEmpty(ctx.view,
            keylet::ownerDir(id)))
        {
            JLOG(ctx.j.trace()) << "Retry: Owner directory not empty.";
            return (ctx.flags & tapRETRY) ? TER {terOWNERS} : TER {tecOWNERS};
        }
    }

    return tesSUCCESS;
}

TER
SetAccount::doApply ()
{
    auto const sle = view().peek(keylet::account(account_));

    std::uint32_t const uFlagsIn = sle->getFieldU32 (sfFlags);
    std::uint32_t uFlagsOut = uFlagsIn;

    STTx const& tx {ctx_.tx};
    std::uint32_t const uSetFlag {tx.getFieldU32 (sfSetFlag)};
    std::uint32_t const uClearFlag {tx.getFieldU32 (sfClearFlag)};

//旧帐户集标志
    std::uint32_t const uTxFlags {tx.getFlags ()};
    bool const bSetRequireDest   {(uTxFlags & TxFlag::requireDestTag) || (uSetFlag == asfRequireDest)};
    bool const bClearRequireDest {(uTxFlags & tfOptionalDestTag) || (uClearFlag == asfRequireDest)};
    bool const bSetRequireAuth   {(uTxFlags & tfRequireAuth) || (uSetFlag == asfRequireAuth)};
    bool const bClearRequireAuth {(uTxFlags & tfOptionalAuth) || (uClearFlag == asfRequireAuth)};
    bool const bSetDisallowXRP   {(uTxFlags & tfDisallowXRP) || (uSetFlag == asfDisallowXRP)};
    bool const bClearDisallowXRP {(uTxFlags & tfAllowXRP) || (uClearFlag == asfDisallowXRP)};

    bool const sigWithMaster {[&tx, &acct = account_] ()
    {
        auto const spk = tx.getSigningPubKey();

        if (publicKeyType (makeSlice (spk)))
        {
            PublicKey const signingPubKey (makeSlice (spk));

            if (calcAccountID(signingPubKey) == acct)
                return true;
        }
        return false;
    }()};

//
//需要量
//
    if (bSetRequireAuth && !(uFlagsIn & lsfRequireAuth))
    {
        JLOG(j_.trace()) << "Set RequireAuth.";
        uFlagsOut |= lsfRequireAuth;
    }

    if (bClearRequireAuth && (uFlagsIn & lsfRequireAuth))
    {
        JLOG(j_.trace()) << "Clear RequireAuth.";
        uFlagsOut &= ~lsfRequireAuth;
    }

//
//需求标签
//
    if (bSetRequireDest && !(uFlagsIn & lsfRequireDestTag))
    {
        JLOG(j_.trace()) << "Set lsfRequireDestTag.";
        uFlagsOut |= lsfRequireDestTag;
    }

    if (bClearRequireDest && (uFlagsIn & lsfRequireDestTag))
    {
        JLOG(j_.trace()) << "Clear lsfRequireDestTag.";
        uFlagsOut &= ~lsfRequireDestTag;
    }

//
//不允许XRP
//
    if (bSetDisallowXRP && !(uFlagsIn & lsfDisallowXRP))
    {
        JLOG(j_.trace()) << "Set lsfDisallowXRP.";
        uFlagsOut |= lsfDisallowXRP;
    }

    if (bClearDisallowXRP && (uFlagsIn & lsfDisallowXRP))
    {
        JLOG(j_.trace()) << "Clear lsfDisallowXRP.";
        uFlagsOut &= ~lsfDisallowXRP;
    }

//
//聋哑人
//
    if ((uSetFlag == asfDisableMaster) && !(uFlagsIn & lsfDisableMaster))
    {
        if (!sigWithMaster)
        {
            JLOG(j_.trace()) << "Must use master key to disable master key.";
            return tecNEED_MASTER_KEY;
        }

        if ((!sle->isFieldPresent (sfRegularKey)) &&
            (!view().peek (keylet::signers (account_))))
        {
//帐户没有常规密钥或多签名者签名者列表。

//在我们准备好之前阻止事务更改。
            if (view().rules().enabled(featureMultiSign))
                return tecNO_ALTERNATIVE_KEY;

            return tecNO_REGULAR_KEY;
        }

        JLOG(j_.trace()) << "Set lsfDisableMaster.";
        uFlagsOut |= lsfDisableMaster;
    }

    if ((uClearFlag == asfDisableMaster) && (uFlagsIn & lsfDisableMaster))
    {
        JLOG(j_.trace()) << "Clear lsfDisableMaster.";
        uFlagsOut &= ~lsfDisableMaster;
    }

//
//德法特普
//
    if (uSetFlag == asfDefaultRipple)
    {
        JLOG(j_.trace()) << "Set lsfDefaultRipple.";
        uFlagsOut |= lsfDefaultRipple;
    }
    else if (uClearFlag == asfDefaultRipple)
    {
        JLOG(j_.trace()) << "Clear lsfDefaultRipple.";
        uFlagsOut &= ~lsfDefaultRipple;
    }

//
//诺弗雷兹
//
    if (uSetFlag == asfNoFreeze)
    {
        if (!sigWithMaster && !(uFlagsIn & lsfDisableMaster))
        {
            JLOG(j_.trace()) << "Must use master key to set NoFreeze.";
            return tecNEED_MASTER_KEY;
        }

        JLOG(j_.trace()) << "Set NoFreeze flag";
        uFlagsOut |= lsfNoFreeze;
    }

//任何人都可以设置全局冻结
    if (uSetFlag == asfGlobalFreeze)
    {
        JLOG(j_.trace()) << "Set GlobalFreeze flag";
        uFlagsOut |= lsfGlobalFreeze;
    }

//如果设置了“不冻结”，则可能无法清除globalfreeze
//这可以防止设置了nofreeze的用户使用
//全球战略。
    if ((uSetFlag != asfGlobalFreeze) && (uClearFlag == asfGlobalFreeze) &&
        ((uFlagsOut & lsfNoFreeze) == 0))
    {
        JLOG(j_.trace()) << "Clear GlobalFreeze flag";
        uFlagsOut &= ~lsfGlobalFreeze;
    }

//
//跟踪此帐户在其根目录中签名的事务ID
//
    if ((uSetFlag == asfAccountTxnID) && !sle->isFieldPresent (sfAccountTxnID))
    {
        JLOG(j_.trace()) << "Set AccountTxnID.";
        sle->makeFieldPresent (sfAccountTxnID);
        }

    if ((uClearFlag == asfAccountTxnID) && sle->isFieldPresent (sfAccountTxnID))
    {
        JLOG(j_.trace()) << "Clear AccountTxnID.";
        sle->makeFieldAbsent (sfAccountTxnID);
    }

//
//沉积矿
//
    if (view().rules().enabled(featureDepositAuth))
    {
        if (uSetFlag == asfDepositAuth)
        {
            JLOG(j_.trace()) << "Set lsfDepositAuth.";
            uFlagsOut |= lsfDepositAuth;
        }
        else if (uClearFlag == asfDepositAuth)
        {
            JLOG(j_.trace()) << "Clear lsfDepositAuth.";
            uFlagsOut &= ~lsfDepositAuth;
        }
    }

//
//电子邮件地址
//
    if (tx.isFieldPresent (sfEmailHash))
    {
        uint128 const uHash = tx.getFieldH128 (sfEmailHash);

        if (!uHash)
        {
            JLOG(j_.trace()) << "unset email hash";
            sle->makeFieldAbsent (sfEmailHash);
        }
        else
        {
            JLOG(j_.trace()) << "set email hash";
            sle->setFieldH128 (sfEmailHash, uHash);
        }
    }

//
//墙面定位器
//
    if (tx.isFieldPresent (sfWalletLocator))
    {
        uint256 const uHash = tx.getFieldH256 (sfWalletLocator);

        if (!uHash)
        {
            JLOG(j_.trace()) << "unset wallet locator";
            sle->makeFieldAbsent (sfWalletLocator);
        }
        else
        {
            JLOG(j_.trace()) << "set wallet locator";
            sle->setFieldH256 (sfWalletLocator, uHash);
        }
    }

//
//消息键
//
    if (tx.isFieldPresent (sfMessageKey))
    {
        Blob const messageKey = tx.getFieldVL (sfMessageKey);

        if (messageKey.empty ())
        {
            JLOG(j_.debug()) << "set message key";
            sle->makeFieldAbsent (sfMessageKey);
        }
        else
        {
            JLOG(j_.debug()) << "set message key";
            sle->setFieldVL (sfMessageKey, messageKey);
        }
    }

//
//领域
//
    if (tx.isFieldPresent (sfDomain))
    {
        Blob const domain = tx.getFieldVL (sfDomain);

        if (domain.empty ())
        {
            JLOG(j_.trace()) << "unset domain";
            sle->makeFieldAbsent (sfDomain);
        }
        else
        {
            JLOG(j_.trace()) << "set domain";
            sle->setFieldVL (sfDomain, domain);
        }
    }

//
//转移率
//
    if (tx.isFieldPresent (sfTransferRate))
    {
        std::uint32_t uRate = tx.getFieldU32 (sfTransferRate);

        if (uRate == 0 || uRate == QUALITY_ONE)
        {
            JLOG(j_.trace()) << "unset transfer rate";
            sle->makeFieldAbsent (sfTransferRate);
        }
        else
        {
            JLOG(j_.trace()) << "set transfer rate";
            sle->setFieldU32 (sfTransferRate, uRate);
        }
    }

//
//尺寸标注
//
    if (tx.isFieldPresent (sfTickSize))
    {
        auto uTickSize = tx[sfTickSize];
        if ((uTickSize == 0) || (uTickSize == Quality::maxTickSize))
        {
            JLOG(j_.trace()) << "unset tick size";
            sle->makeFieldAbsent (sfTickSize);
        }
        else
        {
            JLOG(j_.trace()) << "set tick size";
            sle->setFieldU8 (sfTickSize, uTickSize);
        }
    }

    if (uFlagsIn != uFlagsOut)
        sle->setFieldU32 (sfFlags, uFlagsOut);

    return tesSUCCESS;
}

}
