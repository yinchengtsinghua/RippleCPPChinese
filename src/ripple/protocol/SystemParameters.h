
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

#ifndef RIPPLE_PROTOCOL_SYSTEMPARAMETERS_H_INCLUDED
#define RIPPLE_PROTOCOL_SYSTEMPARAMETERS_H_INCLUDED

#include <cstdint>
#include <string>

namespace ripple {

//各种协议和系统特定的常量全局。

/*系统的名称。*/
static inline
std::string const&
systemName ()
{
    static std::string const name = "ripple";
    return name;
}

/*配置本国货币。*/
static
std::uint64_t const
SYSTEM_CURRENCY_GIFT = 1000;

static
std::uint64_t const
SYSTEM_CURRENCY_USERS = 100000000;

/*每1 xrp的滴数*/
static
std::uint64_t const
SYSTEM_CURRENCY_PARTS = 1000000;

/*Genesis帐户中的滴数。*/
static
std::uint64_t const
SYSTEM_CURRENCY_START = SYSTEM_CURRENCY_GIFT * SYSTEM_CURRENCY_USERS * SYSTEM_CURRENCY_PARTS;

/*本国货币的货币代码。*/
static inline
std::string const&
systemCurrencyCode ()
{
    static std::string const code = "XRP";
    return code;
}

/*xrp分类帐网络允许的最早序列*/
static
std::uint32_t constexpr
XRP_LEDGER_EARLIEST_SEQ {32570};

} //涟漪

#endif
