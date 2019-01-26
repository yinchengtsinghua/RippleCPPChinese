
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//用于内部数据结构的简单哈希函数

#pragma once
#include <stddef.h>
#include <stdint.h>

#include "rocksdb/slice.h"

namespace rocksdb {

extern uint32_t Hash(const char* data, size_t n, uint32_t seed);

inline uint32_t BloomHash(const Slice& key) {
  return Hash(key.data(), key.size(), 0xbc9f1d34);
}

inline uint32_t GetSliceHash(const Slice& s) {
  return Hash(s.data(), s.size(), 397);
}

//std：：哈希兼容接口。
struct SliceHasher {
  uint32_t operator()(const Slice& s) const { return GetSliceHash(s); }
};

}  //命名空间rocksdb
