
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
    版权所有（c）2014 Ripple Labs Inc.

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

#ifndef RIPPLE_BASICS_CONTRACT_H_INCLUDED
#define RIPPLE_BASICS_CONTRACT_H_INCLUDED

#include <ripple/beast/type_name.h>
#include <exception>
#include <string>
#include <typeinfo>
#include <utility>

namespace ripple {

/*按合同编程

    检查时使用此例程
    前提条件、后置条件和不变量。
**/


/*生成并记录调用堆栈*/
void
LogThrow (std::string const& title);

/*重新引发当前正在处理的异常。

    当从catch块内调用时，它将通过
    控制到下一个匹配的异常处理程序（如果有）。
    否则，将调用std：：terminate。
**/

[[noreturn]]
inline
void
Rethrow ()
{
    LogThrow ("Re-throwing exception");
    throw;
}

template <class E, class... Args>
[[noreturn]]
inline
void
Throw (Args&&... args)
{
    static_assert (std::is_convertible<E*, std::exception*>::value,
        "Exception must derive from std::exception.");

    E e(std::forward<Args>(args)...);
    LogThrow (std::string("Throwing exception of type " +
                          beast::type_name<E>() +": ") + e.what());
    throw e;
}

/*当错误的逻辑导致不变量损坏时调用。*/
[[noreturn]]
void
LogicError (
    std::string const& how) noexcept;

} //涟漪

#endif
