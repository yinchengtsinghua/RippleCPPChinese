
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2014，howard hinnant<howard.hinnant@gmail.com>

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

#ifndef BEAST_TYPE_NAME_H_INCLUDED
#define BEAST_TYPE_NAME_H_INCLUDED

#include <type_traits>
#include <typeinfo>
#include <iostream>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif
#include <memory>
#include <string>
#include <cstdlib>
#include <vector>

namespace beast {

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4127) //条件表达式是常量
#endif

template <typename T>
std::string
type_name()
{
    using TR = typename std::remove_reference<T>::type;
    std::unique_ptr<char, void(*)(void*)> own (
    #ifndef _MSC_VER
        abi::__cxa_demangle (typeid(TR).name(), nullptr,
            nullptr, nullptr),
    #else
            nullptr,
    #endif
            std::free
    );
    std::string r = own != nullptr ? own.get() : typeid(TR).name();
    if (std::is_const<TR>::value)
        r += " const";
    if (std::is_volatile<TR>::value)
        r += " volatile";
    if (std::is_lvalue_reference<T>::value)
        r += "&";
    else if (std::is_rvalue_reference<T>::value)
        r += "&&";
    return r;
}

#ifdef _MSC_VER
#pragma warning (pop)
#endif

} //野兽

#endif
