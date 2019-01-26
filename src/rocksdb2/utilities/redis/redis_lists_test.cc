
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
/*
 *基于RockSDB的Redis API测试线束。
 *
 *用法：构建时使用：“make redis_test”（在rocksdb目录中）。
 *运行单元测试时使用：“./redis_test”
 *手动/交互式用户测试：“../redis_test-m”
 *手动用户测试+重启数据库：“/redis-test-m-d”
 *
 *TODO:添加大型随机测试用例以验证效率和可伸缩性
 *
 *@作者Deon Nicholas（dnicholas@fb.com）
 **/


#ifndef ROCKSDB_LITE

#include <iostream>
#include <cctype>

#include "redis_lists.h"
#include "util/testharness.h"
#include "util/random.h"

using namespace rocksdb;

namespace rocksdb {

class RedisListsTest : public testing::Test {
 public:
  static const std::string kDefaultDbName;
  static Options options;

  RedisListsTest() {
    options.create_if_missing = true;
  }
};

const std::string RedisListsTest::kDefaultDbName =
    test::TmpDir() + "/redis_lists_test";
Options RedisListsTest::options = Options();

//运算符==和运算符<<在下面为向量（列表）定义
//断言所需

namespace {
void AssertListEq(const std::vector<std::string>& result,
                  const std::vector<std::string>& expected_result) {
  ASSERT_EQ(result.size(), expected_result.size());
  for (size_t i = 0; i < result.size(); ++i) {
    ASSERT_EQ(result[i], expected_result[i]);
  }
}
}  //命名空间

//右推，长度，索引，范围
TEST_F(RedisListsTest, SimpleTest) {
RedisLists redis(kDefaultDbName, options, true);   //破坏性的

std::string tempv;  //以下用于所有index（）、popRight（）、popLeft（））

//简单的pushright（每次都应返回新的长度）
  ASSERT_EQ(redis.PushRight("k1", "v1"), 1);
  ASSERT_EQ(redis.PushRight("k1", "v2"), 2);
  ASSERT_EQ(redis.PushRight("k1", "v3"), 3);

//检查长度和index（）函数
ASSERT_EQ(redis.Length("k1"), 3);        //检查长度
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
ASSERT_EQ(tempv, "v1");   //检查有效索引
  ASSERT_TRUE(redis.Index("k1", 1, &tempv));
  ASSERT_EQ(tempv, "v2");
  ASSERT_TRUE(redis.Index("k1", 2, &tempv));
  ASSERT_EQ(tempv, "v3");

//检查范围函数和向量
std::vector<std::string> result = redis.Range("k1", 0, 2);   //得到名单
  std::vector<std::string> expected_result(3);
  expected_result[0] = "v1";
  expected_result[1] = "v2";
  expected_result[2] = "v3";
  AssertListEq(result, expected_result);
}

//左推，长度，索引，范围
TEST_F(RedisListsTest, SimpleTest2) {
RedisLists redis(kDefaultDbName, options, true);   //破坏性的

std::string tempv;  //以下用于所有index（）、popRight（）、popLeft（））

//简单右推
  ASSERT_EQ(redis.PushLeft("k1", "v3"), 1);
  ASSERT_EQ(redis.PushLeft("k1", "v2"), 2);
  ASSERT_EQ(redis.PushLeft("k1", "v1"), 3);

//检查长度和index（）函数
ASSERT_EQ(redis.Length("k1"), 3);        //检查长度
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
ASSERT_EQ(tempv, "v1");   //检查有效索引
  ASSERT_TRUE(redis.Index("k1", 1, &tempv));
  ASSERT_EQ(tempv, "v2");
  ASSERT_TRUE(redis.Index("k1", 2, &tempv));
  ASSERT_EQ(tempv, "v3");

//检查范围函数和向量
std::vector<std::string> result = redis.Range("k1", 0, 2);   //得到名单
  std::vector<std::string> expected_result(3);
  expected_result[0] = "v1";
  expected_result[1] = "v2";
  expected_result[2] = "v3";
  AssertListEq(result, expected_result);
}

//index（）函数的详尽测试
TEST_F(RedisListsTest, IndexTest) {
RedisLists redis(kDefaultDbName, options, true);   //破坏性的

std::string tempv;  //以下用于所有index（）、popRight（）、popLeft（））

//空索引检查（返回空，不应崩溃或编辑tempv）
  tempv = "yo";
  ASSERT_TRUE(!redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "yo");
  ASSERT_TRUE(!redis.Index("fda", 3, &tempv));
  ASSERT_EQ(tempv, "yo");
  ASSERT_TRUE(!redis.Index("random", -12391, &tempv));
  ASSERT_EQ(tempv, "yo");

//简单推送（将产生：[V6、V4、V4、V1、V2、V3]
  redis.PushRight("k1", "v1");
  redis.PushRight("k1", "v2");
  redis.PushRight("k1", "v3");
  redis.PushLeft("k1", "v4");
  redis.PushLeft("k1", "v4");
  redis.PushLeft("k1", "v6");

//简单的非负指数
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "v6");
  ASSERT_TRUE(redis.Index("k1", 1, &tempv));
  ASSERT_EQ(tempv, "v4");
  ASSERT_TRUE(redis.Index("k1", 2, &tempv));
  ASSERT_EQ(tempv, "v4");
  ASSERT_TRUE(redis.Index("k1", 3, &tempv));
  ASSERT_EQ(tempv, "v1");
  ASSERT_TRUE(redis.Index("k1", 4, &tempv));
  ASSERT_EQ(tempv, "v2");
  ASSERT_TRUE(redis.Index("k1", 5, &tempv));
  ASSERT_EQ(tempv, "v3");

//负指数
  ASSERT_TRUE(redis.Index("k1", -6, &tempv));
  ASSERT_EQ(tempv, "v6");
  ASSERT_TRUE(redis.Index("k1", -5, &tempv));
  ASSERT_EQ(tempv, "v4");
  ASSERT_TRUE(redis.Index("k1", -4, &tempv));
  ASSERT_EQ(tempv, "v4");
  ASSERT_TRUE(redis.Index("k1", -3, &tempv));
  ASSERT_EQ(tempv, "v1");
  ASSERT_TRUE(redis.Index("k1", -2, &tempv));
  ASSERT_EQ(tempv, "v2");
  ASSERT_TRUE(redis.Index("k1", -1, &tempv));
  ASSERT_EQ(tempv, "v3");

//越界（返回为空，无崩溃）
  ASSERT_TRUE(!redis.Index("k1", 6, &tempv));
  ASSERT_TRUE(!redis.Index("k1", 123219, &tempv));
  ASSERT_TRUE(!redis.Index("k1", -7, &tempv));
  ASSERT_TRUE(!redis.Index("k1", -129, &tempv));
}


