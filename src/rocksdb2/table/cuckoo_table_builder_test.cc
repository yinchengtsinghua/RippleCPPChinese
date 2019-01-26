
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

#include <vector>
#include <string>
#include <map>
#include <utility>

#include "table/meta_blocks.h"
#include "table/cuckoo_table_builder.h"
#include "util/file_reader_writer.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace rocksdb {
extern const uint64_t kCuckooTableMagicNumber;

namespace {
std::unordered_map<std::string, std::vector<uint64_t>> hash_map;

uint64_t GetSliceHash(const Slice& s, uint32_t index,
    uint64_t max_num_buckets) {
  return hash_map[s.ToString()][index];
}
}  //命名空间

class CuckooBuilderTest : public testing::Test {
 public:
  CuckooBuilderTest() {
    env_ = Env::Default();
    Options options;
    options.allow_mmap_reads = true;
    env_options_ = EnvOptions(options);
  }

  void CheckFileContents(const std::vector<std::string>& keys,
      const std::vector<std::string>& values,
      const std::vector<uint64_t>& expected_locations,
      std::string expected_unused_bucket, uint64_t expected_table_size,
      uint32_t expected_num_hash_func, bool expected_is_last_level,
      uint32_t expected_cuckoo_block_size = 1) {
//读取文件
    unique_ptr<RandomAccessFile> read_file;
    ASSERT_OK(env_->NewRandomAccessFile(fname, &read_file, env_options_));
    uint64_t read_file_size;
    ASSERT_OK(env_->GetFileSize(fname, &read_file_size));

	  Options options;
	  options.allow_mmap_reads = true;
	  ImmutableCFOptions ioptions(options);

//断言表属性。
    TableProperties* props = nullptr;
    unique_ptr<RandomAccessFileReader> file_reader(
        new RandomAccessFileReader(std::move(read_file), fname));
    ASSERT_OK(ReadTableProperties(file_reader.get(), read_file_size,
                                  kCuckooTableMagicNumber, ioptions,
                                  &props));
//检查未使用的桶。
    std::string unused_key = props->user_collected_properties[
      CuckooTablePropertyNames::kEmptyKey];
    ASSERT_EQ(expected_unused_bucket.substr(0,
          props->fixed_key_len), unused_key);

    uint64_t value_len_found =
      *reinterpret_cast<const uint64_t*>(props->user_collected_properties[
                CuckooTablePropertyNames::kValueLength].data());
    ASSERT_EQ(values.empty() ? 0 : values[0].size(), value_len_found);
    ASSERT_EQ(props->raw_value_size, values.size()*value_len_found);
    const uint64_t table_size =
      *reinterpret_cast<const uint64_t*>(props->user_collected_properties[
                CuckooTablePropertyNames::kHashTableSize].data());
    ASSERT_EQ(expected_table_size, table_size);
    const uint32_t num_hash_func_found =
      *reinterpret_cast<const uint32_t*>(props->user_collected_properties[
                CuckooTablePropertyNames::kNumHashFunc].data());
    ASSERT_EQ(expected_num_hash_func, num_hash_func_found);
    const uint32_t cuckoo_block_size =
      *reinterpret_cast<const uint32_t*>(props->user_collected_properties[
                CuckooTablePropertyNames::kCuckooBlockSize].data());
    ASSERT_EQ(expected_cuckoo_block_size, cuckoo_block_size);
    const bool is_last_level_found =
      *reinterpret_cast<const bool*>(props->user_collected_properties[
                CuckooTablePropertyNames::kIsLastLevel].data());
    ASSERT_EQ(expected_is_last_level, is_last_level_found);

    ASSERT_EQ(props->num_entries, keys.size());
    ASSERT_EQ(props->fixed_key_len, keys.empty() ? 0 : keys[0].size());
    ASSERT_EQ(props->data_size, expected_unused_bucket.size() *
        (expected_table_size + expected_cuckoo_block_size - 1));
    ASSERT_EQ(props->raw_key_size, keys.size()*props->fixed_key_len);
    ASSERT_EQ(props->column_family_id, 0);
    ASSERT_EQ(props->column_family_name, kDefaultColumnFamilyName);
    delete props;

//检查桶的内容。
    std::vector<bool> keys_found(keys.size(), false);
    size_t bucket_size = expected_unused_bucket.size();
    for (uint32_t i = 0; i < table_size + cuckoo_block_size - 1; ++i) {
      Slice read_slice;
      ASSERT_OK(file_reader->Read(i * bucket_size, bucket_size, &read_slice,
                                  nullptr));
      size_t key_idx =
          std::find(expected_locations.begin(), expected_locations.end(), i) -
          expected_locations.begin();
      if (key_idx == keys.size()) {
//我不是所期望的地点之一。空桶。
        if (read_slice.data() == nullptr) {
          ASSERT_EQ(0, expected_unused_bucket.size());
        } else {
          ASSERT_EQ(read_slice.compare(expected_unused_bucket), 0);
        }
      } else {
        keys_found[key_idx] = true;
        ASSERT_EQ(read_slice.compare(keys[key_idx] + values[key_idx]), 0);
      }
    }
    for (auto key_found : keys_found) {
//检查是否找到所有钥匙。
      ASSERT_TRUE(key_found);
    }
  }

