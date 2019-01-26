
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

#include <ripple/app/paths/Credit.h>
#include <ripple/app/paths/PathState.h>
#include <ripple/basics/Log.h>
#include <ripple/json/to_string.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/JsonFields.h>
#include <boost/lexical_cast.hpp>

namespace ripple {

//优化：计算路径增量时，请注意增量是否消耗所有
//流动性。如果所有的流动性都被使用了，未来就不需要重新审视这条道路。
//

class RippleCalc; //用于测井

void PathState::clear()
{
    saInPass = saInReq.zeroed();
    saOutPass = saOutReq.zeroed();
    unfundedOffers_.clear ();
    umReverse.clear ();

    for (auto& node: nodes_)
        node.clear();
}

void PathState::reset(STAmount const& in, STAmount const& out)
{
    clear();

//更新到当前处理的金额。
    saInAct = in;
    saOutAct = out;

    if (inReq() > beast::zero && inAct() >= inReq())
    {
        JLOG (j_.warn())
            <<  "rippleCalc: DONE:"
            << " inAct()=" << inAct()
            << " inReq()=" << inReq();
    }

    assert (inReq() < beast::zero || inAct() < inReq());
//如果出错的话。

    if (outAct() >= outReq())
    {
        JLOG (j_.warn())
            << "rippleCalc: ALREADY DONE:"
            << " saOutAct=" << outAct()
            << " saOutReq=" << outReq();
    }

    assert(outAct() < outReq());
    assert (nodes().size () >= 2);
}

//返回“真”，如果左后优先权低于右后优先权。
bool PathState::lessPriority (PathState const& lhs, PathState const& rhs)
{
//质量第一。
    if (lhs.uQuality != rhs.uQuality)
return lhs.uQuality > rhs.uQuality;     //越大越好。

//二等是最好的数量。
    if (lhs.saOutPass != rhs.saOutPass)
return lhs.saOutPass < rhs.saOutPass;   //越小越好。

//第三级是路径索引。
return lhs.mIndex > rhs.mIndex;             //越大越好。
}

//确保最后一个路径节点从.issue.account传递到帐户\：currency。
//
//如果参数指定的未添加的下一个节点不能按原样工作，则
//添加必要的节点以使其工作。
//前提条件：路径状态必须为非空。
//
//规则：
//-货币必须通过报价进行转换。
//-节点命名其输出。

//-波纹节点输出颁发者必须是节点的帐户或下一个节点的帐户
//帐户。
//-只有当货币和发行人
//精确的匹配
//-必须为非XRP指定真实发行人。
TER PathState::pushImpliedNodes (
AccountID const& account,    //-->传送到此帐户。
Currency const& currency,  //-->交付这种货币。
AccountID const& issuer)    //-->传递此颁发者。
{
    TER resultCode = tesSUCCESS;

     JLOG (j_.trace()) << "pushImpliedNodes>" <<
         " " << account <<
         " " << currency <<
         " " << issuer;

    if (nodes_.back ().issue_.currency != currency)
    {
//货币不同，需要通过订单报价进行转换
//书。xrpacCount（）在发出信号“这是一个命令”时执行双重任务
//书“。

//对应于图中的“暗示提供目录”，当前
//网址：http://goo.gl/uj3hab。

        auto type = isXRP(currency) ? STPathElement::typeCurrency
            : STPathElement::typeCurrency | STPathElement::typeIssuer;

//这个提议的结果就是现在想要的。
//xrpacCount（）是报价的占位符。
        resultCode = pushNode (type, xrpAccount(), currency, issuer);
    }


//对于Ripple，非XRP，确保发行人至少位于
//交易。
    if (resultCode == tesSUCCESS
        && !isXRP(currency)
        && nodes_.back ().account_ != issuer
//上一个不是发行自己的借据。
        && account != issuer)
//电流没有接收到自己的IOU。
    {
//需要通过发行人的账户进行波动。
//case“表示文档中的另一个节点：（pushImpliedNodes）”。
//中间账户是必要的发行人。
        resultCode = pushNode (
            STPathElement::typeAll, issuer, currency, issuer);
    }

    JLOG (j_.trace())
        << "pushImpliedNodes< : " << transToken (resultCode);

    return resultCode;
}

//附加一个节点，然后在它前面创建并插入任何隐含的节点。秩序
//书籍节点可以背靠背。
//
//对于每一对不匹配的已发行货币，都有一本订购书。
//
//<--resultcode:tessuccess，tembad_path，terno_account，terno_auth，
//Terno_线，Tecpath_干燥
TER PathState::pushNode (
    const int iType,
AccountID const& account,    //如果未指定，则表示订单簿。
Currency const& currency,  //如果未指定，则默认为“上一个”。
AccountID const& issuer)     //如果未指定，则默认为“上一个”。
{
    path::Node node;
    const bool pathIsEmpty = nodes_.empty ();

//托多（汤姆）：如果Pathisempty，我们可能不需要做下面的任何事情。
//实际上，我们一开始可能甚至不会调用pushnode！

    auto const& backNode = pathIsEmpty ? path::Node () : nodes_.back ();

//是的，iff节点是一个Ripple帐户。错误，iff节点是一个提供节点。
    const bool hasAccount = (iType & STPathElement::typeAccount);

//是否为当前节点的输出指定了货币？
    const bool hasCurrency = (iType & STPathElement::typeCurrency);

//为当前节点的输出指定了颁发者。
    const bool hasIssuer = (iType & STPathElement::typeIssuer);

    TER resultCode = tesSUCCESS;

    JLOG (j_.trace())
         << "pushNode> " << iType << ": "
         << (hasAccount ? to_string(account) : std::string("-")) << " "
         << (hasCurrency ? to_string(currency) : std::string("-")) << "/"
         << (hasIssuer ? to_string(issuer) : std::string("-")) << "/";

    node.uFlags = iType;
    node.issue_.currency = hasCurrency ?
            currency : backNode.issue_.currency;

//托多（汤姆）：我们可能只要一碰到
//下一页出错。

    if (iType & ~STPathElement::typeAll)
    {
//当然，这是不可能发生的。
        JLOG (j_.debug()) << "pushNode: bad bits.";
        resultCode = temBAD_PATH;
    }
    else if (hasIssuer && isXRP (node.issue_))
    {
        JLOG (j_.debug()) << "pushNode: issuer specified for XRP.";

        resultCode = temBAD_PATH;
    }
    else if (hasIssuer && !issuer)
    {
        JLOG (j_.debug()) << "pushNode: specified bad issuer.";

        resultCode = temBAD_PATH;
    }
    else if (!hasAccount && !hasCurrency && !hasIssuer)
    {
//不能将所有内容都默认为上一个节点
//没有进步。
        JLOG (j_.debug())
            << "pushNode: offer must specify at least currency or issuer.";
        resultCode = temBAD_PATH;
    }
    else if (hasAccount)
    {
//帐户链接
        node.account_ = account;
        node.issue_.account = hasIssuer ? issuer :
                (isXRP (node.issue_) ? xrpAccount() : account);
//零值-用于帐户。
        node.saRevRedeem = STAmount ({node.issue_.currency, account});
        node.saRevIssue = node.saRevRedeem;

//仅限订单簿-发行人ID为零的货币。
        node.saRevDeliver = STAmount (node.issue_);
        node.saFwdDeliver = node.saRevDeliver;

        if (pathIsEmpty)
        {
//第一个节点总是正确的。
        }
        else if (!account)
        {
            JLOG (j_.debug())
                << "pushNode: specified bad account.";
            resultCode = temBAD_PATH;
        }
        else
        {
//添加需要传递到当前帐户的中间节点。
            JLOG (j_.trace())
                << "pushNode: imply for account.";

            resultCode = pushImpliedNodes (
                node.account_,
                node.issue_.currency,
                isXRP(node.issue_.currency) ? xrpAccount() : account);

//注意：backnode可能不再是前一个节点。
        }

        if (resultCode == tesSUCCESS && !nodes_.empty ())
        {
            auto const& backNode = nodes_.back ();
            if (backNode.isAccount())
            {
                auto sleRippleState = view().peek(
                    keylet::line(backNode.account_, node.account_, backNode.issue_.currency));

//“涟漪地产”是指两个账户之间的余额
//具体货币。
                if (!sleRippleState)
                {
                    JLOG (j_.trace())
                            << "pushNode: No credit line between "
                            << backNode.account_ << " and " << node.account_
                            << " for " << node.issue_.currency << "." ;

                    JLOG (j_.trace()) << getJson ();

                    resultCode   = terNO_LINE;
                }
                else
                {
                    JLOG (j_.trace())
                            << "pushNode: Credit line found between "
                            << backNode.account_ << " and " << node.account_
                            << " for " << node.issue_.currency << "." ;

                    auto sleBck  = view().peek (
                        keylet::account(backNode.account_));
//源帐户是编号最高的帐户ID吗？
                    bool bHigh = backNode.account_ > node.account_;

                    if (!sleBck)
                    {
                        JLOG (j_.warn())
                            << "pushNode: delay: can't receive IOUs from "
                            << "non-existent issuer: " << backNode.account_;

                        resultCode   = terNO_ACCOUNT;
                    }
                    else if ((sleBck->getFieldU32 (sfFlags) & lsfRequireAuth) &&
                             !(sleRippleState->getFieldU32 (sfFlags) &
                                  (bHigh ? lsfHighAuth : lsfLowAuth)) &&
                             sleRippleState->getFieldAmount(sfBalance) == beast::zero)
                    {
                        JLOG (j_.warn())
                                << "pushNode: delay: can't receive IOUs from "
                                << "issuer without auth.";

                        resultCode   = terNO_AUTH;
                    }

                    if (resultCode == tesSUCCESS)
                    {
                        STAmount saOwed = creditBalance (view(),
                            node.account_, backNode.account_,
                            node.issue_.currency);
                        STAmount saLimit;

                        if (saOwed <= beast::zero)
                        {
                            saLimit = creditLimit (view(),
                                node.account_,
                                backNode.account_,
                                node.issue_.currency);
                            if (-saOwed >= saLimit)
                            {
                                JLOG (j_.debug()) <<
                                    "pushNode: dry:" <<
                                    " saOwed=" << saOwed <<
                                    " saLimit=" << saLimit;

                                resultCode   = tecPATH_DRY;
                            }
                        }
                    }
                }
            }
        }

        if (resultCode == tesSUCCESS)
            nodes_.push_back (node);
    }
    else
    {
//提供链接。
//
//提供桥接货币和发行人的变化，或只是
//发行人。
        if (hasIssuer)
            node.issue_.account = issuer;
        else if (isXRP (node.issue_.currency))
            node.issue_.account = xrpAccount();
        else if (isXRP (backNode.issue_.account))
            node.issue_.account = backNode.account_;
        else
            node.issue_.account = backNode.issue_.account;

        node.saRevDeliver = STAmount (node.issue_);
        node.saFwdDeliver = node.saRevDeliver;

        if (!isConsistent (node.issue_))
        {
            JLOG (j_.debug())
                << "pushNode: currency is inconsistent with issuer.";

            resultCode = temBAD_PATH;
        }
        else if (backNode.issue_ == node.issue_)
        {
            JLOG (j_.debug()) <<
                "pushNode: bad path: offer to same currency and issuer";
            resultCode = temBAD_PATH;
        }
        else {
            JLOG (j_.trace()) << "pushNode: imply for offer.";

//如果需要，插入中间发行人帐户。
            resultCode   = pushImpliedNodes (
xrpAccount(), //泛起涟漪，但报价没有账户。
                backNode.issue_.currency,
                backNode.issue_.account);
        }

        if (resultCode == tesSUCCESS)
            nodes_.push_back (node);
    }

    JLOG (j_.trace()) << "pushNode< : " << transToken (resultCode);
    return resultCode;
}

//将此对象设置为SPSourcePath中的扩展路径-采用
//节点并使它们显式。它还清理路径。
//
//只有两种类型的节点：帐户节点和订单簿节点。
//
//可以自动推断某些节点。如果你给我一张比特图章美元，
//然后必须有一个中间位戳节点。
//
//如果你有账户A和B，它们是C发行的交货货币，那么
//中间必须有帐户C的节点。
//
//如果你支付美元和比特币，必须有一个订单在
//之间。
//
//terstatus=tessuccess，tembad_path，terno_line，terno_account，terno_auth，
//或Tembad_Path_Loop
TER PathState::expandPath (
    STPath const& spSourcePath,
    AccountID const& uReceiverID,
    AccountID const& uSenderID)
{
uQuality = 1;            //将路径标记为活动。

    Currency const& uMaxCurrencyID = saInReq.getCurrency ();
    AccountID const& uMaxIssuerID = saInReq.getIssuer ();

    Currency const& currencyOutID = saOutReq.getCurrency ();
    AccountID const& issuerOutID = saOutReq.getIssuer ();
    AccountID const& uSenderIssuerID
        = isXRP(uMaxCurrencyID) ? xrpAccount() : uSenderID;
//对于非XRP，发送方始终是发卡方。

    JLOG (j_.trace())
        << "expandPath> " << spSourcePath.getJson (0);

    terStatus = tesSUCCESS;

//具有颁发者的xrp格式不正确。
    if ((isXRP (uMaxCurrencyID) && !isXRP (uMaxIssuerID))
        || (isXRP (currencyOutID) && !isXRP (issuerOutID)))
    {
        JLOG (j_.debug())
            << "expandPath> issuer with XRP";
        terStatus   = temBAD_PATH;
    }

//按下发送节点。
//对于非XRP，发卡行总是发送账户。
//-尝试扩展，而不是压缩。
//-将遍历每个发行者。
    if (terStatus == tesSUCCESS)
    {
        terStatus   = pushNode (
            !isXRP(uMaxCurrencyID)
            ? STPathElement::typeAccount | STPathElement::typeCurrency |
              STPathElement::typeIssuer
            : STPathElement::typeAccount | STPathElement::typeCurrency,
            uSenderID,
uMaxCurrencyID, //max指定货币。
            uSenderIssuerID);
    }

    JLOG (j_.debug())
        << "expandPath: pushed:"
        << " account=" << uSenderID
        << " currency=" << uMaxCurrencyID
        << " issuer=" << uSenderIssuerID;

//颁发者与发件人不同。
    if (tesSUCCESS == terStatus && uMaxIssuerID != uSenderIssuerID)
    {
//可能具有隐含的帐户节点。
//-如果是XRP，那么发行人会匹配。

//找出隐含节点的下一个节点属性。
        const auto uNxtCurrencyID  = spSourcePath.size ()
                ? Currency(spSourcePath.front ().getCurrency ())
//使用下一个节点。
                : currencyOutID;
//使用发送。

//托多（汤姆）：进一步复杂化下一个逻辑，以防有人
//理解它。
        const auto nextAccountID   = spSourcePath.size ()
                ? AccountID(spSourcePath. front ().getAccountID ())
                : !isXRP(currencyOutID)
                ? (issuerOutID == uReceiverID)
                ? AccountID(uReceiverID)
: AccountID(issuerOutID)                      //使用隐含节点。
                : xrpAccount();

        JLOG (j_.debug())
            << "expandPath: implied check:"
            << " uMaxIssuerID=" << uMaxIssuerID
            << " uSenderIssuerID=" << uSenderIssuerID
            << " uNxtCurrencyID=" << uNxtCurrencyID
            << " nextAccountID=" << nextAccountID;

//不能只使用push隐含，因为它不能补偿next
//帐户。
        if (!uNxtCurrencyID
//接下来是xrp，接下来提供。必须通过发卡机构。
            || uMaxCurrencyID != uNxtCurrencyID
//接下来是不同的货币，接下来提供…
            || uMaxIssuerID != nextAccountID)
//下一个不是暗示的颁发者
        {
            JLOG (j_.debug())
                << "expandPath: sender implied:"
                << " account=" << uMaxIssuerID
                << " currency=" << uMaxCurrencyID
                << " issuer=" << uMaxIssuerID;

//添加sendmax隐含的帐户。
            terStatus = pushNode (
                !isXRP(uMaxCurrencyID)
                    ? STPathElement::typeAccount | STPathElement::typeCurrency |
                      STPathElement::typeIssuer
                    : STPathElement::typeAccount | STPathElement::typeCurrency,
                uMaxIssuerID,
                uMaxCurrencyID,
                uMaxIssuerID);
        }
    }

    for (auto & speElement: spSourcePath)
    {
        if (terStatus == tesSUCCESS)
        {
            JLOG (j_.trace()) << "expandPath: element in path";
            terStatus = pushNode (
                speElement.getNodeType (), speElement.getAccountID (),
                speElement.getCurrency (), speElement.getIssuerID ());
        }
    }

    if (terStatus == tesSUCCESS
&& !isXRP(currencyOutID)               //下一个不是XRP
&& issuerOutID != uReceiverID)         //发出者不是接收者
    {
        assert (!nodes_.empty ());

        auto const& backNode = nodes_.back ();

if (backNode.issue_.currency != currencyOutID  //上一个将提供
|| backNode.account_ != issuerOutID)       //需要隐含发行人
        {
//添加隐含帐户。
            JLOG (j_.debug())
                << "expandPath: receiver implied:"
                << " account=" << issuerOutID
                << " currency=" << currencyOutID
                << " issuer=" << issuerOutID;

            terStatus   = pushNode (
                !isXRP(currencyOutID)
                    ? STPathElement::typeAccount | STPathElement::typeCurrency |
                      STPathElement::typeIssuer
                    : STPathElement::typeAccount | STPathElement::typeCurrency,
                issuerOutID,
                currencyOutID,
                issuerOutID);
        }
    }

    if (terStatus == tesSUCCESS)
    {
//创建接收器节点。
//最后一个节点始终是一个帐户。

        terStatus   = pushNode (
            !isXRP(currencyOutID)
                ? STPathElement::typeAccount | STPathElement::typeCurrency |
                   STPathElement::typeIssuer
               : STPathElement::typeAccount | STPathElement::typeCurrency,
uReceiverID,                                    //接收到输出
currencyOutID,                                 //所需货币
            uReceiverID);
    }

    if (terStatus == tesSUCCESS)
    {
//在节点和检测循环中查找第一个提到源的地方。
//注意：不允许输出为源。
        unsigned int index = 0;
        for (auto& node: nodes_)
        {
            AccountIssue accountIssue (node.account_, node.issue_);
            if (!umForward.insert ({accountIssue, index++}).second)
            {
//插入失败。有一个循环。
                JLOG (j_.debug()) <<
                    "expandPath: loop detected: " << getJson ();

                terStatus = temBAD_PATH_LOOP;
                break;
            }
        }
    }

    JLOG (j_.trace())
        << "expandPath:"
        << " in=" << uMaxCurrencyID
        << "/" << uMaxIssuerID
        << " out=" << currencyOutID
        << "/" << issuerOutID
        << ": " << getJson ();
    return terStatus;
}


/*检查扩展路径是否违反冻结规则*/
void PathState::checkFreeze()
{
    assert (nodes_.size() >= 2);

//没有中间人的道路——纯粹的发行/赎回
//不能冻结。
    if (nodes_.size() == 2)
        return;

    SLE::pointer sle;

    for (std::size_t i = 0; i < (nodes_.size() - 1); ++i)
    {
//检查每个订单簿是否存在全局冻结
        if (nodes_[i].uFlags & STPathElement::typeIssuer)
        {
            sle = view().peek (keylet::account(nodes_[i].issue_.account));

            if (sle && sle->isFlag (lsfGlobalFreeze))
            {
                terStatus = terNO_LINE;
                return;
            }
        }

//检查每个帐户的变化，确保资金可以离开
        if (nodes_[i].uFlags & STPathElement::typeAccount)
        {
            Currency const& currencyID = nodes_[i].issue_.currency;
            AccountID const& inAccount = nodes_[i].account_;
            AccountID const& outAccount = nodes_[i+1].account_;

            if (inAccount != outAccount)
            {
                sle = view().peek (keylet::account(outAccount));

                if (sle && sle->isFlag (lsfGlobalFreeze))
                {
                    terStatus = terNO_LINE;
                    return;
                }

                sle = view().peek (keylet::line(inAccount,
                        outAccount, currencyID));

                if (sle && sle->isFlag (
                    (outAccount > inAccount) ? lsfHighFreeze : lsfLowFreeze))
                {
                    terStatus = terNO_LINE;
                    return;
                }
            }
        }
    }
}

/*检查三个帐户的序列是否违反无波纹约束
    [第一]->[第二]->[第三]
    如果'second'在[first]->[second]上没有波纹，则不允许
    [第二]->[第三]
**/

TER PathState::checkNoRipple (
    AccountID const& firstAccount,
    AccountID const& secondAccount,
//这是我们要检查其限制条件的帐户
    AccountID const& thirdAccount,
    Currency const& currency)
{
//将波纹线从此节点中取出和取出
    SLE::pointer sleIn = view().peek (
        keylet::line(firstAccount, secondAccount, currency));
    SLE::pointer sleOut = view().peek (
        keylet::line(secondAccount, thirdAccount, currency));

    if (!sleIn || !sleOut)
    {
        terStatus = terNO_LINE;
    }
    else if (
        sleIn->getFieldU32 (sfFlags) &
            ((secondAccount > firstAccount) ? lsfHighNoRipple : lsfLowNoRipple) &&
        sleOut->getFieldU32 (sfFlags) &
            ((secondAccount > thirdAccount) ? lsfHighNoRipple : lsfLowNoRipple))
    {
        JLOG (j_.info())
            << "Path violates noRipple constraint between "
            << firstAccount << ", "
            << secondAccount << " and "
            << thirdAccount;

        terStatus = terNO_RIPPLE;
    }
    return terStatus;
}

//检查完全扩展的路径以确保它不会违反无波纹
//设置。
TER PathState::checkNoRipple (
    AccountID const& uDstAccountID,
    AccountID const& uSrcAccountID)
{
//必须至少有一个节点才能有两个连续的波纹
//线。
    if (nodes_.size() == 0)
       return terStatus;

    if (nodes_.size() == 1)
    {
//路径中只有一个链接
//我们只需要检查源节点dest
        if (nodes_[0].isAccount() &&
            (nodes_[0].account_ != uSrcAccountID) &&
            (nodes_[0].account_ != uDstAccountID))
        {
            if (saInReq.getCurrency() != saOutReq.getCurrency())
            {
                terStatus = terNO_LINE;
            }
            else
            {
                terStatus = checkNoRipple (
                    uSrcAccountID, nodes_[0].account_, uDstAccountID,
                    nodes_[0].issue_.currency);
            }
        }
        return terStatus;
    }

//检查源<->First<->Second
    if (nodes_[0].isAccount() &&
        nodes_[1].isAccount() &&
        (nodes_[0].account_ != uSrcAccountID))
    {
        if ((nodes_[0].issue_.currency != nodes_[1].issue_.currency))
        {
            terStatus = terNO_LINE;
            return terStatus;
        }
        else
        {
            terStatus = checkNoRipple (
                uSrcAccountID, nodes_[0].account_, nodes_[1].account_,
                nodes_[0].issue_.currency);
            if (terStatus != tesSUCCESS)
                return terStatus;
        }
    }

//检查最后<->最后<->目的地的第二个_
    size_t s = nodes_.size() - 2;
    if (nodes_[s].isAccount() &&
        nodes_[s + 1].isAccount() &&
        (uDstAccountID != nodes_[s+1].account_))
    {
        if ((nodes_[s].issue_.currency != nodes_[s+1].issue_.currency))
        {
            terStatus = terNO_LINE;
            return terStatus;
        }
        else
        {
            terStatus = checkNoRipple (
                nodes_[s].account_, nodes_[s+1].account_,
                uDstAccountID, nodes_[s].issue_.currency);
            if (tesSUCCESS != terStatus)
                return terStatus;
        }
    }

//循环访问具有先前节点和后续节点的所有节点
//这些节点没有违反任何涟漪约束
    for (int i = 1; i < nodes_.size() - 1; ++i)
    {
        if (nodes_[i - 1].isAccount() &&
            nodes_[i].isAccount() &&
            nodes_[i + 1].isAccount())
{ //两个连续的帐户到帐户链接

            auto const& currencyID = nodes_[i].issue_.currency;
            if ((nodes_[i-1].issue_.currency != currencyID) ||
                (nodes_[i+1].issue_.currency != currencyID))
            {
                terStatus = temBAD_PATH;
                return terStatus;
            }
            terStatus = checkNoRipple (
                nodes_[i-1].account_, nodes_[i].account_, nodes_[i+1].account_,
                currencyID);
            if (terStatus != tesSUCCESS)
                return terStatus;
        }

        if (!nodes_[i - 1].isAccount() &&
            nodes_[i].isAccount() &&
            nodes_[i + 1].isAccount() &&
            nodes_[i -1].issue_.account != nodes_[i].account_)
{ //报价->账户->账户
            auto const& currencyID = nodes_[i].issue_.currency;
            terStatus = checkNoRipple (
                nodes_[i-1].issue_.account, nodes_[i].account_, nodes_[i+1].account_,
                currencyID);
            if (terStatus != tesSUCCESS)
                return terStatus;
        }
    }

    return tesSUCCESS;
}

//这是为了调试而非最终用户。输出名称可以在没有
//警告。
Json::Value PathState::getJson () const
{
    Json::Value jvPathState (Json::objectValue);
    Json::Value jvNodes (Json::arrayValue);

    for (auto const &pnNode: nodes_)
        jvNodes.append (pnNode.getJson ());

    jvPathState[jss::status]   = terStatus;
    jvPathState[jss::index]    = mIndex;
    jvPathState[jss::nodes]    = jvNodes;

    if (saInReq)
        jvPathState["in_req"]   = saInReq.getJson (0);

    if (saInAct)
        jvPathState["in_act"]   = saInAct.getJson (0);

    if (saInPass)
        jvPathState["in_pass"]  = saInPass.getJson (0);

    if (saOutReq)
        jvPathState["out_req"]  = saOutReq.getJson (0);

    if (saOutAct)
        jvPathState["out_act"]  = saOutAct.getJson (0);

    if (saOutPass)
        jvPathState["out_pass"] = saOutPass.getJson (0);

    if (uQuality)
        jvPathState["uQuality"] = boost::lexical_cast<std::string>(uQuality);

    return jvPathState;
}

} //涟漪
