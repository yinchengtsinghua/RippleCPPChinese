
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

#include "rocksdb/utilities/option_change_migration.h"
#include <set>
#include "db/db_test_util.h"
#include "port/stack_trace.h"
namespace rocksdb {

class DBOptionChangeMigrationTests
    : public DBTestBase,
      public testing::WithParamInterface<
          std::tuple<int, int, bool, int, int, bool>> {
 public:
  DBOptionChangeMigrationTests()
      : DBTestBase("/db_option_change_migration_test") {
    level1_ = std::get<0>(GetParam());
    compaction_style1_ = std::get<1>(GetParam());
    is_dynamic1_ = std::get<2>(GetParam());

    level2_ = std::get<3>(GetParam());
    compaction_style2_ = std::get<4>(GetParam());
    is_dynamic2_ = std::get<5>(GetParam());
  }

//如果继承自测试：：WithParamInterface<>
  static void SetUpTestCase() {}
  static void TearDownTestCase() {}

  int level1_;
  int compaction_style1_;
  bool is_dynamic1_;

  int level2_;
  int compaction_style2_;
  bool is_dynamic2_;
};

#ifndef ROCKSDB_LITE
TEST_P(DBOptionChangeMigrationTests, Migrate1) {
  Options old_options = CurrentOptions();
  old_options.compaction_style =
      static_cast<CompactionStyle>(compaction_style1_);
  if (old_options.compaction_style == CompactionStyle::kCompactionStyleLevel) {
    old_options.level_compaction_dynamic_level_bytes = is_dynamic1_;
  }

  old_options.level0_file_num_compaction_trigger = 3;
  old_options.write_buffer_size = 64 * 1024;
  old_options.target_file_size_base = 128 * 1024;
//将l1、l2的级别目标设为200kb、600kb
  old_options.num_levels = level1_;
  old_options.max_bytes_for_level_multiplier = 3;
  old_options.max_bytes_for_level_base = 200 * 1024;

  Reopen(old_options);

  Random rnd(301);
  int key_idx = 0;

//生成至少2MB的数据
  for (int num = 0; num < 20; num++) {
    GenerateNewFile(&rnd, &key_idx);
  }
  dbfull()->TEST_WaitForFlushMemTable();
  dbfull()->TEST_WaitForCompact();

//将确保在迁移后这些密钥正好位于数据库中。
  std::set<std::string> keys;
  {
    std::unique_ptr<Iterator> it(db_->NewIterator(ReadOptions()));
    it->SeekToFirst();
    for (; it->Valid(); it->Next()) {
      keys.insert(it->key().ToString());
    }
  }
  Close();

  Options new_options = old_options;
  new_options.compaction_style =
      static_cast<CompactionStyle>(compaction_style2_);
  if (new_options.compaction_style == CompactionStyle::kCompactionStyleLevel) {
    new_options.level_compaction_dynamic_level_bytes = is_dynamic2_;
  }
  new_options.target_file_size_base = 256 * 1024;
  new_options.num_levels = level2_;
  new_options.max_bytes_for_level_base = 150 * 1024;
  new_options.max_bytes_for_level_multiplier = 4;
  ASSERT_OK(OptionChangeMigration(dbname_, old_options, new_options));
  Reopen(new_options);

//等待压缩完成并确保它可以重新打开
  dbfull()->TEST_WaitForFlushMemTable();
  dbfull()->TEST_WaitForCompact();
  Reopen(new_options);

  {
    std::unique_ptr<Iterator> it(db_->NewIterator(ReadOptions()));
    it->SeekToFirst();
    for (std::string key : keys) {
      ASSERT_TRUE(it->Valid());
      ASSERT_EQ(key, it->key().ToString());
      it->Next();
    }
    ASSERT_TRUE(!it->Valid());
  }
}

TEST_P(DBOptionChangeMigrationTests, Migrate2) {
  Options old_options = CurrentOptions();
  old_options.compaction_style =
      static_cast<CompactionStyle>(compaction_style2_);
  if (old_options.compaction_style == CompactionStyle::kCompactionStyleLevel) {
    old_options.level_compaction_dynamic_level_bytes = is_dynamic2_;
  }
  old_options.level0_file_num_compaction_trigger = 3;
  old_options.write_buffer_size = 64 * 1024;
  old_options.target_file_size_base = 128 * 1024;
//将l1、l2的级别目标设为200kb、600kb
  old_options.num_levels = level2_;
  old_options.max_bytes_for_level_multiplier = 3;
  old_options.max_bytes_for_level_base = 200 * 1024;

  Reopen(old_options);

  Random rnd(301);
  int key_idx = 0;

//生成至少2MB的数据
  for (int num = 0; num < 20; num++) {
    GenerateNewFile(&rnd, &key_idx);
  }
  dbfull()->TEST_WaitForFlushMemTable();
  dbfull()->TEST_WaitForCompact();

//将确保在迁移后这些密钥正好位于数据库中。
  std::set<std::string> keys;
  {
    std::unique_ptr<Iterator> it(db_->NewIterator(ReadOptions()));
    it->SeekToFirst();
    for (; it->Valid(); it->Next()) {
      keys.insert(it->key().ToString());
    }
  }

  Close();

  Options new_options = old_options;
  new_options.compaction_style =
      static_cast<CompactionStyle>(compaction_style1_);
  if (new_options.compaction_style == CompactionStyle::kCompactionStyleLevel) {
    new_options.level_compaction_dynamic_level_bytes = is_dynamic1_;
  }
  new_options.target_file_size_base = 256 * 1024;
  new_options.num_levels = level1_;
  new_options.max_bytes_for_level_base = 150 * 1024;
  new_options.max_bytes_for_level_multiplier = 4;
  ASSERT_OK(OptionChangeMigration(dbname_, old_options, new_options));
  Reopen(new_options);
//等待压缩完成并确保它可以重新打开
  dbfull()->TEST_WaitForFlushMemTable();
  dbfull()->TEST_WaitForCompact();
  Reopen(new_options);

  {
    std::unique_ptr<Iterator> it(db_->NewIterator(ReadOptions()));
    it->SeekToFirst();
    for (std::string key : keys) {
      ASSERT_TRUE(it->Valid());
      ASSERT_EQ(key, it->key().ToString());
      it->Next();
    }
    ASSERT_TRUE(!it->Valid());
  }
}

TEST_P(DBOptionChangeMigrationTests, Migrate3) {
  Options old_options = CurrentOptions();
  old_options.compaction_style =
      static_cast<CompactionStyle>(compaction_style1_);
  if (old_options.compaction_style == CompactionStyle::kCompactionStyleLevel) {
    old_options.level_compaction_dynamic_level_bytes = is_dynamic1_;
  }

  old_options.level0_file_num_compaction_trigger = 3;
  old_options.write_buffer_size = 64 * 1024;
  old_options.target_file_size_base = 128 * 1024;
//将l1、l2的级别目标设为200kb、600kb
  old_options.num_levels = level1_;
  old_options.max_bytes_for_level_multiplier = 3;
  old_options.max_bytes_for_level_base = 200 * 1024;

  Reopen(old_options);
  Random rnd(301);
  for (int num = 0; num < 20; num++) {
    for (int i = 0; i < 50; i++) {
      ASSERT_OK(Put(Key(num * 100 + i), RandomString(&rnd, 900)));
    }
    Flush();
    dbfull()->TEST_WaitForCompact();
    if (num == 9) {
//发出完全压缩以生成一些零输出文件
      CompactRangeOptions cro;
      cro.bottommost_level_compaction = BottommostLevelCompaction::kForce;
      dbfull()->CompactRange(cro, nullptr, nullptr);
    }
  }
  dbfull()->TEST_WaitForFlushMemTable();
  dbfull()->TEST_WaitForCompact();

//将确保在迁移后这些密钥正好位于数据库中。
  std::set<std::string> keys;
  {
    std::unique_ptr<Iterator> it(db_->NewIterator(ReadOptions()));
    it->SeekToFirst();
    for (; it->Valid(); it->Next()) {
      keys.insert(it->key().ToString());
    }
  }
  Close();

  Options new_options = old_options;
  new_options.compaction_style =
      static_cast<CompactionStyle>(compaction_style2_);
  if (new_options.compaction_style == CompactionStyle::kCompactionStyleLevel) {
    new_options.level_compaction_dynamic_level_bytes = is_dynamic2_;
  }
  new_options.target_file_size_base = 256 * 1024;
  new_options.num_levels = level2_;
  new_options.max_bytes_for_level_base = 150 * 1024;
  new_options.max_bytes_for_level_multiplier = 4;
  ASSERT_OK(OptionChangeMigration(dbname_, old_options, new_options));
  Reopen(new_options);

//等待压缩完成并确保它可以重新打开
  dbfull()->TEST_WaitForFlushMemTable();
  dbfull()->TEST_WaitForCompact();
  Reopen(new_options);

  {
    std::unique_ptr<Iterator> it(db_->NewIterator(ReadOptions()));
    it->SeekToFirst();
    for (std::string key : keys) {
      ASSERT_TRUE(it->Valid());
      ASSERT_EQ(key, it->key().ToString());
      it->Next();
    }
    ASSERT_TRUE(!it->Valid());
  }
}

TEST_P(DBOptionChangeMigrationTests, Migrate4) {
  Options old_options = CurrentOptions();
  old_options.compaction_style =
      static_cast<CompactionStyle>(compaction_style2_);
  if (old_options.compaction_style == CompactionStyle::kCompactionStyleLevel) {
    old_options.level_compaction_dynamic_level_bytes = is_dynamic2_;
  }
  old_options.level0_file_num_compaction_trigger = 3;
  old_options.write_buffer_size = 64 * 1024;
  old_options.target_file_size_base = 128 * 1024;
//将l1、l2的级别目标设为200kb、600kb
  old_options.num_levels = level2_;
  old_options.max_bytes_for_level_multiplier = 3;
  old_options.max_bytes_for_level_base = 200 * 1024;

  Reopen(old_options);
  Random rnd(301);
  for (int num = 0; num < 20; num++) {
    for (int i = 0; i < 50; i++) {
      ASSERT_OK(Put(Key(num * 100 + i), RandomString(&rnd, 900)));
    }
    Flush();
    dbfull()->TEST_WaitForCompact();
    if (num == 9) {
//发出完全压缩以生成一些零输出文件
      CompactRangeOptions cro;
      cro.bottommost_level_compaction = BottommostLevelCompaction::kForce;
      dbfull()->CompactRange(cro, nullptr, nullptr);
    }
  }
  dbfull()->TEST_WaitForFlushMemTable();
  dbfull()->TEST_WaitForCompact();

//将确保在迁移后这些密钥正好位于数据库中。
  std::set<std::string> keys;
  {
    std::unique_ptr<Iterator> it(db_->NewIterator(ReadOptions()));
    it->SeekToFirst();
    for (; it->Valid(); it->Next()) {
      keys.insert(it->key().ToString());
    }
  }

  Close();

  Options new_options = old_options;
  new_options.compaction_style =
      static_cast<CompactionStyle>(compaction_style1_);
  if (new_options.compaction_style == CompactionStyle::kCompactionStyleLevel) {
    new_options.level_compaction_dynamic_level_bytes = is_dynamic1_;
  }
  new_options.target_file_size_base = 256 * 1024;
  new_options.num_levels = level1_;
  new_options.max_bytes_for_level_base = 150 * 1024;
  new_options.max_bytes_for_level_multiplier = 4;
  ASSERT_OK(OptionChangeMigration(dbname_, old_options, new_options));
  Reopen(new_options);
//等待压缩完成并确保它可以重新打开
  dbfull()->TEST_WaitForFlushMemTable();
  dbfull()->TEST_WaitForCompact();
  Reopen(new_options);

  {
    std::unique_ptr<Iterator> it(db_->NewIterator(ReadOptions()));
    it->SeekToFirst();
    for (std::string key : keys) {
      ASSERT_TRUE(it->Valid());
      ASSERT_EQ(key, it->key().ToString());
      it->Next();
    }
    ASSERT_TRUE(!it->Valid());
  }
}

INSTANTIATE_TEST_CASE_P(
    DBOptionChangeMigrationTests, DBOptionChangeMigrationTests,
    ::testing::Values(std::make_tuple(3, 0, false, 4, 0, false),
                      std::make_tuple(3, 0, true, 4, 0, true),
                      std::make_tuple(3, 0, true, 4, 0, false),
                      std::make_tuple(3, 0, false, 4, 0, true),
                      std::make_tuple(3, 1, false, 4, 1, false),
                      std::make_tuple(1, 1, false, 4, 1, false),
                      std::make_tuple(3, 0, false, 4, 1, false),
                      std::make_tuple(3, 0, false, 1, 1, false),
                      std::make_tuple(3, 0, true, 4, 1, false),
                      std::make_tuple(3, 0, true, 1, 1, false),
                      std::make_tuple(1, 1, false, 4, 0, false),
                      std::make_tuple(4, 0, false, 1, 2, false),
                      std::make_tuple(3, 0, true, 2, 2, false),
                      std::make_tuple(3, 1, false, 3, 2, false),
                      std::make_tuple(1, 1, false, 4, 2, false)));

class DBOptionChangeMigrationTest : public DBTestBase {
 public:
  DBOptionChangeMigrationTest()
      : DBTestBase("/db_option_change_migration_test2") {}
};

TEST_F(DBOptionChangeMigrationTest, CompactedSrcToUniversal) {
  Options old_options = CurrentOptions();
  old_options.compaction_style = CompactionStyle::kCompactionStyleLevel;
  old_options.max_compaction_bytes = 200 * 1024;
  old_options.level_compaction_dynamic_level_bytes = false;
  old_options.level0_file_num_compaction_trigger = 3;
  old_options.write_buffer_size = 64 * 1024;
  old_options.target_file_size_base = 128 * 1024;
//将l1、l2的级别目标设为200kb、600kb
  old_options.num_levels = 4;
  old_options.max_bytes_for_level_multiplier = 3;
  old_options.max_bytes_for_level_base = 200 * 1024;

  Reopen(old_options);
  Random rnd(301);
  for (int num = 0; num < 20; num++) {
    for (int i = 0; i < 50; i++) {
      ASSERT_OK(Put(Key(num * 100 + i), RandomString(&rnd, 900)));
    }
  }
  Flush();
  CompactRangeOptions cro;
  cro.bottommost_level_compaction = BottommostLevelCompaction::kForce;
  dbfull()->CompactRange(cro, nullptr, nullptr);

//将确保在迁移后这些密钥正好位于数据库中。
  std::set<std::string> keys;
  {
    std::unique_ptr<Iterator> it(db_->NewIterator(ReadOptions()));
    it->SeekToFirst();
    for (; it->Valid(); it->Next()) {
      keys.insert(it->key().ToString());
    }
  }

  Close();

  Options new_options = old_options;
  new_options.compaction_style = CompactionStyle::kCompactionStyleUniversal;
  new_options.target_file_size_base = 256 * 1024;
  new_options.num_levels = 1;
  new_options.max_bytes_for_level_base = 150 * 1024;
  new_options.max_bytes_for_level_multiplier = 4;
  ASSERT_OK(OptionChangeMigration(dbname_, old_options, new_options));
  Reopen(new_options);
//等待压缩完成并确保它可以重新打开
  dbfull()->TEST_WaitForFlushMemTable();
  dbfull()->TEST_WaitForCompact();
  Reopen(new_options);

  {
    std::unique_ptr<Iterator> it(db_->NewIterator(ReadOptions()));
    it->SeekToFirst();
    for (std::string key : keys) {
      ASSERT_TRUE(it->Valid());
      ASSERT_EQ(key, it->key().ToString());
      it->Next();
    }
    ASSERT_TRUE(!it->Valid());
  }
}

#endif  //摇滚乐
}  //命名空间rocksdb

int main(int argc, char** argv) {
  rocksdb::port::InstallStackTraceHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
