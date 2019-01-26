
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

#include <algorithm>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "db/db_test_util.h"
#include "port/port.h"
#include "rocksdb/utilities/debug.h"
#include "util/cast_util.h"
#include "util/random.h"
#include "util/string_util.h"
#include "util/sync_point.h"
#include "util/testharness.h"
#include "utilities/blob_db/blob_db.h"
#include "utilities/blob_db/blob_db_impl.h"
#include "utilities/blob_db/blob_index.h"

namespace rocksdb {
namespace blob_db {

class BlobDBTest : public testing::Test {
 public:
  const int kMaxBlobSize = 1 << 14;

  struct BlobRecord {
    std::string key;
    std::string value;
    uint64_t expiration = 0;
  };

  BlobDBTest()
      : dbname_(test::TmpDir() + "/blob_db_test"),
        mock_env_(new MockTimeEnv(Env::Default())),
        blob_db_(nullptr) {
    Status s = DestroyBlobDB(dbname_, Options(), BlobDBOptions());
    assert(s.ok());
  }

  ~BlobDBTest() { Destroy(); }

  Status TryOpen(BlobDBOptions bdb_options = BlobDBOptions(),
                 Options options = Options()) {
    options.create_if_missing = true;
    return BlobDB::Open(options, bdb_options, dbname_, &blob_db_);
  }

  void Open(BlobDBOptions bdb_options = BlobDBOptions(),
            Options options = Options()) {
    ASSERT_OK(TryOpen(bdb_options, options));
  }

  void Reopen(BlobDBOptions bdb_options = BlobDBOptions(),
              Options options = Options()) {
    assert(blob_db_ != nullptr);
    delete blob_db_;
    blob_db_ = nullptr;
    Open(bdb_options, options);
  }

  void Destroy() {
    if (blob_db_) {
      Options options = blob_db_->GetOptions();
      BlobDBOptions bdb_options = blob_db_->GetBlobDBOptions();
      delete blob_db_;
      ASSERT_OK(DestroyBlobDB(dbname_, options, bdb_options));
      blob_db_ = nullptr;
    }
  }

  BlobDBImpl *blob_db_impl() {
    return reinterpret_cast<BlobDBImpl *>(blob_db_);
  }

  Status Put(const Slice &key, const Slice &value) {
    return blob_db_->Put(WriteOptions(), key, value);
  }

  void Delete(const std::string &key,
              std::map<std::string, std::string> *data = nullptr) {
    ASSERT_OK(blob_db_->Delete(WriteOptions(), key));
    if (data != nullptr) {
      data->erase(key);
    }
  }

  Status PutUntil(const Slice &key, const Slice &value, uint64_t expiration) {
    return blob_db_->PutUntil(WriteOptions(), key, value, expiration);
  }

  void PutRandomWithTTL(const std::string &key, uint64_t ttl, Random *rnd,
                        std::map<std::string, std::string> *data = nullptr) {
    int len = rnd->Next() % kMaxBlobSize + 1;
    std::string value = test::RandomHumanReadableString(rnd, len);
    ASSERT_OK(
        blob_db_->PutWithTTL(WriteOptions(), Slice(key), Slice(value), ttl));
    if (data != nullptr) {
      (*data)[key] = value;
    }
  }

  void PutRandomUntil(const std::string &key, uint64_t expiration, Random *rnd,
                      std::map<std::string, std::string> *data = nullptr) {
    int len = rnd->Next() % kMaxBlobSize + 1;
    std::string value = test::RandomHumanReadableString(rnd, len);
    ASSERT_OK(blob_db_->PutUntil(WriteOptions(), Slice(key), Slice(value),
                                 expiration));
    if (data != nullptr) {
      (*data)[key] = value;
    }
  }

  void PutRandom(const std::string &key, Random *rnd,
                 std::map<std::string, std::string> *data = nullptr) {
    PutRandom(blob_db_, key, rnd, data);
  }

  void PutRandom(DB *db, const std::string &key, Random *rnd,
                 std::map<std::string, std::string> *data = nullptr) {
    int len = rnd->Next() % kMaxBlobSize + 1;
    std::string value = test::RandomHumanReadableString(rnd, len);
    ASSERT_OK(db->Put(WriteOptions(), Slice(key), Slice(value)));
    if (data != nullptr) {
      (*data)[key] = value;
    }
  }

  void PutRandomToWriteBatch(
      const std::string &key, Random *rnd, WriteBatch *batch,
      std::map<std::string, std::string> *data = nullptr) {
    int len = rnd->Next() % kMaxBlobSize + 1;
    std::string value = test::RandomHumanReadableString(rnd, len);
    ASSERT_OK(batch->Put(key, value));
    if (data != nullptr) {
      (*data)[key] = value;
    }
  }

//验证blob db是否包含所需的数据以及其他内容。
  void VerifyDB(const std::map<std::string, std::string> &data) {
    VerifyDB(blob_db_, data);
  }

  void VerifyDB(DB *db, const std::map<std::string, std::string> &data) {
//验证正常GET
    auto* cfh = db->DefaultColumnFamily();
    for (auto &p : data) {
      PinnableSlice value_slice;
      ASSERT_OK(db->Get(ReadOptions(), cfh, p.first, &value_slice));
      ASSERT_EQ(p.second, value_slice.ToString());
      std::string value;
      ASSERT_OK(db->Get(ReadOptions(), cfh, p.first, &value));
      ASSERT_EQ(p.second, value);
    }

//验证迭代器
    Iterator *iter = db->NewIterator(ReadOptions());
    iter->SeekToFirst();
    for (auto &p : data) {
      ASSERT_TRUE(iter->Valid());
      ASSERT_EQ(p.first, iter->key().ToString());
      ASSERT_EQ(p.second, iter->value().ToString());
      iter->Next();
    }
    ASSERT_FALSE(iter->Valid());
    ASSERT_OK(iter->status());
    delete iter;
  }

  void VerifyBaseDB(
      const std::map<std::string, KeyVersion> &expected_versions) {
    auto *bdb_impl = static_cast<BlobDBImpl *>(blob_db_);
    DB *db = blob_db_->GetRootDB();
    std::vector<KeyVersion> versions;
    GetAllKeyVersions(db, "", "", &versions);
    ASSERT_EQ(expected_versions.size(), versions.size());
    size_t i = 0;
    for (auto &key_version : expected_versions) {
      const KeyVersion &expected_version = key_version.second;
      ASSERT_EQ(expected_version.user_key, versions[i].user_key);
      ASSERT_EQ(expected_version.sequence, versions[i].sequence);
      ASSERT_EQ(expected_version.type, versions[i].type);
      if (versions[i].type == kTypeValue) {
        ASSERT_EQ(expected_version.value, versions[i].value);
      } else {
        ASSERT_EQ(kTypeBlobIndex, versions[i].type);
        PinnableSlice value;
        ASSERT_OK(bdb_impl->TEST_GetBlobValue(versions[i].user_key,
                                              versions[i].value, &value));
        ASSERT_EQ(expected_version.value, value.ToString());
      }
      i++;
    }
  }

