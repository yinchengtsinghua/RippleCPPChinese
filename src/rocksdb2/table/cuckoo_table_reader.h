
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
#ifndef ROCKSDB_LITE
#include <string>
#include <memory>
#include <utility>
#include <vector>

#include "db/dbformat.h"
#include "options/cf_options.h"
#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "table/table_reader.h"
#include "util/file_reader_writer.h"

namespace rocksdb {

class Arena;
class TableReader;
class InternalIterator;

class CuckooTableReader: public TableReader {
 public:
  CuckooTableReader(const ImmutableCFOptions& ioptions,
                    std::unique_ptr<RandomAccessFileReader>&& file,
                    uint64_t file_size, const Comparator* user_comparator,
                    uint64_t (*get_slice_hash)(const Slice&, uint32_t,
                                               uint64_t));
  ~CuckooTableReader() {}

  std::shared_ptr<const TableProperties> GetTableProperties() const override {
    return table_props_;
  }

  Status status() const { return status_; }

  Status Get(const ReadOptions& read_options, const Slice& key,
             GetContext* get_context, bool skip_filters = false) override;

  InternalIterator* NewIterator(
      const ReadOptions&, Arena* arena = nullptr,
      bool skip_filters = false) override;
  void Prepare(const Slice& target) override;

//报告内存使用量的近似值。
  size_t ApproximateMemoryUsage() const override;

//布谷鸟表阅读器没有实现以下方法
  uint64_t ApproximateOffsetOf(const Slice& key) override { return 0; }
  void SetupForCompaction() override {}
//方法结尾未实现。

 private:
  friend class CuckooTableIterator;
  void LoadAllKeys(std::vector<std::pair<Slice, uint32_t>>* key_to_bucket_id);
  std::unique_ptr<RandomAccessFileReader> file_;
  Slice file_data_;
  bool is_last_level_;
  bool identity_as_first_hash_;
  bool use_module_hash_;
  std::shared_ptr<const TableProperties> table_props_;
  Status status_;
  uint32_t num_hash_func_;
  std::string unused_key_;
  uint32_t key_length_;
  uint32_t user_key_length_;
  uint32_t value_length_;
  uint32_t bucket_length_;
  uint32_t cuckoo_block_size_;
  uint32_t cuckoo_block_bytes_minus_one_;
  uint64_t table_size_;
  const Comparator* ucomp_;
  uint64_t (*get_slice_hash_)(const Slice& s, uint32_t index,
      uint64_t max_num_buckets);
};

}  //命名空间rocksdb
#endif  //摇滚乐
