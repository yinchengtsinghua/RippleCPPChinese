
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

#pragma once
#include <stdint.h>
#include "rocksdb/slice.h"

#if defined(__x86_64__)
#define MURMUR_HASH MurmurHash64A
uint64_t MurmurHash64A ( const void * key, int len, unsigned int seed );
#define MurmurHash MurmurHash64A
typedef uint64_t murmur_t;

#elif defined(__i386__)
#define MURMUR_HASH MurmurHash2
unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed );
#define MurmurHash MurmurHash2
typedef unsigned int murmur_t;

#else
#define MURMUR_HASH MurmurHashNeutral2
unsigned int MurmurHashNeutral2 ( const void * key, int len, unsigned int seed );
#define MurmurHash MurmurHashNeutral2
typedef unsigned int murmur_t;
#endif

//允许通过杂音散列对切片进行散列。
namespace rocksdb {
struct murmur_hash {
  size_t operator()(const Slice& slice) const {
    return MurmurHash(slice.data(), static_cast<int>(slice.size()), 0);
  }
};
}  //罗克斯德