  void InsertBlobs() {
    WriteOptions wo;
    std::string value;

    Random rnd(301);
    for (size_t i = 0; i < 100000; i++) {
      uint64_t ttl = rnd.Next() % 86400;
      PutRandomWithTTL("key" + ToString(i % 500), ttl, &rnd, nullptr);
    }

    for (size_t i = 0; i < 10; i++) {
      Delete("key" + ToString(i % 500));
    }
  }

  const std::string dbname_;
  std::unique_ptr<MockTimeEnv> mock_env_;
  std::shared_ptr<TTLExtractor> ttl_extractor_;
  BlobDB *blob_db_;
};  //类blobdbtest

TEST_F(BlobDBTest, Put) {
  Random rnd(301);
  BlobDBOptions bdb_options;
  bdb_options.min_blob_size = 0;
  bdb_options.disable_background_tasks = true;
  Open(bdb_options);
  std::map<std::string, std::string> data;
  for (size_t i = 0; i < 100; i++) {
    PutRandom("key" + ToString(i), &rnd, &data);
  }
  VerifyDB(data);
}

TEST_F(BlobDBTest, PutWithTTL) {
  Random rnd(301);
  Options options;
  options.env = mock_env_.get();
  BlobDBOptions bdb_options;
  bdb_options.ttl_range_secs = 1000;
  bdb_options.min_blob_size = 0;
  bdb_options.blob_file_size = 256 * 1000 * 1000;
  bdb_options.disable_background_tasks = true;
  Open(bdb_options, options);
  std::map<std::string, std::string> data;
  mock_env_->set_current_time(50);
  for (size_t i = 0; i < 100; i++) {
    uint64_t ttl = rnd.Next() % 100;
    PutRandomWithTTL("key" + ToString(i), ttl, &rnd,
                     (ttl <= 50 ? nullptr : &data));
  }
  mock_env_->set_current_time(100);
  auto *bdb_impl = static_cast<BlobDBImpl *>(blob_db_);
  auto blob_files = bdb_impl->TEST_GetBlobFiles();
  ASSERT_EQ(1, blob_files.size());
  ASSERT_TRUE(blob_files[0]->HasTTL());
  ASSERT_OK(bdb_impl->TEST_CloseBlobFile(blob_files[0]));
  GCStats gc_stats;
  ASSERT_OK(bdb_impl->TEST_GCFileAndUpdateLSM(blob_files[0], &gc_stats));
  ASSERT_EQ(100 - data.size(), gc_stats.num_deletes);
  ASSERT_EQ(data.size(), gc_stats.num_relocate);
  VerifyDB(data);
}

TEST_F(BlobDBTest, PutUntil) {
  Random rnd(301);
  Options options;
  options.env = mock_env_.get();
  BlobDBOptions bdb_options;
  bdb_options.ttl_range_secs = 1000;
  bdb_options.min_blob_size = 0;
  bdb_options.blob_file_size = 256 * 1000 * 1000;
  bdb_options.disable_background_tasks = true;
  Open(bdb_options, options);
  std::map<std::string, std::string> data;
  mock_env_->set_current_time(50);
  for (size_t i = 0; i < 100; i++) {
    uint64_t expiration = rnd.Next() % 100 + 50;
    PutRandomUntil("key" + ToString(i), expiration, &rnd,
                   (expiration <= 100 ? nullptr : &data));
  }
  mock_env_->set_current_time(100);
  auto *bdb_impl = static_cast<BlobDBImpl *>(blob_db_);
  auto blob_files = bdb_impl->TEST_GetBlobFiles();
  ASSERT_EQ(1, blob_files.size());
  ASSERT_TRUE(blob_files[0]->HasTTL());
  ASSERT_OK(bdb_impl->TEST_CloseBlobFile(blob_files[0]));
  GCStats gc_stats;
  ASSERT_OK(bdb_impl->TEST_GCFileAndUpdateLSM(blob_files[0], &gc_stats));
  ASSERT_EQ(100 - data.size(), gc_stats.num_deletes);
  ASSERT_EQ(data.size(), gc_stats.num_relocate);
  VerifyDB(data);
}

TEST_F(BlobDBTest, TTLExtrator_NoTTL) {
//默认的TTL提取器不为每个键返回TTL。
  ttl_extractor_.reset(new TTLExtractor());
  Random rnd(301);
  Options options;
  options.env = mock_env_.get();
  BlobDBOptions bdb_options;
  bdb_options.ttl_range_secs = 1000;
  bdb_options.min_blob_size = 0;
  bdb_options.blob_file_size = 256 * 1000 * 1000;
  bdb_options.ttl_extractor = ttl_extractor_;
  bdb_options.disable_background_tasks = true;
  Open(bdb_options, options);
  std::map<std::string, std::string> data;
  mock_env_->set_current_time(0);
  for (size_t i = 0; i < 100; i++) {
    PutRandom("key" + ToString(i), &rnd, &data);
  }
//很遥远的将来……
  mock_env_->set_current_time(std::numeric_limits<uint64_t>::max() / 1000000 -
                              10);
  auto *bdb_impl = static_cast<BlobDBImpl *>(blob_db_);
  auto blob_files = bdb_impl->TEST_GetBlobFiles();
  ASSERT_EQ(1, blob_files.size());
  ASSERT_FALSE(blob_files[0]->HasTTL());
  ASSERT_OK(bdb_impl->TEST_CloseBlobFile(blob_files[0]));
  GCStats gc_stats;
  ASSERT_OK(bdb_impl->TEST_GCFileAndUpdateLSM(blob_files[0], &gc_stats));
  ASSERT_EQ(0, gc_stats.num_deletes);
  ASSERT_EQ(100, gc_stats.num_relocate);
  VerifyDB(data);
}

TEST_F(BlobDBTest, TTLExtractor_ExtractTTL) {
  Random rnd(301);
  class TestTTLExtractor : public TTLExtractor {
   public:
    explicit TestTTLExtractor(Random *r) : rnd(r) {}

    virtual bool ExtractTTL(const Slice &key, const Slice &value, uint64_t *ttl,
                            /*：：字符串*/*新的_值*/，
                            bool*/*值更改*/) override {

      *ttl = rnd->Next() % 100;
      if (*ttl > 50) {
        data[key.ToString()] = value.ToString();
      }
      return true;
    }

    Random *rnd;
    std::map<std::string, std::string> data;
  };
  ttl_extractor_.reset(new TestTTLExtractor(&rnd));
  Options options;
  options.env = mock_env_.get();
  BlobDBOptions bdb_options;
  bdb_options.ttl_range_secs = 1000;
  bdb_options.min_blob_size = 0;
  bdb_options.blob_file_size = 256 * 1000 * 1000;
  bdb_options.ttl_extractor = ttl_extractor_;
  bdb_options.disable_background_tasks = true;
  Open(bdb_options, options);
  mock_env_->set_current_time(50);
  for (size_t i = 0; i < 100; i++) {
    PutRandom("key" + ToString(i), &rnd);
  }
  mock_env_->set_current_time(100);
  auto *bdb_impl = static_cast<BlobDBImpl *>(blob_db_);
  auto blob_files = bdb_impl->TEST_GetBlobFiles();
  ASSERT_EQ(1, blob_files.size());
  ASSERT_TRUE(blob_files[0]->HasTTL());
  ASSERT_OK(bdb_impl->TEST_CloseBlobFile(blob_files[0]));
  GCStats gc_stats;
  ASSERT_OK(bdb_impl->TEST_GCFileAndUpdateLSM(blob_files[0], &gc_stats));
  auto &data = static_cast<TestTTLExtractor *>(ttl_extractor_.get())->data;
  ASSERT_EQ(100 - data.size(), gc_stats.num_deletes);
  ASSERT_EQ(data.size(), gc_stats.num_relocate);
  VerifyDB(data);
}

TEST_F(BlobDBTest, TTLExtractor_ExtractExpiration) {
  Random rnd(301);
  class TestTTLExtractor : public TTLExtractor {
   public:
    explicit TestTTLExtractor(Random *r) : rnd(r) {}

    virtual bool ExtractExpiration(const Slice &key, const Slice &value,
                                   /*t64_t/*现在*/，uint64_t*过期，
                                   标准：：字符串*/*新值*/,

                                   /*l*/*值_changed*/）覆盖
      *expiration=rnd->next（）%100+50；
      如果（*expiration>100）
        data[key.toString（）]=value.toString（）；
      }
      回归真实；
    }

    随机*RND；
    std:：map<std:：string，std:：string>data；
  }；
  ttl提取器重置（新的testtlextractor（&rnd））；
  期权期权；
  options.env=mock_env_u.get（）；
  b选项b选项b_选项；
  bdb_options.ttl_range_secs=1000；
  bdb_options.min_blob_size=0；
  bdb_options.blob_file_size=256*1000*1000；
  bdb_options.ttl_提取器=ttl_提取器
  bdb_options.disable_background_tasks=true；
  打开（bdb_选项，选项）；
  模拟环境设置当前时间（50）；
  对于（尺寸_t i=0；i<100；i++）
    PutRandom（“键”+ToString（i），&rnd）；
  }
  模拟环境设置当前时间（100）；
  auto*bdb_impl=static_cast<blob db impl*>（blob_db_uuu）；
  auto blob_files=bdb_impl->test_getblobfiles（）；
  断言_eq（1，blob_files.size（））；
  断言_true（blob_文件[0]->hasttl（））；
  断言ou ok（bdb_impl->test_closeblobfile（blob_files[0]）；
  gc stats gc_状态；
  断言“OK”（bdb_impl->test_gcfileandupdatelsm（blob_files[0]，&gc_stats））；
  auto&data=static_cast<testtlextractor*>（ttl_extractor_u.get（））->data；
  assert_eq（100-data.size（），gc_stats.num_deletes）；
  assert_eq（data.size（），gc_stats.num_relocate）；
  VerifyDB（数据）；
}

