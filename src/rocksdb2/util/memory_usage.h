
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

#pragma once

#include <unordered_map>

namespace rocksdb {

//通过标准容器估计内存使用量的助手方法。

template <class Key, class Value, class Hash>
size_t ApproximateMemoryUsage(
    const std::unordered_map<Key, Value, Hash>& umap) {
  typedef std::unordered_map<Key, Value, Hash> Map;
  return sizeof(umap) +
//所有项目的大小加上每个项目的下一个指针。
         (sizeof(typename Map::value_type) + sizeof(void*)) * umap.size() +
//哈希桶的大小。
         umap.bucket_count() * sizeof(void*);
}

}  //命名空间rocksdb
