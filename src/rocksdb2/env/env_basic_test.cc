
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

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include "env/mock_env.h"
#include "rocksdb/env.h"
#include "rocksdb/utilities/object_registry.h"
#include "util/testharness.h"

namespace rocksdb {

//规范化环境之间的细微差异，以便这些测试用例可以
//在所有环境下运行。
class NormalizingEnvWrapper : public EnvWrapper {
 public:
  explicit NormalizingEnvWrapper(Env* base) : EnvWrapper(base) {}

//移除。并且…从目录列表
  virtual Status GetChildren(const std::string& dir,
                             std::vector<std::string>* result) override {
    Status status = EnvWrapper::GetChildren(dir, result);
    if (status.ok()) {
      result->erase(std::remove_if(result->begin(), result->end(),
                                   [](const std::string& s) {
                                     return s == "." || s == "..";
                                   }),
                    result->end());
    }
    return status;
  }

//移除。并且…从目录列表
  virtual Status GetChildrenFileAttributes(
      const std::string& dir, std::vector<FileAttributes>* result) override {
    Status status = EnvWrapper::GetChildrenFileAttributes(dir, result);
    if (status.ok()) {
      result->erase(std::remove_if(result->begin(), result->end(),
                                   [](const FileAttributes& fa) {
                                     return fa.name == "." || fa.name == "..";
                                   }),
                    result->end());
    }
    return status;
  }
};

class EnvBasicTestWithParam : public testing::Test,
                              public ::testing::WithParamInterface<Env*> {
 public:
  Env* env_;
  const EnvOptions soptions_;
  std::string test_dir_;

  EnvBasicTestWithParam() : env_(GetParam()) {
    test_dir_ = test::TmpDir(env_) + "/env_basic_test";
  }

  void SetUp() {
    env_->CreateDirIfMissing(test_dir_);
  }

  void TearDown() {
    std::vector<std::string> files;
    env_->GetChildren(test_dir_, &files);
    for (const auto& file : files) {
//不知道是文件还是目录，请同时尝试。测试必须
//只创建文件或空目录，因此必须成功，否则
//目录已损坏。
      Status s = env_->DeleteFile(test_dir_ + "/" + file);
      if (!s.ok()) {
        ASSERT_OK(env_->DeleteDir(test_dir_ + "/" + file));
      }
    }
  }
};

class EnvMoreTestWithParam : public EnvBasicTestWithParam {};

static std::unique_ptr<Env> def_env(new NormalizingEnvWrapper(Env::Default()));
INSTANTIATE_TEST_CASE_P(EnvDefault, EnvBasicTestWithParam,
                        ::testing::Values(def_env.get()));
INSTANTIATE_TEST_CASE_P(EnvDefault, EnvMoreTestWithParam,
                        ::testing::Values(def_env.get()));

static std::unique_ptr<Env> mock_env(new MockEnv(Env::Default()));
INSTANTIATE_TEST_CASE_P(MockEnv, EnvBasicTestWithParam,
                        ::testing::Values(mock_env.get()));
#ifndef ROCKSDB_LITE
static std::unique_ptr<Env> mem_env(NewMemEnv(Env::Default()));
INSTANTIATE_TEST_CASE_P(MemEnv, EnvBasicTestWithParam,
                        ::testing::Values(mem_env.get()));

namespace {

//返回0或1 env*的向量，具体取决于env是否注册为
//我是一个特立独行的人。
//
//返回空向量（而不是nullptr）的目的是
//当给定空集合时，valuesin（）将跳过正在运行的测试。
std::vector<Env*> GetCustomEnvs() {
  static Env* custom_env;
  static std::unique_ptr<Env> custom_env_guard;
  static bool init = false;
  if (!init) {
    init = true;
    const char* uri = getenv("TEST_ENV_URI");
    if (uri != nullptr) {
      custom_env = NewCustomObject<Env>(uri, &custom_env_guard);
    }
  }

  std::vector<Env*> res;
  if (custom_env != nullptr) {
    res.emplace_back(custom_env);
  }
  return res;
}

}  //匿名命名空间

INSTANTIATE_TEST_CASE_P(CustomEnv, EnvBasicTestWithParam,
                        ::testing::ValuesIn(GetCustomEnvs()));

INSTANTIATE_TEST_CASE_P(CustomEnv, EnvMoreTestWithParam,
                        ::testing::ValuesIn(GetCustomEnvs()));

#endif  //摇滚乐

TEST_P(EnvBasicTestWithParam, Basics) {
  uint64_t file_size;
  unique_ptr<WritableFile> writable_file;
  std::vector<std::string> children;

//检查目录是否为空。
  ASSERT_EQ(Status::NotFound(), env_->FileExists(test_dir_ + "/non_existent"));
  ASSERT_TRUE(!env_->GetFileSize(test_dir_ + "/non_existent", &file_size).ok());
  ASSERT_OK(env_->GetChildren(test_dir_, &children));
  ASSERT_EQ(0U, children.size());

//创建一个文件。
  ASSERT_OK(env_->NewWritableFile(test_dir_ + "/f", &writable_file, soptions_));
  ASSERT_OK(writable_file->Close());
  writable_file.reset();

//检查文件是否存在。
  ASSERT_OK(env_->FileExists(test_dir_ + "/f"));
  ASSERT_OK(env_->GetFileSize(test_dir_ + "/f", &file_size));
  ASSERT_EQ(0U, file_size);
  ASSERT_OK(env_->GetChildren(test_dir_, &children));
  ASSERT_EQ(1U, children.size());
  ASSERT_EQ("f", children[0]);
  ASSERT_OK(env_->DeleteFile(test_dir_ + "/f"));

//写入文件。
  ASSERT_OK(
      env_->NewWritableFile(test_dir_ + "/f1", &writable_file, soptions_));
  ASSERT_OK(writable_file->Append("abc"));
  ASSERT_OK(writable_file->Close());
  writable_file.reset();
  ASSERT_OK(
      env_->NewWritableFile(test_dir_ + "/f2", &writable_file, soptions_));
  ASSERT_OK(writable_file->Close());
  writable_file.reset();

//检查预期大小。
  ASSERT_OK(env_->GetFileSize(test_dir_ + "/f1", &file_size));
  ASSERT_EQ(3U, file_size);

//检查重命名是否有效。
  ASSERT_TRUE(
      !env_->RenameFile(test_dir_ + "/non_existent", test_dir_ + "/g").ok());
  ASSERT_OK(env_->RenameFile(test_dir_ + "/f1", test_dir_ + "/g"));
  ASSERT_EQ(Status::NotFound(), env_->FileExists(test_dir_ + "/f1"));
  ASSERT_OK(env_->FileExists(test_dir_ + "/g"));
  ASSERT_OK(env_->GetFileSize(test_dir_ + "/g", &file_size));
  ASSERT_EQ(3U, file_size);

//检查重命名覆盖工作
  ASSERT_OK(env_->RenameFile(test_dir_ + "/f2", test_dir_ + "/g"));
  ASSERT_OK(env_->GetFileSize(test_dir_ + "/g", &file_size));
  ASSERT_EQ(0U, file_size);

//检查打开不存在的文件是否失败。
  unique_ptr<SequentialFile> seq_file;
  unique_ptr<RandomAccessFile> rand_file;
  ASSERT_TRUE(!env_->NewSequentialFile(test_dir_ + "/non_existent", &seq_file,
                                       soptions_)
                   .ok());
  ASSERT_TRUE(!seq_file);
  ASSERT_TRUE(!env_->NewRandomAccessFile(test_dir_ + "/non_existent",
                                         &rand_file, soptions_)
                   .ok());
  ASSERT_TRUE(!rand_file);

//检查删除是否有效。
  ASSERT_TRUE(!env_->DeleteFile(test_dir_ + "/non_existent").ok());
  ASSERT_OK(env_->DeleteFile(test_dir_ + "/g"));
  ASSERT_EQ(Status::NotFound(), env_->FileExists(test_dir_ + "/g"));
  ASSERT_OK(env_->GetChildren(test_dir_, &children));
  ASSERT_EQ(0U, children.size());
  ASSERT_TRUE(
      env_->GetChildren(test_dir_ + "/non_existent", &children).IsNotFound());
}

TEST_P(EnvBasicTestWithParam, ReadWrite) {
  unique_ptr<WritableFile> writable_file;
  unique_ptr<SequentialFile> seq_file;
  unique_ptr<RandomAccessFile> rand_file;
  Slice result;
  char scratch[100];

  ASSERT_OK(env_->NewWritableFile(test_dir_ + "/f", &writable_file, soptions_));
  ASSERT_OK(writable_file->Append("hello "));
  ASSERT_OK(writable_file->Append("world"));
  ASSERT_OK(writable_file->Close());
  writable_file.reset();

//按顺序阅读。
  ASSERT_OK(env_->NewSequentialFile(test_dir_ + "/f", &seq_file, soptions_));
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
  ASSERT_OK(env_->NewRandomAccessFile(test_dir_ + "/f", &rand_file, soptions_));
ASSERT_OK(rand_file->Read(6, 5, &result, scratch));  //读“世界”。
  ASSERT_EQ(0, result.compare("world"));
ASSERT_OK(rand_file->Read(0, 5, &result, scratch));  //读“你好”。
  ASSERT_EQ(0, result.compare("hello"));
ASSERT_OK(rand_file->Read(10, 100, &result, scratch));  //读“D”。
  ASSERT_EQ(0, result.compare("d"));

//偏移量过高。
  ASSERT_TRUE(rand_file->Read(1000, 5, &result, scratch).ok());
}

TEST_P(EnvBasicTestWithParam, Misc) {
  unique_ptr<WritableFile> writable_file;
  ASSERT_OK(env_->NewWritableFile(test_dir_ + "/b", &writable_file, soptions_));

//这些不是行动，但我们测试它们是否能成功。
  ASSERT_OK(writable_file->Sync());
  ASSERT_OK(writable_file->Flush());
  ASSERT_OK(writable_file->Close());
  writable_file.reset();
}

TEST_P(EnvBasicTestWithParam, LargeWrite) {
  const size_t kWriteSize = 300 * 1024;
  char* scratch = new char[kWriteSize * 2];

  std::string write_data;
  for (size_t i = 0; i < kWriteSize; ++i) {
    write_data.append(1, static_cast<char>(i));
  }

  unique_ptr<WritableFile> writable_file;
  ASSERT_OK(env_->NewWritableFile(test_dir_ + "/f", &writable_file, soptions_));
  ASSERT_OK(writable_file->Append("foo"));
  ASSERT_OK(writable_file->Append(write_data));
  ASSERT_OK(writable_file->Close());
  writable_file.reset();

  unique_ptr<SequentialFile> seq_file;
  Slice result;
  ASSERT_OK(env_->NewSequentialFile(test_dir_ + "/f", &seq_file, soptions_));
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
  delete [] scratch;
}

TEST_P(EnvMoreTestWithParam, GetModTime) {
  ASSERT_OK(env_->CreateDirIfMissing(test_dir_ + "/dir1"));
  uint64_t mtime1 = 0x0;
  ASSERT_OK(env_->GetFileModificationTime(test_dir_ + "/dir1", &mtime1));
}

TEST_P(EnvMoreTestWithParam, MakeDir) {
  ASSERT_OK(env_->CreateDir(test_dir_ + "/j"));
  ASSERT_OK(env_->FileExists(test_dir_ + "/j"));
  std::vector<std::string> children;
  env_->GetChildren(test_dir_, &children);
  ASSERT_EQ(1U, children.size());
//失败，因为文件已存在
  ASSERT_TRUE(!env_->CreateDir(test_dir_ + "/j").ok());
  ASSERT_OK(env_->CreateDirIfMissing(test_dir_ + "/j"));
  ASSERT_OK(env_->DeleteDir(test_dir_ + "/j"));
  ASSERT_EQ(Status::NotFound(), env_->FileExists(test_dir_ + "/j"));
}

TEST_P(EnvMoreTestWithParam, GetChildren) {
//空文件夹返回空向量
  std::vector<std::string> children;
  std::vector<Env::FileAttributes> childAttr;
  ASSERT_OK(env_->CreateDirIfMissing(test_dir_));
  ASSERT_OK(env_->GetChildren(test_dir_, &children));
  ASSERT_OK(env_->FileExists(test_dir_));
  ASSERT_OK(env_->GetChildrenFileAttributes(test_dir_, &childAttr));
  ASSERT_EQ(0U, children.size());
  ASSERT_EQ(0U, childAttr.size());

//包含内容的文件夹返回测试目录的相对路径
  ASSERT_OK(env_->CreateDirIfMissing(test_dir_ + "/niu"));
  ASSERT_OK(env_->CreateDirIfMissing(test_dir_ + "/you"));
  ASSERT_OK(env_->CreateDirIfMissing(test_dir_ + "/guo"));
  ASSERT_OK(env_->GetChildren(test_dir_, &children));
  ASSERT_OK(env_->GetChildrenFileAttributes(test_dir_, &childAttr));
  ASSERT_EQ(3U, children.size());
  ASSERT_EQ(3U, childAttr.size());
  for (auto each : children) {
    env_->DeleteDir(test_dir_ + "/" + each);
}  //默认posix env所必需的

//不存在的目录返回ioerror
  ASSERT_OK(env_->DeleteDir(test_dir_));
  ASSERT_TRUE(!env_->FileExists(test_dir_).ok());
  ASSERT_TRUE(!env_->GetChildren(test_dir_, &children).ok());
  ASSERT_TRUE(!env_->GetChildrenFileAttributes(test_dir_, &childAttr).ok());

//如果dir是文件，则返回ioerror
  ASSERT_OK(env_->CreateDir(test_dir_));
  unique_ptr<WritableFile> writable_file;
  ASSERT_OK(
      env_->NewWritableFile(test_dir_ + "/file", &writable_file, soptions_));
  ASSERT_OK(writable_file->Close());
  writable_file.reset();
  ASSERT_TRUE(!env_->GetChildren(test_dir_ + "/file", &children).ok());
  ASSERT_EQ(0U, children.size());
}

}  //命名空间rocksdb
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
