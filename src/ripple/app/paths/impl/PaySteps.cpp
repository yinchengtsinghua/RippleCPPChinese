
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

#include <ripple/app/paths/impl/Steps.h>
#include <ripple/basics/contract.h>
#include <ripple/json/json_writer.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/XRPAmount.h>

#include <algorithm>
#include <numeric>
#include <sstream>

namespace ripple {

//检查是否与公差相等
bool checkNear (IOUAmount const& expected, IOUAmount const& actual)
{
    double const ratTol = 0.001;
    if (abs (expected.exponent () - actual.exponent ()) > 1)
        return false;

    if (actual.exponent () < -20)
        return true;

    auto const a = (expected.exponent () < actual.exponent ())
        ? expected.mantissa () / 10
        : expected.mantissa ();
    auto const b = (actual.exponent () < expected.exponent ())
        ? actual.mantissa () / 10
        : actual.mantissa ();
    if (a == b)
        return true;

    double const diff = std::abs (a - b);
    auto const r = diff / std::max (std::abs (a), std::abs (b));
    return r <= ratTol;
};

bool checkNear (XRPAmount const& expected, XRPAmount const& actual)
{
    return expected == actual;
};

static
bool isXRPAccount (STPathElement const& pe)
{
    if (pe.getNodeType () != STPathElement::typeAccount)
        return false;
    return isXRP (pe.getAccountID ());
};


static
std::pair<TER, std::unique_ptr<Step>>
toStep (
    StrandContext const& ctx,
    STPathElement const* e1,
    STPathElement const* e2,
    Issue const& curIssue)
{
    auto& j = ctx.j;

    if (ctx.isFirst && e1->isAccount () &&
        (e1->getNodeType () & STPathElement::typeCurrency) &&
        isXRP (e1->getCurrency ()))
    {
        return make_XRPEndpointStep (ctx, e1->getAccountID ());
    }

    if (ctx.isLast && isXRPAccount (*e1) && e2->isAccount())
        return make_XRPEndpointStep (ctx, e2->getAccountID());

    if (e1->isAccount() && e2->isAccount())
    {
        return make_DirectStepI (ctx, e1->getAccountID (),
            e2->getAccountID (), curIssue.currency);
    }

    if (e1->isOffer() && e2->isAccount())
    {
//应该已经处理好了
        JLOG (j.error())
            << "Found offer/account payment step. Aborting payment strand.";
        assert (0);
        if (ctx.view.rules().enabled(fix1373))
            return {temBAD_PATH, std::unique_ptr<Step>{}};
        Throw<FlowException> (tefEXCEPTION, "Found offer/account payment step.");
    }

    assert ((e2->getNodeType () & STPathElement::typeCurrency) ||
        (e2->getNodeType () & STPathElement::typeIssuer));
    auto const outCurrency = e2->getNodeType () & STPathElement::typeCurrency
        ? e2->getCurrency ()
        : curIssue.currency;
    auto const outIssuer = e2->getNodeType () & STPathElement::typeIssuer
        ? e2->getIssuerID ()
        : curIssue.account;

    if (isXRP (curIssue.currency) && isXRP (outCurrency))
    {
        JLOG (j.warn()) << "Found xrp/xrp offer payment step";
        return {temBAD_PATH, std::unique_ptr<Step>{}};
    }

    assert (e2->isOffer ());

    if (isXRP (outCurrency))
        return make_BookStepIX (ctx, curIssue);

    if (isXRP (curIssue.currency))
        return make_BookStepXI (ctx, {outCurrency, outIssuer});

    return make_BookStepII (ctx, curIssue, {outCurrency, outIssuer});
}

std::pair<TER, Strand>
toStrandV1 (
    ReadView const& view,
    AccountID const& src,
    AccountID const& dst,
    Issue const& deliver,
    boost::optional<Quality> const& limitQuality,
    boost::optional<Issue> const& sendMaxIssue,
    STPath const& path,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    beast::Journal j)
{
    if (isXRP (src))
    {
        JLOG (j.debug()) << "toStrand with xrpAccount as src";
        return {temBAD_PATH, Strand{}};
    }
    if (isXRP (dst))
    {
        JLOG (j.debug()) << "toStrand with xrpAccount as dst";
        return {temBAD_PATH, Strand{}};
    }
    if (!isConsistent (deliver))
    {
        JLOG (j.debug()) << "toStrand inconsistent deliver issue";
        return {temBAD_PATH, Strand{}};
    }
    if (sendMaxIssue && !isConsistent (*sendMaxIssue))
    {
        JLOG (j.debug()) << "toStrand inconsistent sendMax issue";
        return {temBAD_PATH, Strand{}};
    }

    Issue curIssue = [&]
    {
        auto& currency =
            sendMaxIssue ? sendMaxIssue->currency : deliver.currency;
        if (isXRP (currency))
            return xrpIssue ();
        return Issue{currency, src};
    }();

    STPathElement const firstNode (
        STPathElement::typeAll, src, curIssue.currency, curIssue.account);

    boost::optional<STPathElement> sendMaxPE;
    if (sendMaxIssue && sendMaxIssue->account != src &&
        (path.empty () || !path[0].isAccount () ||
            path[0].getAccountID () != sendMaxIssue->account))
        sendMaxPE.emplace (sendMaxIssue->account, boost::none, boost::none);

    STPathElement const lastNode (dst, boost::none, boost::none);

    auto hasCurrency = [](STPathElement const* pe)
    {
        return pe->getNodeType () & STPathElement::typeCurrency;
    };

    boost::optional<STPathElement> deliverOfferNode;
    boost::optional<STPathElement> deliverAccountNode;

    std::vector<STPathElement const*> pes;
//为路径、隐含的源、目标、
//sendmax和deliver。
    pes.reserve (4 + path.size ());
    pes.push_back (&firstNode);
    if (sendMaxPE)
        pes.push_back (&*sendMaxPE);
    for (auto& i : path)
        pes.push_back (&i);

//请注意，对于报价交叉（仅限），即使
//所有的变化都是issue.account。
    STPathElement const* const lastCurrency =
        *std::find_if (pes.rbegin(), pes.rend(), hasCurrency);
    if ((lastCurrency->getCurrency() != deliver.currency) ||
        (offerCrossing && lastCurrency->getIssuerID() != deliver.account))
    {
        deliverOfferNode.emplace (boost::none, deliver.currency, deliver.account);
        pes.push_back (&*deliverOfferNode);
    }
    if (!((pes.back ()->isAccount() && pes.back ()->getAccountID () == deliver.account) ||
          (lastNode.isAccount() && lastNode.getAccountID () == deliver.account)))
    {
        deliverAccountNode.emplace (deliver.account, boost::none, boost::none);
        pes.push_back (&*deliverAccountNode);
    }
    if (*pes.back() != lastNode)
        pes.push_back (&lastNode);

    auto const strandSrc = firstNode.getAccountID ();
    auto const strandDst = lastNode.getAccountID ();
    bool const isDefaultPath = path.empty();

    Strand result;
    result.reserve (2 * pes.size ());

    /*流不能多次包含同一帐户节点
       用同样的货币。在直接步骤中，帐户将显示
       最多两次：一次作为SRC，一次作为DST（因此是两个元素数组）。
       StrandSRC和StrandDST将只显示一次。
    **/

    std::array<boost::container::flat_set<Issue>, 2> seenDirectIssues;
//一股股票不得多次包含同一报价书。
    boost::container::flat_set<Issue> seenBookOuts;
    seenDirectIssues[0].reserve (pes.size());
    seenDirectIssues[1].reserve (pes.size());
    seenBookOuts.reserve (pes.size());
    auto ctx = [&](bool isLast = false)
    {
        return StrandContext{view, result, strandSrc, strandDst, deliver,
            limitQuality, isLast, ownerPaysTransferFee, offerCrossing,
            isDefaultPath, seenDirectIssues, seenBookOuts, j};
    };

    for (int i = 0; i < pes.size () - 1; ++i)
    {
        /*循环访问路径元素，成对考虑它们。
           对的第一个元素是“cur”，第二个元素是
           “下一个”。如果报价是其中一对，则创建的步骤将用于
           “下一个”。这意味着当“cur”是报价，“next”是
           帐户，则不会创建任何步骤，因为已经为创建了步骤
           那个提议。
        **/

        boost::optional<STPathElement> impliedPE;
        auto cur = pes[i];
        auto next = pes[i + 1];

        if (cur->isNone() || next->isNone())
            return {temBAD_PATH, Strand{}};

        /*如果同时设置了帐户和颁发者，请使用该帐户
           （并且不要为发行人插入隐含的节点）。
           这与上一代付款代码的行为匹配
        **/

        if (cur->isAccount())
            curIssue.account = cur->getAccountID ();
        else if (cur->hasIssuer())
            curIssue.account = cur->getIssuerID ();

        if (cur->hasCurrency())
            curIssue.currency = cur->getCurrency ();

        if (cur->isAccount() && next->isAccount())
        {
            if (!isXRP (curIssue.currency) &&
                curIssue.account != cur->getAccountID () &&
                curIssue.account != next->getAccountID ())
            {
                JLOG (j.trace()) << "Inserting implied account";
                auto msr = make_DirectStepI (ctx(), cur->getAccountID (),
                    curIssue.account, curIssue.currency);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
                Currency dummy;
                impliedPE.emplace (STPathElement::typeAccount,
                    curIssue.account, dummy, curIssue.account);
                cur = &*impliedPE;
            }
        }
        else if (cur->isAccount() && next->isOffer())
        {
            if (curIssue.account != cur->getAccountID ())
            {
                JLOG (j.trace()) << "Inserting implied account before offer";
                auto msr = make_DirectStepI (ctx(), cur->getAccountID (),
                    curIssue.account, curIssue.currency);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
                Currency dummy;
                impliedPE.emplace (STPathElement::typeAccount,
                    curIssue.account, dummy, curIssue.account);
                cur = &*impliedPE;
            }
        }
        else if (cur->isOffer() && next->isAccount())
        {
            if (curIssue.account != next->getAccountID () &&
                !isXRP (next->getAccountID ()))
            {
                JLOG (j.trace()) << "Inserting implied account after offer";
                auto msr = make_DirectStepI (ctx(), curIssue.account,
                    next->getAccountID (), curIssue.currency);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
            }
            continue;
        }

        if (!next->isOffer() &&
            next->hasCurrency() && next->getCurrency () != curIssue.currency)
        {
            auto const& nextCurrency = next->getCurrency ();
            auto const& nextIssuer =
                next->hasIssuer () ? next->getIssuerID () : curIssue.account;

            if (isXRP (curIssue.currency))
            {
                JLOG (j.trace()) << "Inserting implied XI offer";
                auto msr = make_BookStepXI (
                    ctx(), {nextCurrency, nextIssuer});
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
            }
            else if (isXRP (nextCurrency))
            {
                JLOG (j.trace()) << "Inserting implied IX offer";
                auto msr = make_BookStepIX (ctx(), curIssue);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
            }
            else
            {
                JLOG (j.trace()) << "Inserting implied II offer";
                auto msr = make_BookStepII (
                    ctx(), curIssue, {nextCurrency, nextIssuer});
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
            }
            impliedPE.emplace (
                boost::none, nextCurrency, nextIssuer);
            cur = &*impliedPE;
            curIssue.currency = nextCurrency;
            curIssue.account = nextIssuer;
        }

        auto s =
            /*tep（ctx（/*islast*/i==pes.size（）-2），cur，next，curissue）；
        如果（s.first==tessuccess）
            result.emplace_back（std:：move（s.second））；
        其他的
        {
            jLog（j.debug（））<“ToStep失败：”<<s.first；
            返回S.First，Strand
        }
    }

    返回tessuccess，std:：move（result）
}


标准：：对<ter，股>
ToSTANDV2（ToSTANDV2）
    readview常量视图（&V）
    accountID const&src，
    accountID const&dst，会计编号
    发布施工与交付，
    boost：：可选<quality>const&limitquality，
    boost：：可选<issue>const&sendmaxissue，
    stpath常量和路径，
    Bool OwnerPaystrasferfee公司，
    布尔报价交叉，
    野兽：：期刊j）
{
    如果（isxrp（src）isxrp（dst）
        ！isconsistent（deliver）（sendmaxissue&&！isconsistent（*sendmaxissue）））
        返回tembad_path，strand

    if（（sendmaxissue&&sendmaxissue->account==noaccount（））
        （src==noaccount（））_
        （dst==noaccount（））_
        （delivery.account==noaccount（））
        返回tembad_path，strand

    用于（auto const&pe:path）
    {
        auto const t=pe.getNodeType（）；

        如果（（t&~stPathElement:：typeAll）！t）
            返回tembad_path，strand

        bool const hasaccount=t&stpathelement:：typeaccount；
        bool const hasIssuer=t&stPathElement:：typeIssuer；
        bool const hascurrency=t&stpathElement:：typecurrency；

        if（hasaccound&（hasIssuer hasCurrency））。
            返回tembad_path，strand

        if（hasIssuer和isxrp（pe.getIssuerID（））
            返回tembad_path，strand

        if（hasAccount和isxrp（pe.getAccountID（））
            返回tembad_path，strand

        如果（有货币和发行人）
            isxrp（pe.getcurrency（））！=isxrp（pe.getIssuerID（））
            返回tembad_path，strand

        if（hasIssuer&（pe.getIssuerID（）==noAccount（））
            返回tembad_path，strand

        if（hasaccount&（pe.getaccountid（）==noaccount（）））
            返回tembad_path，strand
    }

    发行证券=[&]
    {
        自动常量和货币=
            SunMax发行？sendmaxissue->currency:deliver.currency；
        if（isxrp（货币））
            返回xrpissue（）；
        退换货货币，SRC
    }（）；

    自动hasCurrency=[]（stpathElement const pe）
    {
        返回pe.getNodeType（）&stPathElement:：typecurrency；
    }；

    std:：vector<stpathelement>normpath；
    //为路径、隐含的源、目标、
    //sendmax和deliver。
    normpath.reserve（4+path.size（））；
    {
        normpath.emplace_back（
            stpathElement:：typeall，src，curissue.currency，curissue.account）；

        如果（sendmaxissue和sendmaxissue->account！= SRC & &
            （path.empty（）！路径[0].isaccount（）
             路径[0].getAccountID（）！=sendmaxissue->account）
        {
            normpath.emplace_back（sendmaxissue->account，boost:：none，boost:：none）；
        }

        用于（auto const&i:path）
            normpath.推回（i）；

        {
            //请注意，对于报价交叉（仅限），我们确实使用报价书
            //即使所有更改都是issue.account。
            stpathElement常量和lastCurrency=
                *std：：查找_if（normpath.rbegin（），normpath.rend（），
                    HasCurror；
            如果（（lastcurrency.getcurrency（）！=交货货币）
                （报价和
                    lastcurrency.getIssuerID（）！=交货账户）
            {
                normpath.emplace_back（
                    boost：：无，deliver.currency，deliver.account）；
            }
        }

        如果（！）（（normpath.back（）.isaccount（）&&
               normpath.back（）.getaccountid（）==delivery.account）
              （dst==delivery.account）））
        {
            normpath.emplace_back（deliver.account，boost:：none，boost:：none）；
        }

        如果（！）normpath.back（）.isaccount（）
            normpath.back（）.getAccountID（）！= DST）
        {
            normpath.emplace_back（dst，boost:：none，boost:：none）；
        }
    }

    如果（normpath.size（）<2）
        返回tembad_path，strand

    auto const strandsrc=normpath.front（）.getAccountID（）；
    auto const strandst=normpath.back（）.getAccountID（）；
    bool const isdefaultpath=path.empty（）；

    股线结果；
    result.reserve（2*normpath.size（））；

    /*流不能多次包含同一帐户节点
       用同样的货币。在直接步骤中，帐户将显示
       最多两次：一次作为SRC，一次作为DST（因此是两个元素数组）。
       StrandSRC和StrandDST将只显示一次。
    **/

    std::array<boost::container::flat_set<Issue>, 2> seenDirectIssues;
//一股股票不得多次包含同一报价书。
    boost::container::flat_set<Issue> seenBookOuts;
    seenDirectIssues[0].reserve (normPath.size());
    seenDirectIssues[1].reserve (normPath.size());
    seenBookOuts.reserve (normPath.size());
    auto ctx = [&](bool isLast = false)
    {
        return StrandContext{view, result, strandSrc, strandDst, deliver,
            limitQuality, isLast, ownerPaysTransferFee, offerCrossing,
            isDefaultPath, seenDirectIssues, seenBookOuts, j};
    };

    for (std::size_t i = 0; i < normPath.size () - 1; ++i)
    {
        /*循环访问路径元素，成对考虑它们。
           对的第一个元素是“cur”，第二个元素是
           “下一个”。如果报价是其中一对，则创建的步骤将用于
           “下一个”。这意味着当“cur”是报价，“next”是
           帐户，则不会创建任何步骤，因为已经为创建了步骤
           那个提议。
        **/

        boost::optional<STPathElement> impliedPE;
        auto cur = &normPath[i];
        auto const next = &normPath[i + 1];

        if (cur->isAccount())
            curIssue.account = cur->getAccountID ();
        else if (cur->hasIssuer())
            curIssue.account = cur->getIssuerID ();

        if (cur->hasCurrency())
        {
            curIssue.currency = cur->getCurrency ();
            if (isXRP(curIssue.currency))
                curIssue.account = xrpAccount();
        }

        if (cur->isAccount() && next->isAccount())
        {
            if (!isXRP (curIssue.currency) &&
                curIssue.account != cur->getAccountID () &&
                curIssue.account != next->getAccountID ())
            {
                JLOG (j.trace()) << "Inserting implied account";
                auto msr = make_DirectStepI (ctx(), cur->getAccountID (),
                    curIssue.account, curIssue.currency);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
                impliedPE.emplace(STPathElement::typeAccount,
                    curIssue.account, xrpCurrency(), xrpAccount());
                cur = &*impliedPE;
            }
        }
        else if (cur->isAccount() && next->isOffer())
        {
            if (curIssue.account != cur->getAccountID ())
            {
                JLOG (j.trace()) << "Inserting implied account before offer";
                auto msr = make_DirectStepI (ctx(), cur->getAccountID (),
                    curIssue.account, curIssue.currency);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
                impliedPE.emplace(STPathElement::typeAccount,
                    curIssue.account, xrpCurrency(), xrpAccount());
                cur = &*impliedPE;
            }
        }
        else if (cur->isOffer() && next->isAccount())
        {
            if (curIssue.account != next->getAccountID () &&
                !isXRP (next->getAccountID ()))
            {
                if (isXRP(curIssue))
                {
                    if (i != normPath.size() - 2)
                        return {temBAD_PATH, Strand{}};
                    else
                    {
//最后一步。插入xrp端点步骤
                        auto msr = make_XRPEndpointStep (ctx(), next->getAccountID());
                        if (msr.first != tesSUCCESS)
                            return {msr.first, Strand{}};
                        result.push_back(std::move(msr.second));
                    }
                }
                else
                {
                    JLOG(j.trace()) << "Inserting implied account after offer";
                    auto msr = make_DirectStepI(ctx(),
                        curIssue.account, next->getAccountID(), curIssue.currency);
                    if (msr.first != tesSUCCESS)
                        return {msr.first, Strand{}};
                    result.push_back(std::move(msr.second));
                }
            }
            continue;
        }

        if (!next->isOffer() &&
            next->hasCurrency() && next->getCurrency () != curIssue.currency)
        {
//不应该发生
            assert(0);
            return {temBAD_PATH, Strand{}};
        }

        auto s =
            /*tep（ctx（/*islast*/i==normpath.size（）-2），cur，next，curissue）；
        如果（s.first==tessuccess）
            result.emplace_back（std:：move（s.second））；
        其他的
        {
            jLog（j.debug（））<“ToStep失败：”<<s.first；
            返回S.First，Strand
        }
    }

    auto checkstrand=[&]（）->bool_
        auto stepaccts=[]（step const&s）->std:：pair<accountid，accountid>
            if（auto r=s.directstepaccts（））
                返回* R；
            if（auto const r=s.bookstepbook（））
                返回std:：make_pair（r->in.account，r->out.account）；
            抛出<flowException>
                tefexception，“步骤应该是直接步骤或预定步骤”）；
            返回std:：make_pair（xrpaccount（），xrpaccount（））；
        }；

