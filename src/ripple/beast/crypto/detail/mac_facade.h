
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_CRYPTO_MAC_FACADE_H_INCLUDED
#define BEAST_CRYPTO_MAC_FACADE_H_INCLUDED

#include <ripple/beast/crypto/secure_erase.h>
#include <ripple/beast/hash/endian.h>
#include <type_traits>
#include <array>

namespace beast {
namespace detail {

//消息验证代码（MAC）外观
template <class Context, bool Secure>
class mac_facade
{
private:
    Context ctx_;

public:
    static beast::endian const endian =
        beast::endian::native;

    static std::size_t const digest_size =
        Context::digest_size;

    using result_type =
        std::array<std::uint8_t, digest_size>;

    mac_facade() noexcept
    {
        init(ctx_);
    }

    ~mac_facade()
    {
        erase(std::integral_constant<
            bool, Secure>{});
    }

    void
    operator()(void const* data,
        std::size_t size) noexcept
    {
        update(ctx_, data, size);
    }

    explicit
    operator result_type() noexcept
    {
        result_type digest;
        finish(ctx_, &digest[0]);
        return digest;
    }

private:
    inline
    void
    erase (std::false_type) noexcept
    {
    }

    inline
    void
    erase (std::true_type) noexcept
    {
        secure_erase(&ctx_, sizeof(ctx_));
    }
};

} //细节
} //野兽

#endif