//range（）函数的详尽测试
TEST_F(RedisListsTest, RangeTest) {
RedisLists redis(kDefaultDbName, options, true);   //破坏性的

std::string tempv;  //以下用于所有index（）、popRight（）、popLeft（））

//简单推送（将生成[v6、v4、v4、v1、v2、v3]）
  redis.PushRight("k1", "v1");
  redis.PushRight("k1", "v2");
  redis.PushRight("k1", "v3");
  redis.PushLeft("k1", "v4");
  redis.PushLeft("k1", "v4");
  redis.PushLeft("k1", "v6");

//健康检查（检查长度，确保是6）
  ASSERT_EQ(redis.Length("k1"), 6);

//简单范围
  std::vector<std::string> res = redis.Range("k1", 1, 4);
  ASSERT_EQ((int)res.size(), 4);
  ASSERT_EQ(res[0], "v4");
  ASSERT_EQ(res[1], "v4");
  ASSERT_EQ(res[2], "v1");
  ASSERT_EQ(res[3], "v2");

//负指数（即：从末端测量）
  res = redis.Range("k1", 2, -1);
  ASSERT_EQ((int)res.size(), 4);
  ASSERT_EQ(res[0], "v4");
  ASSERT_EQ(res[1], "v1");
  ASSERT_EQ(res[2], "v2");
  ASSERT_EQ(res[3], "v3");

  res = redis.Range("k1", -6, -4);
  ASSERT_EQ((int)res.size(), 3);
  ASSERT_EQ(res[0], "v6");
  ASSERT_EQ(res[1], "v4");
  ASSERT_EQ(res[2], "v4");

  res = redis.Range("k1", -1, 5);
  ASSERT_EQ((int)res.size(), 1);
  ASSERT_EQ(res[0], "v3");

//部分/断裂指数
  res = redis.Range("k1", -3, 1000000);
  ASSERT_EQ((int)res.size(), 3);
  ASSERT_EQ(res[0], "v1");
  ASSERT_EQ(res[1], "v2");
  ASSERT_EQ(res[2], "v3");

  res = redis.Range("k1", -1000000, 1);
  ASSERT_EQ((int)res.size(), 2);
  ASSERT_EQ(res[0], "v6");
  ASSERT_EQ(res[1], "v4");

//无效索引
  res = redis.Range("k1", 7, 9);
  ASSERT_EQ((int)res.size(), 0);

  res = redis.Range("k1", -8, -7);
  ASSERT_EQ((int)res.size(), 0);

  res = redis.Range("k1", 3, 2);
  ASSERT_EQ((int)res.size(), 0);

  res = redis.Range("k1", 5, -2);
  ASSERT_EQ((int)res.size(), 0);

//范围匹配索引
  res = redis.Range("k1", -6, -4);
  ASSERT_TRUE(redis.Index("k1", -6, &tempv));
  ASSERT_EQ(tempv, res[0]);
  ASSERT_TRUE(redis.Index("k1", -5, &tempv));
  ASSERT_EQ(tempv, res[1]);
  ASSERT_TRUE(redis.Index("k1", -4, &tempv));
  ASSERT_EQ(tempv, res[2]);

//最后检查
  res = redis.Range("k1", 0, -6);
  ASSERT_EQ((int)res.size(), 1);
  ASSERT_EQ(res[0], "v6");
}