        auto-curacCount=SRC；
        自动古玩=[&]
            自动货币（&C）
                SunMax发行？sendmaxissue->currency:deliver.currency；
            if（isxrp（货币））
                返回xrpissue（）；
            退换货货币，SRC
        }（）；

        for（auto const&s:结果）
        {
            auto const accts=步骤帐户（*s）；
            如果（帐户优先！=注销）
                返回错误；

            if（auto const b=s->bookstepbook（））
            {
                如果（问题）！= B-> in）
                    返回错误；
                curissue=b->out；
            }
            其他的
            {
                curissue.account=账户秒；
            }

            curacCount=accts.second；
        }
        如果（curaccount！= DST）
            返回错误；
        如果（居里货币！=交货货币）
            返回错误；
        如果（古玩帐户！=交货账户和
            古玩账户！= DST）
            返回错误；
        回归真实；
    }；

    如果（！）检查字符串（）
    {
        jLog（j.warn（））<“流检查流失败”；
        断言（0）；
        返回tembad_path，strand
    }

    返回tessuccess，std:：move（result）
}

标准：：对<ter，股>
托斯特兰
    readview常量视图（&V）
    accountID const&src，
    accountID const&dst，会计编号
    发布施工与交付，
    boost：：可选<quality>const&limitquality，
    boost：：可选<issue>const&sendmaxissue，
    stpath常量和路径，
    Bool OwnerPaystrasferfee公司，
    布尔报价交叉，
    野兽：：期刊j）
{
    if（view.rules（）.enabled（fix1373））。
        返回到trandv2（视图、SRC、DST、交付、限制质量，
            sendmaxissue，path，ownerpaystrasferfee，offerrochscrossing，j）；
    其他的
        返回到trandv1（view、src、dst、delivery、limitquality，
            sendmaxissue，path，ownerpaystrasferfee，offerrochscrossing，j）；
}

