
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

#include "util/testharness.h"
#include <string>

namespace rocksdb {
namespace test {

::testing::AssertionResult AssertStatus(const char* s_expr, const Status& s) {
  if (s.ok()) {
    return ::testing::AssertionSuccess();
  } else {
    return ::testing::AssertionFailure() << s_expr << std::endl
                                         << s.ToString();
  }
}

std::string TmpDir(Env* env) {
  std::string dir;
  Status s = env->GetTestDirectory(&dir);
  EXPECT_TRUE(s.ok()) << s.ToString();
  return dir;
}

int RandomSeed() {
  const char* env = getenv("TEST_RANDOM_SEED");
  int result = (env != nullptr ? atoi(env) : 301);
  if (result <= 0) {
    result = 301;
  }
  return result;
}

}  //命名空间测试
}  //命名空间rocksdb
