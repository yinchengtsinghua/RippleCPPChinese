
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

#ifndef ROCKSDB_LITE

#ifndef OS_WIN
#include <unistd.h>
#endif

#include <map>
#include <memory>

#include "rocksdb/compaction_filter.h"
#include "rocksdb/utilities/date_tiered_db.h"
#include "util/logging.h"
#include "util/string_util.h"
#include "util/testharness.h"

namespace rocksdb {

namespace {

typedef std::map<std::string, std::string> KVMap;
}

class SpecialTimeEnv : public EnvWrapper {
 public:
  explicit SpecialTimeEnv(Env* base) : EnvWrapper(base) {
    base->GetCurrentTime(&current_time_);
  }

  void Sleep(int64_t sleep_time) { current_time_ += sleep_time; }
  virtual Status GetCurrentTime(int64_t* current_time) override {
    *current_time = current_time_;
    return Status::OK();
  }

 private:
  int64_t current_time_ = 0;
};

class DateTieredTest : public testing::Test {
 public:
  DateTieredTest() {
    env_.reset(new SpecialTimeEnv(Env::Default()));
    dbname_ = test::TmpDir() + "/date_tiered";
    options_.create_if_missing = true;
    options_.env = env_.get();
    date_tiered_db_.reset(nullptr);
    DestroyDB(dbname_, Options());
  }

  ~DateTieredTest() {
    CloseDateTieredDB();
    DestroyDB(dbname_, Options());
  }

  void OpenDateTieredDB(int64_t ttl, int64_t column_family_interval,
                        bool read_only = false) {
    ASSERT_TRUE(date_tiered_db_.get() == nullptr);
    DateTieredDB* date_tiered_db = nullptr;
    ASSERT_OK(DateTieredDB::Open(options_, dbname_, &date_tiered_db, ttl,
                                 column_family_interval, read_only));
    date_tiered_db_.reset(date_tiered_db);
  }

  void CloseDateTieredDB() { date_tiered_db_.reset(nullptr); }

  Status AppendTimestamp(std::string* key) {
    char ts[8];
    int bytes_to_fill = 8;
    int64_t timestamp_value = 0;
    Status s = env_->GetCurrentTime(&timestamp_value);
    if (!s.ok()) {
      return s;
    }
    if (port::kLittleEndian) {
      for (int i = 0; i < bytes_to_fill; ++i) {
        ts[i] = (timestamp_value >> ((bytes_to_fill - i - 1) << 3)) & 0xFF;
      }
    } else {
      memcpy(ts, static_cast<void*>(&timestamp_value), bytes_to_fill);
    }
    key->append(ts, 8);
    return Status::OK();
  }

//填充并返回千伏图
  void MakeKVMap(int64_t num_entries, KVMap* kvmap) {
    kvmap->clear();
    int digits = 1;
    for (int64_t dummy = num_entries; dummy /= 10; ++digits) {
    }
    int digits_in_i = 1;
    for (int64_t i = 0; i < num_entries; i++) {
      std::string key = "key";
      std::string value = "value";
      if (i % 10 == 0) {
        digits_in_i++;
      }
      for (int j = digits_in_i; j < digits; j++) {
        key.append("0");
        value.append("0");
      }
      AppendNumberTo(&key, i);
      AppendNumberTo(&value, i);
      ASSERT_OK(AppendTimestamp(&key));
      (*kvmap)[key] = value;
    }
//检查所有插入完成
    ASSERT_EQ(num_entries, static_cast<int64_t>(kvmap->size()));
  }

  size_t GetColumnFamilyCount() {
    DBOptions db_options(options_);
    std::vector<std::string> cf;
    DB::ListColumnFamilies(db_options, dbname_, &cf);
    return cf.size();
  }

  void Sleep(int64_t sleep_time) { env_->Sleep(sleep_time); }

  static const int64_t kSampleSize_ = 100;
  std::string dbname_;
  std::unique_ptr<DateTieredDB> date_tiered_db_;
  std::unique_ptr<SpecialTimeEnv> env_;
  KVMap kvmap_;