//insertbefore（）和insertafter（）的详尽测试
TEST_F(RedisListsTest, InsertTest) {
  RedisLists redis(kDefaultDbName, options, true);

std::string tempv;  //以下用于所有index（）、popRight（）、popLeft（））

//插入空列表（返回0，不要崩溃）
  ASSERT_EQ(redis.InsertBefore("k1", "non-exist", "a"), 0);
  ASSERT_EQ(redis.InsertAfter("k1", "other-non-exist", "c"), 0);
  ASSERT_EQ(redis.Length("k1"), 0);

//推动一些初步的东西[G，F，E，D，C，B，A]
  redis.PushLeft("k1", "a");
  redis.PushLeft("k1", "b");
  redis.PushLeft("k1", "c");
  redis.PushLeft("k1", "d");
  redis.PushLeft("k1", "e");
  redis.PushLeft("k1", "f");
  redis.PushLeft("k1", "g");
  ASSERT_EQ(redis.Length("k1"), 7);

//测试插入前
  int newLength = redis.InsertBefore("k1", "e", "hello");
  ASSERT_EQ(newLength, 8);
  ASSERT_EQ(redis.Length("k1"), newLength);
  ASSERT_TRUE(redis.Index("k1", 1, &tempv));
  ASSERT_EQ(tempv, "f");
  ASSERT_TRUE(redis.Index("k1", 3, &tempv));
  ASSERT_EQ(tempv, "e");
  ASSERT_TRUE(redis.Index("k1", 2, &tempv));
  ASSERT_EQ(tempv, "hello");

//测试插入器之后
  newLength =  redis.InsertAfter("k1", "c", "bye");
  ASSERT_EQ(newLength, 9);
  ASSERT_EQ(redis.Length("k1"), newLength);
  ASSERT_TRUE(redis.Index("k1", 6, &tempv));
  ASSERT_EQ(tempv, "bye");

//在insertbefore上测试错误值
  newLength = redis.InsertBefore("k1", "yo", "x");
  ASSERT_EQ(newLength, 9);
  ASSERT_EQ(redis.Length("k1"), newLength);

//在InsertAfter上测试错误值
  newLength = redis.InsertAfter("k1", "xxxx", "y");
  ASSERT_EQ(newLength, 9);
  ASSERT_EQ(redis.Length("k1"), newLength);

//开始前测试插入
  newLength = redis.InsertBefore("k1", "g", "begggggggggggggggg");
  ASSERT_EQ(newLength, 10);
  ASSERT_EQ(redis.Length("k1"), newLength);

//末端后的测试插入器
  newLength = redis.InsertAfter("k1", "a", "enddd");
  ASSERT_EQ(newLength, 11);
  ASSERT_EQ(redis.Length("k1"), newLength);

//确保没有任何奇怪的事情发生。
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "begggggggggggggggg");
  ASSERT_TRUE(redis.Index("k1", 1, &tempv));
  ASSERT_EQ(tempv, "g");
  ASSERT_TRUE(redis.Index("k1", 2, &tempv));
  ASSERT_EQ(tempv, "f");
  ASSERT_TRUE(redis.Index("k1", 3, &tempv));
  ASSERT_EQ(tempv, "hello");
  ASSERT_TRUE(redis.Index("k1", 4, &tempv));
  ASSERT_EQ(tempv, "e");
  ASSERT_TRUE(redis.Index("k1", 5, &tempv));
  ASSERT_EQ(tempv, "d");
  ASSERT_TRUE(redis.Index("k1", 6, &tempv));
  ASSERT_EQ(tempv, "c");
  ASSERT_TRUE(redis.Index("k1", 7, &tempv));
  ASSERT_EQ(tempv, "bye");
  ASSERT_TRUE(redis.Index("k1", 8, &tempv));
  ASSERT_EQ(tempv, "b");
  ASSERT_TRUE(redis.Index("k1", 9, &tempv));
  ASSERT_EQ(tempv, "a");
  ASSERT_TRUE(redis.Index("k1", 10, &tempv));
  ASSERT_EQ(tempv, "enddd");
}