std:：pair<ter，std:：vector<strand>>
蟾蜍
    readview常量视图（&V）
    accountID const&src，
    accountID const&dst，会计编号
    发布施工与交付，
    boost：：可选<quality>const&limitquality，
    boost：：可选<issue>const&sendmax，
    stpathset常量和路径，
    bool添加默认路径，
    Bool OwnerPaystrasferfee公司，
    布尔报价交叉，
    野兽：：期刊j）
{
    std:：vector<strand>结果；
    result.reserve（1+路径.大小（））；
    //如果流不是向量的一部分，则将其插入到结果中
    自动插入=[&]（股S）
    {
        bool const hasstrand=
            std:：find（result.begin（），result.end（），s）！=结果.结束（）；

        如果（！）哈斯特兰德）
            result.emplace_back（std:：move（s））；
    }；

    if（添加默认路径）
    {
        auto-sp=tostrand（查看、SRC、DST、交付、限制质量，
            sendmax，stpath（），ownerPaystransPerfee，offerrochsing，j）；
        自动常数=sp.first；
        自动&strand=sp.second；

        如果（T）！= TestCuess）
        {
            jLog（j.trace（））<“添加默认路径失败”；
            if（istemalformed（ter）path.empty（））
                返回ter，std:：vector<strand>
            }
        }
        else if（strend.empty（））
        {
            jLog（j.trace（））<“Tostrand failed”；
            throw<flowException>（tefeException，“tostrand returned tes&empty strand”）；
        }
        其他的
        {
            插入（std:：move（strand））；
        }
    }
    else if（paths.empty（））
    {
        jLog（j.debug（））
            <“Flow:Invalid Transaction:No Paths and Direct Ripple not allowed.”。
        返回temripe_empty，std:：vector<strand>
    }

    ter lastfailter=tessuccess；
    for（auto const&p:路径）
    {
        auto-sp=tostrand（视图、SRC、DST、交付、
            limitquality、sendmax、p、ownerpaystrasferfee、offerracross、j）；
        auto ter=sp.first；
        自动&strand=sp.second；

        如果（T）！= TestCuess）
        {
            Lastfailter=ter；
            jLog（j.trace（））
                    <“添加路径失败：ter：”<<ter<“path：”<<p.getjson（0）；
            如果（是格式（ter））
                返回ter，std:：vector<strand>
        }
        else if（strend.empty（））
        {
            jLog（j.trace（））<“Tostrand failed”；
            throw<flowException>（tefeException，“tostrand returned tes&empty strand”）；
        }
        其他的
        {
            插入（std:：move（strand））；
        }
    }

    if（result.empty（））
        返回lastfailter，std:：move（result）

    返回tessuccess，std:：move（result）
}