  std::string GetInternalKey(Slice user_key, bool zero_seqno) {
    IterKey ikey;
    ikey.SetInternalKey(user_key, zero_seqno ? 0 : 1000, kTypeValue);
    return ikey.GetInternalKey().ToString();
  }

  uint64_t NextPowOf2(uint64_t num) {
    uint64_t n = 2;
    while (n <= num) {
      n *= 2;
    }
    return n;
  }

  uint64_t GetExpectedTableSize(uint64_t num) {
    return NextPowOf2(static_cast<uint64_t>(num / kHashTableRatio));
  }


  Env* env_;
  EnvOptions env_options_;
  std::string fname;
  const double kHashTableRatio = 0.9;
};

TEST_F(CuckooBuilderTest, SuccessWithEmptyFile) {
  unique_ptr<WritableFile> writable_file;
  fname = test::TmpDir() + "/EmptyFile";
  ASSERT_OK(env_->NewWritableFile(fname, &writable_file, env_options_));
  unique_ptr<WritableFileWriter> file_writer(
      new WritableFileWriter(std::move(writable_file), EnvOptions()));
  CuckooTableBuilder builder(file_writer.get(), kHashTableRatio, 4, 100,
                             BytewiseComparator(), 1, false, false,
                             /*slicehash，0/*column_family_id*/，
                             kDefaultColumnFamilyName）；
  断言_OK（builder.status（））；
  断言eq（0ul，builder.fileSize（））；
  断言ou ok（builder.finish（））；
  断言_OK（file_writer->close（））；
  检查文件内容（，，，，“”，2，2，false）；
}

测试（杜鹃花构建器测试，WriteSuccessNoColleisionFullKey）
  uint32_t num_hash_fun=4；
  std:：vector<std:：string>user_keys=“key01”，“key02”，“key03”，“key04”
  std:：vector<std:：string>values=“v01”，“v02”，“v03”，“v04”
  //此处需要有一个临时变量，因为vs编译器当前没有
  //支持operator=with initializer_list as a parameter
  std:：unordered_map<std:：string，std:：vector<uint64_t>>hm=
      用户键[0]、0、1、2、3，
      用户键[1]、1、2、3、4，
      用户键[2]、2、3、4、5，
      用户键[3]、3、4、5、6
  hash_map=std:：move（hm）；

  std:：vector<uint64_t>预期的_位置=0，1，2，3_
  std:：vector<std:：string>keys；
  用于（自动和用户密钥：用户密钥）
    keys.push_back（getInternalKey（user_key，false））；
  }
  uint64_t expected_table_size=getExpectedTableSize（keys.size（））；

  唯一的可写文件；
  fname=test:：tmpdir（）+“/nocollisionfullkey”；
  断言_OK（env_->newwritablefile（fname，&writable_file，env_options_uu））；
  唯一的文件写入程序（
      new writablefilewriter（std:：move（writable_file），envOptions（））；
  布谷鸟表生成器（file_writer.get（），khashtableratio，num_hash_fun，
                             100，bytewiseComparator（），1，false，false，
                             getslicehash，0/*列\u族\u id*/,

                             kDefaultColumnFamilyName);
  ASSERT_OK(builder.status());
  for (uint32_t i = 0; i < user_keys.size(); i++) {
    builder.Add(Slice(keys[i]), Slice(values[i]));
    ASSERT_EQ(builder.NumEntries(), i + 1);
    ASSERT_OK(builder.status());
  }
  size_t bucket_size = keys[0].size() + values[0].size();
  ASSERT_EQ(expected_table_size * bucket_size - 1, builder.FileSize());
  ASSERT_OK(builder.Finish());
  ASSERT_OK(file_writer->Close());
  ASSERT_LE(expected_table_size * bucket_size, builder.FileSize());

  std::string expected_unused_bucket = GetInternalKey("key00", true);
  expected_unused_bucket += std::string(values[0].size(), 'a');
  CheckFileContents(keys, values, expected_locations,
      expected_unused_bucket, expected_table_size, 2, false);
}

