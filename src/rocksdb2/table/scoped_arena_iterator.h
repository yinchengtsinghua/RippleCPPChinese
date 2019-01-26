
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
#pragma once

#include "table/internal_iterator.h"
#include "port/port.h"

namespace rocksdb {
class ScopedArenaIterator {

  void reset(InternalIterator* iter) ROCKSDB_NOEXCEPT {
    if (iter_ != nullptr) {
      iter_->~InternalIterator();
    }
    iter_ = iter;
  }

 public:

  explicit ScopedArenaIterator(InternalIterator* iter = nullptr)
      : iter_(iter) {}

  ScopedArenaIterator(const ScopedArenaIterator&) = delete;
  ScopedArenaIterator& operator=(const ScopedArenaIterator&) = delete;

  ScopedArenaIterator(ScopedArenaIterator&& o) ROCKSDB_NOEXCEPT {
    iter_ = o.iter_;
    o.iter_ = nullptr;
  }

  ScopedArenaIterator& operator=(ScopedArenaIterator&& o) ROCKSDB_NOEXCEPT {
    reset(o.iter_);
    o.iter_ = nullptr;
    return *this;
  }

  InternalIterator* operator->() { return iter_; }
  InternalIterator* get() { return iter_; }

  void set(InternalIterator* iter) { reset(iter); }

  InternalIterator* release() {
    assert(iter_ != nullptr);
    auto* res = iter_;
    iter_ = nullptr;
    return res;
  }

  ~ScopedArenaIterator() {
    reset(nullptr);
  }

 private:
  InternalIterator* iter_;
};
}  //命名空间rocksdb
