
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

#ifndef RIPPLE_APP_PATHS_IMPL_PAYSTEPS_H_INCLUDED
#define RIPPLE_APP_PATHS_IMPL_PAYSTEPS_H_INCLUDED

#include <ripple/app/paths/impl/AmountSpec.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/TER.h>

#include <boost/container/flat_set.hpp>
#include <boost/optional.hpp>

namespace ripple {
class PaymentSandbox;
class ReadView;
class ApplyView;

/*
   付款路径中的步骤

   有五个具体步骤：
     DirectStepi是帐户之间的IOU步骤
     BookStepII是借据/借据报价书
     BookStepix是一本IOU/XRP优惠书
     bookstepxi是一本xrp/iou优惠书
     xrpendpointstep是xrp的源或目标帐户

   金额可以通过前进或
   反向。在前进方向，函数“fwd”用于
   找到给定输入量的步骤将输出的量。反过来
   方向，函数“rev”用于查找
   产生所需的输出。

   使用相同质量（质量）的流动性来转换金额
   是出/入的金额）。例如，预订步骤可以使用多个优惠
   当执行'fwd'或'rev'时，所有这些报价将来自相同的
   质量目录。

   一个步骤可能没有足够的流动性来转换整个请求的
   数量。“fwd”和“rev”都返回一对金额（一个用于输入金额，
   一个用于输出量），显示步骤中请求量的多少
   实际上可以使用。
 **/

class Step
{
public:
    /*步进析构函数。*/
    virtual ~Step () = default;

    /*
       找到我们需要放入步骤中的金额，以获得请求的输出
       受流动性限制

       @param sb查看链的平衡和报价状态
       @param afview查看流运行前的平衡状态
              这决定了一个报价是没有资金支持还是被发现没有资金支持。
       @param ofrstorm offers found unfunded or in an error state are added to this collection（未找到资金或处于错误状态的@param ofrstorm提供已添加到此集合中）
       @param out请求的步进输出
       @返回实际步进输入和输出
    **/

    virtual
    std::pair<EitherAmount, EitherAmount>
    rev (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        EitherAmount const& out) = 0;

    /*
       在给定输入的情况下，找出我们从步骤中得到的量。
       受流动性限制

       @param sb查看链的平衡和报价状态
       @param afview查看流运行前的平衡状态
              这决定了一个报价是没有资金支持还是被发现没有资金支持。
       @param ofrstorm offers found unfunded or in an error state are added to this collection（未找到资金或处于错误状态的@param ofrstorm提供已添加到此集合中）
       请求的步骤输入中的@param
       @返回实际步进输入和输出
    **/

    virtual
    std::pair<EitherAmount, EitherAmount>
    fwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        EitherAmount const& in) = 0;

    /*
       最后一次进入步骤计算的货币量
       步骤倒着跑。
    **/

    virtual
    boost::optional<EitherAmount>
    cachedIn () const = 0;

    /*
       上次从步骤中计算出的货币量
       步骤倒着跑。
    **/

    virtual
    boost::optional<EitherAmount>
    cachedOut () const = 0;

    /*
       如果此步骤为directstepi（iou->iou direct step），则返回src
       帐户。这是检查是否可用所需的。
    **/

    virtual boost::optional<AccountID>
    directStepSrcAcct () const
    {
        return boost::none;
    }

//用于调试。返回直接步骤的SRC和DST帐户
//对于xrp端点，SRC或DST中的一个将是根帐户
    virtual boost::optional<std::pair<AccountID,AccountID>>
    directStepAccts () const
    {
        return boost::none;
    }

    /*
       如果此步骤是DirectStepi，并且SRC重新存入DST，则返回true，
       否则返回false。
       如果此步骤是预订步骤，如果所有者支付了转让费，则返回false。
       否则返回true。

       @param sb查看链的平衡和报价状态
       @param fwd false->called from rev（）；true->called from fwd（）。
    **/

