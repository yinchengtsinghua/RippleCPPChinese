
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

#include <ripple/basics/contract.h>
#include <ripple/basics/ByteUtilities.h>
#include <ripple/crypto/csprng.h>
#include <openssl/rand.h>
#include <array>
#include <cassert>
#include <random>
#include <stdexcept>

namespace ripple {

void
csprng_engine::mix (
    void* data, std::size_t size, double bitsPerByte)
{
    assert (data != nullptr);
    assert (size != 0);
    assert (bitsPerByte != 0);

    std::lock_guard<std::mutex> lock (mutex_);
    RAND_add (data, size, (size * bitsPerByte) / 8.0);
}

csprng_engine::csprng_engine ()
{
    mix_entropy ();
}

csprng_engine::~csprng_engine ()
{
    RAND_cleanup ();
}

void
csprng_engine::mix_entropy (void* buffer, std::size_t count)
{
    std::array<std::random_device::result_type, 128> entropy;

    {
//在我们支持的每个平台上，std:：random_设备
//是不确定性的，应该提供一些好处
//质量熵。
        std::random_device rd;

        for (auto& e : entropy)
            e = rd();
    }

//假设系统熵为每字节2位：
    mix (
        entropy.data(),
        entropy.size() * sizeof(std::random_device::result_type),
        2.0);

//我们希望在估计方面非常保守
//用户提供给我们的缓冲区包含多少熵
//假设每个字节的熵只有0.5位：
    if (buffer != nullptr && count != 0)
        mix (buffer, count, 0.5);
}

csprng_engine::result_type
csprng_engine::operator()()
{
    result_type ret;

    std::lock_guard<std::mutex> lock (mutex_);

    auto const result = RAND_bytes (
        reinterpret_cast<unsigned char*>(&ret),
        sizeof(ret));

    if (result == 0)
        Throw<std::runtime_error> ("Insufficient entropy");

    return ret;
}

void
csprng_engine::operator()(void *ptr, std::size_t count)
{
    std::lock_guard<std::mutex> lock (mutex_);

    auto const result = RAND_bytes (
        reinterpret_cast<unsigned char*>(ptr),
        count);

    if (result != 1)
        Throw<std::runtime_error> ("Insufficient entropy");
}

csprng_engine& crypto_prng()
{
    static csprng_engine engine;
    return engine;
}

}
