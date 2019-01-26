
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2016，Red Hat，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#ifndef ROCKSDB_LITE

#include "rocksdb/utilities/env_librados.h"
#include <rados/librados.hpp>
#include "env/mock_env.h"
#include "util/testharness.h"

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "util/random.h"
#include <chrono>
#include <ostream>
#include "rocksdb/utilities/transaction_db.h"

class Timer {
  typedef std::chrono::high_resolution_clock high_resolution_clock;
  typedef std::chrono::milliseconds milliseconds;
public:
  explicit Timer(bool run = false)
  {
    if (run)
      Reset();
  }
  void Reset()
  {
    _start = high_resolution_clock::now();
  }
  milliseconds Elapsed() const
  {
    return std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - _start);
  }
  template <typename T, typename Traits>
  friend std::basic_ostream<T, Traits>& operator<<(std::basic_ostream<T, Traits>& out, const Timer& timer)
  {
    return out << timer.Elapsed().count();
  }
private:
  high_resolution_clock::time_point _start;
};

namespace rocksdb {

class EnvLibradosTest : public testing::Test {
public:
//我们将使用以下所有这些
  const std::string db_name = "env_librados_test_db";
  const std::string db_pool = db_name + "_pool";
  const char *keyring = "admin";
  const char *config = "../ceph/src/ceph.conf";

  EnvLibrados* env_;
  const EnvOptions soptions_;