//集函数的穷举检验
TEST_F(RedisListsTest, SetTest) {
  RedisLists redis(kDefaultDbName, options, true);

std::string tempv;  //以下用于所有index（）、popRight（）、popLeft（））

//设置在空列表上（返回false，不要崩溃）
  ASSERT_EQ(redis.Set("k1", 7, "a"), false);
  ASSERT_EQ(redis.Set("k1", 0, "a"), false);
  ASSERT_EQ(redis.Set("k1", -49, "cx"), false);
  ASSERT_EQ(redis.Length("k1"), 0);

//推动一些初步的东西[G，F，E，D，C，B，A]
  redis.PushLeft("k1", "a");
  redis.PushLeft("k1", "b");
  redis.PushLeft("k1", "c");
  redis.PushLeft("k1", "d");
  redis.PushLeft("k1", "e");
  redis.PushLeft("k1", "f");
  redis.PushLeft("k1", "g");
  ASSERT_EQ(redis.Length("k1"), 7);

//测试规则集
  ASSERT_TRUE(redis.Set("k1", 0, "0"));
  ASSERT_TRUE(redis.Set("k1", 3, "3"));
  ASSERT_TRUE(redis.Set("k1", 6, "6"));
  ASSERT_TRUE(redis.Set("k1", 2, "2"));
  ASSERT_TRUE(redis.Set("k1", 5, "5"));
  ASSERT_TRUE(redis.Set("k1", 1, "1"));
  ASSERT_TRUE(redis.Set("k1", 4, "4"));

ASSERT_EQ(redis.Length("k1"), 7); //大小不应更改
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "0");
  ASSERT_TRUE(redis.Index("k1", 1, &tempv));
  ASSERT_EQ(tempv, "1");
  ASSERT_TRUE(redis.Index("k1", 2, &tempv));
  ASSERT_EQ(tempv, "2");
  ASSERT_TRUE(redis.Index("k1", 3, &tempv));
  ASSERT_EQ(tempv, "3");
  ASSERT_TRUE(redis.Index("k1", 4, &tempv));
  ASSERT_EQ(tempv, "4");
  ASSERT_TRUE(redis.Index("k1", 5, &tempv));
  ASSERT_EQ(tempv, "5");
  ASSERT_TRUE(redis.Index("k1", 6, &tempv));
  ASSERT_EQ(tempv, "6");

//设置为负索引
  ASSERT_TRUE(redis.Set("k1", -7, "a"));
  ASSERT_TRUE(redis.Set("k1", -4, "d"));
  ASSERT_TRUE(redis.Set("k1", -1, "g"));
  ASSERT_TRUE(redis.Set("k1", -5, "c"));
  ASSERT_TRUE(redis.Set("k1", -2, "f"));
  ASSERT_TRUE(redis.Set("k1", -6, "b"));
  ASSERT_TRUE(redis.Set("k1", -3, "e"));

ASSERT_EQ(redis.Length("k1"), 7); //大小不应更改
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "a");
  ASSERT_TRUE(redis.Index("k1", 1, &tempv));
  ASSERT_EQ(tempv, "b");
  ASSERT_TRUE(redis.Index("k1", 2, &tempv));
  ASSERT_EQ(tempv, "c");
  ASSERT_TRUE(redis.Index("k1", 3, &tempv));
  ASSERT_EQ(tempv, "d");
  ASSERT_TRUE(redis.Index("k1", 4, &tempv));
  ASSERT_EQ(tempv, "e");
  ASSERT_TRUE(redis.Index("k1", 5, &tempv));
  ASSERT_EQ(tempv, "f");
  ASSERT_TRUE(redis.Index("k1", 6, &tempv));
  ASSERT_EQ(tempv, "g");

//错误的索引（刚超出界限/一次检查就被取消）
  ASSERT_EQ(redis.Set("k1", -8, "off-by-one in negative index"), false);
  ASSERT_EQ(redis.Set("k1", 7, "off-by-one-error in positive index"), false);
  ASSERT_EQ(redis.Set("k1", 43892, "big random index should fail"), false);
  ASSERT_EQ(redis.Set("k1", -21391, "large negative index should fail"), false);

//最后一次检查（确保没有奇怪的事情发生）
ASSERT_EQ(redis.Length("k1"), 7); //大小不应更改
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "a");
  ASSERT_TRUE(redis.Index("k1", 1, &tempv));
  ASSERT_EQ(tempv, "b");
  ASSERT_TRUE(redis.Index("k1", 2, &tempv));
  ASSERT_EQ(tempv, "c");
  ASSERT_TRUE(redis.Index("k1", 3, &tempv));
  ASSERT_EQ(tempv, "d");
  ASSERT_TRUE(redis.Index("k1", 4, &tempv));
  ASSERT_EQ(tempv, "e");
  ASSERT_TRUE(redis.Index("k1", 5, &tempv));
  ASSERT_EQ(tempv, "f");
  ASSERT_TRUE(redis.Index("k1", 6, &tempv));
  ASSERT_EQ(tempv, "g");
}

