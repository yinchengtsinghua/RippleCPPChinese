
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#pragma once

#ifndef ROCKSDB_LITE

#include <string>
#include "rocksdb/options.h"
#include "rocksdb/table.h"

namespace rocksdb {

struct EnvOptions;

using std::unique_ptr;
class Status;
class RandomAccessFile;
class WritableFile;
class Table;
class TableBuilder;

class AdaptiveTableFactory : public TableFactory {
 public:
  ~AdaptiveTableFactory() {}

  explicit AdaptiveTableFactory(
      std::shared_ptr<TableFactory> table_factory_to_write,
      std::shared_ptr<TableFactory> block_based_table_factory,
      std::shared_ptr<TableFactory> plain_table_factory,
      std::shared_ptr<TableFactory> cuckoo_table_factory);

  const char* Name() const override { return "AdaptiveTableFactory"; }

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

 private:
  std::shared_ptr<TableFactory> table_factory_to_write_;
  std::shared_ptr<TableFactory> block_based_table_factory_;
  std::shared_ptr<TableFactory> plain_table_factory_;
  std::shared_ptr<TableFactory> cuckoo_table_factory_;
};

}  //命名空间rocksdb
#endif  //摇滚乐
