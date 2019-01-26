
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

#pragma once

#ifdef OS_AIX
#include "gtest/gtest.h"
#else
#include <gtest/gtest.h>
#endif

#include <string>
#include "rocksdb/env.h"

namespace rocksdb {
namespace test {

//返回用于临时存储的目录。
std::string TmpDir(Env* env = Env::Default());

//为此运行返回随机种子。通常返回
//重复调用此二进制文件时的相同数字，但自动
//跑步可以改变种子。
int RandomSeed();

::testing::AssertionResult AssertStatus(const char* s_expr, const Status& s);

#define ASSERT_OK(s) ASSERT_PRED_FORMAT1(rocksdb::test::AssertStatus, s)
#define ASSERT_NOK(s) ASSERT_FALSE((s).ok())
#define EXPECT_OK(s) EXPECT_PRED_FORMAT1(rocksdb::test::AssertStatus, s)
#define EXPECT_NOK(s) EXPECT_FALSE((s).ok())

}  //命名空间测试
}  //命名空间rocksdb
