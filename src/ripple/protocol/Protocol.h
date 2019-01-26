
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

#ifndef RIPPLE_PROTOCOL_PROTOCOL_H_INCLUDED
#define RIPPLE_PROTOCOL_PROTOCOL_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/ByteUtilities.h>
#include <cstdint>

namespace ripple {

/*协议特定的常量、类型和数据。

    这一信息隐含地是涟漪的一部分。
    协议。

    @注意更改这些值而不向
          检测“变更前”和“变更后”的服务器
          会产生硬叉。
**/

/*事务的最小合法字节大小。*/
std::size_t constexpr txMinSizeBytes = 32;

/*事务的最大合法字节大小。*/
std::size_t constexpr txMaxSizeBytes = megabytes(1);

/*一次可删除的未供资金的报价的最大数量*/
std::size_t constexpr unfundedOfferRemoveLimit = 1000;

/*一个事务中允许的最大元数据条目数*/
std::size_t constexpr oversizeMetaDataCap = 5200;

/*每个目录页的最大条目数*/
std::size_t constexpr dirNodeMaxEntries = 32;

/*目录中允许的最大页数*/
std::uint64_t constexpr dirNodeMaxPages = 262144;

/*分类帐索引。*/
using LedgerIndex = std::uint32_t;

/*事务标识符。
    该值作为
    规范化、序列化的事务对象。
**/

using TxID = uint256;

using TxSeq = std::uint32_t;

} //涟漪

#endif