strandContext：：strandContext（
    阅读视图常量和视图
    std:：vector<std:：unique_ptr<step>>const&strand_ux，
    //流不能包含内部节点，
    //复制源或目标。
    账户编号：const&strandsrc ，
    账户编号：const&stranddst ，
    发布const&strandedeliver，
    boost：：可选<quality>const&limitquality_，
    波尔伊斯拉斯蒂，
    Bool OwnerPaystrasferfee_uu，
    Bool提供交叉服务
    bool是默认路径，
    std:：array<boost:：container:：flat_set<issue>，2>&seendigissues_，
    Boost:：Container:：Flat_set<issue>和SeenBookouts_，
    野兽杂志
        视图（VIEVY）
        ，Strandsrc（Strandsrc_uuu）
        ，Stranddst（Stranddst_uu）
        ，绞死肝脏（绞死肝脏）
        ，极限质量（极限质量）
        ，isfirst（strend_uu.empty（））
        ，islast（islast_uu）
        ，业主付款方（业主付款方）
        ，要约交叉（要约交叉）
        ，isdefaultpath（isdefaultpath_uu）
        ，绞合（绞合尺寸（））
        ！股空（）？链.back（）.get（）。
                     Null PTR）
        ，方向（方向）
        ，Seenbookouts（Seenbookouts_uuu）
        ，J（JY）
{
}

template<class inamt，class outamt>
布尔
ISdirectxrptoxrp（钢绞线施工和钢绞线）
{
    返回错误；
}

模板< >
布尔
isdirectxrptoxrp<xrpamount，xrpamount>（strand const&strand）
{
    返回（钢绞线尺寸（）==2）；
}

模板
布尔
isdirectxrptoxrp<xrpamount，iouamount>（strand const&strand）；
模板
布尔
isdirectxrptoxrp<iouamount，xrpamount>（strand const&strand）；
模板
布尔
isdirectxrptoxrp<iouamount，iouamount>（strand const&strand）；

} /纹波
