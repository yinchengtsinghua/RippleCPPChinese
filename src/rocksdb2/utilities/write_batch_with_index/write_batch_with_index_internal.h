
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

#ifndef ROCKSDB_LITE

#include <limits>
#include <string>
#include <vector>

#include "options/db_options.h"
#include "port/port.h"
#include "rocksdb/comparator.h"
#include "rocksdb/iterator.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/write_batch_with_index.h"

namespace rocksdb {

class MergeContext;
struct Options;

//跳过列表使用的键，作为writebatchwithindex的二进制可搜索索引。
struct WriteBatchIndexEntry {
  WriteBatchIndexEntry(size_t o, uint32_t c, size_t ko, size_t ksz)
      : offset(o),
        column_family(c),
        key_offset(ko),
        key_size(ksz),
        search_key(nullptr) {}
  WriteBatchIndexEntry(const Slice* sk, uint32_t c)
      : offset(0),
        column_family(c),
        key_offset(0),
        key_size(0),
        search_key(sk) {}

//如果此标志出现在偏移量中，则表示一个较小的键
//比同一个列族的任何其他条目
  static const size_t kFlagMin = port::kMaxSizet;

size_t offset;           //写入批处理的字符串缓冲区中的条目的偏移量。
uint32_t column_family;  //条目的列族。
size_t key_offset;       //写入批处理的字符串缓冲区中键的偏移量。
size_t key_size;         //键的大小。

const Slice* search_key;  //如果不为空，则不是从
//编写批处理，使用它进行比较。这是用的
//查找键。
};

class ReadableWriteBatch : public WriteBatch {
 public:
  explicit ReadableWriteBatch(size_t reserved_bytes = 0, size_t max_bytes = 0)
      : WriteBatch(reserved_bytes, max_bytes) {}
//从写入批处理中的写入条目检索一些信息，给定
//写入项的起始偏移量。
  Status GetEntryFromDataOffset(size_t data_offset, WriteType* type, Slice* Key,
                                Slice* value, Slice* blob, Slice* xid) const;
};

class WriteBatchEntryComparator {
 public:
  WriteBatchEntryComparator(const Comparator* _default_comparator,
                            const ReadableWriteBatch* write_batch)
      : default_comparator_(_default_comparator), write_batch_(write_batch) {}
//比较a和b。如果a小于b，则返回负值；如果a小于b，则返回0
//等于，如果a大于b，则为正值
  int operator()(const WriteBatchIndexEntry* entry1,
                 const WriteBatchIndexEntry* entry2) const;

  int CompareKey(uint32_t column_family, const Slice& key1,
                 const Slice& key2) const;

  void SetComparatorForCF(uint32_t column_family_id,
                          const Comparator* comparator) {
    if (column_family_id >= cf_comparators_.size()) {
      cf_comparators_.resize(column_family_id + 1, nullptr);
    }
    cf_comparators_[column_family_id] = comparator;
  }

  const Comparator* default_comparator() { return default_comparator_; }

 private:
  const Comparator* default_comparator_;
  std::vector<const Comparator*> cf_comparators_;
  const ReadableWriteBatch* write_batch_;
};

class WriteBatchWithIndexInternal {
 public:
  enum Result { kFound, kDeleted, kNotFound, kMergeInProgress, kError };

//如果批包含键的值，请将其存储在*值中并返回kfound。
//如果批处理包含键的删除，则返回“已删除”。
//如果批处理包含作为键的最新项的合并操作，
//合并过程不会停止（未达到值或删除），
//将当前合并操作数前置于*操作数，
//并返回kmergeinprogress
//如果批处理不包含此密钥，则返回knotfound
//否则，返回错误的Kerror，错误状态存储在*s中。
  static WriteBatchWithIndexInternal::Result GetFromBatch(
      const ImmutableDBOptions& ioptions, WriteBatchWithIndex* batch,
      ColumnFamilyHandle* column_family, const Slice& key,
      MergeContext* merge_context, WriteBatchEntryComparator* cmp,
      std::string* value, bool overwrite_key, Status* s);
};

}  //命名空间rocksdb
#endif  //！摇滚乐
