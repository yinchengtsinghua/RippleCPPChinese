
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

#include <ripple/app/main/Application.h>
#include <ripple/app/paths/Tuning.h>
#include <ripple/app/paths/Pathfinder.h>
#include <ripple/app/paths/RippleCalc.h>
#include <ripple/app/paths/RippleLineCache.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/app/ledger/OrderBookDB.h>
#include <ripple/basics/Log.h>
#include <ripple/json/to_string.h>
#include <ripple/core/JobQueue.h>
#include <ripple/core/Config.h>
#include <tuple>

/*

核心寻路引擎

寻路请求按类别、xrp到xrp、xrp到
非瑞波币、非瑞波币对瑞波币、同一货币、非瑞波币对非瑞波币、交叉货币
非xrp到非xrp。对于每个类别，都有一个路径表，
探路者搜索。收集完整的路径。

然后对每个完整路径进行评级和排序。没有或微不足道的路径
流动性下降。否则，路径将根据质量进行排序，
流动性和路径长度。

路径槽按质量（输出与输入之比）顺序填充，使用
最后一条路径必须具有足够的流动性来完成
付款（假设没有流动性重叠）。此外，如果没有选定路径
能够提供足够的流动性自行完成支付，
返回额外的“覆盖”路径。

然后测试所选路径，以确定它们是否可以完成
付款，如果是，费用是多少。如果他们失败了，一条覆盖路径
发现，用覆盖路径重复测试。如果成功，那么
返回最终路径和估计成本。

引擎允许选择搜索深度和路径表
包括找到每个路径类型的深度。搜索深度为零
导致无法进行搜索。也可以注入额外的路径，这
应用于在调用的过程中保留以前找到的路径
相同的路径请求（尤其是搜索深度可能改变时）。

**/


namespace ripple {

namespace {

struct AccountCandidate
{
    int priority;
    AccountID account;

