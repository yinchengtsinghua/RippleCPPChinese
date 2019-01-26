
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

#include <map>
#include <memory>
#include "rocksdb/compaction_filter.h"
#include "rocksdb/utilities/db_ttl.h"
#include "util/string_util.h"
#include "util/testharness.h"
#ifndef OS_WIN
#include <unistd.h>
#endif

namespace rocksdb {

namespace {

typedef std::map<std::string, std::string> KVMap;

enum BatchOperation { OP_PUT = 0, OP_DELETE = 1 };
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

class TtlTest : public testing::Test {
 public:
  TtlTest() {
    env_.reset(new SpecialTimeEnv(Env::Default()));
    dbname_ = test::TmpDir() + "/db_ttl";
    options_.create_if_missing = true;
    options_.env = env_.get();
//确保开始压缩以始终从kvs中除去时间戳
    options_.max_compaction_bytes = 1;
//对于确定性，压实应始终从0级开始。
    db_ttl_ = nullptr;
    DestroyDB(dbname_, Options());
  }

  ~TtlTest() {
    CloseTtl();
    DestroyDB(dbname_, Options());
  }

//当TTL未提供db_ttl_u指针时，使用TTL支持打开数据库
  void OpenTtl() {
    ASSERT_TRUE(db_ttl_ ==
nullptr);  //DB应在再次打开之前关闭
    ASSERT_OK(DBWithTTL::Open(options_, dbname_, &db_ttl_));
  }

//当TTL提供db_ttl_u指针时，使用TTL支持打开数据库
  void OpenTtl(int32_t ttl) {
    ASSERT_TRUE(db_ttl_ == nullptr);
    ASSERT_OK(DBWithTTL::Open(options_, dbname_, &db_ttl_, ttl));
  }

//用testfilter压缩过滤器打开
  void OpenTtlWithTestCompaction(int32_t ttl) {
    options_.compaction_filter_factory =
      std::shared_ptr<CompactionFilterFactory>(
          new TestFilterFactory(kSampleSize_, kNewValue_));
    OpenTtl(ttl);
  }

//以只读模式打开具有TTL支持的数据库
  void OpenReadOnlyTtl(int32_t ttl) {
    ASSERT_TRUE(db_ttl_ == nullptr);
    ASSERT_OK(DBWithTTL::Open(options_, dbname_, &db_ttl_, ttl, true));
  }

