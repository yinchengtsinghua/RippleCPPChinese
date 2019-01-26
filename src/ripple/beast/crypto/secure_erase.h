
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

#ifndef BEAST_CRYPTO_SECURE_ERASE_H_INCLUDED
#define BEAST_CRYPTO_SECURE_ERASE_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <new>

namespace beast {

namespace detail {

class secure_erase_impl
{
private:
    struct base
    {
        virtual void operator()(
            void* dest, std::size_t bytes) const = 0;
        virtual ~base() = default;
        base() = default;
        base(base const&) = delete;
        base& operator=(base const&) = delete;
    };

    struct impl : base
    {
        void operator()(
            void* dest, std::size_t bytes) const override
        {
            char volatile* volatile p =
                const_cast<volatile char*>(
                    reinterpret_cast<char*>(dest));
            if (bytes == 0)
                return;
            do
            {
                *p = 0;
            }
            while(*p++ == 0 && --bytes);
        }
    };

    char buf_[sizeof(impl)];
    base& erase_;

public:
    secure_erase_impl()
        : erase_(*new(buf_) impl)
    {
    }

    void operator()(
        void* dest, std::size_t bytes) const
    {
        return erase_(dest, bytes);
    }
};

}

/*保证用零填充内存*/
template <class = void>
void
secure_erase (void* dest, std::size_t bytes)
{
    static detail::secure_erase_impl const erase;
    erase(dest, bytes);
}

}

#endif
