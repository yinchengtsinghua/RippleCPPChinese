
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

#ifndef RIPPLE_APP_PATHS_PATHSTATE_H_INCLUDED
#define RIPPLE_APP_PATHS_PATHSTATE_H_INCLUDED

#include <ripple/app/paths/Node.h>
#include <ripple/app/paths/Types.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <boost/optional.hpp>

namespace ripple {

//在增量应用程序下保持单路径状态。
class PathState : public CountedObject <PathState>
{
  public:
    using OfferIndexList = std::vector<uint256>;
    using Ptr = std::shared_ptr<PathState>;
    using List = std::vector<Ptr>;

    PathState (PaymentSandbox const& parent,
            STAmount const& saSend,
               STAmount const& saSendMax,
                   beast::Journal j)
        : mIndex (0)
        , uQuality (0)
        , saInReq (saSendMax)
        , saOutReq (saSend)
        , j_ (j)
    {
        view_.emplace(&parent);
    }

    void reset(STAmount const& in, STAmount const& out);

    TER expandPath (
        STPath const&    spSourcePath,
        AccountID const& uReceiverID,
        AccountID const& uSenderID
    );

    path::Node::List& nodes() { return nodes_; }

    STAmount const& inPass() const { return saInPass; }
    STAmount const& outPass() const { return saOutPass; }
    STAmount const& outReq() const { return saOutReq; }

    STAmount const& inAct() const { return saInAct; }
    STAmount const& outAct() const { return saOutAct; }
    STAmount const& inReq() const { return saInReq; }

    void setInPass(STAmount const& sa)
    {
        saInPass = sa;
    }

    void setOutPass(STAmount const& sa)
    {
        saOutPass = sa;
    }

    AccountIssueToNodeIndex const& forward() { return umForward; }
    AccountIssueToNodeIndex const& reverse() { return umReverse; }

    void insertReverse (AccountIssue const& ai, path::NodeIndex i)
    {
        umReverse.insert({ai, i});
    }

    static char const* getCountedObjectName () { return "PathState"; }
    OfferIndexList& unfundedOffers() { return unfundedOffers_; }

    void setStatus(TER status) { terStatus = status; }
    TER status() const { return terStatus; }

    std::uint64_t quality() const { return uQuality; }
    void setQuality (std::uint64_t q) { uQuality = q; }

    void setIndex (int i) { mIndex  = i; }
    int index() const { return mIndex; }

    TER checkNoRipple (AccountID const& destinationAccountID,
                       AccountID const& sourceAccountID);
    void checkFreeze ();

    static bool lessPriority (PathState const& lhs, PathState const& rhs);

    PaymentSandbox&
    view()
    {
        return *view_;
    }

    void resetView (PaymentSandbox const& view)
    {
        view_.emplace(&view);
    }

    bool isDry() const
    {
        return !(saInPass && saOutPass);
    }

private:
    TER checkNoRipple (
        AccountID const&, AccountID const&, AccountID const&, Currency const&);

    /*清除路径结构，并清除每个节点。*/
    void clear();

    TER pushNode (
        int const iType,
        AccountID const& account,
        Currency const& currency,
        AccountID const& issuer);

    TER pushImpliedNodes (
        AccountID const& account,
        Currency const& currency,
        AccountID const& issuer);

    Json::Value getJson () const;

private:
    boost::optional<PaymentSandbox> view_;

int                         mIndex;    //兄弟姐妹的指数/排名。
std::uint64_t               uQuality;  //0=无质量/流动性。

STAmount const&             saInReq;   //-->发送方要花费的最大金额。
STAmount                    saInAct;   //-->到目前为止，发件人花费的金额。
STAmount                    saInPass;  //<--寄件人花费的金额。

STAmount const&             saOutReq;  //-->要发送的金额。
STAmount                    saOutAct;  //-->到目前为止实际发送的金额。
STAmount                    saOutPass; //<--实际发送的金额。

    TER terStatus;

    path::Node::List nodes_;

//处理时，不希望使目录遍历复杂化
//删除。没有资金或完全被消耗掉的提议
//此处，并在结尾处删除。
    OfferIndexList unfundedOffers_;

//作为道路建设的一部分，第一次进行资金扫描
//提到了账户的来源。源只能在那里使用。
    AccountIssueToNodeIndex umForward;

//第一次反向工作使用了资金来源。
//只有在账户未提及的情况下，才能使用来源。
    AccountIssueToNodeIndex umReverse;

    beast::Journal j_;
};

} //涟漪

#endif
