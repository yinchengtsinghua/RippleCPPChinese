
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

#ifndef RIPPLE_TX_APPLYSTEPS_H_INCLUDED
#define RIPPLE_TX_APPLYSTEPS_H_INCLUDED

#include <ripple/ledger/ApplyViewImpl.h>
#include <ripple/beast/utility/Journal.h>

namespace ripple {

class Application;
class STTx;

/*如果交易可以收取费用（tec），则返回true，
    “applyFlags”不允许软故障。
 **/

inline
bool
isTecClaimHardFail(TER ter, ApplyFlags flags)
{
    return isTecClaim(ter) && !(flags & tapRETRY);
}

/*描述“飞行前”检查的结果

    @请注意，所有成员都是常量，以增加难度。
        在不调用“preflight”的情况下“伪造”结果。
    @见飞行前、预索赔、DOAPPLY、APPLY
**/

struct PreflightResult
{
public:
///从输入-事务
    STTx const& tx;
///从输入-规则
    Rules const rules;
///从输入-标志
    ApplyFlags const flags;
///来自输入-日志
    beast::Journal const j;

///中间交易结果
    NotTEC const ter;

///构造函数
    template<class Context>
    PreflightResult(Context const& ctx_,
        NotTEC ter_)
        : tx(ctx_.tx)
        , rules(ctx_.rules)
        , flags(ctx_.flags)
        , j(ctx_.j)
        , ter(ter_)
    {
    }

    PreflightResult(PreflightResult const&) = default;
///已删除复制分配运算符
    PreflightResult& operator=(PreflightResult const&) = delete;
};

/*描述“预索赔”检查的结果

    @请注意，所有成员都是常量，以增加难度。
        在不调用“preclaim”的情况下“伪造”结果。
    @见飞行前、预索赔、DOAPPLY、APPLY
**/

struct PreclaimResult
{
public:
///从输入-分类帐视图
    ReadView const& view;
///从输入-事务
    STTx const& tx;
///从输入-标志
    ApplyFlags const flags;
///来自输入-日志
    beast::Journal const j;

///中间交易结果
    TER const ter;
///success flag-事务是否可能
//索赔费用
    bool const likelyToClaimFee;

///构造函数
    template<class Context>
    PreclaimResult(Context const& ctx_, TER ter_)
        : view(ctx_.view)
        , tx(ctx_.tx)
        , flags(ctx_.flags)
        , j(ctx_.j)
        , ter(ter_)
        , likelyToClaimFee(ter == tesSUCCESS
            || isTecClaimHardFail(ter, flags))
    {
    }

    PreclaimResult(PreclaimResult const&) = default;
///已删除复制分配运算符
    PreclaimResult& operator=(PreclaimResult const&) = delete;
};

/*描述对账户后果的结构
    如果事务消耗
    允许的最大xrp。

    @见计算结果
**/

struct TxConsequences
{
///描述事务如何影响后续事务
///交易
    enum ConsequenceCategory
    {
///移动货币、创建报价等。
        normal = 0,
///影响后续交易的能力
///收取费用。例如'setRegularKey'
        blocker
    };

///描述事务如何影响后续事务
///交易
    ConsequenceCategory const category;
///交易费
    XRPAmount const fee;
///不包括费用。
    XRPAmount const potentialSpend;

///构造函数
    TxConsequences(ConsequenceCategory const category_,
        XRPAmount const fee_, XRPAmount const spend_)
        : category(category_)
        , fee(fee_)
        , potentialSpend(spend_)
    {
    }

///构造函数
    TxConsequences(TxConsequences const&) = default;
///已删除复制分配运算符
    TxConsequences& operator=(TxConsequences const&) = delete;
///构造函数
    TxConsequences(TxConsequences&&) = default;
///已删除复制分配运算符
    TxConsequences& operator=(TxConsequences&&) = delete;

};

/*基于静态信息的事务入口。

    该事务将与所有可能的
    不需要分类帐的有效性约束。

    @param app当前正在运行的“application”。
    @param规则检查时生效的“rules”。
    @param tx要检查的事务。
    @param flags`applyflags`描述处理选项。
    @param j日记。

    @见预照明结果、预索赔、DOAPPLY、APPLY

    @返回一个包含，其中
    其他的东西，“ter”代码。
**/

PreflightResult
preflight(Application& app, Rules const& rules,
    STTx const& tx, ApplyFlags flags,
        beast::Journal j);

/*基于静态分类帐信息的交易。

    该事务将与所有可能的
    确实需要分类帐的有效性约束。

    如果预索赔成功，那么交易
    可能要收费。这将决定
    事务可以安全地在不应用的情况下进行中继
    到未结分类帐。

    在这种情况下，“成功”定义为返回
    “TES”或“TEC”，因为两者都会导致收取费用。

    @pre交易已被检查
    并使用“飞行前”进行验证

    @param preflightresult前一个结果
        调用“preflight”进行交易。
    @param app当前正在运行的“application”。
    @param查看交易的未结分类帐
        将尝试应用于。

    @见预索赔结果，飞行前，DOAPPLY，APPLY

    @返回一个“preclamresult”对象，其中包含
    “ter”代码和基本费用值的其他内容
    本次交易。
**/

PreclaimResult
preclaim(PreflightResult const& preflightResult,
    Application& app, OpenView const& view);

/*只计算交易的预期基本费用。

    基本费用是特定于交易的，因此任何计算
    需要他们必须得到每个交易的基本费用。

    此函数不执行任何验证或暗示任何验证。

    调用者负责处理任何异常。
    因为不应该抛出任何对象，所以通常
    意味着终止。

    @param查看当前打开的分类帐。
    @param tx要检查的事务。

    @返回基本费用。
**/

std::uint64_t
calculateBaseFee(ReadView const& view,
    STTx const& tx);

/*确定交易的xrp余额结果
    使用允许的最大xrp。

    @pre交易已被检查
    并使用“飞行前”进行验证

    @param preflightresult前一个结果
    调用“preflight”进行交易。

    @返回包含“最差”的“txresults”对象
        案例“将此交易应用于
        分类帐

    @见TxReasons
**/

TxConsequences
calculateConsequences(PreflightResult const& preflightResult);

/*将预检查的事务应用于OpenView。

    @pre交易已被检查
    并使用“飞行前”和“预索赔”进行验证

    @param preclaimresult前一个结果
    调用'preclaim'进行交易。
    @param app当前正在运行的“application”。
    @param查看交易的未结分类帐
    将尝试应用于。

    @见飞行前、预漂浮、应用

    @返回一对“ter”和“bool”表示
    是否应用了该事务。
**/

std::pair<TER, bool>
doApply(PreclaimResult const& preclaimResult,
    Application& app, OpenView& view);

}

#endif
