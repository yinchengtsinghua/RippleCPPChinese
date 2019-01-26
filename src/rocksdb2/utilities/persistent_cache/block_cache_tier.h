
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2013，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
#pragma once

#ifndef ROCKSDB_LITE

#ifndef  OS_WIN
#include <unistd.h>
#endif //！奥斯温

#include <atomic>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "rocksdb/cache.h"
#include "rocksdb/comparator.h"
#include "rocksdb/persistent_cache.h"

#include "utilities/persistent_cache/block_cache_tier_file.h"
#include "utilities/persistent_cache/block_cache_tier_metadata.h"
#include "utilities/persistent_cache/persistent_cache_util.h"

#include "memtable/skiplist.h"
#include "monitoring/histogram.h"
#include "port/port.h"
#include "util/arena.h"
#include "util/coding.h"
#include "util/crc32c.h"
#include "util/mutexlock.h"

namespace rocksdb {

//
//块缓存层实现
//
class BlockCacheTier : public PersistentCacheTier {
 public:
  explicit BlockCacheTier(const PersistentCacheConfig& opt)
      : opt_(opt),
        insert_ops_(opt_.max_write_pipeline_backlog_size),
        buffer_allocator_(opt.write_buffer_size, opt.write_buffer_count()),
        writer_(this, opt_.writer_qdepth, opt_.writer_dispatch_size) {
    Info(opt_.log, "Initializing allocator. size=%d B count=%d",
         opt_.write_buffer_size, opt_.write_buffer_count());
  }

  virtual ~BlockCacheTier() {
//close是可重入的，因此我们可以调用close，即使它已经关闭
    Close();
    assert(!insert_th_.joinable());
  }

  Status Insert(const Slice& key, const char* data, const size_t size) override;
  Status Lookup(const Slice& key, std::unique_ptr<char[]>* data,
                size_t* size) override;
  Status Open() override;
  Status Close() override;
  bool Erase(const Slice& key) override;
  bool Reserve(const size_t size) override;

  bool IsCompressed() override { return opt_.is_compressed; }

  std::string GetPrintableOptions() const override { return opt_.ToString(); }

  PersistentCache::StatsType Stats() override;

  void TEST_Flush() override {
    while (insert_ops_.Size()) {
      /*睡眠覆盖*/
      Env::Default()->SleepForMicroseconds(1000000);
    }
  }

 private:
//缓存满时要收回的缓存百分比
  static const size_t kEvictPct = 10;
//在管道模式下，最大尝试插入键、值到缓存
  static const size_t kMaxRetry = 3;

//管道作业
  struct InsertOp {
    explicit InsertOp(const bool signal) : signal_(signal) {}
    explicit InsertOp(std::string&& key, const std::string& data)
        : key_(std::move(key)), data_(data) {}
    ~InsertOp() {}

    InsertOp() = delete;
    InsertOp(InsertOp&& rhs) = default;
    InsertOp& operator=(InsertOp&& rhs) = default;

//用于按有界队列估计大小
    size_t Size() { return data_.size() + key_.size(); }

    std::string key_;
    std::string data_;
const bool signal_ = false;  //请求处理线程退出的信号
  };

//插入螺纹的入口点
  void InsertMain();
//插入实现
  Status InsertImpl(const Slice& key, const Slice& data);
//创建新的缓存文件
  Status NewCacheFile();
//获取缓存目录路径
  std::string GetCachePath() const { return opt_.path + "/cache"; }
//清除文件夹
  Status CleanupCacheFolder(const std::string& folder);

//统计
  struct Statistics {
    HistogramImpl bytes_pipelined_;
    HistogramImpl bytes_written_;
    HistogramImpl bytes_read_;
    HistogramImpl read_hit_latency_;
    HistogramImpl read_miss_latency_;
    HistogramImpl write_latency_;
    std::atomic<uint64_t> cache_hits_{0};
    std::atomic<uint64_t> cache_misses_{0};
    std::atomic<uint64_t> cache_errors_{0};
    std::atomic<uint64_t> insert_dropped_{0};

    double CacheHitPct() const {
      const auto lookups = cache_hits_ + cache_misses_;
      return lookups ? 100 * cache_hits_ / static_cast<double>(lookups) : 0.0;
    }

    double CacheMissPct() const {
      const auto lookups = cache_hits_ + cache_misses_;
      return lookups ? 100 * cache_misses_ / static_cast<double>(lookups) : 0.0;
    }
  };

port::RWMutex lock_;                          //同步
const PersistentCacheConfig opt_;             //块缓存选项
BoundedQueue<InsertOp> insert_ops_;           //等待插入的操作
rocksdb::port::Thread insert_th_;                       //插入螺纹
uint32_t writer_cache_id_ = 0;                //当前缓存文件标识符
WriteableCacheFile* cache_file_ = nullptr;    //当前缓存文件引用
CacheWriteBufferAllocator buffer_allocator_;  //缓冲提供者
ThreadedWriter writer_;                       //写入线程
BlockCacheTierMetadata metadata_;             //缓存元数据管理器
std::atomic<uint64_t> size_{0};               //缓存大小
Statistics stats_;                                 //统计
};

}  //命名空间rocksdb

#endif
