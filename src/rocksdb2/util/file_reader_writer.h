
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
#pragma once
#include <atomic>
#include <string>
#include "port/port.h"
#include "rocksdb/env.h"
#include "rocksdb/rate_limiter.h"
#include "util/aligned_buffer.h"

namespace rocksdb {

class Statistics;
class HistogramImpl;

std::unique_ptr<RandomAccessFile> NewReadaheadRandomAccessFile(
  std::unique_ptr<RandomAccessFile>&& file, size_t readahead_size);

class SequentialFileReader {
 private:
  std::unique_ptr<SequentialFile> file_;
std::atomic<size_t> offset_;  //读取偏移

 public:
  explicit SequentialFileReader(std::unique_ptr<SequentialFile>&& _file)
      : file_(std::move(_file)), offset_(0) {}

  SequentialFileReader(SequentialFileReader&& o) ROCKSDB_NOEXCEPT {
    *this = std::move(o);
  }

  SequentialFileReader& operator=(SequentialFileReader&& o) ROCKSDB_NOEXCEPT {
    file_ = std::move(o.file_);
    return *this;
  }

  SequentialFileReader(const SequentialFileReader&) = delete;
  SequentialFileReader& operator=(const SequentialFileReader&) = delete;

  Status Read(size_t n, Slice* result, char* scratch);

  Status Skip(uint64_t n);

  void Rewind();

  SequentialFile* file() { return file_.get(); }

  bool use_direct_io() const { return file_->use_direct_io(); }
};

class RandomAccessFileReader {
 private:
  std::unique_ptr<RandomAccessFile> file_;
  std::string     file_name_;
  Env*            env_;
  Statistics*     stats_;
  uint32_t        hist_type_;
  HistogramImpl*  file_read_hist_;
  RateLimiter* rate_limiter_;
  bool for_compaction_;

 public:
  explicit RandomAccessFileReader(std::unique_ptr<RandomAccessFile>&& raf,
                                  std::string _file_name,
                                  Env* env = nullptr,
                                  Statistics* stats = nullptr,
                                  uint32_t hist_type = 0,
                                  HistogramImpl* file_read_hist = nullptr,
                                  RateLimiter* rate_limiter = nullptr,
                                  bool for_compaction = false)
      : file_(std::move(raf)),
        file_name_(std::move(_file_name)),
        env_(env),
        stats_(stats),
        hist_type_(hist_type),
        file_read_hist_(file_read_hist),
        rate_limiter_(rate_limiter),
        for_compaction_(for_compaction) {}

  RandomAccessFileReader(RandomAccessFileReader&& o) ROCKSDB_NOEXCEPT {
    *this = std::move(o);
  }

  RandomAccessFileReader& operator=(RandomAccessFileReader&& o)
      ROCKSDB_NOEXCEPT {
    file_ = std::move(o.file_);
    env_ = std::move(o.env_);
    stats_ = std::move(o.stats_);
    hist_type_ = std::move(o.hist_type_);
    file_read_hist_ = std::move(o.file_read_hist_);
    rate_limiter_ = std::move(o.rate_limiter_);
    for_compaction_ = std::move(o.for_compaction_);
    return *this;
  }

  RandomAccessFileReader(const RandomAccessFileReader&) = delete;
  RandomAccessFileReader& operator=(const RandomAccessFileReader&) = delete;

  Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const;

  Status Prefetch(uint64_t offset, size_t n) const {
    return file_->Prefetch(offset, n);
  }

  RandomAccessFile* file() { return file_.get(); }

  std::string file_name() const { return file_name_; }

  bool use_direct_io() const { return file_->use_direct_io(); }
};

//使用posix write将数据写入文件。
class WritableFileWriter {
 private:
  std::unique_ptr<WritableFile> writable_file_;
  AlignedBuffer           buf_;
  size_t                  max_buffer_size_;
//实际写入的数据大小可用于截断
//不计算填充数据
  uint64_t                filesize_;
#ifndef ROCKSDB_LITE
//当我们使用无缓冲访问时，这是必要的
//写入必须在对齐的偏移上进行
//所以我们需要回去再写一页
  uint64_t                next_write_offset_;
#endif  //摇滚乐
  bool                    pending_sync_;
  uint64_t                last_sync_size_;
  uint64_t                bytes_per_sync_;
  RateLimiter*            rate_limiter_;
  Statistics* stats_;

 public:
  WritableFileWriter(std::unique_ptr<WritableFile>&& file,
                     const EnvOptions& options, Statistics* stats = nullptr)
      : writable_file_(std::move(file)),
        buf_(),
        max_buffer_size_(options.writable_file_max_buffer_size),
        filesize_(0),
#ifndef ROCKSDB_LITE
        next_write_offset_(0),
#endif  //摇滚乐
        pending_sync_(false),
        last_sync_size_(0),
        bytes_per_sync_(options.bytes_per_sync),
        rate_limiter_(options.rate_limiter),
        stats_(stats) {
    buf_.Alignment(writable_file_->GetRequiredBufferAlignment());
    buf_.AllocateNewBuffer(std::min((size_t)65536, max_buffer_size_));
  }

  WritableFileWriter(const WritableFileWriter&) = delete;

  WritableFileWriter& operator=(const WritableFileWriter&) = delete;

  ~WritableFileWriter() { Close(); }

  Status Append(const Slice& data);

  Status Flush();

  Status Close();

  Status Sync(bool use_fsync);

//仅同步已刷新的数据（）ed。可以安全地同时调用
//使用append（）和flush（）。如果！可写_file_u->issynchThreadSafe（），
//返回NotSupported状态。
  Status SyncWithoutFlush(bool use_fsync);

  uint64_t GetFileSize() { return filesize_; }

  Status InvalidateCache(size_t offset, size_t length) {
    return writable_file_->InvalidateCache(offset, length);
  }

  WritableFile* writable_file() const { return writable_file_.get(); }

  bool use_direct_io() { return writable_file_->use_direct_io(); }

 private:
//当OS缓冲关闭并且我们正在写入时使用
//直接I/O模式下的DMA
#ifndef ROCKSDB_LITE
  Status WriteDirect();
#endif  //！摇滚乐
//正常写入
  Status WriteBuffered(const char* data, size_t size);
  Status RangeSync(uint64_t offset, uint64_t nbytes);
  Status SyncInternal(bool use_fsync);
};

class FilePrefetchBuffer {
 public:
  Status Prefetch(RandomAccessFileReader* reader, uint64_t offset, size_t n);
  bool TryReadFromCache(uint64_t offset, size_t n, Slice* result) const;

 private:
  AlignedBuffer buffer_;
  uint64_t buffer_offset_;
  size_t buffer_len_;
};

extern Status NewWritableFile(Env* env, const std::string& fname,
                              unique_ptr<WritableFile>* result,
                              const EnvOptions& options);
}  //命名空间rocksdb
