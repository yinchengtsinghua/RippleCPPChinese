
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

#ifndef RIPPLE_TX_APPLY_H_INCLUDED
#define RIPPLE_TX_APPLY_H_INCLUDED

#include <ripple/core/Config.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/TER.h>
#include <ripple/beast/utility/Journal.h>
#include <memory>
#include <utility>

namespace ripple {

class Application;
class HashRouter;

/*描述事务的预处理有效性。

    @参见检查有效性，强制有效性
**/

enum class Validity
{
///签名错误。没有做本地检查。
    SigBad,
///签名良好，但本地检查失败。
    SigGoodOnly,
///签名和本地支票有效/通过。
    Valid
};

/*检查事务签名和本地检查。

    @返回一个“validity”枚举，表示
        “sttx”是原因字符串，如果不是“valid”。

    @note结果在内部缓存，因此测试将不会
        重复多次调用，除非缓存过期。

    @return`std:：pair`，其中`.first`是状态，并且
            如果合适的话，“第二个”就是原因。

    参见有效性
**/

std::pair<Validity, std::string>
checkValidity(HashRouter& router,
    STTx const& tx, Rules const& rules,
        Config const& config);


/*设置缓存中给定事务的有效性。

    @小心使用。

    @注意只能将有效性提升到更有效的状态，
          并且不能重写任何缓存错误的内容。

    @见检查有效性，有效性
**/

void
forceValidity(HashRouter& router, uint256 const& txid,
    Validity validity);

/*将事务应用于“openview”。

    此函数是应用事务的规范方法
    分类帐它滚动验证和应用程序
    单步执行一个函数。要手动执行步骤，请
    正确的呼叫顺序是：
    @代码{CPP}
    飞行前->预索赔->DOAPPLY
    @端码
    一个函数的结果必须传递给下一个函数。
    “preflight”结果可以安全地缓存和重用
    异步，但必须调用'preclaim'和'doapply'
    在同一线程和同一视图中。

    @注意不扔。

    对于未结分类帐，“Transactor”将捕获异常
    并返回“tefeexception”。对于封闭式分类账，
    ‘交易人’将试图只收取费用，
    并返回“tecfailed_processing”。

    如果“transactor”在尝试时遇到异常
    要收取费用，它会被抓住
    变成了“tefexception”。

    对于网络健康，“事务处理程序”使
    尽最大努力至少收取费用，如果
    分类帐已关闭。

    @param app当前正在运行的“application”。
    @param查看交易的未结分类帐
        将尝试应用于。
    @param tx要检查的事务。
    @param flags`applyflags`描述处理选项。
    @param日记。

    @见飞行前、预索赔、DOAPPLY

    @返回一对“ter”和“bool”表示
            是否应用了该事务。
**/

std::pair<TER, bool>
apply (Application& app, OpenView& view,
    STTx const& tx, ApplyFlags flags,
        beast::Journal journal);


/*“applyTransaction”返回值的枚举类

    @见应用转换
**/

enum class ApplyResult
{
///应用于此分类帐
    Success,
///不应在此分类帐中重试
    Fail,
///应在此分类帐中重试
    Retry
};

/*事务应用程序帮助程序

    提供更详细的日志记录和解码
    根据“ter”类型更正行为

    @见应用结果
**/

ApplyResult
applyTransaction(Application& app, OpenView& view,
    STTx const& tx, bool retryAssured, ApplyFlags flags,
    beast::Journal journal);

} //涟漪

#endif
