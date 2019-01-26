
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

#pragma once

#include <algorithm>
#include <array>
#include <utility>

#include "util/autovector.h"

namespace rocksdb {

//这类似于std:：unordered_映射，只是它试图避免
//尽可能多地分配或释放内存。用
//std：：无序映射，每次插入都进行分配/释放
//或者由于迭代器保持有效的要求而删除
//插入或删除。这意味着散列链将
//作为链接列表实现。
//
//这个实现使用autovector作为散列链的表头。
//
template <typename K, typename V, size_t size = 128>
class HashMap {
  std::array<autovector<std::pair<K, V>, 1>, size> table_;

 public:
  bool Contains(K key) {
    auto& bucket = table_[key % size];
    auto it = std::find_if(
        bucket.begin(), bucket.end(),
        [key](const std::pair<K, V>& p) { return p.first == key; });
    return it != bucket.end();
  }

  void Insert(K key, V value) {
    auto& bucket = table_[key % size];
    bucket.push_back({key, value});
  }

  void Delete(K key) {
    auto& bucket = table_[key % size];
    auto it = std::find_if(
        bucket.begin(), bucket.end(),
        [key](const std::pair<K, V>& p) { return p.first == key; });
    if (it != bucket.end()) {
      auto last = bucket.end() - 1;
      if (it != last) {
        *it = *last;
      }
      bucket.pop_back();
    }
  }

  V& Get(K key) {
    auto& bucket = table_[key % size];
    auto it = std::find_if(
        bucket.begin(), bucket.end(),
        [key](const std::pair<K, V>& p) { return p.first == key; });
    return it->second;
  }
};

}  //命名空间rocksdb
