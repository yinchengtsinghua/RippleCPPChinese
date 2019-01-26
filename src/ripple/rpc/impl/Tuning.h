
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

#ifndef RIPPLE_RPC_TUNING_H_INCLUDED
#define RIPPLE_RPC_TUNING_H_INCLUDED

namespace ripple {
namespace RPC {

/*调谐常数。*/
/*@ {*/
namespace Tuning {

/*表示具有最小值、默认值和最大值的RPC限制参数值。*/
struct LimitRange {
    unsigned int rmin, rdefault, rmax;
};

/*帐户行命令的限制。*/
static LimitRange const accountLines = {10, 200, 400};

/*帐户通道命令的限制。*/
static LimitRange const accountChannels = {10, 200, 400};

/*帐户对象命令的限制。*/
static LimitRange const accountObjects = {10, 200, 400};

/*帐户限制\提供命令。*/
static LimitRange const accountOffers = {10, 200, 400};

/*限制书提供命令。*/
static LimitRange const bookOffers = {0, 300, 400};

/*no_ripple_check命令的限制。*/
static LimitRange const noRippleCheck = {10, 300, 400};

static int const defaultAutoFillFeeMultiplier = 10;
static int const defaultAutoFillFeeDivisor = 1;
static int const maxPathfindsInProgress = 2;
static int const maxPathfindJobCount = 50;
static int const maxJobQueueClients = 500;
auto constexpr maxValidatedLedgerAge = std::chrono::minutes {2};
static int const maxRequestSize = 1000000;

/*来自二进制LedgerData请求的一个响应中的最大页数。*/
static int const binaryPageLength = 2048;

/*来自JSON LedgerData请求的一个响应中的最大页数。*/
static int const jsonPageLength = 256;

/*LedgerData响应中的最大页数。*/
inline int pageLength(bool isBinary)
{
    return isBinary ? binaryPageLength : jsonPageLength;
}

/*路径查找请求中允许的最大源货币数。*/
static int const max_src_cur = 18;

/*路径查找请求中自动源货币的最大数目。*/
static int const max_auto_src_cur = 88;

} //调谐
/*@ }*/

} //RPC
} //涟漪

#endif
