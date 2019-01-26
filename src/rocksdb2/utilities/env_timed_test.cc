
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

#ifndef ROCKSDB_LITE

#include "rocksdb/env.h"
#include "rocksdb/perf_context.h"
#include "util/testharness.h"

namespace rocksdb {

class TimedEnvTest : public testing::Test {
};

TEST_F(TimedEnvTest, BasicTest) {
  SetPerfLevel(PerfLevel::kEnableTime);
  ASSERT_EQ(0, get_perf_context()->env_new_writable_file_nanos);

  std::unique_ptr<Env> mem_env(NewMemEnv(Env::Default()));
  std::unique_ptr<Env> timed_env(NewTimedEnv(mem_env.get()));
  std::unique_ptr<WritableFile> writable_file;
  timed_env->NewWritableFile("f", &writable_file, EnvOptions());

  ASSERT_GT(get_perf_context()->env_new_writable_file_nanos, 0);
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else  //摇滚乐
#include <stdio.h>

int main(int argc, char** argv) {
  fprintf(stderr, "SKIPPED as TimedEnv is not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //摇滚乐
