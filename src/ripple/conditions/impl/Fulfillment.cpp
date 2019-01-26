
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

#include <ripple/basics/safe_cast.h>
#include <ripple/conditions/Condition.h>
#include <ripple/conditions/Fulfillment.h>
#include <ripple/conditions/impl/PreimageSha256.h>
#include <ripple/conditions/impl/utils.h>
#include <boost/regex.hpp>
#include <boost/optional.hpp>
#include <type_traits>
#include <vector>

namespace ripple {
namespace cryptoconditions {

bool
match (
    Fulfillment const& f,
    Condition const& c)
{
//快速检查：履行的类型必须与
//条件类型：
    if (f.type() != c.type)
        return false;

//从给定的实现中派生条件
//并确保它符合给定的条件。
    return c == f.condition();
}

bool
validate (
    Fulfillment const& f,
    Condition const& c,
    Slice m)
{
    return match (f, c) && f.validate (m);
}

bool
validate (
    Fulfillment const& f,
    Condition const& c)
{
    return validate (f, c, {});
}

std::unique_ptr<Fulfillment>
Fulfillment::deserialize(
    Slice s,
    std::error_code& ec)
{
//根据RFC，在实现中，我们选择基于类型的
//在项目的标签上，我们包含：
//
//履行：：=选择
//预成像256[0]预成像实现，
//前缀256[1]前缀实现，
//阈值256[2]阈值实现，
//RSASHA256[3]RSASHA256履行，
//ED25519sha256[4]ED25519sha512履行
//}

    if (s.empty())
    {
        ec = error::buffer_empty;
        return nullptr;
    }

    using namespace der;

    auto const p = parsePreamble(s, ec);
    if (ec)
        return nullptr;

//所有实现都是上下文特定的构造类型
    if (!isConstructed(p) || !isContextSpecific(p))
    {
        ec = error::malformed_encoding;
        return nullptr;
    }

    if (p.length > s.size())
    {
        ec = error::buffer_underfull;
        return {};
    }

    if (p.length < s.size())
    {
        ec = error::buffer_overfull;
        return {};
    }

    if (p.length > maxSerializedFulfillment)
    {
        ec = error::large_size;
        return {};
    }

    std::unique_ptr<Fulfillment> f;

    using TagType = decltype(p.tag);
    switch (p.tag)
    {
    case safe_cast<TagType>(Type::preimageSha256):
        f = PreimageSha256::deserialize(Slice(s.data(), p.length), ec);
        if (ec)
            return {};
        s += p.length;
        break;

    case safe_cast<TagType>(Type::prefixSha256):
        ec = error::unsupported_type;
        return {};
        break;

    case safe_cast<TagType>(Type::thresholdSha256):
        ec = error::unsupported_type;
        return {};
        break;

    case safe_cast<TagType>(Type::rsaSha256):
        ec = error::unsupported_type;
        return {};
        break;

    case safe_cast<TagType>(Type::ed25519Sha256):
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

    return f;
}

}
}
