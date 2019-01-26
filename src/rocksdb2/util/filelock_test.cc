
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
#include "rocksdb/status.h"
#include "rocksdb/env.h"

#include <vector>
#include "util/coding.h"
#include "util/testharness.h"

namespace rocksdb {

class LockTest : public testing::Test {
 public:
  static LockTest* current_;
  std::string file_;
  rocksdb::Env* env_;

  LockTest() : file_(test::TmpDir() + "/db_testlock_file"),
               env_(rocksdb::Env::Default()) {
    current_ = this;
  }

  ~LockTest() {
  }

  Status LockFile(FileLock** db_lock) {
    return env_->LockFile(file_, db_lock);
  }

  Status UnlockFile(FileLock* db_lock) {
    return env_->UnlockFile(db_lock);
  }
};
LockTest* LockTest::current_;

TEST_F(LockTest, LockBySameThread) {
  FileLock* lock1;
  FileLock* lock2;

//获取文件的锁
  ASSERT_OK(LockFile(&lock1));

//重新获取同一文件上的锁。这应该会失败。
  ASSERT_TRUE(LockFile(&lock2).IsIOError());

//释放锁
  ASSERT_OK(UnlockFile(lock1));

}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