    virtual bool
    redeems (ReadView const& sb, bool fwd) const
    {
        return false;
    }

    /*
        如果此步骤是DirectStepi，则返回DST帐户的质量输入。
     **/

    virtual std::uint32_t
    lineQualityIn (ReadView const&) const
    {
        return QUALITY_ONE;
    }

    /*
       找到步骤的质量上限

       @param v view从中查询分类帐状态
       @param赎回输入/输出参数。如果上一步赎回，则设置为真。
       如果此步骤赎回，将设置为真；如果此步骤赎回，将设置为假
       步骤不能兑现。
       @返回步骤的质量上限，如果
       步骤是干燥的。
     **/

    virtual boost::optional<Quality>
    qualityUpperBound(ReadView const& v, bool& redeems) const = 0;

    /*
       如果此步骤是预订步骤，请返回书籍。
    **/

    virtual boost::optional<Book>
    bookStepBook () const
    {
        return boost::none;
    }

    /*
       检查金额是否为零
    **/

    virtual
    bool
    isZero (EitherAmount const& out) const = 0;

    /*
       如果该步骤应被视为不活动，则返回true。
       如果一个步骤
       已经消耗了太多的报价。
     **/

    virtual
    bool inactive() const{
        return false;
    }

    /*
       如果超出lhs==超出rhs，则返回true。
    **/

    virtual
    bool
    equalOut (
        EitherAmount const& lhs,
        EitherAmount const& rhs) const = 0;

    /*
       如果lhs的in==rhs的in，则返回true。
    **/

    virtual bool equalIn (
        EitherAmount const& lhs,
        EitherAmount const& rhs) const = 0;

    /*
       检查步骤是否可以在前进方向正确执行。

       @param sb查看平衡和报价的链状态
       @param afview查看流运行前的平衡状态
       这决定了一个报价是没有资金支持还是被发现没有资金支持。
       请求的步骤输入中的@param
       @return first元素为真，如果步骤有效，则第二个元素为out amount
    **/

    virtual
    std::pair<bool, EitherAmount>
    validFwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        EitherAmount const& in) = 0;

    /*如果lhs==rhs，则返回true。

        @param lhs要比较的步骤。
        @param rhs要比较的步骤。
        @如果lhs==rhs，则返回true。
    **/

    friend bool operator==(Step const& lhs, Step const& rhs)
    {
        return lhs.equal (rhs);
    }

    /*如果是左，返回真！= RHS。

        @param lhs要比较的步骤。
        @param rhs要比较的步骤。
        @如果是，返回真！= RHS。
    **/

    friend bool operator!=(Step const& lhs, Step const& rhs)
    {
        return ! (lhs == rhs);
    }

    /*一个步骤的流操作程序。*/
    friend
    std::ostream&
    operator << (
        std::ostream& stream,
        Step const& step)
    {
        stream << step.logString ();
        return stream;
    }

private:
    virtual
    std::string
    logString () const = 0;

    virtual bool equal (Step const& rhs) const = 0;
};

///@cond内部
using Strand = std::vector<std::unique_ptr<Step>>;
///EndCOND

///@cond内部
inline
bool operator==(Strand const& lhs, Strand const& rhs)
{
    if (lhs.size () != rhs.size ())
        return false;
    for (size_t i = 0, e = lhs.size (); i != e; ++i)
        if (*lhs[i] != *rhs[i])
            return false;
    return true;
}
///EndCOND

/*
   通过插入隐含的帐户和产品来规范化路径

   发送资产的@param src帐户
   正在接收资产的@param dst帐户
   @param交付DST帐户将接收的资产
   （如果交付发行人=DST，则接受任何发行人）
   @param sendmax要发送的可选资产。
   @param path用于此付款流的流动性源。小径
               包含要使用和
               要翻阅的帐户。
   @返回错误代码和规范化路径
**/

std::pair<TER, STPath>
normalizePath(AccountID const& src,
    AccountID const& dst,
    Issue const& deliver,
    boost::optional<Issue> const& sendMaxIssue,
    STPath const& path);