测试_f（blobdbtest，ttlextractor_changevalue）
  类testtlextractor:public ttlextractor_
   公众：
    const slice kttlsuffix=slice（“ttl：”）；

    bool extracttl（const slice&/*ke*/, const Slice &value, uint64_t *ttl,

                    std::string *new_value, bool *value_changed) override {
      if (value.size() < 12) {
        return false;
      }
      const char *p = value.data() + value.size() - 12;
      if (kTTLSuffix != Slice(p, 4)) {
        return false;
      }
      *ttl = DecodeFixed64(p + 4);
      *new_value = Slice(value.data(), value.size() - 12).ToString();
      *value_changed = true;
      return true;
    }
  };
  Random rnd(301);
  Options options;
  options.env = mock_env_.get();
  BlobDBOptions bdb_options;
  bdb_options.ttl_range_secs = 1000;
  bdb_options.min_blob_size = 0;
  bdb_options.blob_file_size = 256 * 1000 * 1000;
  bdb_options.ttl_extractor = std::make_shared<TestTTLExtractor>();
  bdb_options.disable_background_tasks = true;
  Open(bdb_options, options);
  std::map<std::string, std::string> data;
  mock_env_->set_current_time(50);
  for (size_t i = 0; i < 100; i++) {
    int len = rnd.Next() % kMaxBlobSize + 1;
    std::string key = "key" + ToString(i);
    std::string value = test::RandomHumanReadableString(&rnd, len);
    uint64_t ttl = rnd.Next() % 100;
    std::string value_ttl = value + "ttl:";
    PutFixed64(&value_ttl, ttl);
    ASSERT_OK(blob_db_->Put(WriteOptions(), Slice(key), Slice(value_ttl)));
    if (ttl > 50) {
      data[key] = value;
    }
  }
  mock_env_->set_current_time(100);
  auto *bdb_impl = static_cast<BlobDBImpl *>(blob_db_);
  auto blob_files = bdb_impl->TEST_GetBlobFiles();
  ASSERT_EQ(1, blob_files.size());
  ASSERT_TRUE(blob_files[0]->HasTTL());
  ASSERT_OK(bdb_impl->TEST_CloseBlobFile(blob_files[0]));
  GCStats gc_stats;
  ASSERT_OK(bdb_impl->TEST_GCFileAndUpdateLSM(blob_files[0], &gc_stats));
  ASSERT_EQ(100 - data.size(), gc_stats.num_deletes);
  ASSERT_EQ(data.size(), gc_stats.num_relocate);
  VerifyDB(data);
}