//在混合环境中测试插入、推送和设置
TEST_F(RedisListsTest, InsertPushSetTest) {
RedisLists redis(kDefaultDbName, options, true);   //破坏性的

std::string tempv;  //以下用于所有index（）、popRight（）、popLeft（））

//一系列的推动和插入
//将导致[newbegin，z，a，aftera，x，newend]
//另外，有时检查返回值（应该返回长度）
  int lengthCheck;
  lengthCheck = redis.PushLeft("k1", "a");
  ASSERT_EQ(lengthCheck, 1);
  redis.PushLeft("k1", "z");
  redis.PushRight("k1", "x");
  lengthCheck = redis.InsertAfter("k1", "a", "aftera");
  ASSERT_EQ(lengthCheck , 4);
redis.InsertBefore("k1", "z", "newbegin");  //在列表开始之前插入
redis.InsertAfter("k1", "x", "newend");     //在列表结尾后插入

//检查
std::vector<std::string> res = redis.Range("k1", 0, -1); //得到名单
  ASSERT_EQ((int)res.size(), 6);
  ASSERT_EQ(res[0], "newbegin");
  ASSERT_EQ(res[5], "newend");
  ASSERT_EQ(res[3], "aftera");

//测试重复值/数据透视（多次出现“a”）。
ASSERT_TRUE(redis.Set("k1", 0, "a"));     //[A、Z、A、AfterA、X、Newend]
redis.InsertAfter("k1", "a", "happy");    //[A，高兴，Z，A，AfterA，…]
  ASSERT_TRUE(redis.Index("k1", 1, &tempv));
  ASSERT_EQ(tempv, "happy");
redis.InsertBefore("k1", "a", "sad");     //[悲伤，A，快乐，Z，A，Aftera，…]
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "sad");
  ASSERT_TRUE(redis.Index("k1", 2, &tempv));
  ASSERT_EQ(tempv, "happy");
  ASSERT_TRUE(redis.Index("k1", 5, &tempv));
  ASSERT_EQ(tempv, "aftera");
redis.InsertAfter("k1", "a", "zz");         //[悲伤，A，ZZ，快乐，Z，A，Aftera，…]
  ASSERT_TRUE(redis.Index("k1", 2, &tempv));
  ASSERT_EQ(tempv, "zz");
  ASSERT_TRUE(redis.Index("k1", 6, &tempv));
  ASSERT_EQ(tempv, "aftera");
ASSERT_TRUE(redis.Set("k1", 1, "nota"));    //[悲伤，nota，z z，快乐，z，a，…]
redis.InsertBefore("k1", "a", "ba");        //[悲伤，nota，z z，快乐，z，ba，a，…]
  ASSERT_TRUE(redis.Index("k1", 4, &tempv));
  ASSERT_EQ(tempv, "z");
  ASSERT_TRUE(redis.Index("k1", 5, &tempv));
  ASSERT_EQ(tempv, "ba");
  ASSERT_TRUE(redis.Index("k1", 6, &tempv));
  ASSERT_EQ(tempv, "a");

//我们现在有：【悲伤，nota，z z，快乐，z，ba，a，aftera，x，newend】
//redis.print（“k1”）；//手工检查

//在不存在的值之前/之后插入测试
lengthCheck = redis.Length("k1"); //确保长度不变
  ASSERT_EQ(lengthCheck, 10);
  ASSERT_EQ(redis.InsertBefore("k1", "non-exist", "randval"), lengthCheck);
  ASSERT_EQ(redis.InsertAfter("k1", "nothing", "a"), lengthCheck);
ASSERT_EQ(redis.InsertAfter("randKey", "randVal", "ranValue"), 0); //空的
ASSERT_EQ(redis.Length("k1"), lengthCheck); //长度不应该改变

//只需测试set（）函数
  redis.Set("k1", 5, "ba2");
  redis.InsertBefore("k1", "ba2", "beforeba2");
  ASSERT_TRUE(redis.Index("k1", 4, &tempv));
  ASSERT_EQ(tempv, "z");
  ASSERT_TRUE(redis.Index("k1", 5, &tempv));
  ASSERT_EQ(tempv, "beforeba2");
  ASSERT_TRUE(redis.Index("k1", 6, &tempv));
  ASSERT_EQ(tempv, "ba2");
  ASSERT_TRUE(redis.Index("k1", 7, &tempv));
  ASSERT_EQ(tempv, "a");

//我们有：【悲伤，nota，z z，快乐，z，beforeba2，ba2，a，aftera，x，newend】

//带负索引的set（）。
  redis.Set("k1", -1, "endprank");
  ASSERT_TRUE(!redis.Index("k1", 11, &tempv));
  ASSERT_TRUE(redis.Index("k1", 10, &tempv));
