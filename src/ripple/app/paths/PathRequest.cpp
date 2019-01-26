
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

#include <ripple/app/paths/AccountCurrencies.h>
#include <ripple/app/paths/RippleCalc.h>
#include <ripple/app/paths/PathRequest.h>
#include <ripple/app/paths/PathRequests.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/rpc/impl/Tuning.h>
#include <ripple/beast/core/LexicalCast.h>
#include <boost/algorithm/clamp.hpp>
#include <boost/optional.hpp>
#include <tuple>

namespace ripple {

PathRequest::PathRequest (
    Application& app,
    const std::shared_ptr<InfoSub>& subscriber,
    int id,
    PathRequests& owner,
    beast::Journal journal)
        : app_ (app)
        , m_journal (journal)
        , mOwner (owner)
        , wpSubscriber (subscriber)
        , consumer_(subscriber->getConsumer())
        , jvStatus (Json::objectValue)
        , mLastIndex (0)
        , mInProgress (false)
        , iLevel (0)
        , bLastSuccess (false)
        , iIdentifier (id)
        , created_ (std::chrono::steady_clock::now())
{
    JLOG(m_journal.debug())
        << iIdentifier << " created";
}

PathRequest::PathRequest (
    Application& app,
    std::function <void(void)> const& completion,
    Resource::Consumer& consumer,
    int id,
    PathRequests& owner,
    beast::Journal journal)
        : app_ (app)
        , m_journal (journal)
        , mOwner (owner)
        , fCompletion (completion)
        , consumer_ (consumer)
        , jvStatus (Json::objectValue)
        , mLastIndex (0)
        , mInProgress (false)
        , iLevel (0)
        , bLastSuccess (false)
        , iIdentifier (id)
        , created_ (std::chrono::steady_clock::now())
{
    JLOG(m_journal.debug())
        << iIdentifier << " created";
}

PathRequest::~PathRequest()
{
    using namespace std::chrono;
    auto stream = m_journal.info();
    if (! stream)
        return;

    std::string fast, full;
    if (quick_reply_ != steady_clock::time_point{})
    {
        fast = " fast:";
        fast += std::to_string(
            duration_cast<milliseconds>(quick_reply_ - created_).count());
        fast += "ms";
    }
    if (full_reply_ != steady_clock::time_point{})
    {
        full = " full:";
        full += std::to_string(
            duration_cast<milliseconds>(full_reply_ - created_).count());
        full += "ms";
    }
    stream << iIdentifier << " complete:" << fast << full <<
        " total:" << duration_cast<milliseconds>(steady_clock::now() -
        created_).count() << "ms";
}

bool PathRequest::isNew ()
{
    ScopedLockType sl (mIndexLock);

//此路径请求是否仍需要其第一个完整路径
    return mLastIndex == 0;
}

bool PathRequest::needsUpdate (bool newOnly, LedgerIndex index)
{
    ScopedLockType sl (mIndexLock);

    if (mInProgress)
    {
//另一个线程正在处理此问题
        return false;
    }

    if (newOnly && (mLastIndex != 0))
    {
//只处理新请求，这不是新的
        return false;
    }

    if (mLastIndex >= index)
    {
        return false;
    }

    mInProgress = true;
    return true;
}

bool PathRequest::hasCompletion ()
{
    return bool (fCompletion);
}

void PathRequest::updateComplete ()
{
    ScopedLockType sl (mIndexLock);

    assert (mInProgress);
    mInProgress = false;

    if (fCompletion)
    {
        fCompletion();
        fCompletion = std::function<void (void)>();
    }
}

bool PathRequest::isValid (std::shared_ptr<RippleLineCache> const& crCache)
{
    if (! raSrcAccount || ! raDstAccount)
        return false;

    if (! convert_all_ && (saSendMax || saDstAmount <= beast::zero))
    {
//如果指定了发送最大值，则dst amt必须为-1。
        jvStatus = rpcError(rpcDST_AMT_MALFORMED);
        return false;
    }

    auto const& lrLedger = crCache->getLedger();

    if (! lrLedger->exists(keylet::account(*raSrcAccount)))
    {
//源帐户不存在。
        jvStatus = rpcError (rpcSRC_ACT_NOT_FOUND);
        return false;
    }

    auto const sleDest = lrLedger->read(
        keylet::account(*raDstAccount));

    Json::Value& jvDestCur =
        (jvStatus[jss::destination_currencies] = Json::arrayValue);

    if (! sleDest)
    {
        jvDestCur.append (Json::Value (systemCurrencyCode()));
        if (! saDstAmount.native ())
        {
//只有xrp可以发送到不存在的帐户。
            jvStatus = rpcError (rpcACT_NOT_FOUND);
            return false;
        }

        if (! convert_all_ && saDstAmount <
            STAmount (lrLedger->fees().accountReserve (0)))
        {
//付款必须符合准备金。
            jvStatus = rpcError (rpcDST_AMT_MALFORMED);
            return false;
        }
    }
    else
    {
        bool const disallowXRP (
            sleDest->getFlags() & lsfDisallowXRP);

        auto usDestCurrID = accountDestCurrencies (
                *raDstAccount, crCache, ! disallowXRP);

        for (auto const& currency : usDestCurrID)
            jvDestCur.append (to_string (currency));
        jvStatus[jss::destination_tag] =
            (sleDest->getFlags() & lsfRequireDestTag);
    }

    jvStatus[jss::ledger_hash] = to_string (lrLedger->info().hash);
    jvStatus[jss::ledger_index] = lrLedger->seq();
    return true;
}

/*如果这是一个普通的路径请求，我们现在就要“快速”运行它一次。
    给出初步结果。

    如果这是一个遗留路径请求，我们只运行一次，
    我们现在不能完全运行它，所以我们根本不想运行它。

    如果有错误，我们需要确保将其返回给呼叫方
    在所有情况下。
**/

std::pair<bool, Json::Value>
PathRequest::doCreate (
    std::shared_ptr<RippleLineCache> const& cache,
    Json::Value const& value)
{
    bool valid = false;

    if (parseJson (value) != PFR_PJ_INVALID)
    {
        valid = isValid (cache);
        if (! hasCompletion() && valid)
            doUpdate(cache, true);
    }

    if (auto stream = m_journal.debug())
    {
        if (valid)
        {
            stream << iIdentifier <<
                " valid: " << toBase58(*raSrcAccount);
            stream << iIdentifier <<
                " deliver: " << saDstAmount.getFullText ();
        }
        else
        {
            stream << iIdentifier << " invalid";
        }
    }

    return { valid, jvStatus };
}

int PathRequest::parseJson (Json::Value const& jvParams)
{
    if (! jvParams.isMember(jss::source_account))
    {
        jvStatus = rpcError(rpcSRC_ACT_MISSING);
        return PFR_PJ_INVALID;
    }

    if (! jvParams.isMember(jss::destination_account))
    {
        jvStatus = rpcError(rpcDST_ACT_MISSING);
        return PFR_PJ_INVALID;
    }

    if (! jvParams.isMember(jss::destination_amount))
    {
        jvStatus = rpcError(rpcDST_AMT_MISSING);
        return PFR_PJ_INVALID;
    }

    raSrcAccount = parseBase58<AccountID>(
        jvParams[jss::source_account].asString());
    if (! raSrcAccount)
    {
        jvStatus = rpcError (rpcSRC_ACT_MALFORMED);
        return PFR_PJ_INVALID;
    }

    raDstAccount = parseBase58<AccountID>(
        jvParams[jss::destination_account].asString());
    if (! raDstAccount)
    {
        jvStatus = rpcError (rpcDST_ACT_MALFORMED);
        return PFR_PJ_INVALID;
    }

    if (! amountFromJsonNoThrow (
        saDstAmount, jvParams[jss::destination_amount]))
    {
        jvStatus = rpcError (rpcDST_AMT_MALFORMED);
        return PFR_PJ_INVALID;
    }

    convert_all_ = saDstAmount == STAmount(saDstAmount.issue(), 1u, 0, true);

    if ((saDstAmount.getCurrency ().isZero () &&
            saDstAmount.getIssuer ().isNonZero ()) ||
        (saDstAmount.getCurrency () == badCurrency ()) ||
        (! convert_all_ && saDstAmount <= beast::zero))
    {
        jvStatus = rpcError (rpcDST_AMT_MALFORMED);
        return PFR_PJ_INVALID;
    }

    if (jvParams.isMember(jss::send_max))
    {
//send_max要求目标金额为-1。
        if (! convert_all_)
        {
            jvStatus = rpcError(rpcDST_AMT_MALFORMED);
            return PFR_PJ_INVALID;
        }

        saSendMax.emplace();
        if (! amountFromJsonNoThrow(
            *saSendMax, jvParams[jss::send_max]) ||
            (saSendMax->getCurrency().isZero() &&
                saSendMax->getIssuer().isNonZero()) ||
            (saSendMax->getCurrency() == badCurrency()) ||
            (*saSendMax <= beast::zero &&
                *saSendMax != STAmount(saSendMax->issue(), 1u, 0, true)))
        {
            jvStatus = rpcError(rpcSENDMAX_MALFORMED);
            return PFR_PJ_INVALID;
        }
    }

    if (jvParams.isMember (jss::source_currencies))
    {
        Json::Value const& jvSrcCurrencies = jvParams[jss::source_currencies];
        if (! jvSrcCurrencies.isArray() || jvSrcCurrencies.size() == 0 ||
            jvSrcCurrencies.size() > RPC::Tuning::max_src_cur)
        {
            jvStatus = rpcError (rpcSRC_CUR_MALFORMED);
            return PFR_PJ_INVALID;
        }

        sciSourceCurrencies.clear ();

        for (auto const& c : jvSrcCurrencies)
        {
//法定货币
            Currency srcCurrencyID;
            if (! c.isObject() ||
                ! c.isMember(jss::currency) ||
                ! c[jss::currency].isString() ||
                ! to_currency(srcCurrencyID, c[jss::currency].asString()))
            {
                jvStatus = rpcError (rpcSRC_CUR_MALFORMED);
                return PFR_PJ_INVALID;
            }

//可选择发行人
            AccountID srcIssuerID;
            if (c.isMember (jss::issuer) &&
                (! c[jss::issuer].isString() ||
                    ! to_issuer(srcIssuerID, c[jss::issuer].asString())))
            {
                jvStatus = rpcError (rpcSRC_ISR_MALFORMED);
                return PFR_PJ_INVALID;
            }

            if (srcCurrencyID.isZero())
            {
                if (srcIssuerID.isNonZero())
                {
                    jvStatus = rpcError(rpcSRC_CUR_MALFORMED);
                    return PFR_PJ_INVALID;
                }
            }
            else if (srcIssuerID.isZero())
            {
                srcIssuerID = *raSrcAccount;
            }

            if (saSendMax)
            {
//如果货币不匹配，则忽略源货币。
                if (srcCurrencyID == saSendMax->getCurrency())
                {
//如果源和它们都不相等，那么
//源颁发者非法。
                    if (srcIssuerID != *raSrcAccount &&
                        saSendMax->getIssuer() != *raSrcAccount &&
                            srcIssuerID != saSendMax->getIssuer())
                    {
                        jvStatus = rpcError (rpcSRC_ISR_MALFORMED);
                        return PFR_PJ_INVALID;
                    }

//如果两者都是源，请使用源。
//否则，请使用非源代码。
                    if (srcIssuerID != *raSrcAccount)
                    {
                        sciSourceCurrencies.insert(
                            {srcCurrencyID, srcIssuerID});
                    }
                    else if (saSendMax->getIssuer() != *raSrcAccount)
                    {
                        sciSourceCurrencies.insert(
                            {srcCurrencyID, saSendMax->getIssuer()});
                    }
                    else
                    {
                        sciSourceCurrencies.insert(
                            {srcCurrencyID, *raSrcAccount});
                    }
                }
            }
            else
            {
                sciSourceCurrencies.insert({srcCurrencyID, srcIssuerID});
            }
        }
    }

    if (jvParams.isMember (jss::id))
        jvId = jvParams[jss::id];

    return PFR_PJ_NOCHANGE;
}

Json::Value PathRequest::doClose (Json::Value const&)
{
    JLOG(m_journal.debug()) << iIdentifier << " closed";
    ScopedLockType sl (mLock);
    jvStatus[jss::closed] = true;
    return jvStatus;
}

Json::Value PathRequest::doStatus (Json::Value const&)
{
    ScopedLockType sl (mLock);
    jvStatus[jss::status] = jss::success;
    return jvStatus;
}

std::unique_ptr<Pathfinder> const&
PathRequest::getPathFinder(std::shared_ptr<RippleLineCache> const& cache,
    hash_map<Currency, std::unique_ptr<Pathfinder>>& currency_map,
        Currency const& currency, STAmount const& dst_amount,
            int const level)
{
    auto i = currency_map.find(currency);
    if (i != currency_map.end())
        return i->second;
    auto pathfinder = std::make_unique<Pathfinder>(
        cache, *raSrcAccount, *raDstAccount, currency,
            boost::none, dst_amount, saSendMax, app_);
    if (pathfinder->findPaths(level))
        pathfinder->computePathRanks(max_paths_);
    else
pathfinder.reset();  //这是一个糟糕的请求-清除它。
    return currency_map[currency] = std::move(pathfinder);
}

bool
PathRequest::findPaths (std::shared_ptr<RippleLineCache> const& cache,
    int const level, Json::Value& jvArray)
{
    auto sourceCurrencies = sciSourceCurrencies;
    if (sourceCurrencies.empty ())
    {
        auto currencies = accountSourceCurrencies(*raSrcAccount, cache, true);
        bool const sameAccount = *raSrcAccount == *raDstAccount;
        for (auto const& c : currencies)
        {
            if (! sameAccount || c != saDstAmount.getCurrency())
            {
                if (sourceCurrencies.size() >= RPC::Tuning::max_auto_src_cur)
                    return false;
                sourceCurrencies.insert(
                    {c, c.isZero() ? xrpAccount() : *raSrcAccount});
            }
        }
    }

    auto const dst_amount = convert_all_ ?
        STAmount(saDstAmount.issue(), STAmount::cMaxValue, STAmount::cMaxOffset)
            : saDstAmount;
    hash_map<Currency, std::unique_ptr<Pathfinder>> currency_map;
    for (auto const& issue : sourceCurrencies)
    {
        JLOG(m_journal.debug())
            << iIdentifier
            << " Trying to find paths: "
            << STAmount(issue, 1).getFullText();

        auto& pathfinder = getPathFinder(cache, currency_map,
            issue.currency, dst_amount, level);
        if (! pathfinder)
        {
            assert(false);
            JLOG(m_journal.debug()) << iIdentifier << " No paths found";
            continue;
        }

        STPath fullLiquidityPath;
        auto ps = pathfinder->getBestPaths(max_paths_,
            fullLiquidityPath, mContext[issue], issue.account);
        mContext[issue] = ps;

        auto& sourceAccount = ! isXRP(issue.account)
            ? issue.account
            : isXRP(issue.currency)
            ? xrpAccount()
            : *raSrcAccount;
        STAmount saMaxAmount = saSendMax.value_or(
            STAmount({issue.currency, sourceAccount}, 1u, 0, true));

        JLOG(m_journal.debug()) << iIdentifier
            << " Paths found, calling rippleCalc";

        path::RippleCalc::Input rcInput;
        if (convert_all_)
            rcInput.partialPaymentAllowed = true;
        auto sandbox = std::make_unique<PaymentSandbox>
            (&*cache->getLedger(), tapNONE);
        auto rc = path::RippleCalc::rippleCalculate(
            *sandbox,
saMaxAmount,    //>发送的金额不受限制
//得到一个估计。
dst_amount,     //>交货量。
*raDstAccount,  //>要送达的帐户。
*raSrcAccount,  //>帐户发送自。
ps,             //>路径集。
            app_.logs(),
            &rcInput);

        if (! convert_all_ &&
            ! fullLiquidityPath.empty() &&
            (rc.result() == terNO_LINE || rc.result() == tecPATH_PARTIAL))
        {
            JLOG(m_journal.debug()) << iIdentifier
                << " Trying with an extra path element";

            ps.push_back(fullLiquidityPath);
            sandbox = std::make_unique<PaymentSandbox>
                (&*cache->getLedger(), tapNONE);
            rc = path::RippleCalc::rippleCalculate(
                *sandbox,
saMaxAmount,    //>发送的金额不受限制
//得到一个估计。
dst_amount,     //>交货量。
*raDstAccount,  //>要送达的帐户。
*raSrcAccount,  //>帐户发送自。
ps,             //>路径集。
                app_.logs());

            if (rc.result() != tesSUCCESS)
            {
                JLOG(m_journal.warn()) << iIdentifier
                    << " Failed with covering path "
                    << transHuman(rc.result());
            }
            else
            {
                JLOG(m_journal.debug()) << iIdentifier
                    << " Extra path element gives "
                    << transHuman(rc.result());
            }
        }

        if (rc.result () == tesSUCCESS)
        {
            Json::Value jvEntry (Json::objectValue);
            rc.actualAmountIn.setIssuer (sourceAccount);
            jvEntry[jss::source_amount] = rc.actualAmountIn.getJson (0);
            jvEntry[jss::paths_computed] = ps.getJson(0);

            if (convert_all_)
                jvEntry[jss::destination_amount] = rc.actualAmountOut.getJson(0);

            if (hasCompletion ())
            {
//旧Ripple_Path_Find API需要此功能
                jvEntry[jss::paths_canonical] = Json::arrayValue;
            }

            jvArray.append (jvEntry);
        }
        else
        {
            JLOG(m_journal.debug()) << iIdentifier << " rippleCalc returns "
                << transHuman(rc.result());
        }
    }

    /*资源费基于使用的源货币数量。
        最低成本是50，最高是400。成本增加了
        四种来源货币之后，50-（4*4）=34。
    **/

    int const size = sourceCurrencies.size();
    consumer_.charge({boost::algorithm::clamp(size * size + 34, 50, 400),
        "path update"});
    return true;
}

Json::Value PathRequest::doUpdate(
    std::shared_ptr<RippleLineCache> const& cache, bool fast)
{
    using namespace std::chrono;
    JLOG(m_journal.debug()) << iIdentifier
        << " update " << (fast ? "fast" : "normal");

    {
        ScopedLockType sl (mLock);

        if (!isValid (cache))
            return jvStatus;
    }

    Json::Value newStatus = Json::objectValue;

    if (hasCompletion ())
    {
//旧Ripple_Path_Find API提供目的地货币
        auto& destCurrencies = (newStatus[jss::destination_currencies] = Json::arrayValue);
        auto usCurrencies = accountDestCurrencies (*raDstAccount, cache, true);
        for (auto const& c : usCurrencies)
            destCurrencies.append (to_string (c));
    }

    newStatus[jss::source_account] = app_.accountIDCache().toBase58(*raSrcAccount);
    newStatus[jss::destination_account] = app_.accountIDCache().toBase58(*raDstAccount);
    newStatus[jss::destination_amount] = saDstAmount.getJson (0);
    newStatus[jss::full_reply] = ! fast;

    if (jvId)
        newStatus[jss::id] = jvId;

    bool loaded = app_.getFeeTrack().isLoadedLocal();

    if (iLevel == 0)
    {
//第一次通过
        if (loaded || fast)
            iLevel = app_.config().PATH_SEARCH_FAST;
        else
            iLevel = app_.config().PATH_SEARCH;
    }
    else if ((iLevel == app_.config().PATH_SEARCH_FAST) && !fast)
    {
//离开快速寻路
        iLevel = app_.config().PATH_SEARCH;
        if (loaded && (iLevel > app_.config().PATH_SEARCH_FAST))
            --iLevel;
    }
    else if (bLastSuccess)
    {
//如果可能，减量
        if (iLevel > app_.config().PATH_SEARCH ||
            (loaded && (iLevel > app_.config().PATH_SEARCH_FAST)))
            --iLevel;
    }
    else
    {
//根据需要调整
        if (!loaded && (iLevel < app_.config().PATH_SEARCH_MAX))
            ++iLevel;
        if (loaded && (iLevel > app_.config().PATH_SEARCH_FAST))
            --iLevel;
    }

    JLOG(m_journal.debug()) << iIdentifier
        << " processing at level " << iLevel;

    Json::Value jvArray = Json::arrayValue;
    if (findPaths(cache, iLevel, jvArray))
    {
        bLastSuccess = jvArray.size() != 0;
        newStatus[jss::alternatives] = std::move (jvArray);
    }
    else
    {
        bLastSuccess = false;
        newStatus = rpcError(rpcINTERNAL);
    }

    if (fast && quick_reply_ == steady_clock::time_point{})
    {
        quick_reply_ = steady_clock::now();
        mOwner.reportFast(duration_cast<milliseconds>(quick_reply_ - created_));
    }
    else if (! fast && full_reply_ == steady_clock::time_point{})
    {
        full_reply_ = steady_clock::now();
        mOwner.reportFull(duration_cast<milliseconds>(full_reply_ - created_));
    }

    {
        ScopedLockType sl(mLock);
        jvStatus = newStatus;
    }

    return newStatus;
}

InfoSub::pointer PathRequest::getSubscriber ()
{
    return wpSubscriber.lock ();
}

} //涟漪
