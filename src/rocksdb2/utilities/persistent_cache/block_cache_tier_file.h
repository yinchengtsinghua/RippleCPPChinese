
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

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "rocksdb/comparator.h"
#include "rocksdb/env.h"

#include "utilities/persistent_cache/block_cache_tier_file_buffer.h"
#include "utilities/persistent_cache/lrulist.h"
#include "utilities/persistent_cache/persistent_cache_tier.h"
#include "utilities/persistent_cache/persistent_cache_util.h"

#include "port/port.h"
#include "util/crc32c.h"
#include "util/file_reader_writer.h"
#include "util/mutexlock.h"

//持久缓存的IO代码路径使用流水线结构
//
//client->in queue<--blockcachetier->out queue<--writer<->kernel
//
//这将使系统能够按GB/s的吞吐量进行扩展，即
//期待与现代魔鬼像NVM。
//
//文件级操作封装在以下抽象中
//
//禁区
//^
//γ
//γ
//randomaccesscachefile（用于读取）
//^
//γ
//γ
//可写缓存文件（用于写入）
//
//写入IO代码路径：
//
namespace rocksdb {

class WriteableCacheFile;
struct BlockInfo;

//表示设备上的逻辑记录
//
//（l）逻辑（b）锁（地址=缓存文件ID、偏移量、大小
struct LogicalBlockAddress {
  LogicalBlockAddress() {}
  explicit LogicalBlockAddress(const uint32_t cache_id, const uint32_t off,
                               const uint16_t size)
      : cache_id_(cache_id), off_(off), size_(size) {}

  uint32_t cache_id_ = 0;
  uint32_t off_ = 0;
  uint32_t size_ = 0;
};

typedef LogicalBlockAddress LBA;

//班主任
//
//writer是用于将数据写入文件的抽象。组件可以是
//多线程的。这是写入管道的最后一步
class Writer {
 public:
  explicit Writer(PersistentCacheTier* const cache) : cache_(cache) {}
  virtual ~Writer() {}

//以给定的偏移量将缓冲区写入文件
  virtual void Write(WritableFile* const file, CacheWriteBuffer* buf,
                     const uint64_t file_off,
                     const std::function<void()> callback) = 0;
//阻止作者
  virtual void Stop() = 0;

  PersistentCacheTier* const cache_;
};

//类块缓存文件
//
//支持专门用于读/写的构建文件的通用接口
class BlockCacheFile : public LRUElement<BlockCacheFile> {
 public:
  explicit BlockCacheFile(const uint32_t cache_id)
      : LRUElement<BlockCacheFile>(), cache_id_(cache_id) {}

  explicit BlockCacheFile(Env* const env, const std::string& dir,
                          const uint32_t cache_id)
      : LRUElement<BlockCacheFile>(),
        env_(env),
        dir_(dir),
        cache_id_(cache_id) {}

  virtual ~BlockCacheFile() {}

//将键/值附加到文件并将LBA定位器返回给用户
  virtual bool Append(const Slice& key, const Slice& val, LBA* const lba) {
    assert(!"not implemented");
    return false;
  }

//读取记录定位器（LBA）并返回键、值和状态
  virtual bool Read(const LBA& lba, Slice* key, Slice* block, char* scratch) {
    assert(!"not implemented");
    return false;
  }

//获取文件路径
  std::string Path() const {
    return dir_ + "/" + std::to_string(cache_id_) + ".rc";
  }
//获取缓存ID
  uint32_t cacheid() const { return cache_id_; }
//向文件数据添加块信息
//块信息是此文件的索引引用列表
  virtual void Add(BlockInfo* binfo) {
    WriteLock _(&rwlock_);
    block_infos_.push_back(binfo);
  }
//获取块信息
  std::list<BlockInfo*>& block_infos() { return block_infos_; }
//删除文件并返回文件大小
  virtual Status Delete(uint64_t* size);

 protected:
port::RWMutex rwlock_;               //同步互斥
Env* const env_ = nullptr;           //IO公司
const std::string dir_;              //目录名
const uint32_t cache_id_;            //文件的缓存ID
std::list<BlockInfo*> block_infos_;  //映射到的索引项列表
//文件内容
};

//RandomAccessFile类
//
//从文件中读取随机数据的线程安全实现
class RandomAccessCacheFile : public BlockCacheFile {
 public:
  explicit RandomAccessCacheFile(Env* const env, const std::string& dir,
                                 const uint32_t cache_id,
                                 const shared_ptr<Logger>& log)
      : BlockCacheFile(env, dir, cache_id), log_(log) {}

  virtual ~RandomAccessCacheFile() {}

//打开文件进行读取
  bool Open(const bool enable_direct_reads);
//从磁盘读取数据
  bool Read(const LBA& lba, Slice* key, Slice* block, char* scratch) override;

 private:
  std::unique_ptr<RandomAccessFileReader> freader_;