  EnvLibradosTest()
    : env_(new EnvLibrados(db_name, config, db_pool)) {
  }
  ~EnvLibradosTest() {
    delete env_;
    librados::Rados rados;
    int ret = 0;
    do {
ret = rados.init("admin"); //只需使用client.admin密钥环
if (ret < 0) { //我们来处理可能出现的任何错误
        std::cerr << "couldn't initialize rados! error " << ret << std::endl;
        ret = EXIT_FAILURE;
        break;
      }

      ret = rados.conf_read_file(config);
      if (ret < 0) {
//如果配置文件格式不正确，这可能会失败，但这很困难。
        std::cerr << "failed to parse config file " << config
                  << "! error" << ret << std::endl;
        ret = EXIT_FAILURE;
        break;
      }

      /*
       *接下来，我们实际连接到集群
       **/


      ret = rados.connect();
      if (ret < 0) {
        std::cerr << "couldn't connect to cluster! error " << ret << std::endl;
        ret = EXIT_FAILURE;
        break;
      }

      /*
       *现在我们结束了，我们把游泳池移走，然后
       *请优雅地关闭连接。
       **/

      int delete_ret = rados.pool_delete(db_pool.c_str());
      if (delete_ret < 0) {
//小心不要
        std::cerr << "We failed to delete our test pool!" << db_pool << delete_ret << std::endl;
        ret = EXIT_FAILURE;
      }
    } while (0);
  }
};

TEST_F(EnvLibradosTest, Basics) {
  uint64_t file_size;
  unique_ptr<WritableFile> writable_file;
  std::vector<std::string> children;

  ASSERT_OK(env_->CreateDir("/dir"));
//检查目录是否为空。
  ASSERT_EQ(Status::NotFound(), env_->FileExists("/dir/non_existent"));
  ASSERT_TRUE(!env_->GetFileSize("/dir/non_existent", &file_size).ok());
  ASSERT_OK(env_->GetChildren("/dir", &children));
  ASSERT_EQ(0U, children.size());

//创建一个文件。
  ASSERT_OK(env_->NewWritableFile("/dir/f", &writable_file, soptions_));
  writable_file.reset();

//检查文件是否存在。
  ASSERT_OK(env_->FileExists("/dir/f"));
  ASSERT_OK(env_->GetFileSize("/dir/f", &file_size));
  ASSERT_EQ(0U, file_size);
  ASSERT_OK(env_->GetChildren("/dir", &children));
  ASSERT_EQ(1U, children.size());
  ASSERT_EQ("f", children[0]);

//写入文件。
  ASSERT_OK(env_->NewWritableFile("/dir/f", &writable_file, soptions_));
  ASSERT_OK(writable_file->Append("abc"));
  writable_file.reset();


//检查预期大小。
  ASSERT_OK(env_->GetFileSize("/dir/f", &file_size));
  ASSERT_EQ(3U, file_size);


//检查重命名是否有效。
  ASSERT_TRUE(!env_->RenameFile("/dir/non_existent", "/dir/g").ok());
  ASSERT_OK(env_->RenameFile("/dir/f", "/dir/g"));
  ASSERT_EQ(Status::NotFound(), env_->FileExists("/dir/f"));
  ASSERT_OK(env_->FileExists("/dir/g"));
  ASSERT_OK(env_->GetFileSize("/dir/g", &file_size));
  ASSERT_EQ(3U, file_size);

//检查打开不存在的文件是否失败。
  unique_ptr<SequentialFile> seq_file;
  unique_ptr<RandomAccessFile> rand_file;
  ASSERT_TRUE(
    !env_->NewSequentialFile("/dir/non_existent", &seq_file, soptions_).ok());
  ASSERT_TRUE(!seq_file);
  ASSERT_TRUE(!env_->NewRandomAccessFile("/dir/non_existent", &rand_file,
                                         soptions_).ok());
  ASSERT_TRUE(!rand_file);

//检查删除是否有效。
  ASSERT_TRUE(!env_->DeleteFile("/dir/non_existent").ok());
  ASSERT_OK(env_->DeleteFile("/dir/g"));
  ASSERT_EQ(Status::NotFound(), env_->FileExists("/dir/g"));
  ASSERT_OK(env_->GetChildren("/dir", &children));
  ASSERT_EQ(0U, children.size());
  ASSERT_OK(env_->DeleteDir("/dir"));
}

TEST_F(EnvLibradosTest, ReadWrite) {
  unique_ptr<WritableFile> writable_file;
  unique_ptr<SequentialFile> seq_file;
  unique_ptr<RandomAccessFile> rand_file;
  Slice result;
  char scratch[100];

  ASSERT_OK(env_->CreateDir("/dir"));

  ASSERT_OK(env_->NewWritableFile("/dir/f", &writable_file, soptions_));
  ASSERT_OK(writable_file->Append("hello "));
  ASSERT_OK(writable_file->Append("world"));
  writable_file.reset();

//按顺序阅读。
  ASSERT_OK(env_->NewSequentialFile("/dir/f", &seq_file, soptions_));
ASSERT_OK(seq_file->Read(5, &result, scratch));  //读“你好”。
  ASSERT_EQ(0, result.compare("hello"));
  ASSERT_OK(seq_file->Skip(1));
ASSERT_OK(seq_file->Read(1000, &result, scratch));  //读“世界”。
  ASSERT_EQ(0, result.compare("world"));
ASSERT_OK(seq_file->Read(1000, &result, scratch));  //试着读完EOF。
  ASSERT_EQ(0U, result.size());
ASSERT_OK(seq_file->Skip(100));  //尝试跳过文件结尾。
  ASSERT_OK(seq_file->Read(1000, &result, scratch));
  ASSERT_EQ(0U, result.size());

//随机读取。
  ASSERT_OK(env_->NewRandomAccessFile("/dir/f", &rand_file, soptions_));
ASSERT_OK(rand_file->Read(6, 5, &result, scratch));  //读“世界”。
  ASSERT_EQ(0, result.compare("world"));
ASSERT_OK(rand_file->Read(0, 5, &result, scratch));  //读“你好”。
  ASSERT_EQ(0, result.compare("hello"));
ASSERT_OK(rand_file->Read(10, 100, &result, scratch));  //读“D”。
  ASSERT_EQ(0, result.compare("d"));

//偏移量过高。
  ASSERT_OK(rand_file->Read(1000, 5, &result, scratch));
}

TEST_F(EnvLibradosTest, Locks) {
  FileLock* lock = nullptr;
  unique_ptr<WritableFile> writable_file;

  ASSERT_OK(env_->CreateDir("/dir"));

  ASSERT_OK(env_->NewWritableFile("/dir/f", &writable_file, soptions_));

//这些不是行动，但我们测试它们是否能成功。
  ASSERT_OK(env_->LockFile("some file", &lock));
  ASSERT_OK(env_->UnlockFile(lock));

  ASSERT_OK(env_->LockFile("/dir/f", &lock));
  ASSERT_OK(env_->UnlockFile(lock));
}

TEST_F(EnvLibradosTest, Misc) {
  std::string test_dir;
  ASSERT_OK(env_->GetTestDirectory(&test_dir));
  ASSERT_TRUE(!test_dir.empty());

  unique_ptr<WritableFile> writable_file;
  ASSERT_TRUE(!env_->NewWritableFile("/a/b", &writable_file, soptions_).ok());

  ASSERT_OK(env_->NewWritableFile("/a", &writable_file, soptions_));
//这些不是行动，但我们测试它们是否能成功。
  ASSERT_OK(writable_file->Sync());
  ASSERT_OK(writable_file->Flush());
  ASSERT_OK(writable_file->Close());
  writable_file.reset();
}

TEST_F(EnvLibradosTest, LargeWrite) {
  const size_t kWriteSize = 300 * 1024;
  char* scratch = new char[kWriteSize * 2];

  std::string write_data;
  for (size_t i = 0; i < kWriteSize; ++i) {
    write_data.append(1, 'h');
  }

  unique_ptr<WritableFile> writable_file;
  ASSERT_OK(env_->CreateDir("/dir"));
  ASSERT_OK(env_->NewWritableFile("/dir/g", &writable_file, soptions_));
  ASSERT_OK(writable_file->Append("foo"));
  ASSERT_OK(writable_file->Append(write_data));
  writable_file.reset();

  unique_ptr<SequentialFile> seq_file;
  Slice result;
  ASSERT_OK(env_->NewSequentialFile("/dir/g", &seq_file, soptions_));
ASSERT_OK(seq_file->Read(3, &result, scratch));  //读“FO”。
  ASSERT_EQ(0, result.compare("foo"));

  size_t read = 0;
  std::string read_data;
  while (read < kWriteSize) {
    ASSERT_OK(seq_file->Read(kWriteSize - read, &result, scratch));
    read_data.append(result.data(), result.size());
    read += result.size();
  }
  ASSERT_TRUE(write_data == read_data);
  delete[] scratch;
}

TEST_F(EnvLibradosTest, FrequentlySmallWrite) {
  const size_t kWriteSize = 1 << 10;
  char* scratch = new char[kWriteSize * 2];

  std::string write_data;
  for (size_t i = 0; i < kWriteSize; ++i) {
    write_data.append(1, 'h');
  }

  unique_ptr<WritableFile> writable_file;
  ASSERT_OK(env_->CreateDir("/dir"));
  ASSERT_OK(env_->NewWritableFile("/dir/g", &writable_file, soptions_));
  ASSERT_OK(writable_file->Append("foo"));

  for (size_t i = 0; i < kWriteSize; ++i) {
    ASSERT_OK(writable_file->Append("h"));
  }
  writable_file.reset();

  unique_ptr<SequentialFile> seq_file;
  Slice result;
  ASSERT_OK(env_->NewSequentialFile("/dir/g", &seq_file, soptions_));
ASSERT_OK(seq_file->Read(3, &result, scratch));  //读“FO”。
  ASSERT_EQ(0, result.compare("foo"));

  size_t read = 0;
  std::string read_data;
  while (read < kWriteSize) {
    ASSERT_OK(seq_file->Read(kWriteSize - read, &result, scratch));
    read_data.append(result.data(), result.size());
    read += result.size();
  }
  ASSERT_TRUE(write_data == read_data);
  delete[] scratch;
}

TEST_F(EnvLibradosTest, Truncate) {
  const size_t kWriteSize = 300 * 1024;
  const size_t truncSize = 1024;
  std::string write_data;
  for (size_t i = 0; i < kWriteSize; ++i) {
    write_data.append(1, 'h');
  }

  unique_ptr<WritableFile> writable_file;
  ASSERT_OK(env_->CreateDir("/dir"));
  ASSERT_OK(env_->NewWritableFile("/dir/g", &writable_file, soptions_));
  ASSERT_OK(writable_file->Append(write_data));
  ASSERT_EQ(writable_file->GetFileSize(), kWriteSize);
  ASSERT_OK(writable_file->Truncate(truncSize));
  ASSERT_EQ(writable_file->GetFileSize(), truncSize);
  writable_file.reset();
}

TEST_F(EnvLibradosTest, DBBasics) {
  std::string kDBPath = "/tmp/DBBasics";
  DB* db;
  Options options;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options.create_if_missing = true;
  options.env = env_;

//开放数据库
  Status s = DB::Open(options, kDBPath, &db);
  assert(s.ok());

//设置关键值
  s = db->Put(WriteOptions(), "key1", "value");
  assert(s.ok());
  std::string value;
//获取价值
  s = db->Get(ReadOptions(), "key1", &value);
  assert(s.ok());
  assert(value == "value");

//自动应用一组更新
  {
    WriteBatch batch;
    batch.Delete("key1");
    batch.Put("key2", value);
    s = db->Write(WriteOptions(), &batch);
  }

  s = db->Get(ReadOptions(), "key1", &value);
  assert(s.IsNotFound());

  db->Get(ReadOptions(), "key2", &value);
  assert(value == "value");

  delete db;
}

TEST_F(EnvLibradosTest, DBLoadKeysInRandomOrder) {
  char key[20] = {0}, value[20] = {0};
  int max_loop = 1 << 10;
  Timer timer(false);
  std::cout << "Test size : loop(" << max_loop << ")" << std::endl;
  /****************************
            使用默认的Env
  **********************/

  std::string kDBPath1 = "/tmp/DBLoadKeysInRandomOrder1";
  DB* db1;
  Options options1;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options1.IncreaseParallelism();
  options1.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options1.create_if_missing = true;

//开放数据库
  Status s1 = DB::Open(options1, kDBPath1, &db1);
  assert(s1.ok());

  rocksdb::Random64 r1(time(nullptr));

  timer.Reset();
  for (int i = 0; i < max_loop; ++i) {
    snprintf(key,
             20,
             "%16lx",
             (unsigned long)r1.Uniform(std::numeric_limits<uint64_t>::max()));
    snprintf(value,
             20,
             "%16lx",
             (unsigned long)r1.Uniform(std::numeric_limits<uint64_t>::max()));
//设置关键值
    s1 = db1->Put(WriteOptions(), key, value);
    assert(s1.ok());
  }
  std::cout << "Time by default : " << timer << "ms" << std::endl;
  delete db1;

  /****************************
            使用libados env
  **********************/

  std::string kDBPath2 = "/tmp/DBLoadKeysInRandomOrder2";
  DB* db2;
  Options options2;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options2.IncreaseParallelism();
  options2.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options2.create_if_missing = true;
  options2.env = env_;

//开放数据库
  Status s2 = DB::Open(options2, kDBPath2, &db2);
  assert(s2.ok());

  rocksdb::Random64 r2(time(nullptr));

  timer.Reset();
  for (int i = 0; i < max_loop; ++i) {
    snprintf(key,
             20,
             "%16lx",
             (unsigned long)r2.Uniform(std::numeric_limits<uint64_t>::max()));
    snprintf(value,
             20,
             "%16lx",
             (unsigned long)r2.Uniform(std::numeric_limits<uint64_t>::max()));
//设置关键值
    s2 = db2->Put(WriteOptions(), key, value);
    assert(s2.ok());
  }
  std::cout << "Time by librados : " << timer << "ms" << std::endl;
  delete db2;
}

TEST_F(EnvLibradosTest, DBBulkLoadKeysInRandomOrder) {
  char key[20] = {0}, value[20] = {0};
  int max_loop = 1 << 6;
  int bulk_size = 1 << 15;
  Timer timer(false);
  std::cout << "Test size : loop(" << max_loop << "); bulk_size(" << bulk_size << ")" << std::endl;
  /****************************
            使用默认的Env
  **********************/

  std::string kDBPath1 = "/tmp/DBBulkLoadKeysInRandomOrder1";
  DB* db1;
  Options options1;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options1.IncreaseParallelism();
  options1.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options1.create_if_missing = true;

//开放数据库
  Status s1 = DB::Open(options1, kDBPath1, &db1);
  assert(s1.ok());

  rocksdb::Random64 r1(time(nullptr));

  timer.Reset();
  for (int i = 0; i < max_loop; ++i) {
    WriteBatch batch;
    for (int j = 0; j < bulk_size; ++j) {
      snprintf(key,
               20,
               "%16lx",
               (unsigned long)r1.Uniform(std::numeric_limits<uint64_t>::max()));
      snprintf(value,
               20,
               "%16lx",
               (unsigned long)r1.Uniform(std::numeric_limits<uint64_t>::max()));
      batch.Put(key, value);
    }
    s1 = db1->Write(WriteOptions(), &batch);
    assert(s1.ok());
  }
  std::cout << "Time by default : " << timer << "ms" << std::endl;
  delete db1;

  /****************************
            使用libados env
  **********************/

  std::string kDBPath2 = "/tmp/DBBulkLoadKeysInRandomOrder2";
  DB* db2;
  Options options2;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options2.IncreaseParallelism();
  options2.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options2.create_if_missing = true;
  options2.env = env_;

//开放数据库
  Status s2 = DB::Open(options2, kDBPath2, &db2);
  assert(s2.ok());

  rocksdb::Random64 r2(time(nullptr));

  timer.Reset();
  for (int i = 0; i < max_loop; ++i) {
    WriteBatch batch;
    for (int j = 0; j < bulk_size; ++j) {
      snprintf(key,
               20,
               "%16lx",
               (unsigned long)r2.Uniform(std::numeric_limits<uint64_t>::max()));
      snprintf(value,
               20,
               "%16lx",
               (unsigned long)r2.Uniform(std::numeric_limits<uint64_t>::max()));
      batch.Put(key, value);
    }
    s2 = db2->Write(WriteOptions(), &batch);
    assert(s2.ok());
  }
  std::cout << "Time by librados : " << timer << "ms" << std::endl;
  delete db2;
}

TEST_F(EnvLibradosTest, DBBulkLoadKeysInSequentialOrder) {
  char key[20] = {0}, value[20] = {0};
  int max_loop = 1 << 6;
  int bulk_size = 1 << 15;
  Timer timer(false);
  std::cout << "Test size : loop(" << max_loop << "); bulk_size(" << bulk_size << ")" << std::endl;
  /****************************
            使用默认的Env
  **********************/

  std::string kDBPath1 = "/tmp/DBBulkLoadKeysInSequentialOrder1";
  DB* db1;
  Options options1;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options1.IncreaseParallelism();
  options1.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options1.create_if_missing = true;

//开放数据库
  Status s1 = DB::Open(options1, kDBPath1, &db1);
  assert(s1.ok());

  rocksdb::Random64 r1(time(nullptr));

  timer.Reset();
  for (int i = 0; i < max_loop; ++i) {
    WriteBatch batch;
    for (int j = 0; j < bulk_size; ++j) {
      snprintf(key,
               20,
               "%019lld",
               (long long)(i * bulk_size + j));
      snprintf(value,
               20,
               "%16lx",
               (unsigned long)r1.Uniform(std::numeric_limits<uint64_t>::max()));
      batch.Put(key, value);
    }
    s1 = db1->Write(WriteOptions(), &batch);
    assert(s1.ok());
  }
  std::cout << "Time by default : " << timer << "ms" << std::endl;
  delete db1;

  /****************************
            使用libados env
  **********************/

  std::string kDBPath2 = "/tmp/DBBulkLoadKeysInSequentialOrder2";
  DB* db2;
  Options options2;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options2.IncreaseParallelism();
  options2.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options2.create_if_missing = true;
  options2.env = env_;

//开放数据库
  Status s2 = DB::Open(options2, kDBPath2, &db2);
  assert(s2.ok());

  rocksdb::Random64 r2(time(nullptr));

  timer.Reset();
  for (int i = 0; i < max_loop; ++i) {
    WriteBatch batch;
    for (int j = 0; j < bulk_size; ++j) {
      snprintf(key,
               20,
               "%16lx",
               (unsigned long)r2.Uniform(std::numeric_limits<uint64_t>::max()));
      snprintf(value,
               20,
               "%16lx",
               (unsigned long)r2.Uniform(std::numeric_limits<uint64_t>::max()));
      batch.Put(key, value);
    }
    s2 = db2->Write(WriteOptions(), &batch);
    assert(s2.ok());
  }
  std::cout << "Time by librados : " << timer << "ms" << std::endl;
  delete db2;
}

TEST_F(EnvLibradosTest, DBRandomRead) {
  char key[20] = {0}, value[20] = {0};
  int max_loop = 1 << 6;
  int bulk_size = 1 << 10;
  int read_loop = 1 << 20;
  Timer timer(false);
  std::cout << "Test size : keys_num(" << max_loop << ", " << bulk_size << "); read_loop(" << read_loop << ")" << std::endl;
  /****************************
            使用默认的Env
  **********************/

  std::string kDBPath1 = "/tmp/DBRandomRead1";
  DB* db1;
  Options options1;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options1.IncreaseParallelism();
  options1.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options1.create_if_missing = true;

//开放数据库
  Status s1 = DB::Open(options1, kDBPath1, &db1);
  assert(s1.ok());

  rocksdb::Random64 r1(time(nullptr));


  for (int i = 0; i < max_loop; ++i) {
    WriteBatch batch;
    for (int j = 0; j < bulk_size; ++j) {
      snprintf(key,
               20,
               "%019lld",
               (long long)(i * bulk_size + j));
      snprintf(value,
               20,
               "%16lx",
               (unsigned long)r1.Uniform(std::numeric_limits<uint64_t>::max()));
      batch.Put(key, value);
    }
    s1 = db1->Write(WriteOptions(), &batch);
    assert(s1.ok());
  }
  timer.Reset();
  int base1 = 0, offset1 = 0;
  for (int i = 0; i < read_loop; ++i) {
    base1 = r1.Uniform(max_loop);
    offset1 = r1.Uniform(bulk_size);
    std::string value1;
    snprintf(key,
             20,
             "%019lld",
             (long long)(base1 * bulk_size + offset1));
    s1 = db1->Get(ReadOptions(), key, &value1);
    assert(s1.ok());
  }
  std::cout << "Time by default : " << timer << "ms" << std::endl;
  delete db1;

  /****************************
            使用libados env
  **********************/

  std::string kDBPath2 = "/tmp/DBRandomRead2";
  DB* db2;
  Options options2;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options2.IncreaseParallelism();
  options2.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options2.create_if_missing = true;
  options2.env = env_;

//开放数据库
  Status s2 = DB::Open(options2, kDBPath2, &db2);
  assert(s2.ok());

  rocksdb::Random64 r2(time(nullptr));

  for (int i = 0; i < max_loop; ++i) {
    WriteBatch batch;
    for (int j = 0; j < bulk_size; ++j) {
      snprintf(key,
               20,
               "%019lld",
               (long long)(i * bulk_size + j));
      snprintf(value,
               20,
               "%16lx",
               (unsigned long)r2.Uniform(std::numeric_limits<uint64_t>::max()));
      batch.Put(key, value);
    }
    s2 = db2->Write(WriteOptions(), &batch);
    assert(s2.ok());
  }

  timer.Reset();
  int base2 = 0, offset2 = 0;
  for (int i = 0; i < read_loop; ++i) {
    base2 = r2.Uniform(max_loop);
    offset2 = r2.Uniform(bulk_size);
    std::string value2;
    snprintf(key,
             20,
             "%019lld",
             (long long)(base2 * bulk_size + offset2));
    s2 = db2->Get(ReadOptions(), key, &value2);
    if (!s2.ok()) {
      std::cout << s2.ToString() << std::endl;
    }
    assert(s2.ok());
  }
  std::cout << "Time by librados : " << timer << "ms" << std::endl;
  delete db2;
}

class EnvLibradosMutipoolTest : public testing::Test {
public:
//我们将使用以下所有这些
  const std::string client_name = "client.admin";
  const std::string cluster_name = "ceph";
  const uint64_t flags = 0;
  const std::string db_name = "env_librados_test_db";
  const std::string db_pool = db_name + "_pool";
  const std::string wal_dir = "/wal";
  const std::string wal_pool = db_name + "_wal_pool";
  const size_t write_buffer_size = 1 << 20;
  const char *keyring = "admin";
  const char *config = "../ceph/src/ceph.conf";

