
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//
/*
  来自http://sites.google.com/site/humbolhash/

  所有代码都发布到公共域。出于商业目的，杂音杂音
  是麻省理工学院的执照。
**/

#include "murmurhash.h"

#if defined(__x86_64__)

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//与32位杂音相同的警告2适用于这里-注意对齐
//如果跨多个平台使用，则会出现端点问题。
//
//64位平台的64位哈希

uint64_t MurmurHash64A ( const void * key, int len, unsigned int seed )
{
    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t * data = (const uint64_t *)key;
    const uint64_t * end = data + (len/8);

    while(data != end)
    {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char * data2 = (const unsigned char*)data;

    switch(len & 7)
    {
case 7: h ^= ((uint64_t)data2[6]) << 48; //坠落
case 6: h ^= ((uint64_t)data2[5]) << 40; //坠落
case 5: h ^= ((uint64_t)data2[4]) << 32; //坠落
case 4: h ^= ((uint64_t)data2[3]) << 24; //坠落
case 3: h ^= ((uint64_t)data2[2]) << 16; //坠落
case 2: h ^= ((uint64_t)data2[1]) << 8; //坠落
    case 1: h ^= ((uint64_t)data2[0]);
        h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

#elif defined(__i386__)

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//注意-此代码对机器的行为进行了一些假设-
//
//1。我们可以从任何地址读取一个4字节的值，而不会崩溃。
//2。sizeof（int）==4
//
//它有一些局限性-
//
//1。它不会递增地工作。
//2。它不会在小端数和大端数上产生相同的结果
//机器。

unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed )
{
//“m”和“r”是脱机生成的混合常量。
//他们不是真正的“魔法”，他们只是碰巧工作得很好。

    const unsigned int m = 0x5bd1e995;
    const int r = 24;

//将哈希初始化为“随机”值

    unsigned int h = seed ^ len;

//在哈希中一次混合4个字节

    const unsigned char * data = (const unsigned char *)key;

    while(len >= 4)
    {
        unsigned int k = *(unsigned int *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

//处理输入数组的最后几个字节

    switch(len)
    {
case 3: h ^= data[2] << 16; //坠落
case 2: h ^= data[1] << 8; //坠落
    case 1: h ^= data[0];
        h *= m;
    };

//做一些哈希的最后混合以确保最后几个
//字节合并得很好。

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

#else

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//与杂音hash2相同，但尾音和排列中性。
//不过，一半的速度，唉。

unsigned int MurmurHashNeutral2 ( const void * key, int len, unsigned int seed )
{
    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    unsigned int h = seed ^ len;

    const unsigned char * data = (const unsigned char *)key;

    while(len >= 4)
    {
        unsigned int k;

        k  = data[0];
        k |= data[1] << 8;
        k |= data[2] << 16;
        k |= data[3] << 24;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch(len)
    {
case 3: h ^= data[2] << 16; //坠落
case 2: h ^= data[1] << 8; //坠落
    case 1: h ^= data[0];
        h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

#endif
