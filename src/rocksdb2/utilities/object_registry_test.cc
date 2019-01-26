
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2016至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#ifndef ROCKSDB_LITE

#include "rocksdb/utilities/object_registry.h"
#include "util/testharness.h"

namespace rocksdb {

class EnvRegistryTest : public testing::Test {
 public:
  static int num_a, num_b;
};

int EnvRegistryTest::num_a = 0;
int EnvRegistryTest::num_b = 0;

static Registrar<Env> test_reg_a("a://.*“，[]（const std:：string&uri，
                                              std::unique_ptr<Env>* env_guard) {
  ++EnvRegistryTest::num_a;
  return Env::Default();
});

static Registrar<Env> test_reg_b("b://.*“，[]（const std:：string&uri，
                                              std::unique_ptr<Env>* env_guard) {
  ++EnvRegistryTest::num_b;
//env:：default（）是单例的，因此我们不能直接将所有权授予
//打电话的-我们必须先把它包起来。
  env_guard->reset(new EnvWrapper(Env::Default()));
  return env_guard->get();
});

TEST_F(EnvRegistryTest, Basics) {
  std::unique_ptr<Env> env_guard;
auto res = NewCustomObject<Env>("a://测试“&env_guard）；
  ASSERT_NE(res, nullptr);
  ASSERT_EQ(env_guard, nullptr);
  ASSERT_EQ(1, num_a);
  ASSERT_EQ(0, num_b);

res = NewCustomObject<Env>("b://测试“&env_guard）；
  ASSERT_NE(res, nullptr);
  ASSERT_NE(env_guard, nullptr);
  ASSERT_EQ(1, num_a);
  ASSERT_EQ(1, num_b);

res = NewCustomObject<Env>("c://测试“&env_guard）；
  ASSERT_EQ(res, nullptr);
  ASSERT_EQ(env_guard, nullptr);
  ASSERT_EQ(1, num_a);
  ASSERT_EQ(1, num_b);
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else  //摇滚乐
#include <stdio.h>

int main(int argc, char** argv) {
  fprintf(stderr, "SKIPPED as EnvRegistry is not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //摇滚乐
