
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
#include <ripple/conditions/impl/error.h>
#include <system_error>
#include <string>
#include <type_traits>

namespace ripple {
namespace cryptoconditions {
namespace detail {

class cryptoconditions_error_category
    : public std::error_category
{
public:
    explicit cryptoconditions_error_category() = default;

    const char*
    name() const noexcept override
    {
        return "cryptoconditions";
    }

    std::string
    message(int ev) const override
    {
        switch (safe_cast<error>(ev))
        {
        case error::unsupported_type:
            return "Specification: Requested type not supported.";

        case error::unsupported_subtype:
            return "Specification: Requested subtype not supported.";

        case error::unknown_type:
            return "Specification: Requested type not recognized.";

        case error::unknown_subtype:
            return "Specification: Requested subtypes not recognized.";

        case error::fingerprint_size:
            return "Specification: Incorrect fingerprint size.";

        case error::incorrect_encoding:
            return "Specification: Incorrect encoding.";

        case error::trailing_garbage:
            return "Bad buffer: contains trailing garbage.";

        case error::buffer_empty:
            return "Bad buffer: no data.";

        case error::buffer_overfull:
            return "Bad buffer: overfull.";

        case error::buffer_underfull:
            return "Bad buffer: underfull.";

        case error::malformed_encoding:
            return "Malformed DER encoding.";

        case error::unexpected_tag:
            return "Malformed DER encoding: Unexpected tag.";

        case error::short_preamble:
            return "Malformed DER encoding: Short preamble.";

        case error::long_tag:
            return "Implementation limit: Overlong tag.";

        case error::large_size:
            return "Implementation limit: Large payload.";

        case error::preimage_too_long:
            return "Implementation limit: Specified preimage is too long.";

        case error::generic:
        default:
            return "generic error";
        }
    }

    std::error_condition
    default_error_condition(int ev) const noexcept override
    {
        return std::error_condition{ ev, *this };
    }

    bool
    equivalent(
        int ev,
        std::error_condition const& condition) const noexcept override
    {
        return &condition.category() == this &&
            condition.value() == ev;
    }

    bool
    equivalent(
        std::error_code const& error,
        int ev) const noexcept override
    {
        return &error.category() == this &&
            error.value() == ev;
    }
};

inline
std::error_category const&
get_cryptoconditions_error_category()
{
    static cryptoconditions_error_category const cat{};
    return cat;
}

} //细节

std::error_code
make_error_code(error ev)
{
    return std::error_code {
        safe_cast<std::underlying_type<error>::type>(ev),
        detail::get_cryptoconditions_error_category()
    };
}

}
}
