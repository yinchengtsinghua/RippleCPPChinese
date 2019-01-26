
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2017至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#include <iostream>
#include "rocksdb/db.h"
#include "db/db_impl.h"
#include "rocksdb/merge_operator.h"
#include "rocksdb/utilities/db_ttl.h"
#include "util/testharness.h"
#include "util/random.h"
#include "utilities/merge_operators.h"
#include "utilities/cassandra/cassandra_compaction_filter.h"
#include "utilities/cassandra/merge_operator.h"
#include "utilities/cassandra/test_utils.h"

using namespace rocksdb;

namespace rocksdb {
namespace cassandra {

//文件系统上数据库的路径
const std::string kDbName = test::TmpDir() + "/cassandra_functional_test";

class CassandraStore {
 public:
  explicit CassandraStore(std::shared_ptr<DB> db)
      : db_(db),
        merge_option_(),
        get_option_() {
    assert(db);
  }

  bool Append(const std::string& key, const RowValue& val){
    std::string result;
    val.Serialize(&result);
    Slice valSlice(result.data(), result.size());
    auto s = db_->Merge(merge_option_, key, valSlice);

    if (s.ok()) {
      return true;
    } else {
      std::cerr << "ERROR " << s.ToString() << std::endl;
      return false;
    }
  }

  void Flush() {
    dbfull()->TEST_FlushMemTable();
    dbfull()->TEST_WaitForCompact();
  }

  void Compact() {
    dbfull()->TEST_CompactRange(
      0, nullptr, nullptr, db_->DefaultColumnFamily());
  }

  std::tuple<bool, RowValue> Get(const std::string& key){
    std::string result;
    auto s = db_->Get(get_option_, key, &result);

    if (s.ok()) {
      return std::make_tuple(true,
                             RowValue::Deserialize(result.data(),
                                                   result.size()));
    }

    if (!s.IsNotFound()) {
      std::cerr << "ERROR " << s.ToString() << std::endl;
    }

    return std::make_tuple(false, RowValue(0, 0));
  }

 private:
  std::shared_ptr<DB> db_;
  WriteOptions merge_option_;
  ReadOptions get_option_;

  DBImpl* dbfull() { return reinterpret_cast<DBImpl*>(db_.get()); }

};

class TestCompactionFilterFactory : public CompactionFilterFactory {
public:
  explicit TestCompactionFilterFactory(bool purge_ttl_on_expiration)
    : purge_ttl_on_expiration_(purge_ttl_on_expiration) {}

  virtual std::unique_ptr<CompactionFilter> CreateCompactionFilter(
      const CompactionFilter::Context& context) override {
    return unique_ptr<CompactionFilter>(new CassandraCompactionFilter(purge_ttl_on_expiration_));
  }

  virtual const char* Name() const override {
    return "TestCompactionFilterFactory";
  }

private:
  bool purge_ttl_on_expiration_;
};


//单元测试类
class CassandraFunctionalTest : public testing::Test {
public:
  CassandraFunctionalTest() {
DestroyDB(kDbName, Options());    //用新的数据库开始每个测试
  }

  std::shared_ptr<DB> OpenDb() {
    DB* db;
    Options options;
    options.create_if_missing = true;
    options.merge_operator.reset(new CassandraValueMergeOperator());
    auto* cf_factory = new TestCompactionFilterFactory(purge_ttl_on_expiration_);
    options.compaction_filter_factory.reset(cf_factory);
    EXPECT_OK(DB::Open(options, kDbName, &db));
    return std::shared_ptr<DB>(db);
  }

