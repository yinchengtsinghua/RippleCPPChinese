
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
//WriteBufferManager用于管理一个或多个内存分配
//MemTables。

#pragma once

#include <atomic>
#include <cstddef>
#include "rocksdb/cache.h"

namespace rocksdb {

class WriteBufferManager {
 public:
//_buffer_size=0表示没有限制。记忆不会被封住。
//内存使用（）将无效，shouldFlush（）将始终返回true。
//如果提供了“cache”，我们将在cache和cost中放置虚拟条目
//分配给缓存的内存。即使缓冲区大小=0，也可以使用。
  explicit WriteBufferManager(size_t _buffer_size,
                              std::shared_ptr<Cache> cache = {});
  ~WriteBufferManager();

  bool enabled() const { return buffer_size_ != 0; }

//仅在启用（）时有效
  size_t memory_usage() const {
    return memory_used_.load(std::memory_order_relaxed);
  }
  size_t mutable_memtable_memory_usage() const {
    return memory_active_.load(std::memory_order_relaxed);
  }
  size_t buffer_size() const { return buffer_size_; }

//只能从写入线程调用
  bool ShouldFlush() const {
    if (enabled()) {
      if (mutable_memtable_memory_usage() > mutable_limit_) {
        return true;
      }
      if (memory_usage() >= buffer_size_ &&
          mutable_memtable_memory_usage() >= buffer_size_ / 2) {
//如果内存超过了缓冲区的大小，我们会触发更大的攻击性
//冲洗。但是如果已经有超过一半的内存被刷新，
//触发更多的刷新可能没有帮助。我们还是拿着吧。
        return true;
      }
    }
    return false;
  }

  void ReserveMem(size_t mem) {
    if (cache_rep_ != nullptr) {
      ReserveMemWithCache(mem);
    } else if (enabled()) {
      memory_used_.fetch_add(mem, std::memory_order_relaxed);
    }
    if (enabled()) {
      memory_active_.fetch_add(mem, std::memory_order_relaxed);
    }
  }
//我们正在释放'mem'字节，因此不考虑
//检查软极限时。
  void ScheduleFreeMem(size_t mem) {
    if (enabled()) {
      memory_active_.fetch_sub(mem, std::memory_order_relaxed);
    }
  }
  void FreeMem(size_t mem) {
    if (cache_rep_ != nullptr) {
      FreeMemWithCache(mem);
    } else if (enabled()) {
      memory_used_.fetch_sub(mem, std::memory_order_relaxed);
    }
  }

 private:
  const size_t buffer_size_;
  const size_t mutable_limit_;
  std::atomic<size_t> memory_used_;
//尚未计划释放的内存。
  std::atomic<size_t> memory_active_;
  struct CacheRep;
  std::unique_ptr<CacheRep> cache_rep_;

  void ReserveMemWithCache(size_t mem);
  void FreeMemWithCache(size_t mem);

//不允许复制
  WriteBufferManager(const WriteBufferManager&) = delete;
  WriteBufferManager& operator=(const WriteBufferManager&) = delete;
};
}  //命名空间rocksdb