TEST_F(CuckooBuilderTest, WriteSuccessWithCollisionFullKey) {
  uint32_t num_hash_fun = 4;
  std::vector<std::string> user_keys = {"key01", "key02", "key03", "key04"};
  std::vector<std::string> values = {"v01", "v02", "v03", "v04"};
//这里需要有一个临时变量，因为vs编译器当前没有
//支持operator=with initializer_list as a parameter
  std::unordered_map<std::string, std::vector<uint64_t>> hm = {
      {user_keys[0], {0, 1, 2, 3}},
      {user_keys[1], {0, 1, 2, 3}},
      {user_keys[2], {0, 1, 2, 3}},
      {user_keys[3], {0, 1, 2, 3}},
  };
  hash_map = std::move(hm);

  std::vector<uint64_t> expected_locations = {0, 1, 2, 3};
  std::vector<std::string> keys;
  for (auto& user_key : user_keys) {
    keys.push_back(GetInternalKey(user_key, false));
  }
  uint64_t expected_table_size = GetExpectedTableSize(keys.size());

  unique_ptr<WritableFile> writable_file;
  fname = test::TmpDir() + "/WithCollisionFullKey";
  ASSERT_OK(env_->NewWritableFile(fname, &writable_file, env_options_));
  unique_ptr<WritableFileWriter> file_writer(
      new WritableFileWriter(std::move(writable_file), EnvOptions()));
  CuckooTableBuilder builder(file_writer.get(), kHashTableRatio, num_hash_fun,
                             100, BytewiseComparator(), 1, false, false,
                             /*slicehash，0/*column_family_id*/，
                             kDefaultColumnFamilyName）；
  断言_OK（builder.status（））；
  对于（uint32_t i=0；i<user_keys.size（）；i++）
    builder.add（slice（key[i]），slice（values[i]））；
    断言eq（builder.numEntries（），i+1）；
    断言_OK（builder.status（））；
  }
  size_t bucket_size=keys[0].size（）+values[0].size（）；
  assert_eq（预期的_table_size*bucket_size-1，builder.fileSize（））；
  断言ou ok（builder.finish（））；
  断言_OK（file_writer->close（））；
  assert_le（预期的_table_size*bucket_size，builder.fileSize（））；

  std：：字符串应为“u unused”bucket=getInternalKey（“key00”，true）；
  应输入“u unused”bucket+=std:：string（值[0].size（），“a”）；
  检查文件内容（键、值、预期的位置，
      预期的未使用的存储桶，预期的表大小，4，错误）；
}

