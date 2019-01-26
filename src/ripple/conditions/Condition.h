
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

#ifndef RIPPLE_CONDITIONS_CONDITION_H
#define RIPPLE_CONDITIONS_CONDITION_H

#include <ripple/basics/Buffer.h>
#include <ripple/basics/Slice.h>
#include <ripple/conditions/impl/utils.h>
#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <system_error>
#include <vector>

namespace ripple {
namespace cryptoconditions {

enum class Type
    : std::uint8_t
{
    preimageSha256 = 0,
    prefixSha256 = 1,
    thresholdSha256 = 2,
    rsaSha256 = 3,
    ed25519Sha256 = 4
};

class Condition
{
public:
    /*我们支持的最大二进制条件。

        @注意这个值将来会增加，但是
              决不能减少，因为那样会导致条件
              以前被认为不再有效的
              被允许。
    **/

    static constexpr std::size_t maxSerializedCondition = 128;

    /*从其二进制形式加载条件

        @param s包含要加载的实现的缓冲区。
        @param ec设置为错误（如果发生）。

        条件的二进制格式在
        密码条件。见：

        https://tools.ietf.org/html/draft-thomas-crypto-conditions-02第7.2节
    **/

    static
    std::unique_ptr<Condition>
    deserialize(Slice s, std::error_code& ec);

public:
    Type type;

    /*此条件的标识符。

        这个指纹只有在
        关于同一类型的其他条件。
    **/

    Buffer fingerprint;

    /*与此条件关联的成本。*/
    std::uint32_t cost;

    /*对于复合条件，一组条件包括*/
    std::set<Type> subtypes;

    Condition(Type t, std::uint32_t c, Slice fp)
        : type(t)
        , fingerprint(fp)
        , cost(c)
    {
    }

    Condition(Type t, std::uint32_t c, Buffer&& fp)
        : type(t)
        , fingerprint(std::move(fp))
        , cost(c)
    {
    }

    ~Condition() = default;

    Condition(Condition const&) = default;
    Condition(Condition&&) = default;

    Condition() = delete;
};

inline
bool
operator== (Condition const& lhs, Condition const& rhs)
{
    return
        lhs.type == rhs.type &&
            lhs.cost == rhs.cost &&
                lhs.subtypes == rhs.subtypes &&
                    lhs.fingerprint == rhs.fingerprint;
}

inline
bool
operator!= (Condition const& lhs, Condition const& rhs)
{
    return !(lhs == rhs);
}

}

}

#endif
