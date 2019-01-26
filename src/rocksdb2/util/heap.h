
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

#include <algorithm>
#include <cstdint>
#include <functional>
#include "port/port.h"
#include "util/autovector.h"

namespace rocksdb {

//二进制堆实现已优化，可用于多方式合并排序。
//与std:：priority_队列的比较：
//-在libstdc++中，std:：priority_queue:：pop（）通常只执行超过logn的操作。
//比较，但决不会少。
//-std:：priority_队列没有replace top操作，需要
//POP+推送。如果更换元件是新的顶部，则需要
//2登录比较。
//-此堆的pop（）使用了一个“schoolbook”downseap，它最多需要~2logn
//比较。
//-此堆提供了一个replace_top（）操作，该操作需要[1，2logn]
//比较。当更换元件也是新的顶部时，此
//只需要1或2个比较。
//
//最后一个属性可以产生一个数量级的性能改进
//合并时对现实世界中的非随机数据进行排序。如果合并操作是
//可能从同一个输入流中获取大量元素，只有1个
//需要对每个元素进行比较。在Rocksdb Land，当
//压缩一个不在l0中随机分布键的数据库
//文件，但附近的键可能在同一个l0文件中。
//
//容器使用与
//std:：priority_queue:比较运算符应提供
//小于关系，但top（）将返回最大值。

template<typename T, typename Compare = std::less<T>>
class BinaryHeap {
 public:
  BinaryHeap() { }
  explicit BinaryHeap(Compare cmp) : cmp_(std::move(cmp)) { }

  void push(const T& value) {
    data_.push_back(value);
    upheap(data_.size() - 1);
  }

  void push(T&& value) {
    data_.push_back(std::move(value));
    upheap(data_.size() - 1);
  }

  const T& top() const {
    assert(!empty());
    return data_.front();
  }

  void replace_top(const T& value) {
    assert(!empty());
    data_.front() = value;
    downheap(get_root());
  }

  void replace_top(T&& value) {
    assert(!empty());
    data_.front() = std::move(value);
    downheap(get_root());
  }

  void pop() {
    assert(!empty());
    data_.front() = std::move(data_.back());
    data_.pop_back();
    if (!empty()) {
      downheap(get_root());
    } else {
      reset_root_cmp_cache();
    }
  }

  void swap(BinaryHeap &other) {
    std::swap(cmp_, other.cmp_);
    data_.swap(other.data_);
    std::swap(root_cmp_cache_, other.root_cmp_cache_);
  }

  void clear() {
    data_.clear();
    reset_root_cmp_cache();
  }

  bool empty() const {
    return data_.empty();
  }

  void reset_root_cmp_cache() { root_cmp_cache_ = port::kMaxSizet; }

 private:
  static inline size_t get_root() { return 0; }
  static inline size_t get_parent(size_t index) { return (index - 1) / 2; }
  static inline size_t get_left(size_t index) { return 2 * index + 1; }
  static inline size_t get_right(size_t index) { return 2 * index + 2; }

  void upheap(size_t index) {
    T v = std::move(data_[index]);
    while (index > get_root()) {
      const size_t parent = get_parent(index);
      if (!cmp_(data_[parent], v)) {
        break;
      }
      data_[index] = std::move(data_[parent]);
      index = parent;
    }
    data_[index] = std::move(v);
    reset_root_cmp_cache();
  }

  void downheap(size_t index) {
    T v = std::move(data_[index]);

    size_t picked_child = port::kMaxSizet;
    while (1) {
      const size_t left_child = get_left(index);
      if (get_left(index) >= data_.size()) {
        break;
      }
      const size_t right_child = left_child + 1;
      assert(right_child == get_right(index));
      picked_child = left_child;
      if (index == 0 && root_cmp_cache_ < data_.size()) {
        picked_child = root_cmp_cache_;
      } else if (right_child < data_.size() &&
                 cmp_(data_[left_child], data_[right_child])) {
        picked_child = right_child;
      }
      if (!cmp_(v, data_[picked_child])) {
        break;
      }
      data_[index] = std::move(data_[picked_child]);
      index = picked_child;
    }

    if (index == 0) {
//除了值之外，我们没有更改树中的任何内容
//根节点的左右子节点没有改变，我们可以
//缓存“picked_child”是最小的子级
//所以下次我们直接比较一下
      root_cmp_cache_ = picked_child;
    } else {
//树已更改，重置缓存
      reset_root_cmp_cache();
    }

    data_[index] = std::move(v);
  }

  Compare cmp_;
  autovector<T> data_;
//用于减少downseap（）中的cmp_u调用数
  size_t root_cmp_cache_ = port::kMaxSizet;
};

}  //命名空间rocksdb
