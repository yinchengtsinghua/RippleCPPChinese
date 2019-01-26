
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
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <cstdio>
#include <cstdlib>

#ifndef ROCKSDB_LITE
#ifdef GFLAGS

#include <gflags/gflags.h>
#include <inttypes.h>
#include <vector>
#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "table/block_based_table_builder.h"
#include "table/block_based_table_reader.h"
#include "table/format.h"
#include "tools/sst_dump_tool_imp.h"
#include "util/compression.h"
#include "util/stop_watch.h"
#include "utilities/col_buf_encoder.h"
#include "utilities/column_aware_encoding_util.h"

using GFLAGS::ParseCommandLineFlags;
DEFINE_string(encoded_file, "", "file to store encoded data blocks");
DEFINE_string(decoded_file, "",
              "file to store decoded data blocks after encoding");
DEFINE_string(format, "col", "Output Format. Can be 'row' or 'col'");
//TODO（JHLI）：选项“col”应删除并替换为“general”
//柱规格。
DEFINE_string(index_type, "col", "Index type. Can be 'primary' or 'secondary'");
DEFINE_string(dump_file, "",
              "Dump data blocks separated by columns in human-readable format");
DEFINE_bool(decode, false, "Deocde blocks after they are encoded");
DEFINE_bool(stat, false,
            "Print column distribution statistics. Cannot decode in this mode");
DEFINE_string(compression_type, "kNoCompression",
              "The compression algorithm used to compress data blocks");

namespace rocksdb {

class ColumnAwareEncodingExp {
 public:
  static void Run(const std::string& sst_file) {
    bool decode = FLAGS_decode;
    if (FLAGS_decoded_file.size() > 0) {
      decode = true;
    }
    if (FLAGS_stat) {
      decode = false;
    }

    ColumnAwareEncodingReader reader(sst_file);
    std::vector<ColDeclaration>* key_col_declarations;
    std::vector<ColDeclaration>* value_col_declarations;
    ColDeclaration* value_checksum_declaration;
    if (FLAGS_index_type == "primary") {
      ColumnAwareEncodingReader::GetColDeclarationsPrimary(
          &key_col_declarations, &value_col_declarations,
          &value_checksum_declaration);
    } else {
      ColumnAwareEncodingReader::GetColDeclarationsSecondary(
          &key_col_declarations, &value_col_declarations,
          &value_checksum_declaration);
    }
    KVPairColDeclarations kvp_cd(key_col_declarations, value_col_declarations,
                                 value_checksum_declaration);

    if (!FLAGS_dump_file.empty()) {
      std::vector<KVPairBlock> kv_pair_blocks;
      reader.GetKVPairsFromDataBlocks(&kv_pair_blocks);
      reader.DumpDataColumns(FLAGS_dump_file, kvp_cd, kv_pair_blocks);
      return;
    }
    std::unordered_map<std::string, CompressionType> compressions = {
        {"kNoCompression", CompressionType::kNoCompression},
        {"kZlibCompression", CompressionType::kZlibCompression},
        {"kZSTD", CompressionType::kZSTD}};

//查找压缩
    CompressionType compression_type = compressions[FLAGS_compression_type];
    EnvOptions env_options;
    if (CompressionTypeSupported(compression_type)) {
      fprintf(stdout, "[%s]\n", FLAGS_compression_type.c_str());
      unique_ptr<WritableFile> encoded_out_file;

      std::unique_ptr<Env> env(NewMemEnv(Env::Default()));
      if (!FLAGS_encoded_file.empty()) {
        env->NewWritableFile(FLAGS_encoded_file, &encoded_out_file,
                             env_options);
      }

      std::vector<KVPairBlock> kv_pair_blocks;
      reader.GetKVPairsFromDataBlocks(&kv_pair_blocks);

      std::vector<std::string> encoded_blocks;
      StopWatchNano sw(env.get(), true);
      if (FLAGS_format == "col") {
        reader.EncodeBlocks(kvp_cd, encoded_out_file.get(), compression_type,
                            kv_pair_blocks, &encoded_blocks, FLAGS_stat);
} else {  //行格式
        reader.EncodeBlocksToRowFormat(encoded_out_file.get(), compression_type,
                                       kv_pair_blocks, &encoded_blocks);
      }
      if (encoded_out_file != nullptr) {
        uint64_t size = 0;
        env->GetFileSize(FLAGS_encoded_file, &size);
        fprintf(stdout, "File size: %" PRIu64 "\n", size);
      }
      /*t64_t编码_time=sw.elapsednanossafe（false/*重置*/）；
      fprintf（stdout，“编码时间%”priu64“\n”，编码时间）；
      如果（译码）{
        唯一的_ptr<writablefile>decoded_out_file；
        如果（！）标记_解码_file.empty（））
          env->newwritablefile（flags_decoded_file，&decoded_out_file，
                               Env期权）；
        }
        SW（）
        if（flags_format==“col”）
          reader.decodeblocks（kvp_cd，decoded_out_file.get（），&encoded_blocks）；
        }否则{
          reader.decodeBlocksFromRowFormat（decoded_out_file.get（），
                                           &encoded_块）；
        }
        uint64解码时间=sw.elapsednanossafe（真/*重置*/);

        fprintf(stdout, "Decode time: %" PRIu64 "\n", decode_time);
      }
    } else {
      fprintf(stdout, "Unsupported compression type: %s.\n",
              FLAGS_compression_type.c_str());
    }
    delete key_col_declarations;
    delete value_col_declarations;
    delete value_checksum_declaration;
  }
};

}  //命名空间rocksdb

int main(int argc, char** argv) {
  int arg_idx = ParseCommandLineFlags(&argc, &argv, true);
  if (arg_idx >= argc) {
    fprintf(stdout, "SST filename required.\n");
    exit(1);
  }
  std::string sst_file(argv[arg_idx]);
  if (FLAGS_format != "row" && FLAGS_format != "col") {
    fprintf(stderr, "Format must be 'row' or 'col'\n");
    exit(1);
  }
  if (FLAGS_index_type != "primary" && FLAGS_index_type != "secondary") {
    fprintf(stderr, "Format must be 'primary' or 'secondary'\n");
    exit(1);
  }
  rocksdb::ColumnAwareEncodingExp::Run(sst_file);
  return 0;
}

#else
int main() {
  fprintf(stderr, "Please install gflags to run rocksdb tools\n");
  return 1;
}
#endif  //GFLAGS
#else
int main(int argc, char** argv) {
  fprintf(stderr, "Not supported in lite mode.\n");
  return 1;
}
#endif  //摇滚乐