/*
   为指定路径创建链

   @param sb信任行、余额和诸如auth和freeze等属性的视图
   发送资产的@param src帐户
   正在接收资产的@param dst帐户
   @param交付DST帐户将接收的资产
                  （如果交付发行人=DST，则接受任何发行人）
   @param limitquality offer crossing booksteps在
                       优化。如果在直接报价交叉时
                       书尖的质量低于这个值，
                       然后评估钢绞线可以停止。
   @param sendmaxissue要发送的可选资产。
   @param path用于此付款流的流动性源。小径
               包含要使用和
               要翻阅的帐户。
   @param ownerpaystrasferfee false->charge sender；true->charge offer owner
   @param offerrocking false->payment；true->offerrocking
   用于记录消息的@param j日志
   @返回错误代码和构造的链
**/

std::pair<TER, Strand>
toStrand (
    ReadView const& sb,
    AccountID const& src,
    AccountID const& dst,
    Issue const& deliver,
    boost::optional<Quality> const& limitQuality,
    boost::optional<Issue> const& sendMaxIssue,
    STPath const& path,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    beast::Journal j);

/*
   为每个指定路径（包括默认路径，如果
   指示）

   @param sb信任行、余额和诸如auth和freeze等属性的视图
   发送资产的@param src帐户
   正在接收资产的@param dst帐户
   @param交付DST帐户将接收的资产
                  （如果交付发行人=DST，则接受任何发行人）
   @param limitquality offer crossing booksteps在
                       优化。如果在直接报价交叉时
                       书尖的质量低于这个值，
                       然后评估钢绞线可以停止。
   @param sendmax要发送的可选资产。
   @param path用于完成付款的路径。路径集中的每个路径
                包含要使用和
                要翻阅的帐户。
   @param adddefaultpath确定是否应包括默认路径
   @param ownerpaystrasferfee false->charge sender；true->charge offer owner
   @param offerrocking false->payment；true->offerrocking
   用于记录消息的@param j日志
   @返回错误代码并收集股线
**/

std::pair<TER, std::vector<Strand>>
toStrands (ReadView const& sb,
    AccountID const& src,
    AccountID const& dst,
    Issue const& deliver,
    boost::optional<Quality> const& limitQuality,
    boost::optional<Issue> const& sendMax,
    STPathSet const& paths,
    bool addDefaultPath,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    beast::Journal j);

///@cond内部
template <class TIn, class TOut, class TDerived>
struct StepImp : public Step
{
    explicit StepImp() = default;

    std::pair<EitherAmount, EitherAmount>
    rev (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        EitherAmount const& out) override
    {
        auto const r =
            static_cast<TDerived*> (this)->revImp (sb, afView, ofrsToRm, get<TOut>(out));
        return {EitherAmount (r.first), EitherAmount (r.second)};
    }

//给定要使用的请求数量，计算生成的数量。
//返回消耗/生产的
    std::pair<EitherAmount, EitherAmount>
    fwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        EitherAmount const& in) override
    {
        auto const r =
            static_cast<TDerived*> (this)->fwdImp (sb, afView, ofrsToRm, get<TIn>(in));
        return {EitherAmount (r.first), EitherAmount (r.second)};
    }

    bool
    isZero (EitherAmount const& out) const override
    {
        return get<TOut>(out) == beast::zero;
    }

    bool
    equalOut (EitherAmount const& lhs, EitherAmount const& rhs) const override
    {
        return get<TOut> (lhs) == get<TOut> (rhs);
    }

    bool
    equalIn (EitherAmount const& lhs, EitherAmount const& rhs) const override
    {
        return get<TIn> (lhs) == get<TIn> (rhs);
    }
};
///EndCOND

///@cond内部
//发生意外错误时引发
class FlowException : public std::runtime_error
{
public:
    TER ter;
    std::string msg;