测试（布谷鸟建设者测试，与碰撞和布谷鸟块写入成功）
  uint32_t num_hash_fun=4；
  std:：vector<std:：string>user_keys=“key01”，“key02”，“key03”，“key04”
  std:：vector<std:：string>values=“v01”，“v02”，“v03”，“v04”
  //此处需要有一个临时变量，因为vs编译器当前没有
  //支持operator=with initializer_list as a parameter
  std:：unordered_map<std:：string，std:：vector<uint64_t>>hm=
      用户键[0]、0、1、2、3，
      用户键[1]、0、1、2、3，
      用户键[2]、0、1、2、3，
      用户键[3]、0、1、2、3，
  }；
  hash_map=std:：move（hm）；

  std:：vector<uint64_t>预期的_位置=0，1，2，3_
  std:：vector<std:：string>keys；
  用于（自动和用户密钥：用户密钥）
    keys.push_back（getInternalKey（user_key，false））；
  }
  uint64_t expected_table_size=getExpectedTableSize（keys.size（））；

  唯一的可写文件；
  uint32_t布谷鸟块大小=2；
  fname=test:：tmpdir（）+“/withcollisionfullkey2”；
  断言_OK（env_->newwritablefile（fname，&writable_file，env_options_uu））；
  唯一的文件写入程序（
      new writablefilewriter（std:：move（writable_file），envOptions（））；
  布谷台建筑商（
      file_writer.get（），khashtableratio，num_hash_fun，100，
      bytewiseComparator（），布谷鸟块大小，false，false，getsliceHash，
      0/*列\u族\u id*/, kDefaultColumnFamilyName);

  ASSERT_OK(builder.status());
  for (uint32_t i = 0; i < user_keys.size(); i++) {
    builder.Add(Slice(keys[i]), Slice(values[i]));
    ASSERT_EQ(builder.NumEntries(), i + 1);
    ASSERT_OK(builder.status());
  }
  size_t bucket_size = keys[0].size() + values[0].size();
  ASSERT_EQ(expected_table_size * bucket_size - 1, builder.FileSize());
  ASSERT_OK(builder.Finish());
  ASSERT_OK(file_writer->Close());
  ASSERT_LE(expected_table_size * bucket_size, builder.FileSize());

  std::string expected_unused_bucket = GetInternalKey("key00", true);
  expected_unused_bucket += std::string(values[0].size(), 'a');
  CheckFileContents(keys, values, expected_locations,
      expected_unused_bucket, expected_table_size, 3, false, cuckoo_block_size);
}

TEST_F(CuckooBuilderTest, WithCollisionPathFullKey) {
//有两个哈希函数。插入具有重叠哈希值的元素。
//最后在中间插入一个哈希值为的元素
//这样它就把所有元素都移走了。
  uint32_t num_hash_fun = 2;
  std::vector<std::string> user_keys = {"key01", "key02", "key03",
    "key04", "key05"};
  std::vector<std::string> values = {"v01", "v02", "v03", "v04", "v05"};
//这里需要有一个临时变量，因为vs编译器当前没有
//支持operator=with initializer_list as a parameter
  std::unordered_map<std::string, std::vector<uint64_t>> hm = {
      {user_keys[0], {0, 1}},
      {user_keys[1], {1, 2}},
      {user_keys[2], {2, 3}},
      {user_keys[3], {3, 4}},
      {user_keys[4], {0, 2}},
  };
  hash_map = std::move(hm);

  std::vector<uint64_t> expected_locations = {0, 1, 3, 4, 2};
  std::vector<std::string> keys;
  for (auto& user_key : user_keys) {
    keys.push_back(GetInternalKey(user_key, false));
  }
  uint64_t expected_table_size = GetExpectedTableSize(keys.size());

  unique_ptr<WritableFile> writable_file;
  fname = test::TmpDir() + "/WithCollisionPathFullKey";
  ASSERT_OK(env_->NewWritableFile(fname, &writable_file, env_options_));
  unique_ptr<WritableFileWriter> file_writer(
      new WritableFileWriter(std::move(writable_file), EnvOptions()));
  CuckooTableBuilder builder(file_writer.get(), kHashTableRatio, num_hash_fun,
                             100, BytewiseComparator(), 1, false, false,
                             /*slicehash，0/*column_family_id*/，
                             kDefaultColumnFamilyName）；
  断言_OK（builder.status（））；
  对于（uint32_t i=0；i<user_keys.size（）；i++）
    builder.add（slice（key[i]），slice（values[i]））；
    断言eq（builder.numEntries（），i+1）；
    断言_OK（builder.status（））；
  }
  size_t bucket_size=keys[0].size（）+values[0].size（）；
  assert_eq（预期的_table_size*bucket_size-1，builder.fileSize（））；
  断言ou ok（builder.finish（））；
  断言_OK（file_writer->close（））；
  assert_le（预期的_table_size*bucket_size，builder.fileSize（））；

  std：：字符串应为“u unused”bucket=getInternalKey（“key00”，true）；
  应输入“u unused”bucket+=std:：string（值[0].size（），“a”）；
  检查文件内容（键、值、预期的位置，
      预期_未使用的_bucket，预期_table_大小，2，假）；
}

