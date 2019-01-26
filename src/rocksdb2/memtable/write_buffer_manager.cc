
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

#include "rocksdb/write_buffer_manager.h"
#include <mutex>
#include "util/coding.h"

namespace rocksdb {
#ifndef ROCKSDB_LITE
namespace {
const size_t kSizeDummyEntry = 1024 * 1024;
//密钥将比sst文件中块的密钥长，因此它们不会
//冲突。
const size_t kCacheKeyPrefix = kMaxVarint64Length * 4 + 1;
}  //命名空间

struct WriteBufferManager::CacheRep {
  std::shared_ptr<Cache> cache_;
  std::mutex cache_mutex_;
  std::atomic<size_t> cache_allocated_size_;
//非前缀部分将根据要使用的ID进行更新。
  char cache_key_[kCacheKeyPrefix + kMaxVarint64Length];
  uint64_t next_cache_key_id_ = 0;
  std::vector<Cache::Handle*> dummy_handles_;

  explicit CacheRep(std::shared_ptr<Cache> cache)
      : cache_(cache), cache_allocated_size_(0) {
    memset(cache_key_, 0, kCacheKeyPrefix);
    size_t pointer_size = sizeof(const void*);
    assert(pointer_size <= kCacheKeyPrefix);
    memcpy(cache_key_, static_cast<const void*>(this), pointer_size);
  }

  Slice GetNextCacheKey() {
    memset(cache_key_ + kCacheKeyPrefix, 0, kMaxVarint64Length);
    char* end =
        EncodeVarint64(cache_key_ + kCacheKeyPrefix, next_cache_key_id_++);
    return Slice(cache_key_, static_cast<size_t>(end - cache_key_));
  }
};
#else
struct WriteBufferManager::CacheRep {};
#endif  //摇滚乐

WriteBufferManager::WriteBufferManager(size_t _buffer_size,
                                       std::shared_ptr<Cache> cache)
    : buffer_size_(_buffer_size),
      mutable_limit_(buffer_size_ * 7 / 8),
      memory_used_(0),
      memory_active_(0),
      cache_rep_(nullptr) {
#ifndef ROCKSDB_LITE
  if (cache) {
//使用指向此的指针构造缓存键。
    cache_rep_.reset(new CacheRep(cache));
  }
#endif  //摇滚乐
}

WriteBufferManager::~WriteBufferManager() {
#ifndef ROCKSDB_LITE
  if (cache_rep_) {
    for (auto* handle : cache_rep_->dummy_handles_) {
      cache_rep_->cache_->Release(handle, true);
    }
  }
#endif  //摇滚乐
}

//只能从写入线程调用
void WriteBufferManager::ReserveMemWithCache(size_t mem) {
#ifndef ROCKSDB_LITE
  assert(cache_rep_ != nullptr);
//使用互斥体保护各种数据结构。可以优化为
//如果最终出现性能瓶颈，则无锁解决方案。
  std::lock_guard<std::mutex> lock(cache_rep_->cache_mutex_);

  size_t new_mem_used = memory_used_.load(std::memory_order_relaxed) + mem;
  memory_used_.store(new_mem_used, std::memory_order_relaxed);
  while (new_mem_used > cache_rep_->cache_allocated_size_) {
//将大小至少扩展1 MB。
//向缓存中添加虚拟记录
    Cache::Handle* handle;
    cache_rep_->cache_->Insert(cache_rep_->GetNextCacheKey(), nullptr,
                               kSizeDummyEntry, nullptr, &handle);
    cache_rep_->dummy_handles_.push_back(handle);
    cache_rep_->cache_allocated_size_ += kSizeDummyEntry;
  }
#endif  //摇滚乐
}

void WriteBufferManager::FreeMemWithCache(size_t mem) {
#ifndef ROCKSDB_LITE
  assert(cache_rep_ != nullptr);
//使用互斥体保护各种数据结构。可以优化为
//如果最终出现性能瓶颈，则无锁解决方案。
  std::lock_guard<std::mutex> lock(cache_rep_->cache_mutex_);
  size_t new_mem_used = memory_used_.load(std::memory_order_relaxed) - mem;
  memory_used_.store(new_mem_used, std::memory_order_relaxed);
//如果实际
//使用率低于我们从块缓存中保留的值的3/4。
//我们这样做是因为：
//1。我们不需要立即支付块缓存的费用。
//释放，因为块缓存插入很昂贵；
//2。最后，如果我们放弃临时的内存表大小增加，
//我们确保随着时间的推移压缩块缓存中的内存开销。
//这样，即使有足够的内存，我们也只会明显地缩减成本。
//保证金。
  if (new_mem_used < cache_rep_->cache_allocated_size_ / 4 * 3 &&
      cache_rep_->cache_allocated_size_ - kSizeDummyEntry > new_mem_used) {
    assert(!cache_rep_->dummy_handles_.empty());
    cache_rep_->cache_->Release(cache_rep_->dummy_handles_.back(), true);
    cache_rep_->dummy_handles_.pop_back();
    cache_rep_->cache_allocated_size_ -= kSizeDummyEntry;
  }
#endif  //摇滚乐
}
}  //命名空间rocksdb