  void CloseTtl() {
    delete db_ttl_;
    db_ttl_ = nullptr;
  }

//填充并返回千伏图
  void MakeKVMap(int64_t num_entries) {
    kvmap_.clear();
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
      for(int j = digits_in_i; j < digits; j++) {
        key.append("0");
        value.append("0");
      }
      AppendNumberTo(&key, i);
      AppendNumberTo(&value, i);
      kvmap_[key] = value;
    }
    ASSERT_EQ(static_cast<int64_t>(kvmap_.size()),
num_entries);  //检查所有插入完成
  }

//使用kvmap_uu中的键val和'write's it'生成一个写入批处理
  void MakePutWriteBatch(const BatchOperation* batch_ops, int64_t num_ops) {
    ASSERT_LE(num_ops, static_cast<int64_t>(kvmap_.size()));
    static WriteOptions wopts;
    static FlushOptions flush_opts;
    WriteBatch batch;
    kv_it_ = kvmap_.begin();
    for (int64_t i = 0; i < num_ops && kv_it_ != kvmap_.end(); i++, ++kv_it_) {
      switch (batch_ops[i]) {
        case OP_PUT:
          batch.Put(kv_it_->first, kv_it_->second);
          break;
        case OP_DELETE:
          batch.Delete(kv_it_->first);
          break;
        default:
          FAIL();
      }
    }
    db_ttl_->Write(wopts, &batch);
    db_ttl_->Flush(flush_opts);
  }

//将从kvmap开始的num_条目放入数据库
  void PutValues(int64_t start_pos_map, int64_t num_entries, bool flush = true,
                 ColumnFamilyHandle* cf = nullptr) {
    ASSERT_TRUE(db_ttl_);
    ASSERT_LE(start_pos_map + num_entries, static_cast<int64_t>(kvmap_.size()));
    static WriteOptions wopts;
    static FlushOptions flush_opts;
    kv_it_ = kvmap_.begin();
    advance(kv_it_, start_pos_map);
    for (int64_t i = 0; kv_it_ != kvmap_.end() && i < num_entries;
         i++, ++kv_it_) {
      ASSERT_OK(cf == nullptr
                    ? db_ttl_->Put(wopts, kv_it_->first, kv_it_->second)
                    : db_ttl_->Put(wopts, cf, kv_it_->first, kv_it_->second));
    }
//在末尾放置一个模拟的kv，因为compactionfilter不会删除最后一个键
    ASSERT_OK(cf == nullptr ? db_ttl_->Put(wopts, "keymock", "valuemock")
                            : db_ttl_->Put(wopts, cf, "keymock", "valuemock"));
    if (flush) {
      if (cf == nullptr) {
        db_ttl_->Flush(flush_opts);
      } else {
        db_ttl_->Flush(flush_opts, cf);
      }
    }
  }

//运行手动压缩
  void ManualCompact(ColumnFamilyHandle* cf = nullptr) {
    if (cf == nullptr) {
      db_ttl_->CompactRange(CompactRangeOptions(), nullptr, nullptr);
    } else {
      db_ttl_->CompactRange(CompactRangeOptions(), cf, nullptr, nullptr);
    }
  }

//检查整个kvmap，使用keymayexist返回正确的值
  void SimpleKeyMayExistCheck() {
    static ReadOptions ropts;
    bool value_found;
    std::string val;
    for(auto &kv : kvmap_) {
      bool ret = db_ttl_->KeyMayExist(ropts, kv.first, &val, &value_found);
      if (ret == false || value_found == false) {
        fprintf(stderr, "KeyMayExist could not find key=%s in the database but"
                        " should have\n", kv.first.c_str());
        FAIL();
      } else if (val.compare(kv.second) != 0) {
        fprintf(stderr, " value for key=%s present in database is %s but"
                        " should be %s\n", kv.first.c_str(), val.c_str(),
                        kv.second.c_str());
        FAIL();
      }
    }
  }

//使用multiget检查整个kvmap返回正确值
  void SimpleMultiGetTest() {
    static ReadOptions ropts;
    std::vector<Slice> keys;
    std::vector<std::string> values;

    for (auto& kv : kvmap_) {
      keys.emplace_back(kv.first);
    }

    auto statuses = db_ttl_->MultiGet(ropts, keys, &values);
    size_t i = 0;
    for (auto& kv : kvmap_) {
      ASSERT_OK(statuses[i]);
      ASSERT_EQ(values[i], kv.second);
      ++i;
    }
  }

//为slp-tim睡觉，然后手动压缩
//检查范围从数据库中kvmap的st_pos开始
//如果check为true和false，则gets应返回true，否则返回false
//还要检查我们得到的值是否与插入的值相同；并且=KnewValue
//如果试验压实度变化为真
  void SleepCompactCheck(int slp_tim, int64_t st_pos, int64_t span,
                         bool check = true, bool test_compaction_change = false,
                         ColumnFamilyHandle* cf = nullptr) {
    ASSERT_TRUE(db_ttl_);

    env_->Sleep(slp_tim);
    ManualCompact(cf);
    static ReadOptions ropts;
    kv_it_ = kvmap_.begin();
    advance(kv_it_, st_pos);
    std::string v;
    for (int64_t i = 0; kv_it_ != kvmap_.end() && i < span; i++, ++kv_it_) {
      Status s = (cf == nullptr) ? db_ttl_->Get(ropts, kv_it_->first, &v)
                                 : db_ttl_->Get(ropts, cf, kv_it_->first, &v);
      if (s.ok() != check) {
        fprintf(stderr, "key=%s ", kv_it_->first.c_str());
        if (!s.ok()) {
          fprintf(stderr, "is absent from db but was expected to be present\n");
        } else {
          fprintf(stderr, "is present in db but was expected to be absent\n");
        }
        FAIL();
      } else if (s.ok()) {
          if (test_compaction_change && v.compare(kNewValue_) != 0) {
            fprintf(stderr, " value for key=%s present in database is %s but "
                            " should be %s\n", kv_it_->first.c_str(), v.c_str(),
                            kNewValue_.c_str());
            FAIL();
          } else if (!test_compaction_change && v.compare(kv_it_->second) !=0) {
            fprintf(stderr, " value for key=%s present in database is %s but "
                            " should be %s\n", kv_it_->first.c_str(), v.c_str(),
                            kv_it_->second.c_str());
            FAIL();
          }
      }
    }
  }

//类似于sleepCompactCheck，但使用ttLiterator从数据库读取
  void SleepCompactCheckIter(int slp, int st_pos, int64_t span,
                             bool check = true) {
    ASSERT_TRUE(db_ttl_);
    env_->Sleep(slp);
    ManualCompact();
    static ReadOptions ropts;
    Iterator *dbiter = db_ttl_->NewIterator(ropts);
    kv_it_ = kvmap_.begin();
    advance(kv_it_, st_pos);

    dbiter->Seek(kv_it_->first);
    if (!check) {
      if (dbiter->Valid()) {
        ASSERT_NE(dbiter->value().compare(kv_it_->second), 0);
      }
} else {  //DBITER应该已经找到kvmap_uu[st_pos]
      for (int64_t i = st_pos; kv_it_ != kvmap_.end() && i < st_pos + span;
           i++, ++kv_it_) {
        ASSERT_TRUE(dbiter->Valid());
        ASSERT_EQ(dbiter->value().compare(kv_it_->second), 0);
        dbiter->Next();
      }
    }
    delete dbiter;
  }

