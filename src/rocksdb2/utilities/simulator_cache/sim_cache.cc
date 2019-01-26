
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

#include "rocksdb/utilities/sim_cache.h"
#include <atomic>
#include "monitoring/statistics.h"
#include "port/port.h"
#include "rocksdb/env.h"
#include "util/file_reader_writer.h"
#include "util/mutexlock.h"
#include "util/string_util.h"

namespace rocksdb {

namespace {

class CacheActivityLogger {
 public:
  CacheActivityLogger()
      : activity_logging_enabled_(false), max_logging_size_(0) {}

  ~CacheActivityLogger() {
    MutexLock l(&mutex_);

    StopLoggingInternal();
  }

  Status StartLogging(const std::string& activity_log_file, Env* env,
                      uint64_t max_logging_size = 0) {
    assert(activity_log_file != "");
    assert(env != nullptr);

    Status status;
    EnvOptions env_opts;
    std::unique_ptr<WritableFile> log_file;

    MutexLock l(&mutex_);

//停止现有日志记录（如果有）
    StopLoggingInternal();

//打开日志文件
    status = env->NewWritableFile(activity_log_file, &log_file, env_opts);
    if (!status.ok()) {
      return status;
    }
    file_writer_.reset(new WritableFileWriter(std::move(log_file), env_opts));

    max_logging_size_ = max_logging_size;
    activity_logging_enabled_.store(true);

    return status;
  }

  void StopLogging() {
    MutexLock l(&mutex_);

    StopLoggingInternal();
  }

  void ReportLookup(const Slice& key) {
    if (activity_logging_enabled_.load() == false) {
      return;
    }

    std::string log_line = "LOOKUP - " + key.ToString(true) + "\n";

//行格式：“lookup-<key>”
    MutexLock l(&mutex_);
    Status s = file_writer_->Append(log_line);
    if (!s.ok() && bg_status_.ok()) {
      bg_status_ = s;
    }
    if (MaxLoggingSizeReached() || !bg_status_.ok()) {
//如果已达到最大文件大小或
//遇到错误
      StopLoggingInternal();
    }
  }

  void ReportAdd(const Slice& key, size_t size) {
    if (activity_logging_enabled_.load() == false) {
      return;
    }

    std::string log_line = "ADD - ";
    log_line += key.ToString(true);
    log_line += " - ";
    AppendNumberTo(&log_line, size);
		log_line += "\n";

//行格式：“添加-<key>-<key-size>”
    MutexLock l(&mutex_);
    Status s = file_writer_->Append(log_line);
    if (!s.ok() && bg_status_.ok()) {
      bg_status_ = s;
    }

    if (MaxLoggingSizeReached() || !bg_status_.ok()) {
//如果已达到最大文件大小或
//遇到错误
      StopLoggingInternal();
    }
  }

  Status& bg_status() {
    MutexLock l(&mutex_);
    return bg_status_;
  }

 private:
  bool MaxLoggingSizeReached() {
    mutex_.AssertHeld();

    return (max_logging_size_ > 0 &&
            file_writer_->GetFileSize() >= max_logging_size_);
  }

  void StopLoggingInternal() {
    mutex_.AssertHeld();

    if (!activity_logging_enabled_) {
      return;
    }

    activity_logging_enabled_.store(false);
    Status s = file_writer_->Close();
    if (!s.ok() && bg_status_.ok()) {
      bg_status_ = s;
    }
  }

//用于同步写入文件写入程序的互斥体，以及以下所有操作
//类数据成员
  port::Mutex mutex_;
//指示当前是否启用日志记录
//允许不使用互斥体进行读取的原子
  std::atomic<bool> activity_logging_enabled_;
//到达后，我们将停止日志记录并关闭文件
//值0表示无限制
  uint64_t max_logging_size_;
  std::unique_ptr<WritableFileWriter> file_writer_;
  Status bg_status_;
};

//SimcacheImpl定义
class SimCacheImpl : public SimCache {
 public:
//实际缓存容量（sharedlrucache）
//仅密钥缓存的测试容量
  SimCacheImpl(std::shared_ptr<Cache> cache, size_t sim_capacity,
               int num_shard_bits)
      : cache_(cache),
        key_only_cache_(NewLRUCache(sim_capacity, num_shard_bits)),
        miss_times_(0),
        hit_times_(0) {}

  virtual ~SimCacheImpl() {}
  virtual void SetCapacity(size_t capacity) override {
    cache_->SetCapacity(capacity);
  }

  virtual void SetStrictCapacityLimit(bool strict_capacity_limit) override {
    cache_->SetStrictCapacityLimit(strict_capacity_limit);
  }

  virtual Status Insert(const Slice& key, void* value, size_t charge,
                        void (*deleter)(const Slice& key, void* value),
                        Handle** handle, Priority priority) override {
//传入的句柄和值用于实际缓存，因此我们传递nullptr
//只为两者输入缓存。同时，删除函数指针
//将由用户调用以执行一些应
//只能应用一次。因此，key_only_缓存接受空函数。
//*没有捕获的lambda函数可以被认为是函数指针
    Handle* h = key_only_cache_->Lookup(key);
    if (h == nullptr) {
      key_only_cache_->Insert(key, nullptr, charge,
                              [](const Slice& k, void* v) {}, nullptr,
                              priority);
    } else {
      key_only_cache_->Release(h);
    }

    cache_activity_logger_.ReportAdd(key, charge);

    return cache_->Insert(key, value, charge, deleter, handle, priority);
  }