 private:
  Options options_;
  KVMap::iterator kv_it_;
  const std::string kNewValue_ = "new_value";
  unique_ptr<CompactionFilter> test_comp_filter_;
};

//放置一组值并在TTL期间使用GET检查其存在性
TEST_F(DateTieredTest, KeyLifeCycle) {
  WriteOptions wopts;
  ReadOptions ropts;

//t=0，打开数据库并插入数据
  OpenDateTieredDB(2, 2);
  ASSERT_TRUE(date_tiered_db_.get() != nullptr);

//创建要插入的键值对
  KVMap map_insert;
  MakeKVMap(kSampleSize_, &map_insert);

//将数据放入数据库
  for (auto& kv : map_insert) {
    ASSERT_OK(date_tiered_db_->Put(wopts, kv.first, kv.second));
  }

  Sleep(1);
//t=1，键仍应驻留在数据库中
  for (auto& kv : map_insert) {
    std::string value;
    ASSERT_OK(date_tiered_db_->Get(ropts, kv.first, &value));
    ASSERT_EQ(value, kv.second);
  }

  Sleep(1);
//t=2，不应检索密钥
  for (auto& kv : map_insert) {
    std::string value;
    auto s = date_tiered_db_->Get(ropts, kv.first, &value);
    ASSERT_TRUE(s.IsNotFound());
  }

  CloseDateTieredDB();
}

TEST_F(DateTieredTest, DeleteTest) {
  WriteOptions wopts;
  ReadOptions ropts;

//t=0，打开数据库并插入数据
  OpenDateTieredDB(2, 2);
  ASSERT_TRUE(date_tiered_db_.get() != nullptr);

//创建要插入的键值对
  KVMap map_insert;
  MakeKVMap(kSampleSize_, &map_insert);

//将数据放入数据库
  for (auto& kv : map_insert) {
    ASSERT_OK(date_tiered_db_->Put(wopts, kv.first, kv.second));
  }

  Sleep(1);
//删除未过时的密钥
  for (auto& kv : map_insert) {
    ASSERT_OK(date_tiered_db_->Delete(wopts, kv.first));
  }

//找不到密钥
  for (auto& kv : map_insert) {
    std::string value;
    auto s = date_tiered_db_->Get(ropts, kv.first, &value);
    ASSERT_TRUE(s.IsNotFound());
  }
}

TEST_F(DateTieredTest, KeyMayExistTest) {
  WriteOptions wopts;
  ReadOptions ropts;

//t=0，打开数据库并插入数据
  OpenDateTieredDB(2, 2);
  ASSERT_TRUE(date_tiered_db_.get() != nullptr);

//创建要插入的键值对
  KVMap map_insert;
  MakeKVMap(kSampleSize_, &map_insert);

//将数据放入数据库
  for (auto& kv : map_insert) {
    ASSERT_OK(date_tiered_db_->Put(wopts, kv.first, kv.second));
  }

  Sleep(1);
//t=1，键仍应驻留在数据库中
  for (auto& kv : map_insert) {
    std::string value;
    ASSERT_TRUE(date_tiered_db_->KeyMayExist(ropts, kv.first, &value));
    ASSERT_EQ(value, kv.second);
  }
}

//数据库打开和关闭不应影响
TEST_F(DateTieredTest, MultiOpen) {
  WriteOptions wopts;
  ReadOptions ropts;

//t=0，打开数据库并插入数据
  OpenDateTieredDB(4, 4);
  ASSERT_TRUE(date_tiered_db_.get() != nullptr);

//创建要插入的键值对
  KVMap map_insert;
  MakeKVMap(kSampleSize_, &map_insert);

//将数据放入数据库
  for (auto& kv : map_insert) {
    ASSERT_OK(date_tiered_db_->Put(wopts, kv.first, kv.second));
  }
  CloseDateTieredDB();

  Sleep(1);
  OpenDateTieredDB(2, 2);
//t=1，键仍应驻留在数据库中
  for (auto& kv : map_insert) {
    std::string value;
    ASSERT_OK(date_tiered_db_->Get(ropts, kv.first, &value));
    ASSERT_EQ(value, kv.second);
  }

  Sleep(1);
//t=2，不应检索密钥
  for (auto& kv : map_insert) {
    std::string value;
    auto s = date_tiered_db_->Get(ropts, kv.first, &value);
    ASSERT_TRUE(s.IsNotFound());
  }

  CloseDateTieredDB();
}

//如果put（）中的键已过时，则不应将数据写入数据库。
TEST_F(DateTieredTest, InsertObsoleteDate) {
  WriteOptions wopts;
  ReadOptions ropts;

//t=0，打开数据库并插入数据
  OpenDateTieredDB(2, 2);
  ASSERT_TRUE(date_tiered_db_.get() != nullptr);

//创建要插入的键值对
  KVMap map_insert;
  MakeKVMap(kSampleSize_, &map_insert);

  Sleep(2);
//t=2，放入数据库的键已经过时
//将数据放入数据库。操作不应返回OK
  for (auto& kv : map_insert) {
    auto s = date_tiered_db_->Put(wopts, kv.first, kv.second);
    ASSERT_TRUE(s.IsInvalidArgument());
  }

//在数据库中找不到数据
  for (auto& kv : map_insert) {
    std::string value;
    auto s = date_tiered_db_->Get(ropts, kv.first, &value);
    ASSERT_TRUE(s.IsNotFound());
  }

  CloseDateTieredDB();
}

//通过更新和检查一组kv的时间戳来重置它们
//未根据旧时间戳删除
TEST_F(DateTieredTest, ColumnFamilyCounts) {
  WriteOptions wopts;
  ReadOptions ropts;

//t=0，打开数据库并插入数据
  OpenDateTieredDB(4, 2);
  ASSERT_TRUE(date_tiered_db_.get() != nullptr);
//仅限默认列族
  ASSERT_EQ(1, GetColumnFamilyCount());

//创建要插入的键值对
  KVMap map_insert;
  MakeKVMap(kSampleSize_, &map_insert);
  for (auto& kv : map_insert) {
    ASSERT_OK(date_tiered_db_->Put(wopts, kv.first, kv.second));
  }
//创建时间序列列族
  ASSERT_EQ(2, GetColumnFamilyCount());

  Sleep(2);
  KVMap map_insert2;
  MakeKVMap(kSampleSize_, &map_insert2);
  for (auto& kv : map_insert2) {
    ASSERT_OK(date_tiered_db_->Put(wopts, kv.first, kv.second));
  }
//创建另一个时间序列列族
  ASSERT_EQ(3, GetColumnFamilyCount());

  Sleep(4);

//在数据库中找不到数据
  for (auto& kv : map_insert) {
    std::string value;
    auto s = date_tiered_db_->Get(ropts, kv.first, &value);
    ASSERT_TRUE(s.IsNotFound());
  }

//显式删除过时的列族
  date_tiered_db_->DropObsoleteColumnFamilies();

//从数据库中删除第一个列族
  ASSERT_EQ(2, GetColumnFamilyCount());

  CloseDateTieredDB();
}

//在TTL期间放置一组值并使用迭代器检查其存在性
TEST_F(DateTieredTest, IteratorLifeCycle) {
  WriteOptions wopts;
  ReadOptions ropts;

//t=0，打开数据库并插入数据
  OpenDateTieredDB(2, 2);
  ASSERT_TRUE(date_tiered_db_.get() != nullptr);

//创建要插入的键值对
  KVMap map_insert;
  MakeKVMap(kSampleSize_, &map_insert);
  Iterator* dbiter;

//将数据放入数据库
  for (auto& kv : map_insert) {
    ASSERT_OK(date_tiered_db_->Put(wopts, kv.first, kv.second));
  }

  Sleep(1);
  ASSERT_EQ(2, GetColumnFamilyCount());
//t=1，键仍应驻留在数据库中
  dbiter = date_tiered_db_->NewIterator(ropts);
  dbiter->SeekToFirst();
  for (auto& kv : map_insert) {
    ASSERT_TRUE(dbiter->Valid());
    ASSERT_EQ(0, dbiter->value().compare(kv.second));
    dbiter->Next();
  }
  delete dbiter;

  Sleep(4);
//t=5，不应检索密钥
  for (auto& kv : map_insert) {
    std::string value;
    auto s = date_tiered_db_->Get(ropts, kv.first, &value);
    ASSERT_TRUE(s.IsNotFound());
  }

//显式删除过时的列族
  date_tiered_db_->DropObsoleteColumnFamilies();

//仅限默认列族
  ASSERT_EQ(1, GetColumnFamilyCount());

//空迭代器
  dbiter = date_tiered_db_->NewIterator(ropts);
  dbiter->Seek(map_insert.begin()->first);
  ASSERT_FALSE(dbiter->Valid());
  delete dbiter;

  CloseDateTieredDB();
}

//迭代器应该能够合并来自多个列族的数据
TEST_F(DateTieredTest, IteratorMerge) {
  WriteOptions wopts;
  ReadOptions ropts;

//t=0，打开数据库并插入数据
  OpenDateTieredDB(4, 2);
  ASSERT_TRUE(date_tiered_db_.get() != nullptr);

  Iterator* dbiter;

//将数据放入数据库
  KVMap map_insert1;
  MakeKVMap(kSampleSize_, &map_insert1);
  for (auto& kv : map_insert1) {
    ASSERT_OK(date_tiered_db_->Put(wopts, kv.first, kv.second));
  }
  ASSERT_EQ(2, GetColumnFamilyCount());

  Sleep(2);
//增加数据
  KVMap map_insert2;
  MakeKVMap(kSampleSize_, &map_insert2);
  for (auto& kv : map_insert2) {
    ASSERT_OK(date_tiered_db_->Put(wopts, kv.first, kv.second));
  }
//时间序列数据的多列族
  ASSERT_EQ(3, GetColumnFamilyCount());

//迭代器应该能够合并来自不同列族的数据
  dbiter = date_tiered_db_->NewIterator(ropts);
  dbiter->SeekToFirst();
  KVMap::iterator iter1 = map_insert1.begin();
  KVMap::iterator iter2 = map_insert2.begin();
  for (; iter1 != map_insert1.end() && iter2 != map_insert2.end();
       iter1++, iter2++) {
    ASSERT_TRUE(dbiter->Valid());
    ASSERT_EQ(0, dbiter->value().compare(iter1->second));
    dbiter->Next();

    ASSERT_TRUE(dbiter->Valid());
    ASSERT_EQ(0, dbiter->value().compare(iter2->second));
    dbiter->Next();
  }
  delete dbiter;

  CloseDateTieredDB();
}

}  //命名空间rocksdb

//RocksDB周围日期数据库的黑盒测试
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else
#include <stdio.h>

int main(int argc, char** argv) {
  fprintf(stderr, "SKIPPED as DateTieredDB is not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //！摇滚乐