    FlowException (TER t, std::string const& msg)
        : std::runtime_error (msg)
        , ter (t)
    {
    }

    explicit
    FlowException (TER t)
        : std::runtime_error (transHuman (t))
        , ter (t)
    {
    }
};
///EndCOND

///@cond内部
//检查是否与公差相等
bool checkNear (IOUAmount const& expected, IOUAmount const& actual);
bool checkNear (XRPAmount const& expected, XRPAmount const& actual);
///EndCOND

/*
   构建链步骤和错误检查所需的上下文
 **/

struct StrandContext
{
ReadView const& view;                        ///<当前阅读视图
AccountID const strandSrc;                   ///<链源帐户
AccountID const strandDst;                   ///<strend目标帐户
Issue const strandDeliver;                   ///<发行Strand交付
boost::optional<Quality> const limitQuality; ///<最差验收质量
bool const isFirst;                ///<如果step是strend中的第一个，则为true
bool const isLast = false;         ///<如果步进是最后一步，则为真
bool const ownerPaysTransferFee;   ///<true如果所有者而不是发送者支付费用
bool const offerCrossing;          ///<如果报价交叉，则为真，而不是付款
bool const isDefaultPath;          ///<true，如果strend是默认路径
size_t const strandSize;           ///<钢绞线长度
    /*链的上一步。需要检查无波纹
        约束
     **/

    Step const* const prevStep = nullptr;
    /*流不能多次包含同一帐户节点
        用同样的货币。在直接步骤中，帐户将显示
        最多两次：一次作为SRC，一次作为DST（因此是两个元素数组）。
        StrandSRC和StrandDST将只显示一次。
    **/

    std::array<boost::container::flat_set<Issue>, 2>& seenDirectIssues;
    /*一股股票可能不包括输出同一发行更多股票的报价。
        不止一次
    **/

    boost::container::flat_set<Issue>& seenBookOuts;
beast::Journal j;                    ///<日志记录

    /*StrandContext构造函数。*/
    StrandContext (ReadView const& view_,
        std::vector<std::unique_ptr<Step>> const& strand_,
//股线不能包括内部节点
//复制源或目标。
        AccountID const& strandSrc_,
        AccountID const& strandDst_,
        Issue const& strandDeliver_,
        boost::optional<Quality> const& limitQuality_,
        bool isLast_,
        bool ownerPaysTransferFee_,
        bool offerCrossing_,
        bool isDefaultPath_,
        std::array<boost::container::flat_set<Issue>, 2>&
seenDirectIssues_,                             ///<用于检测货币循环
boost::container::flat_set<Issue>& seenBookOuts_,  ///<用于检测书籍循环
beast::Journal j_);                                ///<日志记录
};

///@cond内部
namespace test {
//测试所需
bool directStepEqual (Step const& step,
    AccountID const& src,
    AccountID const& dst,
    Currency const& currency);

bool xrpEndpointStepEqual (Step const& step, AccountID const& acc);

bool bookStepEqual (Step const& step, ripple::Book const& book);
}

std::pair<TER, std::unique_ptr<Step>>
make_DirectStepI (
    StrandContext const& ctx,
    AccountID const& src,
    AccountID const& dst,
    Currency const& c);

std::pair<TER, std::unique_ptr<Step>>
make_BookStepII (
    StrandContext const& ctx,
    Issue const& in,
    Issue const& out);

std::pair<TER, std::unique_ptr<Step>>
make_BookStepIX (
    StrandContext const& ctx,
    Issue const& in);

std::pair<TER, std::unique_ptr<Step>>
make_BookStepXI (
    StrandContext const& ctx,
    Issue const& out);

std::pair<TER, std::unique_ptr<Step>>
make_XRPEndpointStep (
    StrandContext const& ctx,
    AccountID const& acc);

template<class InAmt, class OutAmt>
bool
isDirectXrpToXrp(Strand const& strand);
///EndCOND

} //涟漪

#endif