    static const int highPriority = 10000;
};

bool compareAccountCandidate (
    std::uint32_t seq,
    AccountCandidate const& first, AccountCandidate const& second)
{
    if (first.priority < second.priority)
        return false;

    if (first.account > second.account)
        return true;

    return (first.priority ^ seq) < (second.priority ^ seq);
}

using AccountCandidates = std::vector<AccountCandidate>;

struct CostedPath
{
    int searchLevel;
    Pathfinder::PathType type;
};

using CostedPathList = std::vector<CostedPath>;

using PathTable = std::map<Pathfinder::PaymentType, CostedPathList>;

struct PathCost {
    int cost;
    char const* path;
};
using PathCostList = std::vector<PathCost>;

static PathTable mPathTable;

std::string pathTypeToString (Pathfinder::PathType const& type)
{
    std::string ret;

    for (auto const& node : type)
    {
        switch (node)
        {
            case Pathfinder::nt_SOURCE:
                ret.append("s");
                break;
            case Pathfinder::nt_ACCOUNTS:
                ret.append("a");
                break;
            case Pathfinder::nt_BOOKS:
                ret.append("b");
                break;
            case Pathfinder::nt_XRP_BOOK:
                ret.append("x");
                break;
            case Pathfinder::nt_DEST_BOOK:
                ret.append("f");
                break;
            case Pathfinder::nt_DESTINATION:
                ret.append("d");
                break;
        }
    }

    return ret;
}

}  //命名空间

Pathfinder::Pathfinder (
    std::shared_ptr<RippleLineCache> const& cache,
    AccountID const& uSrcAccount,
    AccountID const& uDstAccount,
    Currency const& uSrcCurrency,
    boost::optional<AccountID> const& uSrcIssuer,
    STAmount const& saDstAmount,
    boost::optional<STAmount> const& srcAmount,
    Application& app)
    :   mSrcAccount (uSrcAccount),
        mDstAccount (uDstAccount),
        mEffectiveDst (isXRP(saDstAmount.getIssuer ()) ?
            uDstAccount : saDstAmount.getIssuer ()),
        mDstAmount (saDstAmount),
        mSrcCurrency (uSrcCurrency),
        mSrcIssuer (uSrcIssuer),
        mSrcAmount (srcAmount.value_or(STAmount({uSrcCurrency,
            uSrcIssuer.value_or(isXRP(uSrcCurrency) ?
                xrpAccount() : uSrcAccount)}, 1u, 0, true))),
        convert_all_ (mDstAmount ==
            STAmount(mDstAmount.issue(), STAmount::cMaxValue, STAmount::cMaxOffset)),
        mLedger (cache->getLedger ()),
        mRLCache (cache),
        app_ (app),
        j_ (app.journal ("Pathfinder"))
{
    assert (! uSrcIssuer || isXRP(uSrcCurrency) == isXRP(uSrcIssuer.get()));
}

bool Pathfinder::findPaths (int searchLevel)
{
    if (mDstAmount == beast::zero)
    {
//不用寄零钱。
        JLOG (j_.debug()) << "Destination amount was zero.";
        mLedger.reset ();
        return false;

//托多（汤姆）：我们为什么要重设分类帐，在这种情况下，和一个
//下面-为什么我们不每次返回错误时都这样做？
    }

    if (mSrcAccount == mDstAccount &&
        mDstAccount == mEffectiveDst &&
        mSrcCurrency == mDstAmount.getCurrency ())
    {
//无需发送到具有相同货币的同一帐户。
        JLOG (j_.debug()) << "Tried to send to same issuer";
        mLedger.reset ();
        return false;
    }

    if (mSrcAccount == mEffectiveDst &&
        mSrcCurrency == mDstAmount.getCurrency())
    {
//默认路径可能有效，但任何路径都会循环
        return true;
    }

    m_loadEvent = app_.getJobQueue ().makeLoadEvent (
        jtPATH_FIND, "FindPath");
    auto currencyIsXRP = isXRP (mSrcCurrency);

    bool useIssuerAccount
            = mSrcIssuer && !currencyIsXRP && !isXRP (*mSrcIssuer);
    auto& account = useIssuerAccount ? *mSrcIssuer : mSrcAccount;
    auto issuer = currencyIsXRP ? AccountID() : account;
    mSource = STPathElement (account, mSrcCurrency, issuer);
    auto issuerString = mSrcIssuer
            ? to_string (*mSrcIssuer) : std::string ("none");
    JLOG (j_.trace())
            << "findPaths>"
            << " mSrcAccount=" << mSrcAccount
            << " mDstAccount=" << mDstAccount
            << " mDstAmount=" << mDstAmount.getFullText ()
            << " mSrcCurrency=" << mSrcCurrency
            << " mSrcIssuer=" << issuerString;

    if (!mLedger)
    {
        JLOG (j_.debug()) << "findPaths< no ledger";
        return false;
    }

    bool bSrcXrp = isXRP (mSrcCurrency);
    bool bDstXrp = isXRP (mDstAmount.getCurrency());

    if (! mLedger->exists (keylet::account(mSrcAccount)))
    {
//我们甚至不能在没有源帐户的情况下开始。
        JLOG (j_.debug()) << "invalid source account";
        return false;
    }

    if ((mEffectiveDst != mDstAccount) &&
        ! mLedger->exists (keylet::account(mEffectiveDst)))
    {
        JLOG (j_.debug())
            << "Non-existent gateway";
        return false;
    }

    if (! mLedger->exists (keylet::account (mDstAccount)))
    {
//找不到目标帐户-我们必须为新帐户提供资金
//帐户。
        if (!bDstXrp)
        {
            JLOG (j_.debug())
                    << "New account not being funded in XRP ";
            return false;
        }

        auto const reserve = STAmount (mLedger->fees().accountReserve (0));
        if (mDstAmount < reserve)
        {
            JLOG (j_.debug())
                    << "New account not getting enough funding: "
                    << mDstAmount << " < " << reserve;
            return false;
        }
    }

//现在根据源和目标的类型计算付款类型
//货币。
    PaymentType paymentType;
    if (bSrcXrp && bDstXrp)
    {
//XRP＞XRP
        JLOG (j_.debug()) << "XRP to XRP payment";
        paymentType = pt_XRP_to_XRP;
    }
    else if (bSrcXrp)
    {
//XRP＞非XRP
        JLOG (j_.debug()) << "XRP to non-XRP payment";
        paymentType = pt_XRP_to_nonXRP;
    }
    else if (bDstXrp)
    {
//非XRP＞XRP
        JLOG (j_.debug()) << "non-XRP to XRP payment";
        paymentType = pt_nonXRP_to_XRP;
    }
    else if (mSrcCurrency == mDstAmount.getCurrency ())
    {
//非xrp->非xrp-相同货币
        JLOG (j_.debug()) << "non-XRP to non-XRP - same currency";
        paymentType = pt_nonXRP_to_same;
    }
    else
    {
//非瑞波对非瑞波-不同货币
        JLOG (j_.debug()) << "non-XRP to non-XRP - cross currency";
        paymentType = pt_nonXRP_to_nonXRP;
    }

//现在迭代该paymentType的所有路径。
    for (auto const& costedPath : mPathTable[paymentType])
    {
//最多只能使用当前搜索级别的路径。
        if (costedPath.searchLevel <= searchLevel)
        {
            addPathsForType (costedPath.type);

//托多（汤姆）：我们可能会错过其他的好方法。
//任意切断。
            if (mCompletePaths.size () > PATHFINDER_MAX_COMPLETE_PATHS)
                break;
        }
    }

    JLOG (j_.debug())
            << mCompletePaths.size () << " complete paths found";

//即使我们找不到路径，默认路径也可以工作，而且我们不检查它们。
//目前。
    return true;
}

TER Pathfinder::getPathLiquidity (
STPath const& path,            //在：要检查的路径。
STAmount const& minDstAmount,  //in:此路径必须的最小输出
//交付是值得保留的。
STAmount& amountOut,           //流出：流动性的实际路径。
uint64_t& qualityOut) const    //输出：返回的初始质量
{
    STPathSet pathSet;
    pathSet.push_back (path);

    path::RippleCalc::Input rcInput;
    rcInput.defaultPathsAllowed = false;

    PaymentSandbox sandbox (&*mLedger, tapNONE);

    try
    {
//计算至少提供最低流动性的路径。
        if (convert_all_)
            rcInput.partialPaymentAllowed = true;

        auto rc = path::RippleCalc::rippleCalculate (
            sandbox,
            mSrcAmount,
            minDstAmount,
            mDstAccount,
            mSrcAccount,
            pathSet,
            app_.logs(),
            &rcInput);
//如果我们连最低流动性要求都得不到，我们就完了。
        if (rc.result () != tesSUCCESS)
            return rc.result ();

        qualityOut = getRate (rc.actualAmountOut, rc.actualAmountIn);
        amountOut = rc.actualAmountOut;

        if (! convert_all_)
        {
//现在试着计算剩余的流动性。
            rcInput.partialPaymentAllowed = true;
            rc = path::RippleCalc::rippleCalculate (
                sandbox,
                mSrcAmount,
                mDstAmount - amountOut,
                mDstAccount,
                mSrcAccount,
                pathSet,
                app_.logs (),
                &rcInput);

//如果我们发现更多的流动性，将其添加到结果中。
            if (rc.result () == tesSUCCESS)
                amountOut += rc.actualAmountOut;
        }

        return tesSUCCESS;
    }
    catch (std::exception const& e)
    {
        JLOG (j_.info()) <<
            "checkpath: exception (" << e.what() << ") " <<
            path.getJson (0);
        return tefEXCEPTION;
    }
}

namespace {

//返回给定金额的最小可用流动性，以及
//必须评估的路径总数。
STAmount smallestUsefulAmount (STAmount const& amount, int maxPaths)
{
    return divide (amount, STAmount (maxPaths + 2), amount.issue ());
}

} //命名空间

void Pathfinder::computePathRanks (int maxPaths)
{
    mRemainingAmount = convert_all_ ?
        STAmount(mDstAmount.issue(), STAmount::cMaxValue,
            STAmount::cMaxOffset)
        : mDstAmount;

//必须从剩余金额中减去默认路径中的流动性。
    try
    {
        PaymentSandbox sandbox (&*mLedger, tapNONE);

        path::RippleCalc::Input rcInput;
        rcInput.partialPaymentAllowed = true;
        auto rc = path::RippleCalc::rippleCalculate (
            sandbox,
            mSrcAmount,
            mRemainingAmount,
            mDstAccount,
            mSrcAccount,
            STPathSet(),
            app_.logs (),
            &rcInput);

        if (rc.result () == tesSUCCESS)
        {
            JLOG (j_.debug())
                    << "Default path contributes: " << rc.actualAmountIn;
            mRemainingAmount -= rc.actualAmountOut;
        }
        else
        {
            JLOG (j_.debug())
                << "Default path fails: " << transToken (rc.result ());
        }
    }
    catch (std::exception const&)
    {
        JLOG (j_.debug()) << "Default path causes exception";
    }

    rankPaths (maxPaths, mCompletePaths, mPathRanks);
}

static bool isDefaultPath (STPath const& path)
{
//TODO（TOM）：默认路径可以由多个帐户组成：
//https://forum.ripple.com/viewtopic.php？F=2&T=8206&Start=10 P57713
//
//乔尔卡茨写道：
//因此，测试路径是否为默认路径是不正确的。我不是
//当然，修复的复杂性是值得的。如果我们要修复
//我建议这样做：
//
//1）计算默认路径，可能使用“expand path”展开
//空路径。2）切断源节点和目的节点。
//
//3）在寻径循环中，如果源颁发者不是发送者，
//拒绝不以颁发者的帐户节点开头或不匹配的所有路径
//我们在第2步建立的路径。
    return path.size() == 1;
}

static STPath removeIssuer (STPath const& path)
{
//此路径以颁发者开始，这已经是隐含的
//所以移除头部节点
    STPath ret;

    for (auto it = path.begin() + 1; it != path.end(); ++it)
        ret.push_back (*it);

    return ret;
}

//对于输入路径集中的每个有用路径，
//在路径等级的输出向量中创建等级条目
void Pathfinder::rankPaths (
    int maxPaths,
    STPathSet const& paths,
    std::vector <PathRank>& rankedPaths)
{
    rankedPaths.clear ();
    rankedPaths.reserve (paths.size());

    STAmount saMinDstAmount;
    if (convert_all_)
    {
//在转换时，“允许的所有部分付款”将设置为“真”
//而要求巨额资金将是流动性最高的。
        saMinDstAmount = STAmount(mDstAmount.issue(),
            STAmount::cMaxValue, STAmount::cMaxOffset);
    }
    else
    {
//忽略移动量非常小的路径。
        saMinDstAmount = smallestUsefulAmount(mDstAmount, maxPaths);
    }

    for (int i = 0; i < paths.size (); ++i)
    {
        auto const& currentPath = paths[i];
        if (! currentPath.empty())
        {
            STAmount liquidity;
            uint64_t uQuality;
            auto const resultCode = getPathLiquidity (
                currentPath, saMinDstAmount, liquidity, uQuality);
            if (resultCode != tesSUCCESS)
            {
                JLOG (j_.debug()) <<
                    "findPaths: dropping : " <<
                    transToken (resultCode) <<
                    ": " << currentPath.getJson (0);
            }
            else
            {
                JLOG (j_.debug()) <<
                    "findPaths: quality: " << uQuality <<
                    ": " << currentPath.getJson (0);

                rankedPaths.push_back ({uQuality,
                    currentPath.size (), liquidity, i});
            }
        }
    }

//按以下方式排序路径：
//路径成本（考虑质量时）
//路径宽度
//路径长度
//一个更好的路径等级越低，最好从一开始就排序。
    std::sort(rankedPaths.begin(), rankedPaths.end(),
        [&](Pathfinder::PathRank const& a, Pathfinder::PathRank const& b)
    {
//1）质量越高（成本越低）越好
        if (! convert_all_ && a.quality != b.quality)
            return a.quality < b.quality;

//2）流动性越大（交易量越大）越好
        if (a.liquidity != b.liquidity)
            return a.liquidity > b.liquidity;

//3）路径越短越好
        if (a.length != b.length)
            return a.length < b.length;

//4）打捆机
        return a.index > b.index;
    });
}

STPathSet
Pathfinder::getBestPaths (
    int maxPaths,
    STPath& fullLiquidityPath,
    STPathSet const& extraPaths,
    AccountID const& srcIssuer)
{
    JLOG (j_.debug()) << "findPaths: " <<
        mCompletePaths.size() << " paths and " <<
        extraPaths.size () << " extras";

    if (mCompletePaths.empty() && extraPaths.empty())
        return mCompletePaths;

    assert (fullLiquidityPath.empty ());
    const bool issuerIsSender = isXRP (mSrcCurrency) || (srcIssuer == mSrcAccount);

    std::vector <PathRank> extraPathRanks;
    rankPaths (maxPaths, extraPaths, extraPathRanks);

    STPathSet bestPaths;

//最好的路径等级现在已经开始了。把它们拉到
//填充最佳路径，然后查看其余部分以找到最佳个人
//能够满足整个流动性的途径——如果存在的话。
    STAmount remaining = mRemainingAmount;

    auto pathsIterator = mPathRanks.begin();
    auto extraPathsIterator = extraPathRanks.begin();

    while (pathsIterator != mPathRanks.end() ||
        extraPathsIterator != extraPathRanks.end())
    {
        bool usePath = false;
        bool useExtraPath = false;

        if (pathsIterator == mPathRanks.end())
            useExtraPath = true;
        else if (extraPathsIterator == extraPathRanks.end())
            usePath = true;
        else if (extraPathsIterator->quality < pathsIterator->quality)
            useExtraPath = true;
        else if (extraPathsIterator->quality > pathsIterator->quality)
            usePath = true;
        else if (extraPathsIterator->liquidity > pathsIterator->liquidity)
            useExtraPath = true;
        else if (extraPathsIterator->liquidity < pathsIterator->liquidity)
            usePath = true;
        else
        {
//风险高，流动性相同
            useExtraPath = true;
            usePath = true;
        }

        auto& pathRank = usePath ? *pathsIterator : *extraPathsIterator;

        auto const& path = usePath ? mCompletePaths[pathRank.index] :
            extraPaths[pathRank.index];

        if (useExtraPath)
            ++extraPathsIterator;

        if (usePath)
            ++pathsIterator;

        auto iPathsLeft = maxPaths - bestPaths.size ();
        if (!(iPathsLeft > 0 || fullLiquidityPath.empty ()))
            break;

        if (path.empty ())
        {
            assert (false);
            continue;
        }

        bool startsWithIssuer = false;

        if (! issuerIsSender && usePath)
        {
//需要确保路径与颁发者约束匹配
            if (isDefaultPath(path) ||
                path.front().getAccountID() != srcIssuer)
            {
                continue;
            }

            startsWithIssuer = true;
        }

        if (iPathsLeft > 1 ||
            (iPathsLeft > 0 && pathRank.liquidity >= remaining))
//最后一条路径必须填充
        {
            --iPathsLeft;
            remaining -= pathRank.liquidity;
            bestPaths.push_back (startsWithIssuer ? removeIssuer (path) : path);
        }
        else if (iPathsLeft == 0 &&
                 pathRank.liquidity >= mDstAmount &&
                 fullLiquidityPath.empty ())
        {
//我们找到了一条可以移动整个数量的额外路径。
            fullLiquidityPath = (startsWithIssuer ? removeIssuer (path) : path);
            JLOG (j_.debug()) <<
                "Found extra full path: " << fullLiquidityPath.getJson (0);
        }
        else
        {
            JLOG (j_.debug()) <<
                "Skipping a non-filling path: " << path.getJson (0);
        }
    }

    if (remaining > beast::zero)
    {
        assert (fullLiquidityPath.empty ());
        JLOG (j_.info()) <<
            "Paths could not send " << remaining << " of " << mDstAmount;
    }
    else
    {
        JLOG (j_.debug()) <<
            "findPaths: RESULTS: " << bestPaths.getJson (0);
    }
    return bestPaths;
}

bool Pathfinder::issueMatchesOrigin (Issue const& issue)
{
    bool matchingCurrency = (issue.currency == mSrcCurrency);
    bool matchingAccount =
            isXRP (issue.currency) ||
            (mSrcIssuer && issue.account == mSrcIssuer) ||
            issue.account == mSrcAccount;

    return matchingCurrency && matchingAccount;
}

int Pathfinder::getPathsOut (
    Currency const& currency,
    AccountID const& account,
    bool isDstCurrency,
    AccountID const& dstAccount)
{
    Issue const issue (currency, account);

    auto it = mPathsOutCountMap.emplace (issue, 0);

//如果它已经存在，则返回存储的路径数
    if (!it.second)
        return it.first->second;

    auto sleAccount = mLedger->read(keylet::account (account));

    if (!sleAccount)
        return 0;

    int aFlags = sleAccount->getFieldU32 (sfFlags);
    bool const bAuthRequired = (aFlags & lsfRequireAuth) != 0;
    bool const bFrozen = ((aFlags & lsfGlobalFreeze) != 0);

    int count = 0;

    if (!bFrozen)
    {
        count = app_.getOrderBookDB ().getBookSize (issue);

        for (auto const& item : mRLCache->getRippleLines (account))
        {
            RippleState* rspEntry = (RippleState*) item.get ();

            if (currency != rspEntry->getLimit ().getCurrency ())
            {
            }
            else if (rspEntry->getBalance () <= beast::zero &&
                     (!rspEntry->getLimitPeer ()
                      || -rspEntry->getBalance () >= rspEntry->getLimitPeer ()
                      ||  (bAuthRequired && !rspEntry->getAuth ())))
            {
            }
            else if (isDstCurrency &&
                     dstAccount == rspEntry->getAccountIDPeer ())
            {
count += 10000; //额外计算到目的地的路径
            }
            else if (rspEntry->getNoRipplePeer ())
            {
//这可能不是一条有用的出路
            }
            else if (rspEntry->getFreezePeer ())
            {
//不是一条有用的出路
            }
            else
            {
                ++count;
            }
        }
    }
    it.first->second = count;
    return count;
}

void Pathfinder::addLinks (
STPathSet const& currentPaths,  //从中构建的路径
STPathSet& incompletePaths,     //我们添加到的部分路径集
    int addFlags)
{
    JLOG (j_.debug())
        << "addLink< on " << currentPaths.size ()
        << " source(s), flags=" << addFlags;
    for (auto const& path: currentPaths)
        addLink (path, incompletePaths, addFlags);
}

STPathSet& Pathfinder::addPathsForType (PathType const& pathType)
{
//查看此类型的路径集是否已存在。
    auto it = mPaths.find (pathType);
    if (it != mPaths.end ())
        return it->second;

//否则，如果类型没有节点，则返回空路径。
    if (pathType.empty ())
        return mPaths[pathType];

//否则，通过调用
//递归添加路径类型。
    PathType parentPathType = pathType;
    parentPathType.pop_back ();

    STPathSet const& parentPaths = addPathsForType (parentPathType);
    STPathSet& pathsOut = mPaths[pathType];

    JLOG (j_.debug())
        << "getPaths< adding onto '"
        << pathTypeToString (parentPathType) << "' to get '"
        << pathTypeToString (pathType) << "'";

    int initialSize = mCompletePaths.size ();

//将最后一个节点类型添加到列表中。
    auto nodeType = pathType.back ();
    switch (nodeType)
    {
    case nt_SOURCE:
//源必须始终在开始处，因此pathsout必须为空。
        assert (pathsOut.empty ());
        pathsOut.push_back (STPath ());
        break;

    case nt_ACCOUNTS:
        addLinks (parentPaths, pathsOut, afADD_ACCOUNTS);
        break;

    case nt_BOOKS:
        addLinks (parentPaths, pathsOut, afADD_BOOKS);
        break;

    case nt_XRP_BOOK:
        addLinks (parentPaths, pathsOut, afADD_BOOKS | afOB_XRP);
        break;

    case nt_DEST_BOOK:
        addLinks (parentPaths, pathsOut, afADD_BOOKS | afOB_LAST);
        break;

    case nt_DESTINATION:
//Fixme:如果在
//目的地金额？
//托多（汤姆）：这是什么意思？应该是圣战吗？
        addLinks (parentPaths, pathsOut, afADD_ACCOUNTS | afAC_LAST);
        break;
    }

    if (mCompletePaths.size () != initialSize)
    {
        JLOG (j_.debug())
            << (mCompletePaths.size () - initialSize)
            << " complete paths added";
    }

    JLOG (j_.debug())
        << "getPaths> " << pathsOut.size () << " partial paths found";
    return pathsOut;
}

bool Pathfinder::isNoRipple (
    AccountID const& fromAccount,
    AccountID const& toAccount,
    Currency const& currency)
{
    auto sleRipple = mLedger->read(keylet::line(
        toAccount, fromAccount, currency));

    auto const flag ((toAccount > fromAccount)
                     ? lsfHighNoRipple : lsfLowNoRipple);

    return sleRipple && (sleRipple->getFieldU32 (sfFlags) & flag);
}

//此路径是否以最后一个帐户具有的“帐户到帐户”链接结束？
//在链接上设置“无波纹”？
bool Pathfinder::isNoRippleOut (STPath const& currentPath)
{
//必须至少有一个链接。
    if (currentPath.empty ())
        return false;

//最后一个链接必须是帐户。
    STPathElement const& endElement = currentPath.back ();
    if (!(endElement.getNodeType () & STPathElement::typeAccount))
        return false;

//如果路径中只有一个项，则如果该项指定了
//输出无纹波。输出上没有波纹的路径不能
//然后是输入端没有纹波的链路。
    auto const& fromAccount = (currentPath.size () == 1)
        ? mSrcAccount
        : (currentPath.end () - 2)->getAccountID ();
    auto const& toAccount = endElement.getAccountID ();
    return isNoRipple (fromAccount, toAccount, endElement.getCurrency ());
}

void addUniquePath (STPathSet& pathSet, STPath const& path)
{
//todo（tom）：以这种方式构建stpathset的大小是二次的
//在太空船上！
    for (auto const& p : pathSet)
    {
        if (p == path)
            return;
    }
    pathSet.push_back (path);
}

void Pathfinder::addLink (
const STPath& currentPath,      //从中构建的路径
STPathSet& incompletePaths,     //我们添加到的部分路径集
    int addFlags)
{
    auto const& pathEnd = currentPath.empty() ? mSource : currentPath.back ();
    auto const& uEndCurrency = pathEnd.getCurrency ();
    auto const& uEndIssuer = pathEnd.getIssuerID ();
    auto const& uEndAccount = pathEnd.getAccountID ();
    bool const bOnXRP = uEndCurrency.isZero ();

//寻路真的需要把这个
//网关（目的地金额的颁发者）
//而不是最终目的地？
    bool const hasEffectiveDestination = mEffectiveDst != mDstAccount;

    JLOG (j_.trace()) << "addLink< flags="
                                   << addFlags << " onXRP=" << bOnXRP;
    JLOG (j_.trace()) << currentPath.getJson (0);

    if (addFlags & afADD_ACCOUNTS)
    {
//增加帐户
        if (bOnXRP)
        {
            if (mDstAmount.native () && !currentPath.empty ())
{ //到xrp目的地的非默认路径
                JLOG (j_.trace())
                    << "complete path found ax: " << currentPath.getJson(0);
                addUniquePath (mCompletePaths, currentPath);
            }
        }
        else
        {
//搜索要添加的帐户
            auto const sleEnd = mLedger->read(keylet::account(uEndAccount));

            if (sleEnd)
            {
                bool const bRequireAuth (
                    sleEnd->getFieldU32 (sfFlags) & lsfRequireAuth);
                bool const bIsEndCurrency (
                    uEndCurrency == mDstAmount.getCurrency ());
                bool const bIsNoRippleOut (
                    isNoRippleOut (currentPath));
                bool const bDestOnly (
                    addFlags & afAC_LAST);

                auto& rippleLines (mRLCache->getRippleLines (uEndAccount));

                AccountCandidates candidates;
                candidates.reserve (rippleLines.size ());

                for (auto const& item : rippleLines)
                {
                    auto* rs = dynamic_cast<RippleState const *> (item.get ());
                    if (!rs)
                    {
                        JLOG (j_.error())
                                << "Couldn't decipher RippleState";
                        continue;
                    }
                    auto const& acct = rs->getAccountIDPeer ();

                    if (hasEffectiveDestination && (acct == mDstAccount))
                    {
//我们跳过了入口
                        continue;
                    }

                    bool bToDestination = acct == mEffectiveDst;

                    if (bDestOnly && !bToDestination)
                    {
                        continue;
                    }

                    if ((uEndCurrency == rs->getLimit ().getCurrency ()) &&
                        !currentPath.hasSeen (acct, uEndCurrency, acct))
                    {
//路径是正确的货币，尚未看到
                        if (rs->getBalance () <= beast::zero
                            && (!rs->getLimitPeer ()
                                || -rs->getBalance () >= rs->getLimitPeer ()
                                || (bRequireAuth && !rs->getAuth ())))
                        {
//路径没有信用
                        }
                        else if (bIsNoRippleOut && rs->getNoRipple ())
                        {
//不能离开这条路
                        }
                        else if (bToDestination)
                        {
//目的地总是值得尝试的
                            if (uEndCurrency == mDstAmount.getCurrency ())
                            {
//这是一条完整的路
                                if (!currentPath.empty ())
                                {
                                    JLOG (j_.trace())
                                            << "complete path found ae: "
                                            << currentPath.getJson (0);
                                    addUniquePath
                                            (mCompletePaths, currentPath);
                                }
                            }
                            else if (!bDestOnly)
                            {
//这是一个高优先级的候选人
                                candidates.push_back (
                                    {AccountCandidate::highPriority, acct});
                            }
                        }
                        else if (acct == mSrcAccount)
                        {
//回到源头是不好的
                        }
                        else
                        {
//保存此候选人
                            int out = getPathsOut (
                                uEndCurrency,
                                acct,
                                bIsEndCurrency,
                                mEffectiveDst);
                            if (out)
                                candidates.push_back ({out, acct});
                        }
                    }
                }

                if (!candidates.empty())
                {
                    std::sort (candidates.begin (), candidates.end (),
                        std::bind(compareAccountCandidate,
                                  mLedger->seq(),
                                  std::placeholders::_1,
                                  std::placeholders::_2));

                    int count = candidates.size ();
//允许来自源的更多路径
                    if ((count > 10) && (uEndAccount != mSrcAccount))
                        count = 10;
                    else if (count > 50)
                        count = 50;

                    auto it = candidates.begin();
                    while (count-- != 0)
                    {
//向不完整路径添加帐户
                        STPathElement pathElement (
                            STPathElement::typeAccount,
                            it->account,
                            uEndCurrency,
                            it->account);
                        incompletePaths.assembleAdd (currentPath, pathElement);
                        ++it;
                    }
                }

            }
            else
            {
                JLOG (j_.warn())
                    << "Path ends on non-existent issuer";
            }
        }
    }
    if (addFlags & afADD_BOOKS)
    {
//增加订单图书
        if (addFlags & afOB_XRP)
        {
//仅限于XRP
            if (!bOnXRP && app_.getOrderBookDB ().isBookToXRP (
                    {uEndCurrency, uEndIssuer}))
            {
                STPathElement pathElement(
                    STPathElement::typeCurrency,
                    xrpAccount (),
                    xrpCurrency (),
                    xrpAccount ());
                incompletePaths.assembleAdd (currentPath, pathElement);
            }
        }
        else
        {
            bool bDestOnly = (addFlags & afOB_LAST) != 0;
            auto books = app_.getOrderBookDB ().getBooksByTakerPays(
                {uEndCurrency, uEndIssuer});
            JLOG (j_.trace())
                << books.size () << " books found from this currency/issuer";

            for (auto const& book : books)
            {
                if (!currentPath.hasSeen (
                        xrpAccount(),
                        book->getCurrencyOut (),
                        book->getIssuerOut ()) &&
                    !issueMatchesOrigin (book->book ().out) &&
                    (!bDestOnly ||
                     (book->getCurrencyOut () == mDstAmount.getCurrency ())))
                {
                    STPath newPath (currentPath);

                    if (book->getCurrencyOut().isZero())
{ //XRP

//添加订单簿本身
                        newPath.emplace_back (
                            STPathElement::typeCurrency,
                            xrpAccount (),
                            xrpCurrency (),
                            xrpAccount ());

                        if (mDstAmount.getCurrency ().isZero ())
                        {
//目的地是xrp，添加帐户和路径是
//完成
                            JLOG (j_.trace())
                                << "complete path found bx: "
                                << currentPath.getJson(0);
                            addUniquePath (mCompletePaths, newPath);
                        }
                        else
                            incompletePaths.push_back (newPath);
                    }
                    else if (!currentPath.hasSeen(
                        book->getIssuerOut (),
                        book->getCurrencyOut (),
                        book->getIssuerOut ()))
                    {
//如果我们已经看过发行人，就不要这本书了
//账簿->账户->账簿
                        if ((newPath.size() >= 2) &&
                            (newPath.back().isAccount ()) &&
                            (newPath[newPath.size() - 2].isOffer ()))
                        {
//将多余的帐户替换为订单簿
                            newPath[newPath.size() - 1] = STPathElement (
                                STPathElement::typeCurrency | STPathElement::typeIssuer,
                                xrpAccount(), book->getCurrencyOut(),
                                book->getIssuerOut());
                        }
                        else
                        {
//添加订单簿
                            newPath.emplace_back(
                                STPathElement::typeCurrency | STPathElement::typeIssuer,
                                xrpAccount(), book->getCurrencyOut(),
                                book->getIssuerOut());
                        }

                        if (hasEffectiveDestination &&
                            book->getIssuerOut() == mDstAccount &&
                            book->getCurrencyOut() == mDstAmount.getCurrency())
                        {
//我们跳过了一个必需的发行人
                        }
                        else if (book->getIssuerOut() == mEffectiveDst &&
                            book->getCurrencyOut() == mDstAmount.getCurrency())
{ //使用目标帐户，此路径已完成
                            JLOG (j_.trace())
                                << "complete path found ba: "
                                << currentPath.getJson(0);
                            addUniquePath (mCompletePaths, newPath);
                        }
                        else
                        {
//添加颁发者的帐户，路径仍不完整
                            incompletePaths.assembleAdd(newPath,
                                STPathElement (STPathElement::typeAccount,
                                               book->getIssuerOut (),
                                               book->getCurrencyOut (),
                                               book->getIssuerOut ()));
                        }
                    }

                }
            }
        }
    }
}

namespace {

Pathfinder::PathType makePath (char const *string)
{
    Pathfinder::PathType ret;

    while (true)
    {
        switch (*string++)
        {
case 's': //来源
                ret.push_back (Pathfinder::nt_SOURCE);
                break;

case 'a': //账户
                ret.push_back (Pathfinder::nt_ACCOUNTS);
                break;

case 'b': //书
                ret.push_back (Pathfinder::nt_BOOKS);
                break;

case 'x': //XRP图书
                ret.push_back (Pathfinder::nt_XRP_BOOK);
                break;

case 'f': //记账本位币
                ret.push_back (Pathfinder::nt_DEST_BOOK);
                break;

            case 'd':
//目的地（有账户，如果需要，但还没有
//现在）。
                ret.push_back (Pathfinder::nt_DESTINATION);
                break;

            case 0:
                return ret;
        }
    }
}

void fillPaths (Pathfinder::PaymentType type, PathCostList const& costs)
{
    auto& list = mPathTable[type];
    assert (list.empty());
    for (auto& cost: costs)
        list.push_back ({cost.cost, makePath (cost.path)});
}

} //命名空间


//成本：
//0=最低付款额
//1=包括使普通案例工作的普通路径
//4=正常快速搜索级别
//7=正常缓慢搜索级别
//10=最强烈

void Pathfinder::initPathTable ()
{
//注意：不要包含构建默认路径的规则

    mPathTable.clear();
    fillPaths (pt_XRP_to_XRP, {});

    fillPaths(
        pt_XRP_to_nonXRP, {
{1, "sfd"},   //来源->书籍->网关
{3, "sfad"},  //来源->账簿->账户->目的地
{5, "sfaad"}, //来源->账簿->账户->账户->目的地
{6, "sbfd"},  //来源->书籍->书籍->目的地
{8, "sbafd"}, //来源->账簿->账户->账簿->目的地
{9, "sbfad"}, //来源->账簿->账簿->账户->目的地
            {10, "sbafad"}
        });

    fillPaths(
        pt_nonXRP_to_XRP, {
{1, "sxd"},       //Gateway购买XRP
{2, "saxd"},      //source->gateway->book（xrp）->dest
            {6, "saaxd"},
            {7, "sbxd"},
            {8, "sabxd"},
            {9, "sabaxd"}
        });

//非瑞波对非瑞波（同一货币）
    fillPaths(
        pt_nonXRP_to_same,  {
{1, "sad"},     //源->网关->目标
{1, "sfd"},     //来源->书籍->目的地
{4, "safd"},    //来源->网关->书籍->目的地
            {4, "sfad"},
            {5, "saad"},
            {5, "sbfd"},
            {6, "sxfad"},
            {6, "safad"},
{6, "saxfd"},   //源->网关->book to xrp->book->
//目的地
            {6, "saxfad"},
{6, "sabfd"},   //来源->网关->书籍->书籍->目的地
            {7, "saaad"},
        });

//非瑞波对非瑞波（不同货币）
    fillPaths(
        pt_nonXRP_to_nonXRP, {
            {1, "sfad"},
            {1, "safd"},
            {3, "safad"},
            {4, "sxfd"},
            {5, "saxfd"},
            {5, "sxfad"},
            {5, "sbfd"},
            {6, "saxfad"},
            {6, "sabfd"},
            {7, "saafd"},
            {8, "saafad"},
            {9, "safaad"},
        });
}

} //涟漪