ASSERT_EQ(tempv, "endprank"); //确保设置正确工作
  redis.Set("k1", -11, "t");
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "t");

//测试越界集
  ASSERT_EQ(redis.Set("k1", -12, "ssd"), false);
  ASSERT_EQ(redis.Set("k1", 11, "sasd"), false);
  ASSERT_EQ(redis.Set("k1", 1200, "big"), false);
}

//测试阀内件，POP
TEST_F(RedisListsTest, TrimPopTest) {
RedisLists redis(kDefaultDbName, options, true);   //破坏性的

std::string tempv;  //以下用于所有index（）、popRight（）、popLeft（））

//一系列的推动和插入
//将导致[newbegin，z，a，aftera，x，newend]
  redis.PushLeft("k1", "a");
  redis.PushLeft("k1", "z");
  redis.PushRight("k1", "x");
redis.InsertBefore("k1", "z", "newbegin");    //在列表开始之前插入
redis.InsertAfter("k1", "x", "newend");       //在列表结尾后插入
  redis.InsertAfter("k1", "a", "aftera");

//简单PopLeft/Right测试
  ASSERT_TRUE(redis.PopLeft("k1", &tempv));
  ASSERT_EQ(tempv, "newbegin");
  ASSERT_EQ(redis.Length("k1"), 5);
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "z");
  ASSERT_TRUE(redis.PopRight("k1", &tempv));
  ASSERT_EQ(tempv, "newend");
  ASSERT_EQ(redis.Length("k1"), 4);
  ASSERT_TRUE(redis.Index("k1", -1, &tempv));
  ASSERT_EQ(tempv, "x");

//现在有了：[z，a，aftera，x]

//试验纵倾
ASSERT_TRUE(redis.Trim("k1", 0, -1));       //[Z，A，AfterA，X]（什么都不做）
  ASSERT_EQ(redis.Length("k1"), 4);
ASSERT_TRUE(redis.Trim("k1", 0, 2));                     //[Z，A，后]
  ASSERT_EQ(redis.Length("k1"), 3);
  ASSERT_TRUE(redis.Index("k1", -1, &tempv));
  ASSERT_EQ(tempv, "aftera");
ASSERT_TRUE(redis.Trim("k1", 1, 1));                     //[A]
  ASSERT_EQ(redis.Length("k1"), 1);
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "a");

//测试越界（空）微调
  ASSERT_TRUE(redis.Trim("k1", 1, 0));
  ASSERT_EQ(redis.Length("k1"), 0);

//弹出空列表（返回空无错误）
  ASSERT_TRUE(!redis.PopLeft("k1", &tempv));
  ASSERT_TRUE(!redis.PopRight("k1", &tempv));
  ASSERT_TRUE(redis.Trim("k1", 0, 5));

//全面修剪试验（阴性和无效指标）
//将从[newbegin，z，a，aftera，x，newend]开始
  redis.PushLeft("k1", "a");
  redis.PushLeft("k1", "z");
  redis.PushRight("k1", "x");
redis.InsertBefore("k1", "z", "newbegin");    //在列表开始之前插入
redis.InsertAfter("k1", "x", "newend");       //在列表结尾后插入
  redis.InsertAfter("k1", "a", "aftera");
ASSERT_TRUE(redis.Trim("k1", -6, -1));                     //什么都不应该做
  ASSERT_EQ(redis.Length("k1"), 6);
  ASSERT_TRUE(redis.Trim("k1", 1, -2));
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "z");
  ASSERT_TRUE(redis.Index("k1", 3, &tempv));
  ASSERT_EQ(tempv, "x");
  ASSERT_EQ(redis.Length("k1"), 4);
  ASSERT_TRUE(redis.Trim("k1", -3, -2));
  ASSERT_EQ(redis.Length("k1"), 2);
}

//测试移除、移除第一个、移除
TEST_F(RedisListsTest, RemoveTest) {
RedisLists redis(kDefaultDbName, options, true);   //破坏性的

std::string tempv;  //以下用于所有index（）、popRight（）、popLeft（））

//一系列的推动和插入
//将导致[newbegin，z，a，aftera，x，newend，a，a]
  redis.PushLeft("k1", "a");
  redis.PushLeft("k1", "z");
  redis.PushRight("k1", "x");
redis.InsertBefore("k1", "z", "newbegin");    //在列表开始之前插入
redis.InsertAfter("k1", "x", "newend");       //在列表结尾后插入
  redis.InsertAfter("k1", "a", "aftera");
  redis.PushRight("k1", "a");
  redis.PushRight("k1", "a");

//验证
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "newbegin");
  ASSERT_TRUE(redis.Index("k1", -1, &tempv));
  ASSERT_EQ(tempv, "a");

