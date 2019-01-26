
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
#include <vector>
#include "db/dbformat.h"
#include "include/rocksdb/env.h"
#include "include/rocksdb/listener.h"
#include "include/rocksdb/options.h"
#include "include/rocksdb/status.h"
#include "options/cf_options.h"
#include "table/block_based_table_reader.h"

namespace rocksdb {

struct ColDeclaration;
struct KVPairColDeclarations;

class ColumnAwareEncodingReader {
 public:
  explicit ColumnAwareEncodingReader(const std::string& file_name);

  void GetKVPairsFromDataBlocks(std::vector<KVPairBlock>* kv_pair_blocks);

  void EncodeBlocksToRowFormat(WritableFile* out_file,
                               CompressionType compression_type,
                               const std::vector<KVPairBlock>& kv_pair_blocks,
                               std::vector<std::string>* blocks);

  void DecodeBlocksFromRowFormat(WritableFile* out_file,
                                 const std::vector<std::string>* blocks);

  void DumpDataColumns(const std::string& filename,
                       const KVPairColDeclarations& kvp_col_declarations,
                       const std::vector<KVPairBlock>& kv_pair_blocks);

  Status EncodeBlocks(const KVPairColDeclarations& kvp_col_declarations,
                      WritableFile* out_file, CompressionType compression_type,
                      const std::vector<KVPairBlock>& kv_pair_blocks,
                      std::vector<std::string>* blocks, bool print_column_stat);

  void DecodeBlocks(const KVPairColDeclarations& kvp_col_declarations,
                    WritableFile* out_file,
                    const std::vector<std::string>* blocks);

  static void GetColDeclarationsPrimary(
      std::vector<ColDeclaration>** key_col_declarations,
      std::vector<ColDeclaration>** value_col_declarations,
      ColDeclaration** value_checksum_declaration);

  static void GetColDeclarationsSecondary(
      std::vector<ColDeclaration>** key_col_declarations,
      std::vector<ColDeclaration>** value_col_declarations,
      ColDeclaration** value_checksum_declaration);

 private:
//初始化sst文件的tablereader
  void InitTableReader(const std::string& file_path);

  std::string file_name_;
  EnvOptions soptions_;

  Options options_;

  Status init_result_;
  std::unique_ptr<BlockBasedTable> table_reader_;
  std::unique_ptr<RandomAccessFileReader> file_;

  const ImmutableCFOptions ioptions_;
  InternalKeyComparator internal_comparator_;
  std::unique_ptr<TableProperties> table_properties_;
};

}  //命名空间rocksdb

#endif  //摇滚乐