TEST_F(BlobDBTest, StackableDBGet) {
  Random rnd(301);
  BlobDBOptions bdb_options;
  bdb_options.min_blob_size = 0;
  bdb_options.disable_background_tasks = true;
  Open(bdb_options);
  std::map<std::string, std::string> data;
  for (size_t i = 0; i < 100; i++) {
    PutRandom("key" + ToString(i), &rnd, &data);
  }
  for (size_t i = 0; i < 100; i++) {
    StackableDB *db = blob_db_;
    ColumnFamilyHandle *column_family = db->DefaultColumnFamily();
    std::string key = "key" + ToString(i);
    PinnableSlice pinnable_value;
    ASSERT_OK(db->Get(ReadOptions(), column_family, key, &pinnable_value));
    std::string string_value;
    ASSERT_OK(db->Get(ReadOptions(), column_family, key, &string_value));
    ASSERT_EQ(string_value, pinnable_value.ToString());
    ASSERT_EQ(string_value, data[key]);
  }
}

TEST_F(BlobDBTest, WriteBatch) {
  Random rnd(301);
  BlobDBOptions bdb_options;
  bdb_options.min_blob_size = 0;
  bdb_options.disable_background_tasks = true;
  Open(bdb_options);
  std::map<std::string, std::string> data;
  for (size_t i = 0; i < 100; i++) {
    WriteBatch batch;
    for (size_t j = 0; j < 10; j++) {
      PutRandomToWriteBatch("key" + ToString(j * 100 + i), &rnd, &batch, &data);
    }
    blob_db_->Write(WriteOptions(), &batch);
  }
  VerifyDB(data);
}

TEST_F(BlobDBTest, Delete) {
  Random rnd(301);
  BlobDBOptions bdb_options;
  bdb_options.min_blob_size = 0;
  bdb_options.disable_background_tasks = true;
  Open(bdb_options);
  std::map<std::string, std::string> data;
  for (size_t i = 0; i < 100; i++) {
    PutRandom("key" + ToString(i), &rnd, &data);
  }
  for (size_t i = 0; i < 100; i += 5) {
    Delete("key" + ToString(i), &data);
  }
  VerifyDB(data);
}

TEST_F(BlobDBTest, DeleteBatch) {
  Random rnd(301);
  BlobDBOptions bdb_options;
  bdb_options.min_blob_size = 0;
  bdb_options.disable_background_tasks = true;
  Open(bdb_options);
  for (size_t i = 0; i < 100; i++) {
    PutRandom("key" + ToString(i), &rnd);
  }
  WriteBatch batch;
  for (size_t i = 0; i < 100; i++) {
    batch.Delete("key" + ToString(i));
  }
  ASSERT_OK(blob_db_->Write(WriteOptions(), &batch));
//数据库应为空。
  VerifyDB({});
}

TEST_F(BlobDBTest, Override) {
  Random rnd(301);
  BlobDBOptions bdb_options;
  bdb_options.min_blob_size = 0;
  bdb_options.disable_background_tasks = true;
  Open(bdb_options);
  std::map<std::string, std::string> data;
  for (int i = 0; i < 10000; i++) {
    PutRandom("key" + ToString(i), &rnd, nullptr);
  }
//覆盖所有键
  for (int i = 0; i < 10000; i++) {
    PutRandom("key" + ToString(i), &rnd, &data);
  }
  VerifyDB(data);
}

#ifdef SNAPPY
TEST_F(BlobDBTest, Compression) {
  Random rnd(301);
  BlobDBOptions bdb_options;
  bdb_options.min_blob_size = 0;
  bdb_options.disable_background_tasks = true;
  bdb_options.compression = CompressionType::kSnappyCompression;
  Open(bdb_options);
  std::map<std::string, std::string> data;
  for (size_t i = 0; i < 100; i++) {
    PutRandom("put-key" + ToString(i), &rnd, &data);
  }
  for (int i = 0; i < 100; i++) {
    WriteBatch batch;
    for (size_t j = 0; j < 10; j++) {
      PutRandomToWriteBatch("write-batch-key" + ToString(j * 100 + i), &rnd,
                            &batch, &data);
    }
    blob_db_->Write(WriteOptions(), &batch);
  }
  VerifyDB(data);
}

TEST_F(BlobDBTest, DecompressAfterReopen) {
  Random rnd(301);
  BlobDBOptions bdb_options;
  bdb_options.min_blob_size = 0;
  bdb_options.disable_background_tasks = true;
  bdb_options.compression = CompressionType::kSnappyCompression;
  Open(bdb_options);
  std::map<std::string, std::string> data;
  for (size_t i = 0; i < 100; i++) {
    PutRandom("put-key" + ToString(i), &rnd, &data);
  }
  VerifyDB(data);
  bdb_options.compression = CompressionType::kNoCompression;
  Reopen(bdb_options);
  VerifyDB(data);
}

#endif

TEST_F(BlobDBTest, MultipleWriters) {
  Open(BlobDBOptions());

  std::vector<port::Thread> workers;
  std::vector<std::map<std::string, std::string>> data_set(10);
  for (uint32_t i = 0; i < 10; i++)
    workers.push_back(port::Thread(
        [&](uint32_t id) {
          Random rnd(301 + id);
          for (int j = 0; j < 100; j++) {
            std::string key = "key" + ToString(id) + "_" + ToString(j);
            if (id < 5) {
              PutRandom(key, &rnd, &data_set[id]);
            } else {
              WriteBatch batch;
              PutRandomToWriteBatch(key, &rnd, &batch, &data_set[id]);
              blob_db_->Write(WriteOptions(), &batch);
            }
          }
        },
        i));
  std::map<std::string, std::string> data;
  for (size_t i = 0; i < 10; i++) {
    workers[i].join();
    data.insert(data_set[i].begin(), data_set[i].end());
  }
  VerifyDB(data);
}

TEST_F(BlobDBTest, GCAfterOverwriteKeys) {
  Random rnd(301);
  BlobDBOptions bdb_options;
  bdb_options.min_blob_size = 0;
  bdb_options.disable_background_tasks = true;
  Open(bdb_options);
  DBImpl *db_impl = static_cast_with_check<DBImpl, DB>(blob_db_->GetBaseDB());
  std::map<std::string, std::string> data;
  for (int i = 0; i < 200; i++) {
    PutRandom("key" + ToString(i), &rnd, &data);
  }
  auto blob_files = blob_db_impl()->TEST_GetBlobFiles();
  ASSERT_EQ(1, blob_files.size());
  ASSERT_OK(blob_db_impl()->TEST_CloseBlobFile(blob_files[0]));
//在SST中测试数据
  size_t new_keys = 0;
  for (int i = 0; i < 100; i++) {
    if (rnd.Next() % 2 == 1) {
      new_keys++;
      PutRandom("key" + ToString(i), &rnd, &data);
    }
  }
  /*impl->test_flushmemtable（true/*wait*/）；
  //测试memtable中的数据
  对于（int i=100；i<200；i++）
    if（rnd.next（）%2==1）
      NexyKix++；
      putrandom（“key”+to字符串（i）、&rnd和data）；
    }
  }
  gc stats gc_状态；
  断言“OK”（blob_db_impl（）->test_gcfileandupdatelsm（blob_files[0]，&gc_stats））；
  断言eq（200，gc-stats.blob-count）；
  断言_Eq（0，gc_stats.num_deletes）；
  断言_eq（200-新的_键，gc_stats.num_relocate）；
  VerifyDB（数据）；
}