测试F（布谷鸟建筑测试，碰撞路径完整，布谷鸟块）
  uint32_t num_hash_fun=2；
  std:：vector<std:：string>user“key01”，“key02”，“key03”，
    “键04”，“键05”；
  std:：vector<std:：string>values=“v01”，“v02”，“v03”，“v04”，“v05”
  //此处需要有一个临时变量，因为vs编译器当前没有
  //支持operator=with initializer_list as a parameter
  std:：unordered_map<std:：string，std:：vector<uint64_t>>hm=
      用户键[0]、0，1，
      用户键[1]、1、2，
      用户键[2]、3、4，
      用户键[3]、4、5，
      用户键[4]、0、3，
  }；
  hash_map=std:：move（hm）；

  std:：vector<uint64_t>预期的_位置=2，1，3，4，0_
  std:：vector<std:：string>keys；
  用于（自动和用户密钥：用户密钥）
    keys.push_back（getInternalKey（user_key，false））；
  }
  uint64_t expected_table_size=getExpectedTableSize（keys.size（））；

  唯一的可写文件；
  fname=test:：tmpdir（）+“/withcollisionpathfullkeyandcuckooblock”；
  断言_OK（env_->newwritablefile（fname，&writable_file，env_options_uu））；
  唯一的文件写入程序（
      new writablefilewriter（std:：move（writable_file），envOptions（））；
  布谷鸟表生成器（file_writer.get（），khashtableratio，num_hash_fun，
                             100，bytewiseComparator（），2，false，false，
                             getslicehash，0/*列\u族\u id*/,

                             kDefaultColumnFamilyName);
  ASSERT_OK(builder.status());
  for (uint32_t i = 0; i < user_keys.size(); i++) {
    builder.Add(Slice(keys[i]), Slice(values[i]));
    ASSERT_EQ(builder.NumEntries(), i + 1);
    ASSERT_OK(builder.status());
  }
  size_t bucket_size = keys[0].size() + values[0].size();
  ASSERT_EQ(expected_table_size * bucket_size - 1, builder.FileSize());
  ASSERT_OK(builder.Finish());
  ASSERT_OK(file_writer->Close());
  ASSERT_LE(expected_table_size * bucket_size, builder.FileSize());

  std::string expected_unused_bucket = GetInternalKey("key00", true);
  expected_unused_bucket += std::string(values[0].size(), 'a');
  CheckFileContents(keys, values, expected_locations,
      expected_unused_bucket, expected_table_size, 2, false, 2);
}

TEST_F(CuckooBuilderTest, WriteSuccessNoCollisionUserKey) {
  uint32_t num_hash_fun = 4;
  std::vector<std::string> user_keys = {"key01", "key02", "key03", "key04"};
  std::vector<std::string> values = {"v01", "v02", "v03", "v04"};
//这里需要有一个临时变量，因为vs编译器当前没有
//支持operator=with initializer_list as a parameter
  std::unordered_map<std::string, std::vector<uint64_t>> hm = {
      {user_keys[0], {0, 1, 2, 3}},
      {user_keys[1], {1, 2, 3, 4}},
      {user_keys[2], {2, 3, 4, 5}},
      {user_keys[3], {3, 4, 5, 6}}};
  hash_map = std::move(hm);

  std::vector<uint64_t> expected_locations = {0, 1, 2, 3};
  uint64_t expected_table_size = GetExpectedTableSize(user_keys.size());

  unique_ptr<WritableFile> writable_file;
  fname = test::TmpDir() + "/NoCollisionUserKey";
  ASSERT_OK(env_->NewWritableFile(fname, &writable_file, env_options_));
  unique_ptr<WritableFileWriter> file_writer(
      new WritableFileWriter(std::move(writable_file), EnvOptions()));
  CuckooTableBuilder builder(file_writer.get(), kHashTableRatio, num_hash_fun,
                             100, BytewiseComparator(), 1, false, false,
                             /*slicehash，0/*column_family_id*/，
                             kDefaultColumnFamilyName）；
  断言_OK（builder.status（））；
  对于（uint32_t i=0；i<user_keys.size（）；i++）
    builder.add（slice（getInternalKey（user_keys[i]，true）），slice（values[i]）；
    断言eq（builder.numEntries（），i+1）；
    断言_OK（builder.status（））；
  }
  size_t bucket_size=user_keys[0].size（）+values[0].size（）；
  assert_eq（预期的_table_size*bucket_size-1，builder.fileSize（））；
  断言ou ok（builder.finish（））；
  断言_OK（file_writer->close（））；
  assert_le（预期的_table_size*bucket_size，builder.fileSize（））；

  std：：字符串应为“u unused”bucket=“key00”；
  应输入“u unused”bucket+=std:：string（值[0].size（），“a”）；
  检查文件内容（用户密钥、值、预期位置，
      预期_未使用_bucket，预期_table_大小，2，真）；
}

