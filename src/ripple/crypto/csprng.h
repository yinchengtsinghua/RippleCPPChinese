
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

#ifndef RIPPLE_CRYPTO_RANDOM_H_INCLUDED
#define RIPPLE_CRYPTO_RANDOM_H_INCLUDED

#include <mutex>
#include <string>
#include <type_traits>

namespace ripple {

/*密码安全随机数引擎

    引擎是线程安全的（它使用锁来序列化
    访问），并将自动混合一些随机性
    从std:：random_设备。

    满足统一随机数引擎的要求
**/

class csprng_engine
{
private:
    std::mutex mutex_;

    void
    mix (
        void* buffer,
        std::size_t count,
        double bitsPerByte);

public:
    using result_type = std::uint64_t;

    csprng_engine(csprng_engine const&) =  delete;
    csprng_engine& operator=(csprng_engine const&) = delete;

    csprng_engine(csprng_engine&&) = delete;
    csprng_engine& operator=(csprng_engine&&) = delete;

    csprng_engine ();
    ~csprng_engine ();

    /*混合熵到池中*/
    void
    mix_entropy (void* buffer = nullptr, std::size_t count = 0);

    /*生成随机整数*/
    result_type
    operator()();

    /*用请求的随机数据量填充缓冲区*/
    void
    operator()(void *ptr, std::size_t count);

    /*可返回的最小可能值*/
    static constexpr
    result_type
    min()
    {
        return std::numeric_limits<result_type>::min();
    }

    /*可返回的最大可能值*/
    static constexpr
    result_type
    max()
    {
        return std::numeric_limits<result_type>::max();
    }
};

/*默认的密码安全prng

    当需要生成随机数或
    将用于加密或传入的数据
    加密例程。

    这符合统一随机数引擎的要求
**/

csprng_engine& crypto_prng();

}

#endif