测试_F（blobdbtest，gcRelocateKeyWhileOverwriting）
  随机RND（301）；
  b选项b选项b_选项；
  bdb_options.min_blob_size=0；
  bdb_options.disable_background_tasks=true；
  打开（bdb_选项）；
  断言_OK（blob_db_u->put（writeOptions（），“foo”，“v1”））；
  auto blob_files=blob_db_impl（）->test_getblobfiles（）；
  断言_eq（1，blob_files.size（））；
  断言_OK（blob_db_impl（）->test_closeblobfile（blob_files[0]）；

  同步点：：getInstance（）->LoadDependency（
      “blobdbimpl:：gcfileandupdatelsm:AfterGetFromBaseDB”，
        “blobdbimpl:：puttuntil:开始”，
       “blobdbimpl:：puttol:完成”，
        “blobdbimpl:：gcfileandupdatelsm:beforerelocate”）；
  同步点：：getInstance（）->EnableProcessing（）；

  自动编写器=端口：：线程（
      [此]（）断言_OK（blob_db_->put（writeOptions（），“foo”，“v2”））；）；

  gc stats gc_状态；
  断言“OK”（blob_db_impl（）->test_gcfileandupdatelsm（blob_files[0]，&gc_stats））；
  断言eq（1，gc_stats.blob_count）；
  断言_Eq（0，gc_stats.num_deletes）；
  断言_Eq（1，gc_stats.num_relocate）；
  断言eq（0，gc_stats.relocate_succeeded）；
  断言eq（1，gc_stats.overwrited_while_relocate）；
  作者；
  验证数据库（“foo”，“v2”）；
}

测试_F（blobdbtest，gcExpiredKeywhileOverwriting）
  随机RND（301）；
  期权期权；
  options.env=mock_env_u.get（）；
  b选项b选项b_选项；
  bdb_options.min_blob_size=0；
  bdb_options.disable_background_tasks=true；
  打开（bdb_选项，选项）；
  模拟环境设置当前时间（100）；
  断言_OK（blob_db_u->puttol（writeOptions（），“foo”，“v1”，200））；
  auto blob_files=blob_db_impl（）->test_getblobfiles（）；
  断言_eq（1，blob_files.size（））；
  断言_OK（blob_db_impl（）->test_closeblobfile（blob_files[0]）；
  模拟环境设置当前时间（300）；

  同步点：：getInstance（）->LoadDependency（
      “blobdbimpl:：gcfileandupdatelsm:AfterGetFromBaseDB”，
        “blobdbimpl:：puttuntil:开始”，
       “blobdbimpl:：puttol:完成”，
        “blobdbimpl:：gcfileandupdatelsm:beforeelete”）；
  同步点：：getInstance（）->EnableProcessing（）；

  自动编写器=端口：：线程（[此]（）
    断言_OK（blob_db_u->puttol（writeOptions（），“foo”，“v2”，400））；
  （}）；

  gc stats gc_状态；
  断言“OK”（blob_db_impl（）->test_gcfileandupdatelsm（blob_files[0]，&gc_stats））；
  断言eq（1，gc_stats.blob_count）；
  断言eq（1，gc-stats.num-deletes）；
  断言_eq（0，gc_stats.delete_succeeded）；
  断言eq（1，gc_stats.overwrited_while_delete）；
  断言eu eq（0，gc_stats.num_relocate）；
  作者；
  验证数据库（“foo”，“v2”）；
}

//此测试不再有效，因为我们现在返回错误
//超过配置的blob目录大小。
//稍后需要以继续写入的方式重新写入测试
//在GC发生后。
测试_f（blobdbtest，disabled_coldestsimpleblobfile when no tofspace）
  //使用mock env停止挂钟。
  期权期权；
  options.env=mock_env_u.get（）；
  b选项b选项b_选项；
  bdb_options.blob_dir_size=100；
  bdb_options.blob_file_size=100；
  bdb_options.min_blob_size=0；
  bdb_options.disable_background_tasks=true；
  打开（bdb_选项）；
  std：：字符串值（100，'v'）；
  断言_OK（blob_db_u->putWithTTL（writeOptions（），“key_with_ttl”，value，60））；
  对于（int i=0；i<10；i++）
    断言_OK（blob_db_u->put（writeOptions（），“key”+toString（i），value））；
  }
  auto blob_files=blob_db_impl（）->test_getblobfiles（）；
  断言_eq（11，blob_files.size（））；
  断言_true（blob_文件[0]->hasttl（））；
  断言_true（blob_files[0]->immutable（））；
  对于（int i=1；i<=10；i++）
    断言_false（blob_files[i]->hasttl（））；
    如果（i＜10）{
      断言_true（blob_files[i]->immutable（））；
    }
  }
  blob_db_impl（）->测试_rungc（）；
  //已为GC选择最旧的简单blob文件（即blob_文件[1]）。
  auto obsolete_files=blob_db_impl（）->test_getobsoletefiles（）；
  断言_eq（1，过时的_files.size（））；
  assert_eq（blob_files[1]->blobfilenumber（），
            废弃的_文件[0]->blobfilenumber（））；
}