测试F（BuckooBuilderTest，WriteSuccesswithCollisionUserKey）
  uint32_t num_hash_fun=4；
  std:：vector<std:：string>user_keys=“key01”，“key02”，“key03”，“key04”
  std:：vector<std:：string>values=“v01”，“v02”，“v03”，“v04”
  //此处需要有一个临时变量，因为vs编译器当前没有
  //支持operator=with initializer_list as a parameter
  std:：unordered_map<std:：string，std:：vector<uint64_t>>hm=
      用户键[0]、0、1、2、3，
      用户键[1]、0、1、2、3，
      用户键[2]、0、1、2、3，
      用户键[3]、0、1、2、3，
  }；
  hash_map=std:：move（hm）；

  std:：vector<uint64_t>预期的_位置=0，1，2，3_
  uint64_t expected_table_size=getExpectedTableSize（user_keys.size（））；

  唯一的可写文件；
  fname=test:：tmpdir（）+“/withcollisionuserkey”；
  断言_OK（env_->newwritablefile（fname，&writable_file，env_options_uu））；
  唯一的文件写入程序（
      new writablefilewriter（std:：move（writable_file），envOptions（））；
  布谷鸟表生成器（file_writer.get（），khashtableratio，num_hash_fun，
                             100，bytewiseComparator（），1，false，false，
                             getslicehash，0/*列\u族\u id*/,

                             kDefaultColumnFamilyName);
  ASSERT_OK(builder.status());
  for (uint32_t i = 0; i < user_keys.size(); i++) {
    builder.Add(Slice(GetInternalKey(user_keys[i], true)), Slice(values[i]));
    ASSERT_EQ(builder.NumEntries(), i + 1);
    ASSERT_OK(builder.status());
  }
  size_t bucket_size = user_keys[0].size() + values[0].size();
  ASSERT_EQ(expected_table_size * bucket_size - 1, builder.FileSize());
  ASSERT_OK(builder.Finish());
  ASSERT_OK(file_writer->Close());
  ASSERT_LE(expected_table_size * bucket_size, builder.FileSize());

  std::string expected_unused_bucket = "key00";
  expected_unused_bucket += std::string(values[0].size(), 'a');
  CheckFileContents(user_keys, values, expected_locations,
      expected_unused_bucket, expected_table_size, 4, true);
}

TEST_F(CuckooBuilderTest, WithCollisionPathUserKey) {
  uint32_t num_hash_fun = 2;
  std::vector<std::string> user_keys = {"key01", "key02", "key03",
    "key04", "key05"};
  std::vector<std::string> values = {"v01", "v02", "v03", "v04", "v05"};
//这里需要有一个临时变量，因为vs编译器当前没有
//支持operator=with initializer_list as a parameter
  std::unordered_map<std::string, std::vector<uint64_t>> hm = {
      {user_keys[0], {0, 1}},
      {user_keys[1], {1, 2}},
      {user_keys[2], {2, 3}},
      {user_keys[3], {3, 4}},
      {user_keys[4], {0, 2}},
  };
  hash_map = std::move(hm);

  std::vector<uint64_t> expected_locations = {0, 1, 3, 4, 2};
  uint64_t expected_table_size = GetExpectedTableSize(user_keys.size());

  unique_ptr<WritableFile> writable_file;
  fname = test::TmpDir() + "/WithCollisionPathUserKey";
  ASSERT_OK(env_->NewWritableFile(fname, &writable_file, env_options_));
  unique_ptr<WritableFileWriter> file_writer(
      new WritableFileWriter(std::move(writable_file), EnvOptions()));
  CuckooTableBuilder builder(file_writer.get(), kHashTableRatio, num_hash_fun,
                             2, BytewiseComparator(), 1, false, false,
                             /*slicehash，0/*column_family_id*/，
                             kDefaultColumnFamilyName）；
  断言_OK（builder.status（））；
  对于（uint32_t i=0；i<user_keys.size（）；i++）
    builder.add（slice（getInternalKey（user_keys[i]，true）），slice（values[i]）；
    断言eq（builder.numEntries（），i+1）；
    断言_OK（builder.status（））；
  }
  size_t bucket_size=user_keys[0].size（）+values[0].size（）；
  assert_eq（预期的_table_size*bucket_size-1，builder.fileSize（））；
  断言ou ok（builder.finish（））；
  断言_OK（file_writer->close（））；
  assert_le（预期的_table_size*bucket_size，builder.fileSize（））；

  std：：字符串应为“u unused”bucket=“key00”；
  应输入“u unused”bucket+=std:：string（值[0].size（），“a”）；
  检查文件内容（用户密钥、值、预期位置，
      预期_未使用_bucket，预期_table_大小，2，真）；
}

