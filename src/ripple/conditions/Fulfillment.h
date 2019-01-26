
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

#ifndef RIPPLE_CONDITIONS_FULFILLMENT_H
#define RIPPLE_CONDITIONS_FULFILLMENT_H

#include <ripple/basics/Buffer.h>
#include <ripple/basics/Slice.h>
#include <ripple/conditions/Condition.h>
#include <ripple/conditions/impl/utils.h>
#include <boost/optional.hpp>

namespace ripple {
namespace cryptoconditions {

struct Fulfillment
{
public:
    /*我们支持的最大的二进制实现。

        @注意这个值将来会增加，但是
              决不能减少，因为那样会导致履行
              以前被认为不再有效的
              被允许。
    **/

    static constexpr std::size_t maxSerializedFulfillment = 256;

    /*从二进制形式加载实现

        @param s包含要加载的实现的缓冲区。
        @param ec设置为错误（如果发生）。

        实现的二进制格式在
        密码条件。见：

        https://tools.ietf.org/html/draft-thomas-crypto-conditions-02第7.3节
    **/

    static
    std::unique_ptr<Fulfillment>
    deserialize(
        Slice s,
        std::error_code& ec);

public:
    virtual ~Fulfillment() = default;

    /*返回履行的指纹：
    
        指纹是一个八进制字符串
        表示此履行的条件
        关于
        相同类型。
   **/

    virtual
    Buffer
    fingerprint() const = 0;

    /*返回此条件的类型。*/
    virtual
    Type
    type () const = 0;

    /*验证实现。*/
    virtual
    bool
    validate (Slice data) const = 0;

    /*计算与此履行关联的成本。*

        成本函数是确定性的，取决于
        条件和实现的类型和属性
        条件是从中生成的。
    **/

    virtual
    std::uint32_t
    cost() const = 0;

    /*返回与给定实现关联的条件。

        这个过程是完全确定的。所有实现
        如果符合，将为
        同样的成就。
    **/

    virtual
    Condition
    condition() const = 0;
};

inline
bool
operator== (Fulfillment const& lhs, Fulfillment const& rhs)
{
//fixme：对于复合条件，还需要检查子类型
    return
        lhs.type() == rhs.type() &&
            lhs.cost() == rhs.cost() &&
                lhs.fingerprint() == rhs.fingerprint();
}

inline
bool
operator!= (Fulfillment const& lhs, Fulfillment const& rhs)
{
    return !(lhs == rhs);
}

/*确定给定的满足和条件是否匹配*/
bool
match (
    Fulfillment const& f,
    Condition const& c);

/*验证给定的消息是否满足实现。

    履行的@param
    @param c条件
    @param m消息
    
    @注意消息与某些条件无关
          一个成就将成功地满足它
          任何给定消息的条件。
**/

bool
validate (
    Fulfillment const& f,
    Condition const& c,
    Slice m);

/*验证CryptoConditional触发器。

    CryptoConditional触发器是具有
    一条空消息。

    使用此类触发器时，建议
    触发器的类型为preimage、prefix或threshold。如果
    使用签名类型（即ED25519或RSA-SHA256）
    那么，ED25519或RSA密钥应该是一次性密钥。

    履行的@param
    @param c条件
**/

bool
validate (
    Fulfillment const& f,
    Condition const& c);

}
}

#endif