测试_f（blobdbtest，readwhilegc）
  //对get（）、multiget（）和迭代器分别运行相同的测试。
  对于（int i=0；i<2；i++）
    b选项b选项b_选项；
    bdb_options.min_blob_size=0；
    bdb_options.disable_background_tasks=true；
    打开（bdb_选项）；
    blob_db_u->put（writeOptions（），“foo”，“bar”）；
    auto blob_files=blob_db_impl（）->test_getblobfiles（）；
    断言_eq（1，blob_files.size（））；
    std:：shared_ptr<blobfile>bfile=blob_files[0]；
    uint64_t bfile_number=bfile->blobfilenumber（）；
    断言_OK（blob_db_impl（）->测试_closeblobfile（bfile））；

    开关（i）{
      案例0：
        同步点：：getInstance（）->LoadDependency（
            “blobdbimpl:：get:afterindexentryget:1”，
              “blobdbtest:：readwhilegc:1”，
             “blobdbtest:：readwhilegc:2”，
              “blobdbimpl:：get:afterindexentryget:2”）；
        断裂；
      案例1：
        同步点：：getInstance（）->LoadDependency（
            123;“BlobBiterator:：UpdateBlobValue:开始：1”，
              “blobdbtest:：readwhilegc:1”，
             “blobdbtest:：readwhilegc:2”，
              “BlobBiterator:：UpdateBlobValue:开始时间：2”）；
        断裂；
    }
    同步点：：getInstance（）->EnableProcessing（）；

    自动读卡器=端口：：线程（[this，i]（）
      std：：字符串值；
      std:：vector<std:：string>值；
      std:：vector<status>status；
      开关（i）{
        案例0：
          断言_OK（blob_db_->get（readoptions（），“foo”，&value））；
          断言eq（“bar”，value）；
          断裂；
        案例1：
          //verifydb使用迭代器扫描db。
          验证数据库（“foo”，“bar”）；
          断裂；
      }
    （}）；

    测试同步点（“blobdbtest:：readwhilegc:1”）；
    gc stats gc_状态；
    断言_OK（blob_db_impl（）->test_gcfileandupdatelsm（bfile，&gc_stats））；
    断言eq（1，gc_stats.blob_count）；
    断言_Eq（1，gc_stats.num_relocate）；
    断言eq（1，gc_stats.relocate_succeeded）；
    blob_db_impl（）->测试_deleteobosoletefiles（）；
    //不应删除该文件
    blob_files=blob_db_impl（）->test_getblobfiles（）；
    断言_eq（2，blob_files.size（））；
    assert_eq（bfile_number，blob_files[0]->blobfilenumber（））；
    auto obsolete_files=blob_db_impl（）->test_getobsoletefiles（）；
    断言_eq（1，过时的_files.size（））；
    assert_eq（bfile_number，obsoleted_files[0]->blobfilenumber（））；
    测试同步点（“blobdbtest:：readwhilegc:2”）；
    Reader。
    syncpoint:：getInstance（）->disableProcessing（）；

    //这次文件被删除
    blob_db_impl（）->测试_deleteobosoletefiles（）；
    blob_files=blob_db_impl（）->test_getblobfiles（）；
    断言_eq（1，blob_files.size（））；
    assert_ne（bfile_number，blob_files[0]->blobfilenumber（））；
    assert_eq（0，blob_db_impl（）->test_getobsoletefiles（）.size（））；
    验证数据库（“foo”，“bar”）；
    销毁（）；
  }
}

测试_F（Blobdbtest、Snapshot和GarbageCollection）
  b选项b选项b_选项；
  bdb_options.min_blob_size=0；
  bdb_options.disable_background_tasks=true；
  //i=何时拍摄快照
  对于（int i=0；i<4；i++）
    for（bool delete_key：真，假）
      const snapshot*snapshot=nullptr；
      销毁（）；
      打开（bdb_选项）；
      //第一文件
      断言ou OK（Put（“key1”，“value”））；
      如果（i＝0）{
        快照=blob_db_u->getsnapshot（）；
      }
      auto blob_files=blob_db_impl（）->test_getblobfiles（）；
      断言_eq（1，blob_files.size（））；
      断言_OK（blob_db_impl（）->test_closeblobfile（blob_files[0]）；
      / /第二文件
      断言ou OK（Put（“key2”，“value”））；
      如果（i＝1）{
        快照=blob_db_u->getsnapshot（）；
      }
      blob_files=blob_db_impl（）->test_getblobfiles（）；
      断言_eq（2，blob_files.size（））；
      auto bfile=blob_文件[1]；
      断言_false（bfile->immutable（））；
      断言_OK（blob_db_impl（）->测试_closeblobfile（bfile））；
      //第三文件
      断言ou OK（Put（“key3”，“value”））；
      如果（i＝2）{
        快照=blob_db_u->getsnapshot（）；
      }
      如果（删除_键）
        删除（“KEY2”）；
      }
      gc stats gc_状态；
      断言_OK（blob_db_impl（）->test_gcfileandupdatelsm（bfile，&gc_stats））；
      断言“真”（bfile->obsolete（））；
      断言eq（1，gc_stats.blob_count）；
      如果（删除_键）
        断言eu eq（0，gc_stats.num_relocate）；
        断言eq（bfile->getSequenceRange（）.second+1，
                  bfile->getobsoletesequence（））；
      }否则{
        断言_Eq（1，gc_stats.num_relocate）；
        assert_eq（blob_db_uu->getLatestSequenceNumber（），
                  bfile->getobsoletesequence（））；
      }
      如果（i＝3）{
        快照=blob_db_u->getsnapshot（）；
      }
      size_t num_files=删除_键？3：4；
      assert_eq（num_files，blob_db_impl（）->test_getblobfiles（）.size（））；
      blob_db_impl（）->测试_deleteobosoletefiles（）；
      如果（i==0 i==3（i==2&&删除键）
        //快照不应在bfile中看到数据
        断言_eq（num_files-1，blob_db_impl（）->test_getblobfiles（）.size（））；
        blob_db_uu->releaseSnapshot（快照）；
      }否则{
        //快照将在bfile中看到数据，因此不应删除该文件
        assert_eq（num_files，blob_db_impl（）->test_getblobfiles（）.size（））；
        blob_db_uu->releaseSnapshot（快照）；
        blob_db_impl（）->测试_deleteobosoletefiles（）；
        断言_eq（num_files-1，blob_db_impl（）->test_getblobfiles（）.size（））；
      }
    }
  }
}

测试_f（blobdbtest，columnFamilyNotSupported）
  期权期权；
  options.env=mock_env_u.get（）；
  模拟环境设置当前时间（0）；
  打开（blobdboptions（），选项）；
  columnFamilyHandle*默认_handle=blob_db_u->defaultColumnFamily（）；
  columnFamilyHandle*句柄=nullptr；
  std：：字符串值；
  std:：vector<std:：string>值；
  //调用只是传递到基数据库。它应该成功。
  断言
      blob_db_u->createColumnFamily（columnFamilyOptions（），“foo”，&handle））；
  assert_true（blob_db_u->put（writeOptions（），handle，“k”，“v”）.isNotSupported（））；
  断言_true（blob_db_u->putWithTTL（writeOptions（），handle，“k”，“v”，60）
                  .isNotSupported（））；
  断言_true（blob_db_u->putUntil（writeOptions（），handle，“k”，“v”，100）
                  .isNotSupported（））；
  写入批处理；
  批量放置（“k1”、“v1”）；
  批量放置（手柄，“k2”，“v2”）；
  assert_true（blob_db_u->write（writeOptions（），&batch）.isNotSupported（））；
  assert_true（blob_db_u->get（readoptions（），“k1”，&value）.isnotfound（））；
  断言（真）
      blob_db_u->get（readoptions（），handle，“k”，&value）.isnotsupported（））；
  自动状态=blob_db_->multiget（readoptions（），默认_handle，handle，
                                     “k1”，“k2”，&values）；
  断言_eq（2，status.size（））；
  断言“真”（状态[0].IsNotSupported（））；
  断言“真”（状态[1].IsNotSupported（））；
  断言_eq（nullptr，blob_db_u->newIterator（readOptions（），handle））；
  删除句柄；
}

