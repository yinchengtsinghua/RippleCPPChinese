
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

#include <ripple/app/paths/Flow.h>
#include <ripple/app/paths/RippleCalc.h>
#include <ripple/app/paths/Tuning.h>
#include <ripple/app/paths/cursor/PathCursor.h>
#include <ripple/app/paths/impl/FlowDebugInfo.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Feature.h>

namespace ripple {
namespace path {

static
TER
deleteOffers (ApplyView& view,
    boost::container::flat_set<uint256> const& offers, beast::Journal j)
{
    for (auto& e: offers)
        if (TER r = offerDelete (view,
                view.peek(keylet::offer(e)), j))
            return r;
    return tesSUCCESS;
}

RippleCalc::Output RippleCalc::rippleCalculate (
    PaymentSandbox& view,

//使用此分类账分录集计算路径。实际上取决于呼叫方
//应用于分类帐。

//发行人：
//xrp:xrpaccount（）。
//非xrp:usrcaccounted（针对任何发行人）或其他账户
//信任节点。
STAmount const& saMaxAmountReq,             //-->-1=无限制。

//发行人：
//xrp:xrpaccount（）。
//非xrp:udstaccounted（针对任何发行人）或其他账户
//信任节点。
    STAmount const& saDstAmountReq,

    AccountID const& uDstAccountID,
    AccountID const& uSrcAccountID,

//一组包含在事务中的路径，我们将
//探索流动性。
    STPathSet const& spsPaths,
    Logs& l,
    Input const* const pInputs)
{
//调用流v1和v2，以便比较结果
    bool const compareFlowV1V2 =
        view.rules ().enabled (featureCompareFlowV1V2);

    bool const useFlowV1Output =
        !view.rules().enabled(featureFlow);
    bool const callFlowV1 = useFlowV1Output || compareFlowV1V2;
    bool const callFlowV2 = !useFlowV1Output || compareFlowV1V2;

    Output flowV1Out;
    PaymentSandbox flowV1SB (&view);

    auto const inNative = saMaxAmountReq.native();
    auto const outNative = saDstAmountReq.native();
    detail::FlowDebugInfo flowV1FlowDebugInfo (inNative, outNative);
    if (callFlowV1)
    {
        auto const timeIt = flowV1FlowDebugInfo.timeBlock ("main");
        RippleCalc rc (
            flowV1SB,
            saMaxAmountReq,
            saDstAmountReq,
            uDstAccountID,
            uSrcAccountID,
            spsPaths,
            l);
        if (pInputs != nullptr)
        {
            rc.inputFlags = *pInputs;
        }

        auto result = rc.rippleCalculate (compareFlowV1V2 ? &flowV1FlowDebugInfo : nullptr);
        flowV1Out.setResult (result);
        flowV1Out.actualAmountIn = rc.actualAmountIn_;
        flowV1Out.actualAmountOut = rc.actualAmountOut_;
        if (result != tesSUCCESS && !rc.permanentlyUnfundedOffers_.empty ())
            flowV1Out.removableOffers = std::move (rc.permanentlyUnfundedOffers_);
    }

    Output flowV2Out;
    PaymentSandbox flowV2SB (&view);
    detail::FlowDebugInfo flowV2FlowDebugInfo (inNative, outNative);
    auto j = l.journal ("Flow");
    if (callFlowV2)
    {
        bool defaultPaths = true;
        bool partialPayment = false;
        boost::optional<Quality> limitQuality;
        boost::optional<STAmount> sendMax;

        if (pInputs)
        {
            defaultPaths = pInputs->defaultPathsAllowed;
            partialPayment = pInputs->partialPaymentAllowed;
            if (pInputs->limitQuality && saMaxAmountReq > beast::zero)
                limitQuality.emplace (
                    Amounts (saMaxAmountReq, saDstAmountReq));
        }

        if (saMaxAmountReq >= beast::zero ||
            saMaxAmountReq.getCurrency () != saDstAmountReq.getCurrency () ||
            saMaxAmountReq.getIssuer () != uSrcAccountID)
        {
            sendMax.emplace (saMaxAmountReq);
        }

        try
        {
            bool const ownerPaysTransferFee =
                    view.rules ().enabled (featureOwnerPaysFee);
            auto const timeIt = flowV2FlowDebugInfo.timeBlock ("main");
            flowV2Out = flow (flowV2SB, saDstAmountReq, uSrcAccountID,
                uDstAccountID, spsPaths, defaultPaths, partialPayment,
                /*erpaystrasferfee，/*offerrishing*/false，limitquality，sendmax，j，
                比较流v1v2？&flowv2flowdebuginfo:nullptr）；
        }
        捕获（std:：exception&e）
        {
            jLog（j.error（））<“exception from flow：”<<e.what（）；
            如果（！）使用流量1输出）
            {
                //返回一个tec，以便存储tx
                路径：：RippleCalc：：输出异常结果；
                exceptresult.setresult（技术内部）；
                返回exceptresult；
            }
        }
    }

    如果（J.Dug（））
    {
        使用balanceddifs=detail:：balanceddifs；
        auto logresult=[&]（std:：string const&algoname，
            输出常量和结果，
            详细信息：flowdebuginfo const&flowdebuginfo，
            boost：：可选<balanceddifs>const&balanceddifs，
            bool输出passinfo，
            bool输出平衡差异）
                j.debug（）<“RippleCalc结果>”<<
                “现状：”<<result.actualamountin<<
                “，实际值：”<<result.actualMountout<<
                “，结果：”<<result.result（）<<
                “，dstamtreq：”<<sadstamountreq<<
                “，sendmax：”<<samaxamountreq<<
                （比较流v1v2？“，“+flowdebuginfo.to_string（outputpassinfo）：”“）<<
                （输出平衡差异和平衡差异
                 “，“+detail:：balanceddiffstostring（balanceddiffs）：”“）<<
                “，algo：”<<algoname；
        }；
        bool outputpassinfo=false；
        bool outputBalancedDiffs=false；
        boost：：可选<balanceddiffs>bdv1，bdv2；
        if（比较流v1v2）
        {
            auto const v1r=flowv1out.result（）；
            auto const v2r=flowv2out.result（）；
            如果（V1R）！＝V2R
                （（v1r==tessuccess）（v1r==tecpath_部分））&&
                    （flow1输出.actualamountin！=
                         流量2输出.实际流量
                        （flow1输出.actualamountout！=
                            流v2out.actualamountout）））
            {
                outputpassinfo=真；
            }
            bdv1=细节：：BalancedDiffs（流程v1sb，视图）；
            bdv2=详细信息：：BalancedDiffs（流程v2sb，视图）；
            outputBalancedDiffs=bdv1！= BDV2；
        }

        如果（CalvFLUV1）
        {
            logresult（“v1”，flowv1out，flowv1flowdebuginfo，bdv1，
                输出传递信息，输出平衡差异）；
        }
        如果（CalvFLUV2）
        {
            logresult（“v2”，flowv2out，flowv2flowdebuginfo，bdv2，
                输出传递信息，输出平衡差异）；
        }
    }

    jLog（j.trace（））<“使用旧流：”<<useflowv1output；

    如果（！）使用流量1输出）
    {
        流程V2SB.Apply（视图）；
        回流阀2出口；
    }
    流程v1sb.apply（视图）；
    回流1出口；
}

bool ripplecalc:：addpathstate（stpath const&path，ter&resultcode）
{
    auto pathstate=std：：使共享
        视图，sadstamountreq_uu，samaxamountreq_u，j_u）；

    如果（！）病态）
    {
        结果代码=TemUnknown；
        返回错误；
    }

    路径状态->扩展路径（
        路径，
        UdStApple
        美国注册会计师；

    如果（pathstate->status（）==tessuccess）
        pathstate->checknoripple（udstaccountd_uu，usrcountd_u）；

    如果（pathstate->status（）==tessuccess）
        pathstate->checkfreeze（）；

    pathstate->setindex（pathstateList_.size（））；

    jLog（j_.debug（））
        <“RippleCalc:build direct:”
        <“状态：”<<transtoken（pathstate->status（））；

    //如果格式不正确，则返回。
    if（istemalformed（pathstate->status（））
    {
        结果code=pathstate->status（）；
        返回错误；
    }

    if（pathstate->status（）==tessuccess）
    {
        结果code=pathstate->status（）；
        pathstateList_u.push_back（pathstate）；
    }
    否则，如果（pathstate->status（）！=特诺诺林
    {
        结果code=pathstate->status（）；
    }

    回归真实；
}

//优化：计算路径增量时，注意增量是否消耗所有
/流动性。如果所有的流动性都被使用了，未来就不需要重新审视这条道路。

//<--ter:仅在允许PartialPayment的情况下返回tepPath_partial。
ter RippleCalc:：RippleCalculate（详细信息：：FlowDebugInfo*FlowDebugInfo）
{
    jLog（j_.trace（））
        <“RippleCalc>”
        <“samaxamountreq”：<<samaxamountreq_
        <“sadstamountreq”<<sadstamountreq；

    ter resultcode=不确定项；
    永久不脱毛的鞋.clear（）；
    mumsource清除（）；

    //YYY可以根据Dopayment对SRC和DST的有效性进行基本检查。

    //增量搜索路径。
    如果（inputflags.defaultPathSallowed）
    {
        如果（！）addPathState（stPath（），resultCode））
            返回结果代码；
    }
    else if（spspaths_u.empty（））
    {
        jLog（j_.debug（））
            <“RippleCalc:无效事务：”
            <“不允许有路径和直接纹波。”；

        返回temriple_empty；
    }

    //生成默认路径。使用sadstamountreq和samaxamountreq来暗示
    / /节点。
    //xxx在默认情况下也可能生成xrp桥。

    jLog（j_.trace（））
        <“RippleCalc:设置中的路径：”<<SPSPATHS_.SIZE（）；

    //现在展开路径状态。
    用于（auto const&sppath:spspaths_uuu）
    {
        如果（！）addPathState（SPPath，结果代码））
            返回结果代码；
    }

    如果（结果代码！= TestCuess）
        返回（resultcode==temUncertain）？terno_行：结果代码；

    结果代码=不确定项；

    actualAmountIn_=samaxAmountReq_.zeroed（）；
    actualMountOut_=sadStamMountReq_.zeroed（）；

    //处理时，我们不想让目录遍历复杂化
    / /删除。
    const std:：uint64？质量限制=inputflags.limitQuality？
            getrate（sadstamountreq_uu，samaxamountreq_u）：0；

    //没有资金支持的报价。
    boost:：container:：flat_set<uint256>unfundedoffersFromBestPaths；

    INPASS＝0；
    auto const dcswitch=fix1141（view.info（）.parentclosetime）；

    while（resultcode==temUncertain）
    {
        iBest=＝1；
        ItIDy＝0；

        //如果计算出多质量，则为真。
        bool multiquality=false；

        if（flowdebuginfo）flowdebuginfo->newliquiditypass（）；
        //找到最佳路径。
        for（自动路径状态：路径状态列表）
        {
            if（pathstate->quality（））
                //只执行活动路径。
            {
                //如果只计算非干燥路径，而不限制质量，
                //计算多质量。
                多质量=dcswitch
                    ？！inputflags.limitQuality和&
                        （（pathstateList_.size（）-idry）==1）
                    ：（（pathstateList_.size（）-idry）==1）；

                //更新到当前处理的金额。
                路径状态->重置（actualamountin、actualamountout）；

                //错误如果完成，则输出满足。
                路径光标PC（*this，*pathstate，multiquality，j_u）；
                pc.nextincrement（）；

                //计算增量。
                jLog（j_.debug（））
                    <“RippleCalc:After:”
                    <“mindex=”<<pathstate->index（））
                    <“uquality=”<<pathstate->quality（））
                    <“rate=”<<amountFromQuality（pathstate->quality（））；

                如果（FlowDebugInfo）
                    流debuginfo->pushliquiditysrc（
                        toeitheramount（路径状态->inpass（）），
                        toeitheramount（pathstate->outpass（））；

                如果（！）路径状态->质量（））
                {
                    //路径干燥。

                    +IDRY；
                }
                else if（pathstate->outpass（）==beast：：零）
                {
                    //路径不干燥，但没有移动资金
                    //这不应该发生。考虑到道路干燥

                    jLog（j_u.warn（））
                        <“RippleCalc:non dry path moves no funds”；

                    断言（假）；

                    路径状态->设置质量（0）；
                    +IDRY；
                }
                其他的
                {
                    如果（！）pathstate->inpass（）！pathstate->outpass（））
                    {
                        jLog（j_.debug（））
                            <“RippleCalc:更好：
                            < UQuest=
                            <<amountFromQuality（pathstate->quality（））
                            <“inpass（）=”<<pathstate->inpass（））
                            <“saoutpass=”<<pathstate->outpass（）；
                    }

                    断言（pathstate->inpass（）&&pathstate->outpass（））；

                    jLog（j_.debug（））
                        <“旧流ITER（ITER，In，Out）：”
                        < IPASS >
                        <<pathstate->inpass（）<<““
                        <<pathstate->outpass（）；

                    如果（）！inputflags.limitquality_
                         pathstate->quality（）<=uQualityLimit）
                        //质量不受限制或允许增量
                        /质量。
                        ＆（iBEST < 0）
                            //尚未设置最佳值。
                            路径状态：：lessPriority（
                                *pathstateList_u[ibest]，*pathstate）））
                        //电流比设置好。
                    {
                        jLog（j_.debug（））
                            <“RippleCalc:更好：
                            <“mindex=”<<pathstate->index（））
                            <“uquality=”<<pathstate->quality（））
                            <率=
                            <<amountFromQuality（pathstate->quality（））
                            <“inpass（）=”<<pathstate->inpass（））
                            <“saoutpass=”<<pathstate->outpass（）；

                        ibest=pathstate->index（）；
                    }
                }
            }
        }

        +IPASS；

        if（auto-stream=j_.debug（））
        {
            流动
                <“RippleCalc:摘要：”
                <“通过：”<<ipass
                <“干：”<<idry
                <“路径：”<<pathstateList_.size（）；
            for（自动路径状态：路径状态列表）
            {
                流动
                    <“RippleCalc:”
                    <“摘要：”<<pathstate->index（）
                    <率：
                    <<amountFromQuality（pathstate->quality（））
                    <“质量：”<<pathstate->quality（）
                    <“最佳：”<（ibest==pathstate->index（））；
            }
        }

        如果（iBEST＞0）
        {
            //应用最佳路径。
            auto pathstate=pathstateList_[ibest]；

            如果（FlowDebugInfo）
                流debuginfo->pushpass（toeitheramount（pathstate->inpass（）），
                    toeitheramount（路径状态->输出通道（）），
                    pathstateList_.size（）-idry）；

            jLog（调试）（）
                <“RippleCalc:最佳：”
                <“uQuality=”<<amountFromQuality（pathstate->quality（））
                <“inpass（）=”<<pathstate->inpass（）。
                <“saoutpass=”<<pathstate->outpass（）<“ibest=”<<ibest；

            //record best pass'提供的没有资金删除
            成功。

            unfundedoffersfrombestpaths.insert（
                pathstate->unfundedoffers（）。开始（），
                pathstate->unfundedoffers（）.end（））；

            //应用最佳传递视图
            pathstate->view（）。应用（view）；

            actualMountin_+=pathstate->inpass（）；
            actualMountOut_+=pathstate->outpass（）；

            jLog（j_.trace（））
                    <“RippleCalc:最佳：”
                    < UQuest=
                    <<amountFromQuality（pathstate->quality（））
                    <“inpass（）=”<<pathstate->inpass（））
                    <“saoutpass=”<<pathstate->outpass（））
                    <“actualn=”<<实际金额
                    <“actualout=”<<actualoamountout\u
                    <“ibest=”<<ibest；

            if（多质量）
            {
                +IDRY；
                路径状态->设置质量（0）；
            }

            如果（actualmountout==sadstamountreq）
            {
                /完成。已送达请求金额。

                结果代码=Tesuccess；
            }
            否则，如果（actualamountout>sadstamountreq）
            {
                jLog（j_.fatal（））
                    <“RippleCalc:太多：”
                    <“actualamountout”：<<actualamountout
                    <“sadstamountreq”<<sadstamountreq；

                返回tefexception；//临时
                断言（假）；
            }
            否则，如果=samaxamountreq&&
                     讨厌！=路径状态列表.SIZE（））
            {
                //未满足请求的数量或最大发送量，请尝试执行
                /更多。准备下一关。
                / /
                //合并最佳过程'umreverse。
                mumsource插入（
                    pathstate->reverse（）.begin（），pathstate->reverse（）.end（））；

                如果（ipass>=付款_max_循环）
                {
                    //这次支付的通行证太多了

                    jLog（j_.error（））
                       <“RippleCalc:通过限制”；

                    结果代码=telfailed_处理；
                }

            }
            否则如果（！）inputflags.partialPayment允许）
            {
                //已发送最大允许值。不允许部分付款。

                结果代码=tecpath_部分；
            }
            其他的
            {
                //已发送最大允许值。允许部分付款。成功。

                结果代码=Tesuccess；
            }
        }
        //未完成，路径不足。
        否则如果（！）inputflags.partialPayment允许）
        {
            //不允许部分付款。
            结果代码=tecpath_部分；
        }
        //部分付款可以。
        否则如果（！）实际支出
        {
            //完全没有付款。
            结果代码=tecpath_dry；
        }
        其他的
        {
            //不应用任何付款增量
            结果代码=Tesuccess；
        }
    }

    如果（resultcode==tessuccess）
    {
        auto viewj=日志日志（“view”）；
        结果代码=DeleteOffers（视图，UnfundedOffersFromBestPaths，视图J）；
        如果（resultcode==tessuccess）
            结果代码=删除报价（视图，永久取消报价人，视图J）；
    }

    //如果isopenledger，那么ledger不是最终的，可以投反对票。
    如果（resultcode==telfailed_处理&！输入标志.isledgeropen）
        退货Tecfailed_处理；
    返回结果代码；
}

}//PATH
} /纹波
