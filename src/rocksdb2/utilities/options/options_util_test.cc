
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

#ifndef ROCKSDB_LITE
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include <cctype>
#include <unordered_map>

#include "options/options_parser.h"
#include "rocksdb/db.h"
#include "rocksdb/table.h"
#include "rocksdb/utilities/options_util.h"
#include "util/random.h"
#include "util/testharness.h"
#include "util/testutil.h"

#ifndef GFLAGS
bool FLAGS_enable_print = false;
#else
#include <gflags/gflags.h>
using GFLAGS::ParseCommandLineFlags;
DEFINE_bool(enable_print, false, "Print options generated to console.");
#endif  //GFLAGS

namespace rocksdb {
class OptionsUtilTest : public testing::Test {
 public:
  OptionsUtilTest() : rnd_(0xFB) {
    env_.reset(new test::StringEnv(Env::Default()));
    dbname_ = test::TmpDir() + "/options_util_test";
  }

 protected:
  std::unique_ptr<test::StringEnv> env_;
  std::string dbname_;
  Random rnd_;
};

bool IsBlockBasedTableFactory(TableFactory* tf) {
  return tf->Name() == BlockBasedTableFactory().Name();
}

TEST_F(OptionsUtilTest, SaveAndLoad) {
  const size_t kCFCount = 5;

  DBOptions db_opt;
  std::vector<std::string> cf_names;
  std::vector<ColumnFamilyOptions> cf_opts;
  test::RandomInitDBOptions(&db_opt, &rnd_);
  for (size_t i = 0; i < kCFCount; ++i) {
    cf_names.push_back(i == 0 ? kDefaultColumnFamilyName
                              : test::RandomName(&rnd_, 10));
    cf_opts.emplace_back();
    test::RandomInitCFOptions(&cf_opts.back(), &rnd_);
  }

  const std::string kFileName = "OPTIONS-123456";
  PersistRocksDBOptions(db_opt, cf_names, cf_opts, kFileName, env_.get());

  DBOptions loaded_db_opt;
  std::vector<ColumnFamilyDescriptor> loaded_cf_descs;
  ASSERT_OK(LoadOptionsFromFile(kFileName, env_.get(), &loaded_db_opt,
                                &loaded_cf_descs));

  ASSERT_OK(RocksDBOptionsParser::VerifyDBOptions(db_opt, loaded_db_opt));
  test::RandomInitDBOptions(&db_opt, &rnd_);
  ASSERT_NOK(RocksDBOptionsParser::VerifyDBOptions(db_opt, loaded_db_opt));

  for (size_t i = 0; i < kCFCount; ++i) {
    ASSERT_EQ(cf_names[i], loaded_cf_descs[i].name);
    ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(
        cf_opts[i], loaded_cf_descs[i].options));
    if (IsBlockBasedTableFactory(cf_opts[i].table_factory.get())) {
      ASSERT_OK(RocksDBOptionsParser::VerifyTableFactory(
          cf_opts[i].table_factory.get(),
          loaded_cf_descs[i].options.table_factory.get()));
    }
    test::RandomInitCFOptions(&cf_opts[i], &rnd_);
    ASSERT_NOK(RocksDBOptionsParser::VerifyCFOptions(
        cf_opts[i], loaded_cf_descs[i].options));
  }

  for (size_t i = 0; i < kCFCount; ++i) {
    if (cf_opts[i].compaction_filter) {
      delete cf_opts[i].compaction_filter;
    }
  }
}

namespace {
class DummyTableFactory : public TableFactory {
 public:
  DummyTableFactory() {}
  virtual ~DummyTableFactory() {}

  virtual const char* Name() const override { return "DummyTableFactory"; }

  virtual Status NewTableReader(
      const TableReaderOptions& table_reader_options,
      unique_ptr<RandomAccessFileReader>&& file, uint64_t file_size,
      unique_ptr<TableReader>* table_reader,
      bool prefetch_index_and_filter_in_cache) const override {
    return Status::NotSupported();
  }

  virtual TableBuilder* NewTableBuilder(
      const TableBuilderOptions& table_builder_options,
      uint32_t column_family_id, WritableFileWriter* file) const override {
    return nullptr;
  }

  virtual Status SanitizeOptions(
      const DBOptions& db_opts,
      const ColumnFamilyOptions& cf_opts) const override {
    return Status::NotSupported();
  }

  virtual std::string GetPrintableTableOptions() const override { return ""; }

  Status GetOptionString(std::string* opt_string,
                         const std::string& delimiter) const override {
    return Status::OK();
  }
};

class DummyMergeOperator : public MergeOperator {
 public:
  DummyMergeOperator() {}
  virtual ~DummyMergeOperator() {}

  virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                           MergeOperationOutput* merge_out) const override {
    return false;
  }

  virtual bool PartialMergeMulti(const Slice& key,
                                 const std::deque<Slice>& operand_list,
                                 std::string* new_value,
                                 Logger* logger) const override {
    return false;
  }

