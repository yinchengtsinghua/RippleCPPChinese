
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

#include <string>
#include "rocksdb/table.h"
#include "util/murmurhash.h"
#include "rocksdb/options.h"

namespace rocksdb {

const uint32_t kCuckooMurmurSeedMultiplier = 816922183;
static inline uint64_t CuckooHash(
    const Slice& user_key, uint32_t hash_cnt, bool use_module_hash,
    uint64_t table_size_, bool identity_as_first_hash,
    uint64_t (*get_slice_hash)(const Slice&, uint32_t, uint64_t)) {
#if !defined NDEBUG || defined OS_WIN
//这个部分只在单元测试中使用，但是我们必须将它保存在Windows中
//在Windows下以调试和发布模式运行测试时生成。
  if (get_slice_hash != nullptr) {
    return get_slice_hash(user_key, hash_cnt, table_size_);
  }
#endif

  uint64_t value = 0;
  if (hash_cnt == 0 && identity_as_first_hash) {
    value = (*reinterpret_cast<const int64_t*>(user_key.data()));
  } else {
    value = MurmurHash(user_key.data(), static_cast<int>(user_key.size()),
                       kCuckooMurmurSeedMultiplier * hash_cnt);
  }
  if (use_module_hash) {
    return value % table_size_;
  } else {
    return value & (table_size_ - 1);
  }
}

//布谷鸟表是为需要快速点查找的应用程序而设计的
//但不是快速扫描。
//
//一些假设：
//-键长度和值长度是固定的。
//-不支持快照。
//-不支持合并操作。
//-不支持前缀bloom过滤器。
class CuckooTableFactory : public TableFactory {
 public:
  explicit CuckooTableFactory(const CuckooTableOptions& table_options)
    : table_options_(table_options) {}
  ~CuckooTableFactory() {}

  const char* Name() const override { return "CuckooTable"; }

  Status NewTableReader(
      const TableReaderOptions& table_reader_options,
      unique_ptr<RandomAccessFileReader>&& file, uint64_t file_size,
      unique_ptr<TableReader>* table,
      bool prefetch_index_and_filter_in_cache = true) const override;

  TableBuilder* NewTableBuilder(
      const TableBuilderOptions& table_builder_options,
      uint32_t column_family_id, WritableFileWriter* file) const override;

//清除指定的数据库选项。
  Status SanitizeOptions(const DBOptions& db_opts,
                         const ColumnFamilyOptions& cf_opts) const override {
    return Status::OK();
  }

  std::string GetPrintableTableOptions() const override;

  void* GetOptions() override { return &table_options_; }

  Status GetOptionString(std::string* opt_string,
                         const std::string& delimiter) const override {
    return Status::OK();
  }

 private:
  CuckooTableOptions table_options_;
};

}  //命名空间rocksdb
#endif  //摇滚乐
