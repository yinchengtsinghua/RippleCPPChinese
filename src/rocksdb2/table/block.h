
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
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>
#ifdef ROCKSDB_MALLOC_USABLE_SIZE
#ifdef OS_FREEBSD
#include <malloc_np.h>
#else
#include <malloc.h>
#endif
#endif

#include "db/dbformat.h"
#include "db/pinned_iterators_manager.h"
#include "rocksdb/iterator.h"
#include "rocksdb/options.h"
#include "rocksdb/statistics.h"
#include "table/block_prefix_index.h"
#include "table/internal_iterator.h"
#include "util/random.h"
#include "util/sync_point.h"
#include "format.h"

namespace rocksdb {

struct BlockContents;
class Comparator;
class BlockIter;
class BlockPrefixIndex;

//BlockReadAmpBitmap是将rocksdb:：block数据字节映射到的位图。
//一个位图，每\位的比率字节为\u。每当我们访问
//我们更新位图和增量读取估计有用字节的块。
class BlockReadAmpBitmap {
 public:
  explicit BlockReadAmpBitmap(size_t block_size, size_t bytes_per_bit,
                              Statistics* statistics)
      : bitmap_(nullptr),
        bytes_per_bit_pow_(0),
        statistics_(statistics),
        rnd_(
            Random::GetTLSInstance()->Uniform(static_cast<int>(bytes_per_bit))) {
    TEST_SYNC_POINT_CALLBACK("BlockReadAmpBitmap:rnd", &rnd_);
    assert(block_size > 0 && bytes_per_bit > 0);

//将字节/位转换为2的幂
    while (bytes_per_bit >>= 1) {
      bytes_per_bit_pow_++;
    }

//num_bits_needed=ceil（块大小/字节/位）
    size_t num_bits_needed =
      ((block_size - 1) >> bytes_per_bit_pow_) + 1;
    assert(num_bits_needed > 0);

//位图_size=ceil（需要num_位/kbitspeentry）
    size_t bitmap_size = (num_bits_needed - 1) / kBitsPerEntry + 1;

//创建位图并将所有位设置为0
    bitmap_ = new std::atomic<uint32_t>[bitmap_size]();

    RecordTick(GetStatistics(), READ_AMP_TOTAL_READ_BYTES, block_size);
  }

  ~BlockReadAmpBitmap() { delete[] bitmap_; }

  void Mark(uint32_t start_offset, uint32_t end_offset) {
    assert(end_offset >= start_offset);
//掩码中第一位的索引
    uint32_t start_bit =
        (start_offset + (1 << bytes_per_bit_pow_) - rnd_ - 1) >>
        bytes_per_bit_pow_;
//掩码中最后一位的索引+1
    uint32_t exclusive_end_bit =
        (end_offset + (1 << bytes_per_bit_pow_) - rnd_) >> bytes_per_bit_pow_;
    if (start_bit >= exclusive_end_bit) {
      return;
    }
    assert(exclusive_end_bit > 0);

    if (GetAndSet(start_bit) == 0) {
      uint32_t new_useful_bytes = (exclusive_end_bit - start_bit)
                                  << bytes_per_bit_pow_;
      RecordTick(GetStatistics(), READ_AMP_ESTIMATE_USEFUL_BYTES,
                 new_useful_bytes);
    }
  }

  Statistics* GetStatistics() {
    return statistics_.load(std::memory_order_relaxed);
  }

  void SetStatistics(Statistics* stats) { statistics_.store(stats); }

  uint32_t GetBytesPerBit() { return 1 << bytes_per_bit_pow_; }

 private:
//在'bit_idx'处获取位的当前值并将其设置为1
  inline bool GetAndSet(uint32_t bit_idx) {
    const uint32_t byte_idx = bit_idx / kBitsPerEntry;
    const uint32_t bit_mask = 1 << (bit_idx % kBitsPerEntry);

    return bitmap_[byte_idx].fetch_or(bit_mask, std::memory_order_relaxed) &
           bit_mask;
  }

const uint32_t kBytesPersEntry = sizeof(uint32_t);   //4字节
const uint32_t kBitsPerEntry = kBytesPersEntry * 8;  //32位

//位图用于记录我们读取的字节，使用原子保护
//针对多个线程更新同一位
  std::atomic<uint32_t>* bitmap_;
//（1<<bytes_per_bit_pow_）是bytes_per_bit。使用2的幂优化
//多学科分工
  uint8_t bytes_per_bit_pow_;
//指向db statistics对象的指针，因为此位图可能比db长
//此指针可能无效，但数据库会将其更新为有效指针
//在调用mark（）之前使用setStatistics（）。
  std::atomic<Statistics*> statistics_;
  uint32_t rnd_;
};

class Block {
 public:
//用指定的内容初始化块。
  explicit Block(BlockContents&& contents, SequenceNumber _global_seqno,
                 size_t read_amp_bytes_per_bit = 0,
                 Statistics* statistics = nullptr);

