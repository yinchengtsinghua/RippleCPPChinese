
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
    版权所有（c）2012-2015 Ripple Labs Inc.

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

#ifndef RIPPLE_BASICS_MULDIV_H_INCLUDED
#define RIPPLE_BASICS_MULDIV_H_INCLUDED

#include <cstdint>
#include <utility>

namespace ripple
{

/*返回值*MUL/DIV准确。
    计算乘法和除法的结果
    一步到位，避免溢出，保持精度高。
    投掷：
        没有
    返回：
        “性病：：配对”：
            “first”是“false”，如果计算溢出，
                如果计算安全，则返回“真”。
            `second`是计算结果，如果
                “first”是“false”，“uint64”的最大值
                如果是真的。
**/

std::pair<bool, std::uint64_t>
mulDiv(std::uint64_t value, std::uint64_t mul, std::uint64_t div);

} //涟漪

#endif
