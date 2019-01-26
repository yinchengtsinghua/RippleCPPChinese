
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

#include <functional>

#include "port/port.h"
#include "port/stack_trace.h"
#include "rocksdb/iostats_context.h"
#include "rocksdb/perf_context.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace rocksdb {

class CleanableTest : public testing::Test {};

//使用此项跟踪实际执行的清理
void Multiplier(void* arg1, void* arg2) {
  int* res = reinterpret_cast<int*>(arg1);
  int* num = reinterpret_cast<int*>(arg2);
  *res *= *num;
}

//第一次清理在堆栈上，其余的在堆上，因此对这两种情况进行测试
TEST_F(CleanableTest, Register) {
  int n2 = 2, n3 = 3;
  int res = 1;
  { Cleanable c1; }
//可清洁的
  ASSERT_EQ(1, res);

  res = 1;
  {
    Cleanable c1;
c1.RegisterCleanup(Multiplier, &res, &n2);  //RES＝2；
  }
//可清洁的
  ASSERT_EQ(2, res);

  res = 1;
  {
    Cleanable c1;
c1.RegisterCleanup(Multiplier, &res, &n2);  //RES＝2；
c1.RegisterCleanup(Multiplier, &res, &n3);  //RES＝2×3；
  }
//可清洁的
  ASSERT_EQ(6, res);

//测试重置是否清除
  res = 1;
  {
    Cleanable c1;
c1.RegisterCleanup(Multiplier, &res, &n2);  //RES＝2；
c1.RegisterCleanup(Multiplier, &res, &n3);  //RES＝2×3；
    c1.Reset();
    ASSERT_EQ(6, res);
  }
//可清洁的
  ASSERT_EQ(6, res);

//重置后测试可使用
  res = 1;
  {
    Cleanable c1;
c1.RegisterCleanup(Multiplier, &res, &n2);  //RES＝2；
    c1.Reset();
    ASSERT_EQ(2, res);
c1.RegisterCleanup(Multiplier, &res, &n3);  //RES＝2×3；
  }
//可清洁的
  ASSERT_EQ(6, res);
}

//第一次清理在堆栈上，其余的在堆栈上，
//所以测试它们的所有组合
TEST_F(CleanableTest, Delegation) {
  int n2 = 2, n3 = 3, n5 = 5, n7 = 7;
  int res = 1;
  {
    Cleanable c2;
    {
      Cleanable c1;
c1.RegisterCleanup(Multiplier, &res, &n2);  //RES＝2；
      c1.DelegateCleanupsTo(&c2);
    }
//可清洁的
    ASSERT_EQ(1, res);
  }
//可清洁的
  ASSERT_EQ(2, res);

  res = 1;
  {
    Cleanable c2;
    {
      Cleanable c1;
      c1.DelegateCleanupsTo(&c2);
    }
//可清洁的
    ASSERT_EQ(1, res);
  }
//可清洁的
  ASSERT_EQ(1, res);

  res = 1;
  {
    Cleanable c2;
    {
      Cleanable c1;
c1.RegisterCleanup(Multiplier, &res, &n2);  //RES＝2；
c1.RegisterCleanup(Multiplier, &res, &n3);  //RES＝2×3；
      c1.DelegateCleanupsTo(&c2);
    }
//可清洁的
    ASSERT_EQ(1, res);
  }
//可清洁的
  ASSERT_EQ(6, res);

  res = 1;
  {
    Cleanable c2;
c2.RegisterCleanup(Multiplier, &res, &n5);  //RES＝5；
    {
      Cleanable c1;
c1.RegisterCleanup(Multiplier, &res, &n2);  //RES＝2；
c1.RegisterCleanup(Multiplier, &res, &n3);  //RES＝2×3；
c1.DelegateCleanupsTo(&c2);                 //Res=2*3*5；
    }
//可清洁的
    ASSERT_EQ(1, res);
  }
//可清洁的
  ASSERT_EQ(30, res);

  res = 1;
  {
    Cleanable c2;
c2.RegisterCleanup(Multiplier, &res, &n5);  //RES＝5；
c2.RegisterCleanup(Multiplier, &res, &n7);  //RES＝5×7；
    {
      Cleanable c1;
c1.RegisterCleanup(Multiplier, &res, &n2);  //RES＝2；
c1.RegisterCleanup(Multiplier, &res, &n3);  //RES＝2×3；
c1.DelegateCleanupsTo(&c2);                 //Res=2*3*5*7；
    }
//可清洁的
    ASSERT_EQ(1, res);
  }
//可清洁的
  ASSERT_EQ(210, res);

  res = 1;
  {
    Cleanable c2;
c2.RegisterCleanup(Multiplier, &res, &n5);  //RES＝5；
c2.RegisterCleanup(Multiplier, &res, &n7);  //RES＝5×7；
    {
      Cleanable c1;
c1.RegisterCleanup(Multiplier, &res, &n2);  //RES＝2；
c1.DelegateCleanupsTo(&c2);                 //Res=2*5*7；
    }
//可清洁的
    ASSERT_EQ(1, res);
  }
//可清洁的
  ASSERT_EQ(70, res);

  res = 1;
  {
    Cleanable c2;
c2.RegisterCleanup(Multiplier, &res, &n5);  //RES＝5；
c2.RegisterCleanup(Multiplier, &res, &n7);  //RES＝5×7；
    {
      Cleanable c1;
c1.DelegateCleanupsTo(&c2);  //RES＝5×7；
    }
//可清洁的
    ASSERT_EQ(1, res);
  }
//可清洁的
  ASSERT_EQ(35, res);

  res = 1;
  {
    Cleanable c2;
c2.RegisterCleanup(Multiplier, &res, &n5);  //RES＝5；
    {
      Cleanable c1;
c1.DelegateCleanupsTo(&c2);  //RES＝5；
    }
//可清洁的
    ASSERT_EQ(1, res);
  }
//可清洁的
  ASSERT_EQ(5, res);
}