  EnvLibrados* env_;
  const EnvOptions soptions_;

  EnvLibradosMutipoolTest() {
    env_ = new EnvLibrados(client_name, cluster_name, flags, db_name, config, db_pool, wal_dir, wal_pool, write_buffer_size);
  }
  ~EnvLibradosMutipoolTest() {
    delete env_;
    librados::Rados rados;
    int ret = 0;
    do {
ret = rados.init("admin"); //只需使用client.admin密钥环
if (ret < 0) { //我们来处理可能出现的任何错误
        std::cerr << "couldn't initialize rados! error " << ret << std::endl;
        ret = EXIT_FAILURE;
        break;
      }

      ret = rados.conf_read_file(config);
      if (ret < 0) {
//如果配置文件格式不正确，这可能会失败，但这很困难。
        std::cerr << "failed to parse config file " << config
                  << "! error" << ret << std::endl;
        ret = EXIT_FAILURE;
        break;
      }

      /*
       *接下来，我们实际连接到集群
       **/


      ret = rados.connect();
      if (ret < 0) {
        std::cerr << "couldn't connect to cluster! error " << ret << std::endl;
        ret = EXIT_FAILURE;
        break;
      }

      /*
       *现在我们结束了，我们把游泳池移走，然后
       *请优雅地关闭连接。
       **/

      int delete_ret = rados.pool_delete(db_pool.c_str());
      if (delete_ret < 0) {
//小心不要
        std::cerr << "We failed to delete our test pool!" << db_pool << delete_ret << std::endl;
        ret = EXIT_FAILURE;
      }
      delete_ret = rados.pool_delete(wal_pool.c_str());
      if (delete_ret < 0) {
//小心不要
        std::cerr << "We failed to delete our test pool!" << wal_pool << delete_ret << std::endl;
        ret = EXIT_FAILURE;
      }
    } while (0);
  }
};

TEST_F(EnvLibradosMutipoolTest, Basics) {
  uint64_t file_size;
  unique_ptr<WritableFile> writable_file;
  std::vector<std::string> children;
  std::vector<std::string> v = {"/tmp/dir1", "/tmp/dir2", "/tmp/dir3", "/tmp/dir4", "dir"};

  for (size_t i = 0; i < v.size(); ++i) {
    std::string dir = v[i];
    std::string dir_non_existent = dir + "/non_existent";
    std::string dir_f = dir + "/f";
    std::string dir_g = dir + "/g";

    ASSERT_OK(env_->CreateDir(dir.c_str()));
//检查目录是否为空。
    ASSERT_EQ(Status::NotFound(), env_->FileExists(dir_non_existent.c_str()));
    ASSERT_TRUE(!env_->GetFileSize(dir_non_existent.c_str(), &file_size).ok());
    ASSERT_OK(env_->GetChildren(dir.c_str(), &children));
    ASSERT_EQ(0U, children.size());

//创建一个文件。
    ASSERT_OK(env_->NewWritableFile(dir_f.c_str(), &writable_file, soptions_));
    writable_file.reset();

//检查文件是否存在。
    ASSERT_OK(env_->FileExists(dir_f.c_str()));
    ASSERT_OK(env_->GetFileSize(dir_f.c_str(), &file_size));
    ASSERT_EQ(0U, file_size);
    ASSERT_OK(env_->GetChildren(dir.c_str(), &children));
    ASSERT_EQ(1U, children.size());
    ASSERT_EQ("f", children[0]);

//写入文件。
    ASSERT_OK(env_->NewWritableFile(dir_f.c_str(), &writable_file, soptions_));
    ASSERT_OK(writable_file->Append("abc"));
    writable_file.reset();


//检查预期大小。
    ASSERT_OK(env_->GetFileSize(dir_f.c_str(), &file_size));
    ASSERT_EQ(3U, file_size);


//
    ASSERT_TRUE(!env_->RenameFile(dir_non_existent.c_str(), dir_g.c_str()).ok());
    ASSERT_OK(env_->RenameFile(dir_f.c_str(), dir_g.c_str()));
    ASSERT_EQ(Status::NotFound(), env_->FileExists(dir_f.c_str()));
    ASSERT_OK(env_->FileExists(dir_g.c_str()));
    ASSERT_OK(env_->GetFileSize(dir_g.c_str(), &file_size));
    ASSERT_EQ(3U, file_size);

//检查打开不存在的文件是否失败。
    unique_ptr<SequentialFile> seq_file;
    unique_ptr<RandomAccessFile> rand_file;
    ASSERT_TRUE(
      !env_->NewSequentialFile(dir_non_existent.c_str(), &seq_file, soptions_).ok());
    ASSERT_TRUE(!seq_file);
    ASSERT_TRUE(!env_->NewRandomAccessFile(dir_non_existent.c_str(), &rand_file,
                                           soptions_).ok());
    ASSERT_TRUE(!rand_file);

//检查删除是否有效。
    ASSERT_TRUE(!env_->DeleteFile(dir_non_existent.c_str()).ok());
    ASSERT_OK(env_->DeleteFile(dir_g.c_str()));
    ASSERT_EQ(Status::NotFound(), env_->FileExists(dir_g.c_str()));
    ASSERT_OK(env_->GetChildren(dir.c_str(), &children));
    ASSERT_EQ(0U, children.size());
    ASSERT_OK(env_->DeleteDir(dir.c_str()));
  }
}

TEST_F(EnvLibradosMutipoolTest, DBBasics) {
  std::string kDBPath = "/tmp/DBBasics";
  std::string walPath = "/tmp/wal";
  DB* db;
  Options options;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options.create_if_missing = true;
  options.env = env_;
  options.wal_dir = walPath;

//开放数据库
  Status s = DB::Open(options, kDBPath, &db);
  assert(s.ok());

//设置关键值
  s = db->Put(WriteOptions(), "key1", "value");
  assert(s.ok());
  std::string value;
//获取价值
  s = db->Get(ReadOptions(), "key1", &value);
  assert(s.ok());
  assert(value == "value");

//自动应用一组更新
  {
    WriteBatch batch;
    batch.Delete("key1");
    batch.Put("key2", value);
    s = db->Write(WriteOptions(), &batch);
  }

  s = db->Get(ReadOptions(), "key1", &value);
  assert(s.IsNotFound());

  db->Get(ReadOptions(), "key2", &value);
  assert(value == "value");

  delete db;
}

TEST_F(EnvLibradosMutipoolTest, DBBulkLoadKeysInRandomOrder) {
  char key[20] = {0}, value[20] = {0};
  int max_loop = 1 << 6;
  int bulk_size = 1 << 15;
  Timer timer(false);
  std::cout << "Test size : loop(" << max_loop << "); bulk_size(" << bulk_size << ")" << std::endl;
  /****************************
            使用默认的Env
  **********************/

  std::string kDBPath1 = "/tmp/DBBulkLoadKeysInRandomOrder1";
  std::string walPath = "/tmp/wal";
  DB* db1;
  Options options1;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options1.IncreaseParallelism();
  options1.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options1.create_if_missing = true;

//开放数据库
  Status s1 = DB::Open(options1, kDBPath1, &db1);
  assert(s1.ok());

  rocksdb::Random64 r1(time(nullptr));

  timer.Reset();
  for (int i = 0; i < max_loop; ++i) {
    WriteBatch batch;
    for (int j = 0; j < bulk_size; ++j) {
      snprintf(key,
               20,
               "%16lx",
               (unsigned long)r1.Uniform(std::numeric_limits<uint64_t>::max()));
      snprintf(value,
               20,
               "%16lx",
               (unsigned long)r1.Uniform(std::numeric_limits<uint64_t>::max()));
      batch.Put(key, value);
    }
    s1 = db1->Write(WriteOptions(), &batch);
    assert(s1.ok());
  }
  std::cout << "Time by default : " << timer << "ms" << std::endl;
  delete db1;

  /****************************
            使用libados env
  **********************/

  std::string kDBPath2 = "/tmp/DBBulkLoadKeysInRandomOrder2";
  DB* db2;
  Options options2;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options2.IncreaseParallelism();
  options2.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options2.create_if_missing = true;
  options2.env = env_;
  options2.wal_dir = walPath;

//开放数据库
  Status s2 = DB::Open(options2, kDBPath2, &db2);
  if (!s2.ok()) {
    std::cerr << s2.ToString() << std::endl;
  }
  assert(s2.ok());

  rocksdb::Random64 r2(time(nullptr));

  timer.Reset();
  for (int i = 0; i < max_loop; ++i) {
    WriteBatch batch;
    for (int j = 0; j < bulk_size; ++j) {
      snprintf(key,
               20,
               "%16lx",
               (unsigned long)r2.Uniform(std::numeric_limits<uint64_t>::max()));
      snprintf(value,
               20,
               "%16lx",
               (unsigned long)r2.Uniform(std::numeric_limits<uint64_t>::max()));
      batch.Put(key, value);
    }
    s2 = db2->Write(WriteOptions(), &batch);
    assert(s2.ok());
  }
  std::cout << "Time by librados : " << timer << "ms" << std::endl;
  delete db2;
}

TEST_F(EnvLibradosMutipoolTest, DBTransactionDB) {
  std::string kDBPath = "/tmp/DBTransactionDB";
//开放数据库
  Options options;
  TransactionDBOptions txn_db_options;
  options.create_if_missing = true;
  options.env = env_;
  TransactionDB* txn_db;

  Status s = TransactionDB::Open(options, txn_db_options, kDBPath, &txn_db);
  assert(s.ok());

  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  std::string value;

///////////////////////////////////////
//
//简单的optimatictransaction示例（“read committed”）。
//
///////////////////////////////////////

//启动事务
  Transaction* txn = txn_db->BeginTransaction(write_options);
  assert(txn);

//读取此事务中的密钥
  s = txn->Get(read_options, "abc", &value);
  assert(s.IsNotFound());

//在此事务中写入密钥
  s = txn->Put("abc", "def");
  assert(s.ok());

//读取此事务外部的密钥。不影响TXN。
  s = txn_db->Get(read_options, "abc", &value);

//在此事务之外写入密钥。
//不会影响txn，因为这是一个不相关的密钥。如果我们写了“ABC”键
//在这里，事务将无法提交。
  s = txn_db->Put(write_options, "xyz", "zzz");

//提交事务
  s = txn->Commit();
  assert(s.ok());
  delete txn;

///////////////////////////////////////
//
//“可重复读取”（快照隔离）示例
//--使用单个快照
//
///////////////////////////////////////

//通过设置set_snapshot=true在事务开始时设置快照
  txn_options.set_snapshot = true;
  txn = txn_db->BeginTransaction(write_options, txn_options);

  const Snapshot* snapshot = txn->GetSnapshot();

//在事务外写入密钥
  s = txn_db->Put(write_options, "abc", "xyz");
  assert(s.ok());

//尝试使用快照读取密钥。这将失败，因为
//此txn之外的上一次写入与此读取冲突。
  read_options.snapshot = snapshot;
  s = txn->GetForUpdate(read_options, "abc", &value);
  assert(s.IsBusy());

  txn->Rollback();

  delete txn;
//从读取选项中清除快照，因为它不再有效
  read_options.snapshot = nullptr;
  snapshot = nullptr;

///////////////////////////////////////
//
//“read committed”（单调原子视图）示例
//--使用多个快照
//
///////////////////////////////////////

//在本例中，我们多次设置快照。这可能是
//只有当你有非常严格的隔离要求
//实施。

//在事务开始时设置快照
  txn_options.set_snapshot = true;
  txn = txn_db->BeginTransaction(write_options, txn_options);

//对键“X”进行读写操作
  read_options.snapshot = txn_db->GetSnapshot();
  s = txn->Get(read_options, "x", &value);
  txn->Put("x", "x");

//在事务外部写入键“Y”
  s = txn_db->Put(write_options, "y", "y");

//在事务中设置新快照
  txn->SetSnapshot();
  txn->SetSavePoint();
  read_options.snapshot = txn_db->GetSnapshot();

//对“Y”键进行读写操作
//由于快照是高级的，因此在
//事务不冲突。
  s = txn->GetForUpdate(read_options, "y", &value);
  txn->Put("y", "y");

//决定我们要从此事务中恢复最后一次写入。
  txn->RollbackToSavePoint();

//承诺。
  s = txn->Commit();
  assert(s.ok());
  delete txn;
//从读取选项中清除快照，因为它不再有效
  read_options.snapshot = nullptr;

//清理
  delete txn_db;
  DestroyDB(kDBPath, options);
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else
#include <stdio.h>

int main(int argc, char** argv) {
  fprintf(stderr, "SKIPPED as EnvMirror is not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //！摇滚乐
