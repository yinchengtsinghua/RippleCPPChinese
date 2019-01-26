
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

#include "rocksdb/slice_transform.h"

#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/statistics.h"
#include "rocksdb/table.h"
#include "util/testharness.h"

namespace rocksdb {

class SliceTransformTest : public testing::Test {};

TEST_F(SliceTransformTest, CapPrefixTransform) {
  std::string s;
  s = "abcdefge";

  unique_ptr<const SliceTransform> transform;

  transform.reset(NewCappedPrefixTransform(6));
  ASSERT_EQ(transform->Transform(s).ToString(), "abcdef");
  ASSERT_TRUE(transform->SameResultWhenAppended("123456"));
  ASSERT_TRUE(transform->SameResultWhenAppended("1234567"));
  ASSERT_TRUE(!transform->SameResultWhenAppended("12345"));

  transform.reset(NewCappedPrefixTransform(8));
  ASSERT_EQ(transform->Transform(s).ToString(), "abcdefge");

  transform.reset(NewCappedPrefixTransform(10));
  ASSERT_EQ(transform->Transform(s).ToString(), "abcdefge");

  transform.reset(NewCappedPrefixTransform(0));
  ASSERT_EQ(transform->Transform(s).ToString(), "");

  transform.reset(NewCappedPrefixTransform(0));
  ASSERT_EQ(transform->Transform("").ToString(), "");
}

class SliceTransformDBTest : public testing::Test {
 private:
  std::string dbname_;
  Env* env_;
  DB* db_;

 public:
  SliceTransformDBTest() : env_(Env::Default()), db_(nullptr) {
    dbname_ = test::TmpDir() + "/slice_transform_db_test";
    EXPECT_OK(DestroyDB(dbname_, last_options_));
  }

  ~SliceTransformDBTest() {
    delete db_;
    EXPECT_OK(DestroyDB(dbname_, last_options_));
  }

  DB* db() { return db_; }

//返回当前选项配置。
  Options* GetOptions() { return &last_options_; }

  void DestroyAndReopen() {
//使用最后一个选项销毁
    Destroy();
    ASSERT_OK(TryReopen());
  }

  void Destroy() {
    delete db_;
    db_ = nullptr;
    ASSERT_OK(DestroyDB(dbname_, last_options_));
  }

  Status TryReopen() {
    delete db_;
    db_ = nullptr;
    last_options_.create_if_missing = true;

    return DB::Open(last_options_, dbname_, &db_);
  }

  Options last_options_;
};

namespace {
uint64_t TestGetTickerCount(const Options& options, Tickers ticker_type) {
  return options.statistics->getTickerCount(ticker_type);
}
}  //命名空间

TEST_F(SliceTransformDBTest, CapPrefix) {
  last_options_.prefix_extractor.reset(NewCappedPrefixTransform(8));
  last_options_.statistics = rocksdb::CreateDBStatistics();
  BlockBasedTableOptions bbto;
  bbto.filter_policy.reset(NewBloomFilterPolicy(10, false));
  bbto.whole_key_filtering = false;
  last_options_.table_factory.reset(NewBlockBasedTableFactory(bbto));
  ASSERT_OK(TryReopen());

  ReadOptions ro;
  FlushOptions fo;
  WriteOptions wo;

  ASSERT_OK(db()->Put(wo, "barbarbar", "foo"));
  ASSERT_OK(db()->Put(wo, "barbarbar2", "foo2"));
  ASSERT_OK(db()->Put(wo, "foo", "bar"));
  ASSERT_OK(db()->Put(wo, "foo3", "bar3"));
  ASSERT_OK(db()->Flush(fo));

  unique_ptr<Iterator> iter(db()->NewIterator(ro));

  iter->Seek("foo");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ(iter->value().ToString(), "bar");
  ASSERT_EQ(TestGetTickerCount(last_options_, BLOOM_FILTER_PREFIX_USEFUL), 0U);

  iter->Seek("foo2");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(!iter->Valid());
  ASSERT_EQ(TestGetTickerCount(last_options_, BLOOM_FILTER_PREFIX_USEFUL), 1U);

  iter->Seek("barbarbar");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ(iter->value().ToString(), "foo");
  ASSERT_EQ(TestGetTickerCount(last_options_, BLOOM_FILTER_PREFIX_USEFUL), 1U);

  iter->Seek("barfoofoo");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(!iter->Valid());
  ASSERT_EQ(TestGetTickerCount(last_options_, BLOOM_FILTER_PREFIX_USEFUL), 2U);

  iter->Seek("foobarbar");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(!iter->Valid());
  ASSERT_EQ(TestGetTickerCount(last_options_, BLOOM_FILTER_PREFIX_USEFUL), 3U);
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
