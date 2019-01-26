
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2017至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#pragma once

#include <cstddef>
#include <thread>
#include <utility>
#include <vector>

#include "port/likely.h"
#include "port/port.h"
#include "util/random.h"

namespace rocksdb {

//核心局部值的数组。理想情况下，值类型t与缓存对齐
//防止错误共享。
template <typename T>
class CoreLocalArray {
 public:
  CoreLocalArray();

  size_t Size() const;
//返回指向与线程的核心对应的元素的指针。
//当前运行于。
  T* Access() const;
//与上面相同，但也返回核心索引，客户机可以缓存该索引
//以减少需要检索核心ID的频率。只有在某些情况下才这样做
//不准确是可以容忍的，因为线程可能迁移到不同的核心。
  std::pair<T*, size_t> AccessElementAndIndex() const;
//返回指向指定核心索引的元素的指针。可以使用，
//例如，对于聚合，或者客户机缓存核心索引。
  T* AccessAtCore(size_t core_idx) const;

 private:
  std::unique_ptr<T[]> data_;
  int size_shift_;
};

template <typename T>
CoreLocalArray<T>::CoreLocalArray() {
  int num_cpus = static_cast<int>(std::thread::hardware_concurrency());
//找到两个>=num_CPU和>=8的幂
  size_shift_ = 3;
  while (1 << size_shift_ < num_cpus) {
    ++size_shift_;
  }
  data_.reset(new T[static_cast<size_t>(1) << size_shift_]);
}

template <typename T>
size_t CoreLocalArray<T>::Size() const {
  return static_cast<size_t>(1) << size_shift_;
}

template <typename T>
T* CoreLocalArray<T>::Access() const {
  return AccessElementAndIndex().first;
}

template <typename T>
std::pair<T*, size_t> CoreLocalArray<T>::AccessElementAndIndex() const {
  int cpuid = port::PhysicalCoreID();
  size_t core_idx;
  if (UNLIKELY(cpuid < 0)) {
//CPU ID不可用，只需随机选择
    core_idx = Random::GetTLSInstance()->Uniform(1 << size_shift_);
  } else {
    core_idx = static_cast<size_t>(cpuid & ((1 << size_shift_) - 1));
  }
  return {AccessAtCore(core_idx), core_idx};
}

template <typename T>
T* CoreLocalArray<T>::AccessAtCore(size_t core_idx) const {
  assert(core_idx < static_cast<size_t>(1) << size_shift_);
  return &data_[core_idx];
}

}  //命名空间rocksdb
