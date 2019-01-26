
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

#include "rocksdb/sst_dump_tool.h"

#include <memory>
#include <string>
#include "db/dbformat.h"
#include "options/cf_options.h"
#include "util/file_reader_writer.h"

namespace rocksdb {

class SstFileReader {
 public:
  explicit SstFileReader(const std::string& file_name, bool verify_checksum,
                         bool output_hex);

  Status ReadSequential(bool print_kv, uint64_t read_num, bool has_from,
                        const std::string& from_key, bool has_to,
                        const std::string& to_key,
                        bool use_from_as_prefix = false);

  Status ReadTableProperties(
      std::shared_ptr<const TableProperties>* table_properties);
  uint64_t GetReadNumber() { return read_num_; }
  TableProperties* GetInitTableProperties() { return table_properties_.get(); }

  Status VerifyChecksum();
  Status DumpTable(const std::string& out_filename);
  Status getStatus() { return init_result_; }

  int ShowAllCompressionSizes(
      size_t block_size,
      const std::vector<std::pair<CompressionType, const char*>>&
          compression_types);

 private:
//获取sst文件的TableReader实现
  Status GetTableReader(const std::string& file_path);
  Status ReadTableProperties(uint64_t table_magic_number,
                             RandomAccessFileReader* file, uint64_t file_size);

  uint64_t CalculateCompressedTableSize(const TableBuilderOptions& tb_options,
                                        size_t block_size);

  Status SetTableOptionsByMagicNumber(uint64_t table_magic_number);
  Status SetOldTableOptions();

//用于使用特定于
//工厂实施
  Status NewTableReader(const ImmutableCFOptions& ioptions,
                        const EnvOptions& soptions,
                        const InternalKeyComparator& internal_comparator,
                        uint64_t file_size,
                        unique_ptr<TableReader>* table_reader);

  std::string file_name_;
  uint64_t read_num_;
  bool verify_checksum_;
  bool output_hex_;
  EnvOptions soptions_;

//选项和内部比较器也将用于
//内部读取顺序（特别是查找相关操作）
  Options options_;

  Status init_result_;
  unique_ptr<TableReader> table_reader_;
  unique_ptr<RandomAccessFileReader> file_;

  const ImmutableCFOptions ioptions_;
  InternalKeyComparator internal_comparator_;
  unique_ptr<TableProperties> table_properties_;
};

}  //命名空间rocksdb

#endif  //摇滚乐