  class TestFilter : public CompactionFilter {
   public:
    TestFilter(const int64_t kSampleSize, const std::string& kNewValue)
      : kSampleSize_(kSampleSize),
        kNewValue_(kNewValue) {
    }

//使用“key<number>形式的键
//如果键末尾的数字在[0，ksampleSize_3]中，则删除键，
//在[ksampleSize_3，2*ksampleSize_3]中保留键，
//如果在[2*ksamplesize_3，ksamplesize_u]中，则更改值
//例如，Ksamplesize_u=6.放置：键0-1…保留：键2-3…更改：键4-5…
    virtual bool Filter(int level, const Slice& key,
                        const Slice& value, std::string* new_value,
                        bool* value_changed) const override {
      assert(new_value != nullptr);

      std::string search_str = "0123456789";
      std::string key_string = key.ToString();
      size_t pos = key_string.find_first_of(search_str);
      int num_key_end;
      if (pos != std::string::npos) {
        auto key_substr = key_string.substr(pos, key.size() - pos);
#ifndef CYGWIN
        num_key_end = std::stoi(key_substr);
#else
        num_key_end = std::strtol(key_substr.c_str(), 0, 10);
#endif

      } else {
return false; //保持键与“key<number>格式不匹配
      }

      int64_t partition = kSampleSize_ / 3;
      if (num_key_end < partition) {
        return true;
      } else if (num_key_end < partition * 2) {
        return false;
      } else {
        *new_value = kNewValue_;
        *value_changed = true;
        return false;
      }
    }

    virtual const char* Name() const override {
      return "TestFilter";
    }

   private:
    const int64_t kSampleSize_;
    const std::string kNewValue_;
  };