测试_f（blobdbtest，getlivefilesmetadata）
  随机RND（301）；
  b选项b选项b_选项；
  bdb_options.min_blob_size=0；
  bdb_options.disable_background_tasks=true；
  打开（bdb_选项）；
  std:：map<std:：string，std:：string>data；
  对于（尺寸_t i=0；i<100；i++）
    putrandom（“key”+to字符串（i）、&rnd和data）；
  }
  auto*bdb_impl=static_cast<blob db impl*>（blob_db_uuu）；
  std:：vector<livefilemetadata>metadata；
  bdb_impl->getlivefilesmetadata（&metadata）；
  断言eq（1U，metadata.size（））；
  std:：string文件名=dbname_+“/blob_dir/000001.blob”；
  断言eq（文件名，元数据[0].name）；
  断言_Eq（“默认”，元数据[0].列_Family_Name）；
  std:：vector<std:：string>livefile；
  UIT64 64 T MFS；
  bdb_impl->getlivefiles（livefile，&mfs，false）；
  断言_eq（4u，livefile.size（））；
  断言eq（文件名，livefile[3]）；
  VerifyDB（数据）；
}

测试_f（blobdbtest，从plainrocksdb迁移）
  施工尺寸_t knumkey=20；
  施工尺寸：knumiteration=10；
  随机RND（301）；
  std:：map<std:：string，std:：string>data；
  std:：vector<bool>是_blob（knumkey，false）；

  //写入plain rocksdb。
  期权期权；
  options.create_if_missing=true；
  db*db=nullptr；
  断言ou ok（db:：open（options，dbname_u，&db））；
  对于（尺寸_t i=0；i<knumiteration；i++）
    auto key_index=rnd.next（）%knumkey；
    std:：string key=“key”+toString（key_index）；
    PutRandom（数据库、密钥、RND和数据）；
  }
  验证数据库（DB，数据）；
  删除数据库；
  dB＝null pTr；

  //作为blob db打开。验证它是否可以读取现有数据。
  打开（）；
  verifydb（blob_u db_u，数据）；
  对于（尺寸_t i=0；i<knumiteration；i++）
    auto key_index=rnd.next（）%knumkey；
    std:：string key=“key”+toString（key_index）；
    为_blob[key_index]=真；
    putrandom（blob_uuu、key、&rnd和data）；
  }
  verifydb（blob_u db_u，数据）；
  删除blob_db_uu；
  blob_db_u=nullptr；

  //验证blob db写入的键的plain db返回错误。
  断言ou ok（db:：open（options，dbname_u，&db））；
  std：：字符串值；
  对于（尺寸_t i=0；i<knumkey；i++）
    std：：string key=“key”+to字符串（i）；
    状态s=db->get（readoptions（），key，&value）；
    if（data.count（key）==0）
      断言“真”（s.isNotFound（））；
    否则，如果（is_blob[i]）
      断言“真”（S.IsNotSupported（））；
    }否则{
      SalpTyk OK（s）；
      断言eq（数据[key]，值）；
    }
  }
  删除数据库；
}

//测试以验证到达时是否返回nospace ioerror状态
//blob_dir_大小限制。
测试_f（blobdbtest，outofspace）
  //使用mock env停止挂钟。
  期权期权；
  options.env=mock_env_u.get（）；
  b选项b选项b_选项；
  bdb_options.blob_dir_size=150；
  bdb_options.disable_background_tasks=true；
  打开（bdb_选项）；

  //当前每个存储的blob的开销约为42个字节。
  //所以一个小的键+一个100字节的blob应该在db中占大约150字节。
  std：：字符串值（100，'v'）；
  断言_OK（blob_db_u->putWithTTL（writeOptions（），“key1”，value，60））；

  //放入另一个blob应该失败，因为加载会超过blob目录大小
  / /限制。
  状态s=blob_db_u->putwitttl（writeOptions（），“key2”，值，60）；
  断言“真”（S.IsioError（））；
  断言“真”（s.isnospace（））；
}

测试_f（blobdbtest，evictoldestfilewhenclosetospaceLimit）
  //使用mock env停止挂钟。
  期权期权；
  b选项b选项b_选项；
  bdb_options.blob_dir_size=270；
  bdb_options.blob_file_size=100；
  bdb_options.disable_background_tasks=true；
  bdb_options.is_fifo=true；
  打开（bdb_选项）；

  //当前每个存储的blob都有32个字节的开销。
  //所以一个100字节的blob应该占132字节。
  std：：字符串值（100，'v'）；
  断言_OK（blob_db_u->putWithTTL（writeOptions（），“key1”，value，10））；

  auto*bdb_impl=static_cast<blob db impl*>（blob_db_uuu）；
  auto blob_files=bdb_impl->test_getblobfiles（）；
  断言_eq（1，blob_files.size（））；

  //再添加一个100字节的blob将使总大小达到264字节
  //（2*132），超过blob目录大小的90%。所以，最古老的文件
  //应被收回并放入过时的文件列表中。
  断言_OK（blob_db_u->putWithTTL（writeOptions（），“key2”，value，60））；

  auto obsolete_files=bdb_impl->test_getobsoletefiles（）；
  断言_eq（1，过时的_files.size（））；
  断言_true（过时的_文件[0]->不可变（））；
  assert_eq（blob_files[0]->blobfilenumber（），
            废弃的_文件[0]->blobfilenumber（））；

  bdb_impl->test_deleteobosoletefiles（）；
  过时的_files=bdb_impl->test_getobsoletefiles（）；
  断言_true（过时的_files.empty（））；
}

