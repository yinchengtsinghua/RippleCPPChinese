
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2015，Red Hat，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#ifndef ROCKSDB_LITE

#include "rocksdb/utilities/env_mirror.h"
#include "env/mock_env.h"
#include "util/testharness.h"

namespace rocksdb {

class EnvMirrorTest : public testing::Test {
 public:
  Env* default_;
  MockEnv* a_, *b_;
  EnvMirror* env_;
  const EnvOptions soptions_;

  EnvMirrorTest()
      : default_(Env::Default()),
        a_(new MockEnv(default_)),
        b_(new MockEnv(default_)),
        env_(new EnvMirror(a_, b_)) {}
  ~EnvMirrorTest() {
    delete env_;
    delete a_;
    delete b_;
  }
};

TEST_F(EnvMirrorTest, Basics) {
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
  ASSERT_OK(a_->FileExists("/dir/f"));
  ASSERT_OK(b_->FileExists("/dir/f"));
  ASSERT_OK(env_->GetFileSize("/dir/f", &file_size));
  ASSERT_EQ(0U, file_size);
  ASSERT_OK(env_->GetChildren("/dir", &children));
  ASSERT_EQ(1U, children.size());
  ASSERT_EQ("f", children[0]);
  ASSERT_OK(a_->GetChildren("/dir", &children));
  ASSERT_EQ(1U, children.size());
  ASSERT_EQ("f", children[0]);
  ASSERT_OK(b_->GetChildren("/dir", &children));
  ASSERT_EQ(1U, children.size());
  ASSERT_EQ("f", children[0]);

//写入文件。
  ASSERT_OK(env_->NewWritableFile("/dir/f", &writable_file, soptions_));
  ASSERT_OK(writable_file->Append("abc"));
  writable_file.reset();

//检查预期大小。
  ASSERT_OK(env_->GetFileSize("/dir/f", &file_size));
  ASSERT_EQ(3U, file_size);
  ASSERT_OK(a_->GetFileSize("/dir/f", &file_size));
  ASSERT_EQ(3U, file_size);
  ASSERT_OK(b_->GetFileSize("/dir/f", &file_size));
  ASSERT_EQ(3U, file_size);

//检查重命名是否有效。
  ASSERT_TRUE(!env_->RenameFile("/dir/non_existent", "/dir/g").ok());
  ASSERT_OK(env_->RenameFile("/dir/f", "/dir/g"));
  ASSERT_EQ(Status::NotFound(), env_->FileExists("/dir/f"));
  ASSERT_OK(env_->FileExists("/dir/g"));
  ASSERT_OK(env_->GetFileSize("/dir/g", &file_size));
  ASSERT_EQ(3U, file_size);
  ASSERT_OK(a_->FileExists("/dir/g"));
  ASSERT_OK(a_->GetFileSize("/dir/g", &file_size));
  ASSERT_EQ(3U, file_size);
  ASSERT_OK(b_->FileExists("/dir/g"));
  ASSERT_OK(b_->GetFileSize("/dir/g", &file_size));
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

TEST_F(EnvMirrorTest, ReadWrite) {
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
  ASSERT_TRUE(!rand_file->Read(1000, 5, &result, scratch).ok());
}

TEST_F(EnvMirrorTest, Locks) {
  FileLock* lock;

//这些不是行动，但我们测试它们是否能成功。
  ASSERT_OK(env_->LockFile("some file", &lock));
  ASSERT_OK(env_->UnlockFile(lock));
}

TEST_F(EnvMirrorTest, Misc) {
  std::string test_dir;
  ASSERT_OK(env_->GetTestDirectory(&test_dir));
  ASSERT_TRUE(!test_dir.empty());

  unique_ptr<WritableFile> writable_file;
  ASSERT_OK(env_->NewWritableFile("/a/b", &writable_file, soptions_));

//这些不是行动，但我们测试它们是否能成功。
  ASSERT_OK(writable_file->Sync());
  ASSERT_OK(writable_file->Flush());
  ASSERT_OK(writable_file->Close());
  writable_file.reset();
}

TEST_F(EnvMirrorTest, LargeWrite) {
  const size_t kWriteSize = 300 * 1024;
  char* scratch = new char[kWriteSize * 2];

  std::string write_data;
  for (size_t i = 0; i < kWriteSize; ++i) {
    write_data.append(1, static_cast<char>(i));
  }

  unique_ptr<WritableFile> writable_file;
  ASSERT_OK(env_->NewWritableFile("/dir/f", &writable_file, soptions_));
  ASSERT_OK(writable_file->Append("foo"));
  ASSERT_OK(writable_file->Append(write_data));
  writable_file.reset();

  unique_ptr<SequentialFile> seq_file;
  Slice result;
  ASSERT_OK(env_->NewSequentialFile("/dir/f", &seq_file, soptions_));
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