  ~Block() = default;

  size_t size() const { return size_; }
  const char* data() const { return data_; }
  bool cachable() const { return contents_.cachable; }
  size_t usable_size() const {
#ifdef ROCKSDB_MALLOC_USABLE_SIZE
    if (contents_.allocation.get() != nullptr) {
      return malloc_usable_size(contents_.allocation.get());
    }
#endif  //岩石尺寸
    return size_;
  }
  uint32_t NumRestarts() const;
  CompressionType compression_type() const {
    return contents_.compression_type;
  }

//如果启用了哈希索引查找，并且“use-hash-index”为true。这个街区
//将对键前缀执行哈希查找。
//
//注意：对于基于哈希的查找，如果键前缀与任何键都不匹配，
//迭代器将简单地设置为“无效”，而不是返回
//只是通过目标密钥的密钥。
//
//如果iter为空，则返回新的迭代器
//如果iter不为空，请更新此项并将其作为迭代器返回*
//
//如果total-order-seek为true，则忽略hash-index和前缀-index。
//此选项仅适用于索引块。对于数据块，哈希索引
//前缀“index”为空，因此此选项无关紧要。
  InternalIterator* NewIterator(const Comparator* comparator,
                                BlockIter* iter = nullptr,
                                bool total_order_seek = true,
                                Statistics* stats = nullptr);
  void SetBlockPrefixIndex(BlockPrefixIndex* prefix_index);

//报告内存使用量的近似值。
  size_t ApproximateMemoryUsage() const;

  SequenceNumber global_seqno() const { return global_seqno_; }

 private:
  BlockContents contents_;
const char* data_;            //目录数据数据
size_t size_;                 //目录数据大小（）
uint32_t restart_offset_;     //重启阵列数据偏移
  std::unique_ptr<BlockPrefixIndex> prefix_index_;
  std::unique_ptr<BlockReadAmpBitmap> read_amp_bitmap_;
//块中的所有键都将具有seqno=global_seqno_u，无论
//编码值（kDisableGlobalSequenceNumber表示禁用）
  const SequenceNumber global_seqno_;

//不允许复制
  Block(const Block&);
  void operator=(const Block&);
};

class BlockIter : public InternalIterator {
 public:
  BlockIter()
      : comparator_(nullptr),
        data_(nullptr),
        restarts_(0),
        num_restarts_(0),
        current_(0),
        restart_index_(0),
        status_(Status::OK()),
        prefix_index_(nullptr),
        key_pinned_(false),
        global_seqno_(kDisableGlobalSequenceNumber),
        read_amp_bitmap_(nullptr),
        last_bitmap_offset_(0) {}

  BlockIter(const Comparator* comparator, const char* data, uint32_t restarts,
            uint32_t num_restarts, BlockPrefixIndex* prefix_index,
            SequenceNumber global_seqno, BlockReadAmpBitmap* read_amp_bitmap)
      : BlockIter() {
    Initialize(comparator, data, restarts, num_restarts, prefix_index,
               global_seqno, read_amp_bitmap);
  }

  void Initialize(const Comparator* comparator, const char* data,
                  uint32_t restarts, uint32_t num_restarts,
                  BlockPrefixIndex* prefix_index, SequenceNumber global_seqno,
                  BlockReadAmpBitmap* read_amp_bitmap) {
assert(data_ == nullptr);           //确保只调用一次
assert(num_restarts > 0);           //确保参数有效

    comparator_ = comparator;
    data_ = data;
    restarts_ = restarts;
    num_restarts_ = num_restarts;
    current_ = restarts_;
    restart_index_ = num_restarts_;
    prefix_index_ = prefix_index;
    global_seqno_ = global_seqno;
    read_amp_bitmap_ = read_amp_bitmap;
    last_bitmap_offset_ = current_ + 1;
  }

  void SetStatus(Status s) {
    status_ = s;
  }