//首先检查删除（删除前两个“a”）。
//结果在[newbegin，z，aftera，x，newend，a]
  int numRemoved = redis.Remove("k1", 2, "a");
  ASSERT_EQ(numRemoved, 2);
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "newbegin");
  ASSERT_TRUE(redis.Index("k1", 1, &tempv));
  ASSERT_EQ(tempv, "z");
  ASSERT_TRUE(redis.Index("k1", 4, &tempv));
  ASSERT_EQ(tempv, "newend");
  ASSERT_TRUE(redis.Index("k1", 5, &tempv));
  ASSERT_EQ(tempv, "a");
  ASSERT_EQ(redis.Length("k1"), 6);

//重新填充一些东西
//结果位于：【x，x，x，x，x，newbegin，z，x，aftera，x，newend，a，x】
  redis.PushLeft("k1", "x");
  redis.PushLeft("k1", "x");
  redis.PushLeft("k1", "x");
  redis.PushLeft("k1", "x");
  redis.PushLeft("k1", "x");
  redis.PushRight("k1", "x");
  redis.InsertAfter("k1", "z", "x");

//从末端移除测试
  numRemoved = redis.Remove("k1", -2, "x");
  ASSERT_EQ(numRemoved, 2);
  ASSERT_TRUE(redis.Index("k1", 8, &tempv));
  ASSERT_EQ(tempv, "aftera");
  ASSERT_TRUE(redis.Index("k1", 9, &tempv));
  ASSERT_EQ(tempv, "newend");
  ASSERT_TRUE(redis.Index("k1", 10, &tempv));
  ASSERT_EQ(tempv, "a");
  ASSERT_TRUE(!redis.Index("k1", 11, &tempv));
  numRemoved = redis.Remove("k1", -2, "x");
  ASSERT_EQ(numRemoved, 2);
  ASSERT_TRUE(redis.Index("k1", 4, &tempv));
  ASSERT_EQ(tempv, "newbegin");
  ASSERT_TRUE(redis.Index("k1", 6, &tempv));
  ASSERT_EQ(tempv, "aftera");

//我们现在有了[X，X，X，X，Newbegin，Z，Aftera，Newend，A]
  ASSERT_EQ(redis.Length("k1"), 9);
  ASSERT_TRUE(redis.Index("k1", -1, &tempv));
  ASSERT_EQ(tempv, "a");
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "x");

//测试过度放炮（删除过多）
  numRemoved = redis.Remove("k1", -9000, "x");
ASSERT_EQ(numRemoved , 4);    //只有真正删除了4
  ASSERT_EQ(redis.Length("k1"), 5);
  ASSERT_TRUE(redis.Index("k1", 0, &tempv));
  ASSERT_EQ(tempv, "newbegin");
  numRemoved = redis.Remove("k1", 1, "x");
  ASSERT_EQ(numRemoved, 0);

//尝试全部删除！
numRemoved = redis.Remove("k1", 0, "newbegin");   //删除0将全部删除！
  ASSERT_EQ(numRemoved, 1);

//从空列表中删除
  ASSERT_TRUE(redis.Trim("k1", 1, 0));
  numRemoved = redis.Remove("k1", 1, "z");
  ASSERT_EQ(numRemoved, 0);
}


//测试多个密钥和持久性
TEST_F(RedisListsTest, PersistenceMultiKeyTest) {
std::string tempv;  //以下用于所有index（）、popRight（）、popLeft（））

//块一：在数据库中填充一个键
  {
RedisLists redis(kDefaultDbName, options, true);   //破坏性的

//一系列的推动和插入
//将导致[newbegin，z，a，aftera，x，newend，a，a]
    redis.PushLeft("k1", "a");
    redis.PushLeft("k1", "z");
    redis.PushRight("k1", "x");
redis.InsertBefore("k1", "z", "newbegin");    //在列表开始之前插入
redis.InsertAfter("k1", "x", "newend");       //在列表结尾后插入
    redis.InsertAfter("k1", "a", "aftera");
    redis.PushRight("k1", "a");
    redis.PushRight("k1", "a");

    ASSERT_TRUE(redis.Index("k1", 3, &tempv));
    ASSERT_EQ(tempv, "aftera");
  }

//阻止2:确保保存了更改并添加了一些其他密钥
  {
RedisLists redis(kDefaultDbName, options, false); //持久的，非破坏性的

//检查
    ASSERT_EQ(redis.Length("k1"), 8);
    ASSERT_TRUE(redis.Index("k1", 3, &tempv));
    ASSERT_EQ(tempv, "aftera");

    redis.PushRight("k2", "randomkey");
    redis.PushLeft("k2", "sas");

    redis.PopLeft("k1", &tempv);
  }

//块3：验证块2的更改
  {
RedisLists redis(kDefaultDbName, options, false); //持久的，非破坏性的

//检查
    ASSERT_EQ(redis.Length("k1"), 7);
    ASSERT_EQ(redis.Length("k2"), 2);
    ASSERT_TRUE(redis.Index("k1", 0, &tempv));
    ASSERT_EQ(tempv, "z");
    ASSERT_TRUE(redis.Index("k2", -2, &tempv));
    ASSERT_EQ(tempv, "sas");
  }
}

