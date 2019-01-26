
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

#ifndef BEAST_HASH_META_H_INCLUDED
#define BEAST_HASH_META_H_INCLUDED

#include <type_traits>

namespace beast {

template <bool ...> struct static_and;

template <bool b0, bool ... bN>
struct static_and <b0, bN...>
    : public std::integral_constant <
        bool, b0 && static_and<bN...>::value>
{
    explicit static_and() = default;
};

template <>
struct static_and<>
    : public std::true_type
{
    explicit static_and() = default;
};

#ifndef __INTELLISENSE__
static_assert( static_and<true, true, true>::value, "");
static_assert(!static_and<true, false, true>::value, "");
#endif

template <std::size_t ...>
struct static_sum;

template <std::size_t s0, std::size_t ...sN>
struct static_sum <s0, sN...>
    : public std::integral_constant <
        std::size_t, s0 + static_sum<sN...>::value>
{
    explicit static_sum() = default;
};

template <>
struct static_sum<>
    : public std::integral_constant<std::size_t, 0>
{
    explicit static_sum() = default;
};

#ifndef __INTELLISENSE__
static_assert(static_sum<5, 2, 17, 0>::value == 24, "");
#endif

template <class T, class U>
struct enable_if_lvalue
    : public std::enable_if
    <
    std::is_same<std::decay_t<T>, U>::value &&
    std::is_lvalue_reference<T>::value
    >
{
    explicit enable_if_lvalue() = default;
};

/*确保常量引用函数参数是有效的lvalues。

    一些函数，特别是类构造函数，接受常量引用和
    保存以备日后使用。如果这些参数中有任何一个是右值对象，
    一旦函数返回，对象将被释放。这可能
    可能导致各种“免费使用”错误。

    如果使用此类型和
    参数引用作为右值引用（例如tx&&），编译器
    如果调用方中提供了右值，则将生成错误。

    @代码
        /例子：
        结构X
        {
        }；
        结构体
        {
        }；

        不安全结构
        {
            不安全（x const&x，y const&y）
                xx（x）
                YYY（Y）
            {
            }

            X const与Xi；
            Y.const & Y.
        }；

        结构安全
        {
            模板<Tx类，Ty类，
            class=beast：：启用_if_lvalue_t<t x，x>，
            class=beast：：启用_if_lvalue_t<t y，y>>
                安全（tx和x，ty和y）
                xx（x）
                YYY（Y）
            {
            }

            X const与Xi；
            Y.const & Y.
        }；

        结构演示
        {
            无效
                创建对象（）
            {
                xx{}；
                Y常数{{}；
                不安全的U1（x，y）；//好
                unsafe u2（x（），y）；//编译，但u2.x_u在行尾无效。
                安全S1（x，y）；//好
                //安全s2（x（），y）；//编译时错误
            }
        }；
    @端码
**/

template <class T, class U>
using enable_if_lvalue_t = typename enable_if_lvalue<T, U>::type;

} //野兽

#endif //包含Beast_实用程序\u meta_h_
