
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
    版权所有（c）2016 Ripple Labs Inc.

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

#include <ripple/basics/contract.h>
#include <ripple/conditions/Condition.h>
#include <ripple/conditions/Fulfillment.h>
#include <ripple/conditions/impl/PreimageSha256.h>
#include <ripple/conditions/impl/utils.h>
#include <boost/regex.hpp>
#include <boost/optional.hpp>
#include <vector>
#include <iostream>

namespace ripple {
namespace cryptoconditions {

namespace detail {
//条件的二进制编码根据其
//类型。所有类型至少定义一个指纹和成本
//子场。一些类型，例如复合条件
//类型，定义需要的其他子字段
//传递密码条件的基本属性（例如
//作为子条件在
//化合物类型）。
//
//条件编码如下：
//
//条件：：=选择
//预成像256[0]单工256条件，
//PrefixSha256[1]复合材料256条件，
//阈值256[2]复合256条件，
//RSASHA256[3]单工256条件，
//ED25519SHA256[4]单工256条件
//}
//
//单工256条件：：=顺序
//指纹八位字节字符串（大小（32）），
//成本整数（0..4294967295）
//}
//
//复合材料256条件：：=序列
//指纹八位字节字符串（大小（32）），
//成本整数（0..4294967295）
//子类型条件类型
//}
//
//条件类型：：=位字符串
//预成像256（0），
//前缀256（1）
//阈值256（2）
//RSASHA256（3）
//ED25519SHA256（4）
//}

constexpr std::size_t fingerprintSize = 32;

std::unique_ptr<Condition>
loadSimpleSha256(Type type, Slice s, std::error_code& ec)
{
    using namespace der;

    auto p = parsePreamble(s, ec);

    if (ec)
        return {};

    if (!isPrimitive(p) || !isContextSpecific(p))
    {
        ec = error::incorrect_encoding;
        return {};
    }

    if (p.tag != 0)
    {
        ec = error::unexpected_tag;
        return {};
    }

    if (p.length != fingerprintSize)
    {
        ec = error::fingerprint_size;
        return {};
    }

    Buffer b = parseOctetString(s, p.length, ec);

    if (ec)
        return {};

    p = parsePreamble(s, ec);

    if (ec)
        return {};

    if (!isPrimitive(p) || !isContextSpecific(p))
    {
        ec = error::malformed_encoding;
        return{};
    }

    if (p.tag != 1)
    {
        ec = error::unexpected_tag;
        return {};
    }

    auto cost = parseInteger<std::uint32_t>(s, p.length, ec);

    if (ec)
        return {};

    if (!s.empty())
    {
        ec = error::trailing_garbage;
        return {};
    }

    switch (type)
    {
    case Type::preimageSha256:
        if (cost > PreimageSha256::maxPreimageLength)
        {
            ec = error::preimage_too_long;
            return {};
        }
        break;

    default:
        break;
    }

    return std::make_unique<Condition>(type, cost, std::move(b));
}

}

std::unique_ptr<Condition>
Condition::deserialize(Slice s, std::error_code& ec)
{
//根据RFC，在某种情况下，我们选择基于类型的
//在项目的标签上，我们包含：
//
//条件：：=选择
//预成像256[0]单工256条件，
//PrefixSha256[1]复合材料256条件，
//阈值256[2]复合256条件，
//RSASHA256[3]单工256条件，
//ED25519SHA256[4]单工256条件
//}
    if (s.empty())
    {
        ec = error::buffer_empty;
        return {};
    }

    using namespace der;

    auto const p = parsePreamble(s, ec);
    if (ec)
        return {};

//所有实现都是上下文特定的、构造的
//类型
    if (!isConstructed(p) || !isContextSpecific(p))
    {
        ec = error::malformed_encoding;
        return {};
    }

    if (p.length > s.size())
    {
        ec = error::buffer_underfull;
        return {};
    }

    if (s.size() > maxSerializedCondition)
    {
        ec = error::large_size;
        return {};
    }

    std::unique_ptr<Condition> c;

    switch (p.tag)
    {
case 0: //预兆HA256
        c = detail::loadSimpleSha256(
            Type::preimageSha256,
            Slice(s.data(), p.length), ec);
        if (!ec)
            s += p.length;
        break;

case 1: //PrimSux256
        ec = error::unsupported_type;
        return {};

case 2: //阈下SAH256
        ec = error::unsupported_type;
        return {};

case 3: //RSASH256
        ec = error::unsupported_type;
        return {};

case 4: //ED25519SAH256
        ec = error::unsupported_type;
        return {};

    default:
        ec = error::unknown_type;
        return {};
    }

    if (!s.empty())
    {
        ec = error::trailing_garbage;
        return {};
    }

    return c;
}

}
}