测试（blobdbtest，inlinesmallvalues）
  constexpr uint64_t kmaxexpiration=1000；
  随机RND（301）；
  b选项b选项b_选项；
  bdb_options.ttl_range_secs=kmaxexpiration；
  bdb_options.min_blob_size=100；
  bdb_options.blob_file_size=256*1000*1000；
  bdb_options.disable_background_tasks=true；
  期权期权；
  options.env=mock_env_u.get（）；
  模拟环境设置当前时间（0）；
  打开（bdb_选项，选项）；
  std:：map<std:：string，std:：string>data；
  std:：map<std:：string，keyversion>versions；
  sequenceNumber first_non_ttl_seq=kmaxSequenceNumber；
  sequenceNumber first_ttl_seq=kmaxSequenceNumber；
  SequenceNumber last_non_ttl_seq=0；
  SequenceNumber last_ttl_seq=0；
  对于（尺寸_t i=0；i<1000；i++）
    bool是_small_value=rnd.next（）%2；
    bool has_ttl=rnd.next（）%2；
    uint64_t expiration=rnd.next（）%kmaxexpiration；
    int len=是小值吗？50：200；
    std：：string key=“key”+to字符串（i）；
    std:：string value=test:：randomhumanreadablestring（&rnd，len）；
    std：：字符串blob_index；
    数据[键]=值；
    sequenceNumber sequence=blob_u db_u->getLatestSequenceNumber（）+1；
    如果（！）HasuTTl）{
      断言_OK（blob_db_u->put（writeOptions（），key，value））；
    }否则{
      断言_OK（blob_db_u->puttol（writeOptions（），key，value，expiration））；
    }
    assert_eq（blob_db_uu->getLatestSequenceNumber（），sequence）；
    版本[密钥]
        键版本（键，值，序列，
                   （是“小”值&&！哈萨特？ktypeValue:ktypeBloBindex）；
    如果（！）小_值）
      如果（！）HasuTTl）{
        first_non_ttl_seq=std:：min（first_non_ttl_seq，sequence）；
        last_non_ttl_seq=std:：max（last_non_ttl_seq，sequence）；
      }否则{
        first_ttl_seq=std:：min（first_ttl_seq，sequence）；
        last_ttl_seq=std:：max（last_ttl_seq，sequence）；
      }
    }
  }
  VerifyDB（数据）；
  verifybasedb（版本）；
  auto*bdb_impl=static_cast<blob db impl*>（blob_db_uuu）；
  auto blob_files=bdb_impl->test_getblobfiles（）；
  断言_eq（2，blob_files.size（））；
  std:：shared_ptr<blobfile>non_ttl_file；
  std:：shared_ptr<blobfile>ttl_file；
  if（blob_文件[0]->hasttl（））
    ttl_file=blob_文件[0]；
    non_ttl_file=blob_文件[1]；
  }否则{
    non_ttl_file=blob_文件[0]；
    ttl_file=blob_文件[1]；
  }
  断言_false（non_ttl_file->hasttl（））；
  断言eq（first_non_ttl_seq，non_ttl_file->getSequenceRange（）.first）；
  断言eq（last_non_ttl_seq，non_ttl_file->getSequenceRange（）.second）；
  断言_true（ttl_file->hasttl（））；
  断言_eq（first_ttl_seq，ttl_file->getSequenceRange（）.first）；
  断言_eq（last_ttl_seq，ttl_file->getSequenceRange（）.second）；
}

测试_f（blobdbtest，compactionfilternotsupported）
  类testcompletionfilter:public compactionfilter_
    virtual const char*name（）const返回“testcompletionfilter”；
  }；
  类TestCompactionFilterFactory:公共CompactionFilterFactory
    virtual const char*name（）const返回“testcompletionfilterFactory”；
    虚拟std:：unique_ptr<compactionfilter>createCompletionfilter（
        const compactionfilter：：上下文&/*上下文*/) {

      return std::unique_ptr<CompactionFilter>(new TestCompactionFilter());
    }
  };
  for (int i = 0; i < 2; i++) {
    Options options;
    if (i == 0) {
      options.compaction_filter = new TestCompactionFilter();
    } else {
      options.compaction_filter_factory.reset(
          new TestCompactionFilterFactory());
    }
    ASSERT_TRUE(TryOpen(BlobDBOptions(), options).IsNotSupported());
    delete options.compaction_filter;
  }
}

TEST_F(BlobDBTest, FilterExpiredBlobIndex) {
  constexpr size_t kNumKeys = 100;
  constexpr size_t kNumPuts = 1000;
  constexpr uint64_t kMaxExpiration = 1000;
  constexpr uint64_t kCompactTime = 500;
  constexpr uint64_t kMinBlobSize = 100;
  Random rnd(301);
  mock_env_->set_current_time(0);
  BlobDBOptions bdb_options;
  bdb_options.min_blob_size = kMinBlobSize;
  bdb_options.disable_background_tasks = true;
  Options options;
  options.env = mock_env_.get();
  Open(bdb_options, options);

  std::map<std::string, std::string> data;
  std::map<std::string, std::string> data_after_compact;
  for (size_t i = 0; i < kNumPuts; i++) {
    bool is_small_value = rnd.Next() % 2;
    bool has_ttl = rnd.Next() % 2;
    uint64_t expiration = rnd.Next() % kMaxExpiration;
    int len = is_small_value ? 10 : 200;
    std::string key = "key" + ToString(rnd.Next() % kNumKeys);
    std::string value = test::RandomHumanReadableString(&rnd, len);
    if (!has_ttl) {
      if (is_small_value) {
        std::string blob_entry;
        BlobIndex::EncodeInlinedTTL(&blob_entry, expiration, value);
//带TTL的假Blob索引。看看它会做什么。
        ASSERT_GT(kMinBlobSize, blob_entry.size());
        value = blob_entry;
      }
      ASSERT_OK(Put(key, value));
      data_after_compact[key] = value;
    } else {
      ASSERT_OK(PutUntil(key, value, expiration));
      if (expiration <= kCompactTime) {
        data_after_compact.erase(key);
      } else {
        data_after_compact[key] = value;
      }
    }
    data[key] = value;
  }
  VerifyDB(data);

  mock_env_->set_current_time(kCompactTime);
//在压缩前拍摄快照。确保过期的blob索引为
//已筛选，不考虑快照。
  const Snapshot *snapshot = blob_db_->GetSnapshot();
//发出手动压实以触发压实过滤器。
  ASSERT_OK(blob_db_->CompactRange(CompactRangeOptions(),
                                   blob_db_->DefaultColumnFamily(), nullptr,
                                   nullptr));
  blob_db_->ReleaseSnapshot(snapshot);
//验证是否筛选过期的blob索引。
  std::vector<KeyVersion> versions;
  GetAllKeyVersions(blob_db_, "", "", &versions);
  ASSERT_EQ(data_after_compact.size(), versions.size());
  for (auto &version : versions) {
    ASSERT_TRUE(data_after_compact.count(version.user_key) > 0);
  }
  VerifyDB(data_after_compact);
}

}  //命名空间blob_db
}  //命名空间rocksdb

//rocksdb周围TTL封装器的黑盒测试
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else
#include <stdio.h>

int main(int argc, char** argv) {
  fprintf(stderr, "SKIPPED as BlobDB is not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //！摇滚乐
