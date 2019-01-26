
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
//版权所有（c）2012 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#ifndef ROCKSDB_LITE

#include <stdint.h>
#include "rocksdb/sst_dump_tool.h"

#include "rocksdb/filter_policy.h"
#include "table/block_based_table_factory.h"
#include "table/table_builder.h"
#include "util/file_reader_writer.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace rocksdb {

const uint32_t optLength = 100;

namespace {
static std::string MakeKey(int i) {
  char buf[100];
  snprintf(buf, sizeof(buf), "k_%04d", i);
  InternalKey key(std::string(buf), 0, ValueType::kTypeValue);
  return key.Encode().ToString();
}

static std::string MakeValue(int i) {
  char buf[100];
  snprintf(buf, sizeof(buf), "v_%04d", i);
  InternalKey key(std::string(buf), 0, ValueType::kTypeValue);
  return key.Encode().ToString();
}

void createSST(const std::string& file_name,
               const BlockBasedTableOptions& table_options) {
  std::shared_ptr<rocksdb::TableFactory> tf;
  tf.reset(new rocksdb::BlockBasedTableFactory(table_options));

  unique_ptr<WritableFile> file;
  Env* env = Env::Default();
  EnvOptions env_options;
  ReadOptions read_options;
  Options opts;
  const ImmutableCFOptions imoptions(opts);
  rocksdb::InternalKeyComparator ikc(opts.comparator);
  unique_ptr<TableBuilder> tb;

  env->NewWritableFile(file_name, &file, env_options);
  opts.table_factory = tf;
  std::vector<std::unique_ptr<IntTblPropCollectorFactory> >
      int_tbl_prop_collector_factories;
  unique_ptr<WritableFileWriter> file_writer(
      new WritableFileWriter(std::move(file), EnvOptions()));
  std::string column_family_name;
  int unknown_level = -1;
  tb.reset(opts.table_factory->NewTableBuilder(
      TableBuilderOptions(imoptions, ikc, &int_tbl_prop_collector_factories,
                          CompressionType::kNoCompression, CompressionOptions(),
                          /*lptr/*压缩\u dict*/，
                          假/*跳过过滤器*/, column_family_name,

                          unknown_level),
      TablePropertiesCollectorFactory::Context::kUnknownColumnFamily,
      file_writer.get()));

//填充略多于1K的键
  uint32_t num_keys = 1024;
  for (uint32_t i = 0; i < num_keys; i++) {
    tb->Add(MakeKey(i), MakeValue(i));
  }
  tb->Finish();
  file_writer->Close();
}

void cleanup(const std::string& file_name) {
  Env* env = Env::Default();
  env->DeleteFile(file_name);
  std::string outfile_name = file_name.substr(0, file_name.length() - 4);
  outfile_name.append("_dump.txt");
  env->DeleteFile(outfile_name);
}
}  //命名空间

//测试SST卸载工具“原始”模式
class SSTDumpToolTest : public testing::Test {
 public:
  BlockBasedTableOptions table_options_;

  SSTDumpToolTest() {}

  ~SSTDumpToolTest() {}
};

TEST_F(SSTDumpToolTest, EmptyFilter) {
  std::string file_name = "rocksdb_sst_test.sst";
  createSST(file_name, table_options_);

  char* usage[3];
  for (int i = 0; i < 3; i++) {
    usage[i] = new char[optLength];
  }
  snprintf(usage[0], optLength, "./sst_dump");
  snprintf(usage[1], optLength, "--command=raw");
  snprintf(usage[2], optLength, "--file=rocksdb_sst_test.sst");

  rocksdb::SSTDumpTool tool;
  ASSERT_TRUE(!tool.Run(3, usage));

  cleanup(file_name);
  for (int i = 0; i < 3; i++) {
    delete[] usage[i];
  }
}

TEST_F(SSTDumpToolTest, FilterBlock) {
  table_options_.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, true));
  std::string file_name = "rocksdb_sst_test.sst";
  createSST(file_name, table_options_);

  char* usage[3];
  for (int i = 0; i < 3; i++) {
    usage[i] = new char[optLength];
  }
  snprintf(usage[0], optLength, "./sst_dump");
  snprintf(usage[1], optLength, "--command=raw");
  snprintf(usage[2], optLength, "--file=rocksdb_sst_test.sst");

  rocksdb::SSTDumpTool tool;
  ASSERT_TRUE(!tool.Run(3, usage));

  cleanup(file_name);
  for (int i = 0; i < 3; i++) {
    delete[] usage[i];
  }
}

TEST_F(SSTDumpToolTest, FullFilterBlock) {
  table_options_.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
  std::string file_name = "rocksdb_sst_test.sst";
  createSST(file_name, table_options_);

  char* usage[3];
  for (int i = 0; i < 3; i++) {
    usage[i] = new char[optLength];
  }
  snprintf(usage[0], optLength, "./sst_dump");
  snprintf(usage[1], optLength, "--command=raw");
  snprintf(usage[2], optLength, "--file=rocksdb_sst_test.sst");

  rocksdb::SSTDumpTool tool;
  ASSERT_TRUE(!tool.Run(3, usage));

  cleanup(file_name);
  for (int i = 0; i < 3; i++) {
    delete[] usage[i];
  }
}

TEST_F(SSTDumpToolTest, GetProperties) {
  table_options_.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
  std::string file_name = "rocksdb_sst_test.sst";
  createSST(file_name, table_options_);

  char* usage[3];
  for (int i = 0; i < 3; i++) {
    usage[i] = new char[optLength];
  }
  snprintf(usage[0], optLength, "./sst_dump");
  snprintf(usage[1], optLength, "--show_properties");
  snprintf(usage[2], optLength, "--file=rocksdb_sst_test.sst");

  rocksdb::SSTDumpTool tool;
  ASSERT_TRUE(!tool.Run(3, usage));

  cleanup(file_name);
  for (int i = 0; i < 3; i++) {
    delete[] usage[i];
  }
}

TEST_F(SSTDumpToolTest, CompressedSizes) {
  table_options_.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
  std::string file_name = "rocksdb_sst_test.sst";
  createSST(file_name, table_options_);

  char* usage[3];
  for (int i = 0; i < 3; i++) {
    usage[i] = new char[optLength];
  }

  snprintf(usage[0], optLength, "./sst_dump");
  snprintf(usage[1], optLength, "--command=recompress");
  snprintf(usage[2], optLength, "--file=rocksdb_sst_test.sst");
  rocksdb::SSTDumpTool tool;
  ASSERT_TRUE(!tool.Run(3, usage));

  cleanup(file_name);
  for (int i = 0; i < 3; i++) {
    delete[] usage[i];
  }
}
}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else
#include <stdio.h>

int main(int argc, char** argv) {
  fprintf(stderr, "SKIPPED as SSTDumpTool is not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //！rocksdb_lite返回run_all_tests（）；