  virtual bool Valid() const override { return current_ < restarts_; }
  virtual Status status() const override { return status_; }
  virtual Slice key() const override {
    assert(Valid());
    return key_.GetInternalKey();
  }
  virtual Slice value() const override {
    assert(Valid());
    if (read_amp_bitmap_ && current_ < restarts_ &&
        current_ != last_bitmap_offset_) {
      /*位图标记（当前输入偏移量*/，
                             nextentryoffset（）-1）；
      最后一个位图偏移量
    }
    返回值；
  }

  virtual void next（）重写；

  virtual void prev（）重写；

  虚拟虚空搜索（const slice&target）覆盖；

  虚拟无效seekforprev（const slice&target）覆盖；

  virtual void seektofirst（）重写；

  virtual void seektolast（）重写；

nIFUDEF NDEUG
  ~块迭（）{
    //断言在启用固定时永远不会删除blockiter。
    断言（！）固定式
           （钉住了“经理”&！pinned_iters_mgr_->pinningEnabled（））；
  }
  虚拟void setpineditersMgr（
      pinnediteratorsmanager*pinned_iters_mgr）override_
    钉住的，钉住的
  }
  pinnediteratorsmanager*固定的\u iters_mgr_u=nullptr；
第二节

  virtual bool iskeypined（）const override返回键固定

  virtual bool isvaluepinned（）const override返回true；

  size_t test_currententrysize（）返回nextentryoffset（）-当前_

  uint32_t valueoffset（）常量
    返回static_cast<uint32_t>（value_.data（）-data_uu）；
  }

 私人：
  常量比较器*比较器
  const char*data_；//基本块内容
  uint32_t restarts_；//重新启动数组的偏移量（fixed32列表）
  uint32_t num_restarts_u；/重新启动数组中的uint32_t条目数

  //当前_u是当前条目的数据_u中的偏移量。>=重新启动uuuif！有效的
  uint32电流
  uint32_t restart_index_；//当前_u所在重新启动块的索引
  迭代密钥；
  切片值；
  状态状态；
  块前缀索引*前缀索引
  布尔键固定；
  SequenceNumber全局编号

  //读取AMP位图
  blockreadampbitmap*读取位图
  //我们报告的最后一个“当前”值读取amp位
  可变uint32_t最后一个位图_偏移量_u；

  结构缓存事件
    显式缓存事件（uint32_t_offset，const char*_key_ptr，
                             大小键偏移，大小键大小，切片值）
        ：偏移（_偏移），
          按键节拍（按键节拍）
          键“偏移”（键“偏移”），
          密钥大小（密钥大小）
          值（_值）

    //块中条目的偏移量
    uint32_t偏移；
    //指向块中键数据的指针（如果键是增量编码的，则为nullptr）
    常量字符*键指针；
    //上一个条目中键的偏移量keys buff（如果键指针不是nullptr，则为0）
    大小键偏移；
    键的大小
    大小键大小；
    //值切片指向块中的数据
    切片值；
  }；
  std:：string prev_entries_keys_buff；
  std:：vector<cachedpEventry>prev_entries_uu；
  int32_t prev_entries_idx_u=-1；

  inline int compare（const slice&a、const slice&b）const_
    返回比较器->比较（A，B）；
  }

  //返回数据中刚好超过当前条目的偏移量。
  inline uint32_t nextentryoffset（）常量
    //注意：我们不支持大于2GB的块
    返回static_cast<uint32_t>（（value_.data（）+value_.size（））-data_uu）；
  }

  uint32_t getrestartpoint（uint32_t index）
    断言（index<num_restarts_uu）；
    返回decodefixed32（数据_+restarts_+index*sizeof（uint32_t））；
  }

  void seektorestartpoint（uint32_t index）
    KEY..Car（）；
    重新启动索引；
    //当前_u将由parseNextKey（）修复；

    //parseNextKey（）从值_uu的末尾开始，因此相应地设置值_u
    uint32_t offset=getRestartPoint（索引）；
    值_ux=切片（数据_ux偏移，0）；
  }

  void corruptionerror（）；

  bool parseNextkey（）；

  bool binaryseek（const slice&target，uint32_t left，uint32_t right，
                  uint32_t*索引）；

  int compareblockkey（uint32_t block_index，const slice&target）；

  bool binaryblockindexseek（const slice&target，uint32_t*块ID，
                            uint32向左，uint32向右，
                            uint32_t*索引）；

  bool prefixseek（const slice&target，uint32_t*索引）；

}；

//命名空间rocksdb