  virtual Handle* Lookup(const Slice& key, Statistics* stats) override {
    Handle* h = key_only_cache_->Lookup(key);
    if (h != nullptr) {
      key_only_cache_->Release(h);
      inc_hit_counter();
      RecordTick(stats, SIM_BLOCK_CACHE_HIT);
    } else {
      inc_miss_counter();
      RecordTick(stats, SIM_BLOCK_CACHE_MISS);
    }

    cache_activity_logger_.ReportLookup(key);

    return cache_->Lookup(key, stats);
  }

  virtual bool Ref(Handle* handle) override { return cache_->Ref(handle); }

  virtual bool Release(Handle* handle, bool force_erase = false) override {
    return cache_->Release(handle, force_erase);
  }

  virtual void Erase(const Slice& key) override {
    cache_->Erase(key);
    key_only_cache_->Erase(key);
  }

  virtual void* Value(Handle* handle) override { return cache_->Value(handle); }

  virtual uint64_t NewId() override { return cache_->NewId(); }

  virtual size_t GetCapacity() const override { return cache_->GetCapacity(); }

  virtual bool HasStrictCapacityLimit() const override {
    return cache_->HasStrictCapacityLimit();
  }

  virtual size_t GetUsage() const override { return cache_->GetUsage(); }

  virtual size_t GetUsage(Handle* handle) const override {
    return cache_->GetUsage(handle);
  }

  virtual size_t GetPinnedUsage() const override {
    return cache_->GetPinnedUsage();
  }

  virtual void DisownData() override {
    cache_->DisownData();
    key_only_cache_->DisownData();
  }

  virtual void ApplyToAllCacheEntries(void (*callback)(void*, size_t),
                                      bool thread_safe) override {
//仅应用于缓存，因为仅密钥缓存不包含值
    cache_->ApplyToAllCacheEntries(callback, thread_safe);
  }

  virtual void EraseUnRefEntries() override {
    cache_->EraseUnRefEntries();
    key_only_cache_->EraseUnRefEntries();
  }

  virtual size_t GetSimCapacity() const override {
    return key_only_cache_->GetCapacity();
  }
  virtual size_t GetSimUsage() const override {
    return key_only_cache_->GetUsage();
  }
  virtual void SetSimCapacity(size_t capacity) override {
    key_only_cache_->SetCapacity(capacity);
  }

  virtual uint64_t get_miss_counter() const override {
    return miss_times_.load(std::memory_order_relaxed);
  }

  virtual uint64_t get_hit_counter() const override {
    return hit_times_.load(std::memory_order_relaxed);
  }

  virtual void reset_counter() override {
    miss_times_.store(0, std::memory_order_relaxed);
    hit_times_.store(0, std::memory_order_relaxed);
    SetTickerCount(stats_, SIM_BLOCK_CACHE_HIT, 0);
    SetTickerCount(stats_, SIM_BLOCK_CACHE_MISS, 0);
  }

  virtual std::string ToString() const override {
    std::string res;
    res.append("SimCache MISSes: " + std::to_string(get_miss_counter()) + "\n");
    res.append("SimCache HITs:    " + std::to_string(get_hit_counter()) + "\n");
    char buff[350];
    auto lookups = get_miss_counter() + get_hit_counter();
    snprintf(buff, sizeof(buff), "SimCache HITRATE: %.2f%%\n",
             (lookups == 0 ? 0 : get_hit_counter() * 100.0f / lookups));
    res.append(buff);
    return res;
  }

  virtual std::string GetPrintableOptions() const override {
    std::string ret;
    ret.reserve(20000);
    ret.append("    cache_options:\n");
    ret.append(cache_->GetPrintableOptions());
    ret.append("    sim_cache_options:\n");
    ret.append(key_only_cache_->GetPrintableOptions());
    return ret;
  }

  virtual Status StartActivityLogging(const std::string& activity_log_file,
                                      Env* env,
                                      uint64_t max_logging_size = 0) override {
    return cache_activity_logger_.StartLogging(activity_log_file, env,
                                               max_logging_size);
  }

  virtual void StopActivityLogging() override {
    cache_activity_logger_.StopLogging();
  }

  virtual Status GetActivityLoggingStatus() override {
    return cache_activity_logger_.bg_status();
  }

 private:
  std::shared_ptr<Cache> cache_;
  std::shared_ptr<Cache> key_only_cache_;
  std::atomic<uint64_t> miss_times_;
  std::atomic<uint64_t> hit_times_;
  Statistics* stats_;
  CacheActivityLogger cache_activity_logger_;

  void inc_miss_counter() {
    miss_times_.fetch_add(1, std::memory_order_relaxed);
  }
  void inc_hit_counter() { hit_times_.fetch_add(1, std::memory_order_relaxed); }
};

}  //结束匿名命名空间

//出于检测目的，请改用newsimcache
std::shared_ptr<SimCache> NewSimCache(std::shared_ptr<Cache> cache,
                                      size_t sim_capacity, int num_shard_bits) {
  if (num_shard_bits >= 20) {
return nullptr;  //缓存不能分割成太多的碎片
  }
  return std::make_shared<SimCacheImpl>(cache, sim_capacity, num_shard_bits);
}

}  //结束命名空间rocksdb