 protected:
  bool OpenImpl(const bool enable_direct_reads);
  bool ParseRec(const LBA& lba, Slice* key, Slice* val, char* scratch);

std::shared_ptr<Logger> log_;  //日志文件
};

//类可写缓存文件
//
//对文件的所有写入都缓存在缓冲区中。缓冲区被刷新到
//磁盘被填满。当文件大小达到一定大小时，新文件
//将在有可用空间的情况下创建
class WriteableCacheFile : public RandomAccessCacheFile {
 public:
  explicit WriteableCacheFile(Env* const env, CacheWriteBufferAllocator* alloc,
                              Writer* writer, const std::string& dir,
                              const uint32_t cache_id, const uint32_t max_size,
                              const std::shared_ptr<Logger>& log)
      : RandomAccessCacheFile(env, dir, cache_id, log),
        alloc_(alloc),
        writer_(writer),
        max_size_(max_size) {}

  virtual ~WriteableCacheFile();

//在磁盘上创建文件
  bool Create(const bool enable_direct_writes, const bool enable_direct_reads);

//从逻辑文件读取数据
  bool Read(const LBA& lba, Slice* key, Slice* block, char* scratch) override {
    ReadLock _(&rwlock_);
    const bool closed = eof_ && bufs_.empty();
    if (closed) {
//文件已关闭，从磁盘读取
      return RandomAccessCacheFile::Read(lba, key, block, scratch);
    }
//文件仍在写入，从缓冲区读取
    return ReadBuffer(lba, key, block, scratch);
  }

//将数据追加到文件结尾
  bool Append(const Slice&, const Slice&, LBA* const) override;
//文件结束
  bool Eof() const { return eof_; }

 private:
  friend class ThreadedWriter;

static const size_t kFileAlignmentSize = 4 * 1024;  //对齐文件大小

  bool ReadBuffer(const LBA& lba, Slice* key, Slice* block, char* scratch);
  bool ReadBuffer(const LBA& lba, char* data);
  bool ExpandBuffer(const size_t size);
  void DispatchBuffer();
  void BufferWriteDone();
  void CloseAndOpenForReading();
  void ClearBuffers();
  void Close();

//内存中的文件布局
//
//+------+------+------+------+------+------+------+
//_b0_b1_b2_b3_b4_b5_
//+------+------+------+------+------+------+------+
//^ ^ ^
//_
//布夫·多夫·布夫·沃夫
//（下一个缓冲区到（下一个要填充的缓冲区）
//刷新到磁盘
//
//对于给定的文件，缓冲区被连续刷新到磁盘。

CacheWriteBufferAllocator* const alloc_ = nullptr;  //缓冲提供者
Writer* const writer_ = nullptr;                    //文件写入线程
std::unique_ptr<WritableFile> file_;   //rocksdb env文件抽象
std::vector<CacheWriteBuffer*> bufs_;  //写入缓冲器
uint32_t size_ = 0;                    //文件大小
const uint32_t max_size_;              //文件的最大大小
bool eof_ = false;                     //文件结束
uint32_t disk_woff_ = 0;               //写入磁盘的偏移量
size_t buf_woff_ = 0;                  //把它写下来
size_t buf_doff_ = 0;                  //进入bufs_uuu进行调度
size_t pending_ios_ = 0;               //正在进行的IO到磁盘的数目
bool enable_direct_reads_ = false;     //我们应该启用直接读取吗
//从磁盘读取时
};

//
//对设备进行写操作的抽象。它是流水线结构的一部分。
//
class ThreadedWriter : public Writer {
 public:
//IO到设备的表示
  struct IO {
    explicit IO(const bool signal) : signal_(signal) {}
    explicit IO(WritableFile* const file, CacheWriteBuffer* const buf,
                const uint64_t file_off, const std::function<void()> callback)
        : file_(file), buf_(buf), file_off_(file_off), callback_(callback) {}

    IO(const IO&) = default;
    IO& operator=(const IO&) = default;
    size_t Size() const { return sizeof(IO); }

WritableFile* file_ = nullptr;           //要写入的文件
CacheWriteBuffer* const buf_ = nullptr;  //写入缓冲区
uint64_t file_off_ = 0;                  //文件偏移量
bool signal_ = false;                    //退出线程循环的信号
std::function<void()> callback_;         //完成时回调
  };

  explicit ThreadedWriter(PersistentCacheTier* const cache, const size_t qdepth,
                          const size_t io_size);
  virtual ~ThreadedWriter() { assert(threads_.empty()); }

  void Stop() override;
  void Write(WritableFile* const file, CacheWriteBuffer* buf,
             const uint64_t file_off,
             const std::function<void()> callback) override;

 private:
  void ThreadMain();
  void DispatchIO(const IO& io);

  const size_t io_size_ = 0;
  BoundedQueue<IO> q_;
  std::vector<port::Thread> threads_;
};

}  //命名空间rocksdb

#endif