static void ReleaseStringHeap(void* s, void*) {
  delete reinterpret_cast<const std::string*>(s);
}

class PinnableSlice4Test : public PinnableSlice {
 public:
  void TestStringIsRegistered(std::string* s) {
    ASSERT_TRUE(cleanup_.function == ReleaseStringHeap);
    ASSERT_EQ(cleanup_.arg1, s);
    ASSERT_EQ(cleanup_.arg2, nullptr);
    ASSERT_EQ(cleanup_.next, nullptr);
  }
};

//将pinnableslice测试放在这里，因为与可清理测试相似
TEST_F(CleanableTest, PinnableSlice) {
  int n2 = 2;
  int res = 1;
  const std::string const_str = "123";

  {
    res = 1;
    PinnableSlice4Test value;
    Slice slice(const_str);
    value.PinSlice(slice, Multiplier, &res, &n2);
    std::string str;
    str.assign(value.data(), value.size());
    ASSERT_EQ(const_str, str);
  }
//可清洁的
  ASSERT_EQ(2, res);

  {
    res = 1;
    PinnableSlice4Test value;
    Slice slice(const_str);
    {
      Cleanable c1;
c1.RegisterCleanup(Multiplier, &res, &n2);  //RES＝2；
      value.PinSlice(slice, &c1);
    }
//可清洁的
ASSERT_EQ(1, res);  //清理必须委托给值
    std::string str;
    str.assign(value.data(), value.size());
    ASSERT_EQ(const_str, str);
  }
//可清洁的
  ASSERT_EQ(2, res);

  {
    PinnableSlice4Test value;
    Slice slice(const_str);
    value.PinSelf(slice);
    std::string str;
    str.assign(value.data(), value.size());
    ASSERT_EQ(const_str, str);
  }

  {
    PinnableSlice4Test value;
    std::string* self_str_ptr = value.GetSelf();
    self_str_ptr->assign(const_str);
    value.PinSelf();
    std::string str;
    str.assign(value.data(), value.size());
    ASSERT_EQ(const_str, str);
  }
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  rocksdb::port::InstallStackTraceHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