  class TestFilterFactory : public CompactionFilterFactory {
    public:
      TestFilterFactory(const int64_t kSampleSize, const std::string& kNewValue)
        : kSampleSize_(kSampleSize),
          kNewValue_(kNewValue) {
      }

      virtual std::unique_ptr<CompactionFilter> CreateCompactionFilter(
          const CompactionFilter::Context& context) override {
        return std::unique_ptr<CompactionFilter>(
            new TestFilter(kSampleSize_, kNewValue_));
      }

      virtual const char* Name() const override {
        return "TestFilterFactory";
      }

    private:
      const int64_t kSampleSize_;
      const std::string kNewValue_;
  };


//仔细选择，以便在1秒的缓冲区内完成放置、获取和压缩。
  static const int64_t kSampleSize_ = 100;
  std::string dbname_;
  DBWithTTL* db_ttl_;
  unique_ptr<SpecialTimeEnv> env_;

 private:
  Options options_;
  KVMap kvmap_;
  KVMap::iterator kv_it_;
  const std::string kNewValue_ = "new_value";
  unique_ptr<CompactionFilter> test_comp_filter_;
}; //类TTLTEST

//如果TTL为非正或未提供，则行为为TTL=无穷大
//此测试使用此类默认行为打开数据库3次，并插入
//每次都有大量的kv。所有kv都应该在db中累积到最后
//将提供的样本大小在boundary1和boundary2上分为3组
TEST_F(TtlTest, NoEffect) {
  MakeKVMap(kSampleSize_);
  int64_t boundary1 = kSampleSize_ / 3;
  int64_t boundary2 = 2 * boundary1;

  OpenTtl();
PutValues(0, boundary1);                       //t=0:set1从未删除
SleepCompactCheck(1, 0, boundary1);            //t=1:set1仍在那里
  CloseTtl();

  OpenTtl(0);
PutValues(boundary1, boundary2 - boundary1);   //t=1:set2从未删除
SleepCompactCheck(1, 0, boundary2);            //T=2：设置1和2仍然存在
  CloseTtl();

  OpenTtl(-1);
PutValues(boundary2, kSampleSize_ - boundary2); //t=3:set3从未删除
SleepCompactCheck(1, 0, kSampleSize_, true);    //t=4：仍有1、2、3组
  CloseTtl();
}

//放置一组值并在TTL期间使用GET检查其存在性
TEST_F(TtlTest, PresentDuringTTL) {
  MakeKVMap(kSampleSize_);

OpenTtl(2);                                 //t=0:打开数据库，TTL=2
PutValues(0, kSampleSize_);                  //t=0：插入set1。t＝2删除
SleepCompactCheck(1, 0, kSampleSize_, true); //t=1:set1应该还在那里
  CloseTtl();
}

//放置一组值并使用get-after-ttl检查其是否存在
TEST_F(TtlTest, AbsentAfterTTL) {
  MakeKVMap(kSampleSize_);

OpenTtl(1);                                  //t=0:打开数据库，TTL=2
PutValues(0, kSampleSize_);                  //t=0：插入set1。t＝2删除
SleepCompactCheck(2, 0, kSampleSize_, false); //t=2:set1不应该在那里
  CloseTtl();
}

//通过更新和检查一组kv的时间戳来重置它们
//未根据旧时间戳删除
TEST_F(TtlTest, ResetTimestamp) {
  MakeKVMap(kSampleSize_);

  OpenTtl(3);
PutValues(0, kSampleSize_);            //t=0：插入set1。t＝3删除
env_->Sleep(2);                        //t＝2
PutValues(0, kSampleSize_);            //t=2：插入set1。t＝5删除
SleepCompactCheck(2, 0, kSampleSize_); //t=4:set1应该还在那里
  CloseTtl();
}

//类似于presentduringttl，但使用迭代器
TEST_F(TtlTest, IterPresentDuringTTL) {
  MakeKVMap(kSampleSize_);

  OpenTtl(2);
PutValues(0, kSampleSize_);                 //T＝0：插入。t＝2删除
SleepCompactCheckIter(1, 0, kSampleSize_);  //T=1：应该有一套
  CloseTtl();
}

//类似于absentafterttl，但使用迭代器
TEST_F(TtlTest, IterAbsentAfterTTL) {
  MakeKVMap(kSampleSize_);

  OpenTtl(1);
PutValues(0, kSampleSize_);                      //T＝0：插入。t＝1删除
SleepCompactCheckIter(2, 0, kSampleSize_, false); //T=2：不应该在那里
  CloseTtl();
}

//使用相同的TTL多次打开同一数据库时检查是否存在
//注意：第二个打开将打开同一个数据库
TEST_F(TtlTest, MultiOpenSamePresent) {
  MakeKVMap(kSampleSize_);

  OpenTtl(2);
PutValues(0, kSampleSize_);                   //T＝0：插入。t＝2删除
  CloseTtl();

OpenTtl(2);                                  //t＝0。t＝2删除
SleepCompactCheck(1, 0, kSampleSize_);        //T=1：应该有一套
  CloseTtl();
}

//使用相同的TTL多次打开同一个数据库时检查是否存在。
//注意：第二个打开将打开同一个数据库
TEST_F(TtlTest, MultiOpenSameAbsent) {
  MakeKVMap(kSampleSize_);

  OpenTtl(1);
PutValues(0, kSampleSize_);                   //T＝0：插入。t＝1删除
  CloseTtl();

OpenTtl(1);                                  //t=0.删除t=1
SleepCompactCheck(2, 0, kSampleSize_, false); //t=2:集合不应该存在
  CloseTtl();
}

//使用较大的TTL多次打开同一数据库时检查是否存在
TEST_F(TtlTest, MultiOpenDifferent) {
  MakeKVMap(kSampleSize_);

  OpenTtl(1);
PutValues(0, kSampleSize_);            //T＝0：插入。t＝1删除
  CloseTtl();

OpenTtl(3);                           //t=0：设为删除t=3
SleepCompactCheck(2, 0, kSampleSize_); //T=2：应该有一套
  CloseTtl();
}

//以只读模式检查TTL期间的状态
TEST_F(TtlTest, ReadOnlyPresentForever) {
  MakeKVMap(kSampleSize_);

OpenTtl(1);                                 //T=0：正常打开DB
PutValues(0, kSampleSize_);                  //t=0：插入set1。t＝1删除
  CloseTtl();

  OpenReadOnlyTtl(1);
SleepCompactCheck(2, 0, kSampleSize_);       //t=2:set1应该还在那里
  CloseTtl();
}

//检查WRITEBATCH是否与TTL一起工作良好
//将kvmap_u中的所有kv放到一批中，然后先写入，然后删除前半部分
TEST_F(TtlTest, WriteBatchTest) {
  MakeKVMap(kSampleSize_);
  BatchOperation batch_ops[kSampleSize_];
  for (int i = 0; i < kSampleSize_; i++) {
    batch_ops[i] = OP_PUT;
  }

  OpenTtl(2);
  MakePutWriteBatch(batch_ops, kSampleSize_);
  for (int i = 0; i < kSampleSize_ / 2; i++) {
    batch_ops[i] = OP_DELETE;
  }
  MakePutWriteBatch(batch_ops, kSampleSize_ / 2);
  SleepCompactCheck(0, 0, kSampleSize_ / 2, false);
  SleepCompactCheck(0, kSampleSize_ / 2, kSampleSize_ - kSampleSize_ / 2);
  CloseTtl();
}

//使用TTL逻辑检查用户压缩过滤器的正确性
TEST_F(TtlTest, CompactionFilter) {
  MakeKVMap(kSampleSize_);

  OpenTtlWithTestCompaction(1);
PutValues(0, kSampleSize_);                  //t=0：插入set1。t＝1删除
//t=2:ttl逻辑优先于testfilter:-set1不应该存在
  SleepCompactCheck(2, 0, kSampleSize_, false);
  CloseTtl();

  OpenTtlWithTestCompaction(3);
PutValues(0, kSampleSize_);                   //t=0：插入set1。
  int64_t partition = kSampleSize_ / 3;
SleepCompactCheck(1, 0, partition, false);                  //部分放弃
SleepCompactCheck(0, partition, partition);                 //部分保留
SleepCompactCheck(0, 2 * partition, partition, true, true); //部分改变
  CloseTtl();
}

//插入一些键可能存在的键值，并检查
//返回的值可以
TEST_F(TtlTest, KeyMayExist) {
  MakeKVMap(kSampleSize_);

  OpenTtl();
  PutValues(0, kSampleSize_, false);

  SimpleKeyMayExistCheck();

  CloseTtl();
}

TEST_F(TtlTest, MultiGetTest) {
  MakeKVMap(kSampleSize_);

  OpenTtl();
  PutValues(0, kSampleSize_, false);

  SimpleMultiGetTest();

  CloseTtl();
}

TEST_F(TtlTest, ColumnFamiliesTest) {
  DB* db;
  Options options;
  options.create_if_missing = true;
  options.env = env_.get();

  DB::Open(options, dbname_, &db);
  ColumnFamilyHandle* handle;
  ASSERT_OK(db->CreateColumnFamily(ColumnFamilyOptions(options),
                                   "ttl_column_family", &handle));

  delete handle;
  delete db;

  std::vector<ColumnFamilyDescriptor> column_families;
  column_families.push_back(ColumnFamilyDescriptor(
      kDefaultColumnFamilyName, ColumnFamilyOptions(options)));
  column_families.push_back(ColumnFamilyDescriptor(
      "ttl_column_family", ColumnFamilyOptions(options)));

  std::vector<ColumnFamilyHandle*> handles;

  ASSERT_OK(DBWithTTL::Open(DBOptions(options), dbname_, column_families,
                            &handles, &db_ttl_, {3, 5}, false));
  ASSERT_EQ(handles.size(), 2U);
  ColumnFamilyHandle* new_handle;
  ASSERT_OK(db_ttl_->CreateColumnFamilyWithTtl(options, "ttl_column_family_2",
                                               &new_handle, 2));
  handles.push_back(new_handle);

  MakeKVMap(kSampleSize_);
  PutValues(0, kSampleSize_, false, handles[0]);
  PutValues(0, kSampleSize_, false, handles[1]);
  PutValues(0, kSampleSize_, false, handles[2]);

//一秒钟后一切都应该在那里
  SleepCompactCheck(1, 0, kSampleSize_, true, false, handles[0]);
  SleepCompactCheck(0, 0, kSampleSize_, true, false, handles[1]);
  SleepCompactCheck(0, 0, kSampleSize_, true, false, handles[2]);

//只有列族1在4秒后才应激活
  SleepCompactCheck(3, 0, kSampleSize_, false, false, handles[0]);
  SleepCompactCheck(0, 0, kSampleSize_, true, false, handles[1]);
  SleepCompactCheck(0, 0, kSampleSize_, false, false, handles[2]);

//6秒钟后不应该有任何东西
  SleepCompactCheck(2, 0, kSampleSize_, false, false, handles[0]);
  SleepCompactCheck(0, 0, kSampleSize_, false, false, handles[1]);
  SleepCompactCheck(0, 0, kSampleSize_, false, false, handles[2]);

  for (auto h : handles) {
    delete h;
  }
  delete db_ttl_;
  db_ttl_ = nullptr;
}

} //命名空间rocksdb

//rocksdb周围TTL封装器的黑盒测试
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else
#include <stdio.h>

int main(int argc, char** argv) {
  fprintf(stderr, "SKIPPED as DBWithTTL is not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //！摇滚乐