  bool purge_ttl_on_expiration_ = false;
};

//测试用例从这里开始

TEST_F(CassandraFunctionalTest, SimpleMergeTest) {
  CassandraStore store(OpenDb());

  store.Append("k1", CreateTestRowValue({
    std::make_tuple(kTombstone, 0, 5),
    std::make_tuple(kColumn, 1, 8),
    std::make_tuple(kExpiringColumn, 2, 5),
  }));
  store.Append("k1",CreateTestRowValue({
    std::make_tuple(kColumn, 0, 2),
    std::make_tuple(kExpiringColumn, 1, 5),
    std::make_tuple(kTombstone, 2, 7),
    std::make_tuple(kExpiringColumn, 7, 17),
  }));
  store.Append("k1", CreateTestRowValue({
    std::make_tuple(kExpiringColumn, 0, 6),
    std::make_tuple(kTombstone, 1, 5),
    std::make_tuple(kColumn, 2, 4),
    std::make_tuple(kTombstone, 11, 11),
  }));

  auto ret = store.Get("k1");

  ASSERT_TRUE(std::get<0>(ret));
  RowValue& merged = std::get<1>(ret);
  EXPECT_EQ(merged.columns_.size(), 5);
  VerifyRowValueColumns(merged.columns_, 0, kExpiringColumn, 0, 6);
  VerifyRowValueColumns(merged.columns_, 1, kColumn, 1, 8);
  VerifyRowValueColumns(merged.columns_, 2, kTombstone, 2, 7);
  VerifyRowValueColumns(merged.columns_, 3, kExpiringColumn, 7, 17);
  VerifyRowValueColumns(merged.columns_, 4, kTombstone, 11, 11);
}

TEST_F(CassandraFunctionalTest,
       CompactionShouldConvertExpiredColumnsToTombstone) {
  CassandraStore store(OpenDb());
  int64_t now= time(nullptr);

  store.Append("k1", CreateTestRowValue({
std::make_tuple(kExpiringColumn, 0, ToMicroSeconds(now - kTtl - 20)), //期满
std::make_tuple(kExpiringColumn, 1, ToMicroSeconds(now - kTtl + 10)), //未过期
    std::make_tuple(kTombstone, 3, ToMicroSeconds(now))
  }));

  store.Flush();

  store.Append("k1",CreateTestRowValue({
std::make_tuple(kExpiringColumn, 0, ToMicroSeconds(now - kTtl - 10)), //期满
    std::make_tuple(kColumn, 2, ToMicroSeconds(now))
  }));

  store.Flush();
  store.Compact();

  auto ret = store.Get("k1");
  ASSERT_TRUE(std::get<0>(ret));
  RowValue& merged = std::get<1>(ret);
  EXPECT_EQ(merged.columns_.size(), 4);
  VerifyRowValueColumns(merged.columns_, 0, kTombstone, 0, ToMicroSeconds(now - 10));
  VerifyRowValueColumns(merged.columns_, 1, kExpiringColumn, 1, ToMicroSeconds(now - kTtl + 10));
  VerifyRowValueColumns(merged.columns_, 2, kColumn, 2, ToMicroSeconds(now));
  VerifyRowValueColumns(merged.columns_, 3, kTombstone, 3, ToMicroSeconds(now));
}


TEST_F(CassandraFunctionalTest,
       CompactionShouldPurgeExpiredColumnsIfPurgeTtlIsOn) {
  purge_ttl_on_expiration_ = true;
  CassandraStore store(OpenDb());
  int64_t now = time(nullptr);

  store.Append("k1", CreateTestRowValue({
std::make_tuple(kExpiringColumn, 0, ToMicroSeconds(now - kTtl - 20)), //期满
std::make_tuple(kExpiringColumn, 1, ToMicroSeconds(now)), //未过期
    std::make_tuple(kTombstone, 3, ToMicroSeconds(now))
  }));

  store.Flush();

  store.Append("k1",CreateTestRowValue({
std::make_tuple(kExpiringColumn, 0, ToMicroSeconds(now - kTtl - 10)), //期满
    std::make_tuple(kColumn, 2, ToMicroSeconds(now))
  }));

  store.Flush();
  store.Compact();

  auto ret = store.Get("k1");
  ASSERT_TRUE(std::get<0>(ret));
  RowValue& merged = std::get<1>(ret);
  EXPECT_EQ(merged.columns_.size(), 3);
  VerifyRowValueColumns(merged.columns_, 0, kExpiringColumn, 1, ToMicroSeconds(now));
  VerifyRowValueColumns(merged.columns_, 1, kColumn, 2, ToMicroSeconds(now));
  VerifyRowValueColumns(merged.columns_, 2, kTombstone, 3, ToMicroSeconds(now));
}

TEST_F(CassandraFunctionalTest,
       CompactionShouldRemoveRowWhenAllColumnsExpiredIfPurgeTtlIsOn) {
  purge_ttl_on_expiration_ = true;
  CassandraStore store(OpenDb());
  int64_t now = time(nullptr);

  store.Append("k1", CreateTestRowValue({
    std::make_tuple(kExpiringColumn, 0, ToMicroSeconds(now - kTtl - 20)),
    std::make_tuple(kExpiringColumn, 1, ToMicroSeconds(now - kTtl - 20)),
  }));

  store.Flush();

  store.Append("k1",CreateTestRowValue({
    std::make_tuple(kExpiringColumn, 0, ToMicroSeconds(now - kTtl - 10)),
  }));

  store.Flush();
  store.Compact();
  ASSERT_FALSE(std::get<0>(store.Get("k1")));
}

} //命名空间cassandra
} //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