  virtual const char* Name() const override { return "DummyMergeOperator"; }
};

class DummySliceTransform : public SliceTransform {
 public:
  DummySliceTransform() {}
  virtual ~DummySliceTransform() {}

//返回此转换的名称。
  virtual const char* Name() const { return "DummySliceTransform"; }

//将域中的SRC转换为范围中的DST
  virtual Slice Transform(const Slice& src) const { return src; }

//在函数应用时确定这是否是有效的SRC
  virtual bool InDomain(const Slice& src) const { return false; }

//确定某些SRC的dst=transform（src）
  virtual bool InRange(const Slice& dst) const { return false; }
};

}  //命名空间

TEST_F(OptionsUtilTest, SanityCheck) {
  DBOptions db_opt;
  std::vector<ColumnFamilyDescriptor> cf_descs;
  const size_t kCFCount = 5;
  for (size_t i = 0; i < kCFCount; ++i) {
    cf_descs.emplace_back();
    cf_descs.back().name =
        (i == 0) ? kDefaultColumnFamilyName : test::RandomName(&rnd_, 10);

    cf_descs.back().options.table_factory.reset(NewBlockBasedTableFactory());
//将非空值分配给前缀提取程序，但第一个CF除外。
    cf_descs.back().options.prefix_extractor.reset(
        i != 0 ? test::RandomSliceTransform(&rnd_) : nullptr);
    cf_descs.back().options.merge_operator.reset(
        test::RandomMergeOperator(&rnd_));
  }

  db_opt.create_missing_column_families = true;
  db_opt.create_if_missing = true;

  DestroyDB(dbname_, Options(db_opt, cf_descs[0].options));
  DB* db;
  std::vector<ColumnFamilyHandle*> handles;
//打开并保留选项
  ASSERT_OK(DB::Open(db_opt, dbname_, cf_descs, &handles, &db));

//关闭数据库
  for (auto* handle : handles) {
    delete handle;
  }
  delete db;

//执行健全性检查
  ASSERT_OK(
      CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));

  ASSERT_GE(kCFCount, 5);
//合并算子
  {
    std::shared_ptr<MergeOperator> merge_op =
        cf_descs[0].options.merge_operator;

    ASSERT_NE(merge_op.get(), nullptr);
    cf_descs[0].options.merge_operator.reset();
    ASSERT_NOK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));

    cf_descs[0].options.merge_operator.reset(new DummyMergeOperator());
    ASSERT_NOK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));

    cf_descs[0].options.merge_operator = merge_op;
    ASSERT_OK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));
  }

//前缀提取程序
  {
    std::shared_ptr<const SliceTransform> prefix_extractor =
        cf_descs[1].options.prefix_extractor;

//可以将前缀提取器设置为nullptr。
    ASSERT_NE(prefix_extractor, nullptr);
    cf_descs[1].options.prefix_extractor.reset();
    ASSERT_OK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));

    cf_descs[1].options.prefix_extractor.reset(new DummySliceTransform());
    ASSERT_OK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));

    cf_descs[1].options.prefix_extractor = prefix_extractor;
    ASSERT_OK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));
  }

//前缀提取程序nullptr case
  {
    std::shared_ptr<const SliceTransform> prefix_extractor =
        cf_descs[0].options.prefix_extractor;

//可以将前缀提取器设置为nullptr。
    ASSERT_EQ(prefix_extractor, nullptr);
    cf_descs[0].options.prefix_extractor.reset();
    ASSERT_OK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));

//可以将前缀提取程序从nullptr更改为非nullptr
    cf_descs[0].options.prefix_extractor.reset(new DummySliceTransform());
    ASSERT_OK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));

    cf_descs[0].options.prefix_extractor = prefix_extractor;
    ASSERT_OK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));
  }

//比较器
  {
    test::SimpleSuffixReverseComparator comparator;

    auto* prev_comparator = cf_descs[2].options.comparator;
    cf_descs[2].options.comparator = &comparator;
    ASSERT_NOK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));

    cf_descs[2].options.comparator = prev_comparator;
    ASSERT_OK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));
  }

//表工厂
  {
    std::shared_ptr<TableFactory> table_factory =
        cf_descs[3].options.table_factory;

    ASSERT_NE(table_factory, nullptr);
    cf_descs[3].options.table_factory.reset(new DummyTableFactory());
    ASSERT_NOK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));

    cf_descs[3].options.table_factory = table_factory;
    ASSERT_OK(
        CheckOptionsCompatibility(dbname_, Env::Default(), db_opt, cf_descs));
  }
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
#ifdef GFLAGS
  ParseCommandLineFlags(&argc, &argv, true);
#endif  //GFLAGS
  return RUN_ALL_TESTS();
}

#else
#include <cstdio>

int main(int argc, char** argv) {
  printf("Skipped in RocksDBLite as utilities are not supported.\n");
  return 0;
}
#endif  //！摇滚乐
