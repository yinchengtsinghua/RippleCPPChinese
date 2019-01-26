
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

#ifndef RIPPLE_CONDITIONS_PREIMAGE_SHA256_H
#define RIPPLE_CONDITIONS_PREIMAGE_SHA256_H

#include <ripple/basics/Buffer.h>
#include <ripple/basics/Slice.h>
#include <ripple/conditions/Condition.h>
#include <ripple/conditions/Fulfillment.h>
#include <ripple/conditions/impl/error.h>
#include <ripple/protocol/digest.h>
#include <memory>

namespace ripple {
namespace cryptoconditions {

class PreimageSha256 final
    : public Fulfillment
{
public:
    /*预映像允许的最大长度。

        规范未指定支持的最小值
        长度，也不需要所有条件来支持
        相同的最小长度。

        而此代码的未来版本永远不会降低
        这一限制，他们可能会选择提高。
    **/

    static constexpr std::size_t maxPreimageLength = 128;

    /*分析preimagesha256条件的有效负载

        @param s包含der编码有效载荷的切片
        @param ec表示操作成功或失败
        @如果成功，返回preimage；否则返回空指针。
    **/

    static
    std::unique_ptr<Fulfillment>
    deserialize(
        Slice s,
        std::error_code& ec)
    {
//根据RFC，一个预映像fullfullent定义为
//跟随：
//
//预映像实现：：=序列
//预映像八位字节字符串
//}

        using namespace der;

        auto p = parsePreamble(s, ec);
        if (ec)
            return nullptr;

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

        if (s.size() != p.length)
        {
            ec = error::trailing_garbage;
            return {};
        }

        if (s.size() > maxPreimageLength)
        {
            ec = error::preimage_too_long;
            return {};
        }

        auto b = parseOctetString(s, p.length, ec);
        if (ec)
            return {};

        return std::make_unique<PreimageSha256>(std::move(b));
    }

private:
    Buffer payload_;

public:
    PreimageSha256(Buffer&& b) noexcept
        : payload_(std::move(b))
    {
    }

    PreimageSha256(Slice s) noexcept
        : payload_(s)
    {
    }

    Type
    type() const override
    {
        return Type::preimageSha256;
    }

    Buffer
    fingerprint() const override
    {
        sha256_hasher h;
        h(payload_.data(), payload_.size());
        auto const d = static_cast<sha256_hasher::result_type>(h);
        return{ d.data(), d.size() };
    }

    std::uint32_t
    cost() const override
    {
        return static_cast<std::uint32_t>(payload_.size());
    }

    Condition
    condition() const override
    {
        return { type(), cost(), fingerprint() };
    }

    bool
    validate(Slice) const override
    {
//也许与直觉相反，信息不是
//相关的。
        return true;
    }
};

}
}

#endif