测试F（布谷鸟建筑测试，在冲突路径工具打开时失败）
  //有两个哈希函数。插入具有重叠哈希值的元素。
  //最后尝试在中间插入哈希值为的元素
  //它应该失败，因为要替换的元素太多。
  uint32_t num_hash_fun=2；
  std:：vector<std:：string>user“key01”，“key02”，“key03”，
    “键04”，“键05”；
  //此处需要有一个临时变量，因为vs编译器当前没有
  //支持operator=with initializer_list as a parameter
  std:：unordered_map<std:：string，std:：vector<uint64_t>>hm=
      用户键[0]、0，1，
      用户键[1]、1、2，
      用户键[2]、2、3，
      用户键[3]、3、4，
      用户键[4]、0，1，
  }；
  hash_map=std:：move（hm）；

  唯一的可写文件；
  fname=test:：tmpdir（）+“/withcollisionpathuserkey”；
  断言_OK（env_->newwritablefile（fname，&writable_file，env_options_uu））；
  唯一的文件写入程序（
      new writablefilewriter（std:：move（writable_file），envOptions（））；
  布谷鸟表生成器（file_writer.get（），khashtableratio，num_hash_fun，
                             2，bytewiseComparator（），1，false，false，
                             getslicehash，0/*列\u族\u id*/,

                             kDefaultColumnFamilyName);
  ASSERT_OK(builder.status());
  for (uint32_t i = 0; i < user_keys.size(); i++) {
    builder.Add(Slice(GetInternalKey(user_keys[i], false)), Slice("value"));
    ASSERT_EQ(builder.NumEntries(), i + 1);
    ASSERT_OK(builder.status());
  }
  ASSERT_TRUE(builder.Finish().IsNotSupported());
  ASSERT_OK(file_writer->Close());
}

TEST_F(CuckooBuilderTest, FailWhenSameKeyInserted) {
//这里需要有一个临时变量，因为vs编译器当前没有
//支持operator=with initializer_list as a parameter
  std::unordered_map<std::string, std::vector<uint64_t>> hm = {
      {"repeatedkey", {0, 1, 2, 3}}};
  hash_map = std::move(hm);
  uint32_t num_hash_fun = 4;
  std::string user_key = "repeatedkey";

  unique_ptr<WritableFile> writable_file;
  fname = test::TmpDir() + "/FailWhenSameKeyInserted";
  ASSERT_OK(env_->NewWritableFile(fname, &writable_file, env_options_));
  unique_ptr<WritableFileWriter> file_writer(
      new WritableFileWriter(std::move(writable_file), EnvOptions()));
  CuckooTableBuilder builder(file_writer.get(), kHashTableRatio, num_hash_fun,
                             100, BytewiseComparator(), 1, false, false,
                             /*slicehash，0/*column_family_id*/，
                             kDefaultColumnFamilyName）；
  断言_OK（builder.status（））；

  builder.add（slice（getInternalKey（user_key，false）），slice（“value1”））；
  断言eq（builder.numEntries（），1U）；
  断言_OK（builder.status（））；
  builder.add（slice（getInternalKey（user_key，true）），slice（“value2”））；
  断言_eq（builder.numEntries（），2u）；
  断言_OK（builder.status（））；

  断言_true（builder.finish（）.isNotSupported（））；
  断言_OK（file_writer->close（））；
}
//命名空间rocksdb

int main（int argc，char**argv）
  ：：测试：：initgoogletest（&argc，argv）；
  返回run_all_tests（）；
}

否则
include<stdio.h>

int main（int argc，char**argv）
  fprintf（stderr，“由于rocksdb-lite中不支持布谷鸟表，所以跳过”）；
  返回0；
}

endif//rocksdb_lite
