
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

#ifndef RIPPLE_PROTOCOL_AMOUNTCONVERSION_H_INCLUDED
#define RIPPLE_PROTOCOL_AMOUNTCONVERSION_H_INCLUDED

#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/XRPAmount.h>
#include <ripple/protocol/STAmount.h>

namespace ripple {

inline
STAmount
toSTAmount (IOUAmount const& iou, Issue const& iss)
{
    bool const isNeg = iou.signum() < 0;
    std::uint64_t const umant = isNeg ? - iou.mantissa () : iou.mantissa ();
    /*urn stamount（iss，umant，iou.exponent（），/*本机*/false，isneg，
                     stamount:：unchecked（））；
}

内联的
斯塔姆
Tostamount（iouamount const&iou）
{
    返回到tamount（iou，noissue（））；
}

内联的
斯塔姆
tostamount（xrpamount const&xrp）
{
    bool const isneg=xrp.signum（）<0；
    std:：uint64不构成=是吗？-xrp.drops（）：xrp.drops（）；
    返回stamount（umant，isneg）；
}

内联的
斯塔姆
tostamount（xrpamount const&xrp，issue const&iss）
{
    断言（isxrp（iss.account）&&isxrp（iss.currency））；
    返回到tamount（xrp）；
}

模板<class t>
T
toamount（stamount const&amt）
{
    static_assert（sizeof（t）=-1，“必须使用专用函数”）；
    返回T（0）；
}

模板< >
内联的
斯塔姆
toamount<stamount>（stamount const&amt）
{
    返回AMT；
}

模板< >
内联的
碘量
toamount<iouamount>（stamount const&amt）
{
    断言（amt.mantissa（）<std:：numeric_limits<std:：int64_t>：：max（））；
    bool const isneg=amt.negative（）；
    标准：：Int64常数=
            伊斯奈格？-标准：：Int64_t（尾数（））：尾数（）；

    断言（！）ISXRP（AMT）；
    返回iouamount（smant，amt.exponent（））；
}

模板< >
内联的
X射线量
toamount<xrpamount>（stamount const&amt）
{
    断言（amt.mantissa（）<std:：numeric_limits<std:：int64_t>：：max（））；
    bool const isneg=amt.negative（）；
    标准：：Int64常数=
            伊斯奈格？-标准：：Int64_t（尾数（））：尾数（）；

    断言（isxrp（amt））；
    返回xrpamount（smant）；
}


}

第二节
