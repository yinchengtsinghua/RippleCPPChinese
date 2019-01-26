
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
    版权所有2014，howard hinnant<howard.hinnant@gmail.com>，
        vinnie falco<vinnie.falco@gmail.com

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

#ifndef BEAST_HASH_HASH_METRICS_H_INCLUDED
#define BEAST_HASH_HASH_METRICS_H_INCLUDED

#include <algorithm>
#include <cmath>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

namespace beast {
namespace hash_metrics {

//衡量容器哈希函数质量的指标

/*返回序列中重复项的分数。*/
template <class FwdIter>
float
collision_factor (FwdIter first, FwdIter last)
{
    std::set <typename FwdIter::value_type> s (first, last);
    return 1 - static_cast <float>(s.size()) / std::distance (first, last);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*返回序列与理想分布的偏差。*/
template <class FwdIter>
float
distribution_factor (FwdIter first, FwdIter last)
{
    using value_type = typename FwdIter::value_type;
    static_assert (std::is_unsigned <value_type>::value, "");

    const unsigned nbits = CHAR_BIT * sizeof(std::size_t);
    const unsigned rows = nbits / 4;
    unsigned counts[rows][16] = {};
    std::for_each (first, last, [&](typename FwdIter::value_type h)
    {
        std::size_t mask = 0xF;
        for (unsigned i = 0; i < rows; ++i, mask <<= 4)
            counts[i][(h & mask) >> 4*i] += 1;
    });
    float mean_rows[rows] = {0};
    float mean_cols[16] = {0};
    for (unsigned i = 0; i < rows; ++i)
    {
        for (unsigned j = 0; j < 16; ++j)
        {
            mean_rows[i] += counts[i][j];
            mean_cols[j] += counts[i][j];
        }
    }
    for (unsigned i = 0; i < rows; ++i)
        mean_rows[i] /= 16;
    for (unsigned j = 0; j < 16; ++j)
        mean_cols[j] /= rows;
    std::pair<float, float> dev[rows][16];
    for (unsigned i = 0; i < rows; ++i)
    {
        for (unsigned j = 0; j < 16; ++j)
        {
            dev[i][j].first = std::abs(counts[i][j] - mean_rows[i]) / mean_rows[i];
            dev[i][j].second = std::abs(counts[i][j] - mean_cols[j]) / mean_cols[j];
        }
    }
    float max_err = 0;
    for (unsigned i = 0; i < rows; ++i)
    {
        for (unsigned j = 0; j < 16; ++j)
        {
            if (max_err < dev[i][j].first)
                max_err = dev[i][j].first;
            if (max_err < dev[i][j].second)
                max_err = dev[i][j].second;
        }
    }
    return max_err;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace detail {

template <class T>
inline
T
sqr(T t)
{
    return t*t;
}

double
score (int const* bins, std::size_t const bincount, double const k)
{
    double const n = bincount;
//计算rms^2值
    double rms_sq = 0;
    for(std::size_t i = 0; i < bincount; ++i)
        rms_sq += sqr(bins[i]);;
    rms_sq /= n;
//计算填充因子
    double const f = (sqr(k) - 1) / (n*rms_sq - k);
//重新缩放到（0,1），0=好，1=坏
    return 1 - (f / n);
}

template <class T>
std::uint32_t
window (T* blob, int start, int count )
{
    std::size_t const len = sizeof(T);
    static_assert((len & 3) == 0, "");
    if(count == 0)
        return 0;
    int const nbits = len * CHAR_BIT;
    start %= nbits;
    int ndwords = len / 4;
    std::uint32_t const* k = static_cast <
        std::uint32_t const*>(static_cast<void const*>(blob));
    int c = start & (32-1);
    int d = start / 32;
    if(c == 0)
        return (k[d] & ((1 << count) - 1));
    int ia = (d + 1) % ndwords;
    int ib = (d + 0) % ndwords;
    std::uint32_t a = k[ia];
    std::uint32_t b = k[ib];
    std::uint32_t t = (a << (32-c)) | (b >> c);
    t &= ((1 << count)-1);
    return t;
}

} //细节

/*使用容器计算窗口度量。
    TODO需要参考（smhasher？）
**/

template <class FwdIter>
double
windowed_score (FwdIter first, FwdIter last)
{
    auto const size (std::distance (first, last));
    int maxwidth = 20;
//为了可靠地测试分布偏差，每个箱子至少需要5把钥匙。
//降到1%，所以不要费心测试比这更稀疏的分布。
    while (static_cast<double>(size) / (1ull << maxwidth) < 5.0)
        maxwidth--;
    double worst = 0;
    std::vector <int> bins (1ull << maxwidth);
    int const hashbits = sizeof(std::size_t) * CHAR_BIT;
    for (int start = 0; start < hashbits; ++start)
    {
        int width = maxwidth;
        bins.assign (1ull << width, 0);
        for (auto iter (first); iter != last; ++iter)
            ++bins[detail::window(&*iter, start, width)];
//测试分布，然后将箱子对折，
//重复一次，直到达到256个箱子
        while (bins.size() >= 256)
        {
            double score (detail::score (
                bins.data(), bins.size(), size));
            worst = std::max(score, worst);
            if (--width < 8)
                break;
            for (std::size_t i = 0, j = bins.size() / 2; j < bins.size(); ++i, ++j)
                bins[i] += bins[j];
            bins.resize(bins.size() / 2);
        }
    }
    return worst;
}

} //哈希度量
} //野兽

#endif