///手动Redis测试从这里开始
///只有在运行以下命令时才会发生此情况：./redis_test-m

namespace {
void MakeUpper(std::string* const s) {
  int len = static_cast<int>(s->length());
  for (int i = 0; i < len; ++i) {
(*s)[i] = toupper((*s)[i]);  //C-version defined in<ctype.h>
  }
}

///允许用户在命令行中输入redis命令。
///这对于手动/交互式测试/调试很有用。
///use destructive=true在使用前清理数据库。
///使用destructive=false记住以前的状态（即：持久）
///应从主函数调用。
int manual_redis_test(bool destructive){
  RedisLists redis(RedisListsTest::kDefaultDbName,
                   RedisListsTest::options,
                   destructive);

//托多：现在，请用空格分隔每个单词。
//在实际redis中，可以使用引号指定复合值
//示例：rpush mylist“这是一个复合值”

  std::string command;
  while(true) {
    std::cin >> command;
    MakeUpper(&command);

    if (command == "LINSERT") {
      std::string k, t, p, v;
      std::cin >> k >> t >> p >> v;
      MakeUpper(&t);
      if (t=="BEFORE") {
        std::cout << redis.InsertBefore(k, p, v) << std::endl;
      } else if (t=="AFTER") {
        std::cout << redis.InsertAfter(k, p, v) << std::endl;
      }
    } else if (command == "LPUSH") {
      std::string k, v;
      std::cin >> k >> v;
      redis.PushLeft(k, v);
    } else if (command == "RPUSH") {
      std::string k, v;
      std::cin >> k >> v;
      redis.PushRight(k, v);
    } else if (command == "LPOP") {
      std::string k;
      std::cin >> k;
      std::string res;
      redis.PopLeft(k, &res);
      std::cout << res << std::endl;
    } else if (command == "RPOP") {
      std::string k;
      std::cin >> k;
      std::string res;
      redis.PopRight(k, &res);
      std::cout << res << std::endl;
    } else if (command == "LREM") {
      std::string k;
      int amt;
      std::string v;

      std::cin >> k >> amt >> v;
      std::cout << redis.Remove(k, amt, v) << std::endl;
    } else if (command == "LLEN") {
      std::string k;
      std::cin >> k;
      std::cout << redis.Length(k) << std::endl;
    } else if (command == "LRANGE") {
      std::string k;
      int i, j;
      std::cin >> k >> i >> j;
      std::vector<std::string> res = redis.Range(k, i, j);
      for (auto it = res.begin(); it != res.end(); ++it) {
        std::cout << " " << (*it);
      }
      std::cout << std::endl;
    } else if (command == "LTRIM") {
      std::string k;
      int i, j;
      std::cin >> k >> i >> j;
      redis.Trim(k, i, j);
    } else if (command == "LSET") {
      std::string k;
      int idx;
      std::string v;
      std::cin >> k >> idx >> v;
      redis.Set(k, idx, v);
    } else if (command == "LINDEX") {
      std::string k;
      int idx;
      std::cin >> k >> idx;
      std::string res;
      redis.Index(k, idx, &res);
      std::cout << res << std::endl;
} else if (command == "PRINT") {      //德翁补充
      std::string k;
      std::cin >> k;
      redis.Print(k);
    } else if (command == "QUIT") {
      return 0;
    } else {
      std::cout << "unknown command: " << command << std::endl;
    }
  }
}
}  //命名空间

} //命名空间rocksdb


//用法：默认为“./redis_test”（单元测试）
//“/redis-test-m”用于手动测试（redis命令api）
//“/redis-test-m-d”用于破坏性手动测试（使用前清除db）


namespace {
//检查参数列表中的“want”参数
bool found_arg(int argc, char* argv[], const char* want){
  for(int i=1; i<argc; ++i){
    if (strcmp(argv[i], want) == 0) {
      return true;
    }
  }
  return false;
}
}  //命名空间

//将运行单元测试。
//但是，如果指定了-m，它将执行用户手动/交互式测试。
//-m-d是手动的和破坏性的（将在使用前清除数据库）
int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  if (found_arg(argc, argv, "-m")) {
    bool destructive = found_arg(argc, argv, "-d");
    return rocksdb::manual_redis_test(destructive);
  } else {
    return RUN_ALL_TESTS();
  }
}

#else
#include <stdio.h>

int main(int argc, char* argv[]) {
  fprintf(stderr, "SKIPPED as redis is not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //！摇滚乐
