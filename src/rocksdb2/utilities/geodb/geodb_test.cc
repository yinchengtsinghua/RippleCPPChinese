
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
#ifndef ROCKSDB_LITE
#include "utilities/geodb/geodb_impl.h"

#include <cctype>
#include "util/testharness.h"

namespace rocksdb {

class GeoDBTest : public testing::Test {
 public:
  static const std::string kDefaultDbName;
  static Options options;
  DB* db;
  GeoDB* geodb;

  GeoDBTest() {
    GeoDBOptions geodb_options;
    EXPECT_OK(DestroyDB(kDefaultDbName, options));
    options.create_if_missing = true;
    Status status = DB::Open(options, kDefaultDbName, &db);
    geodb =  new GeoDBImpl(db, geodb_options);
  }

  ~GeoDBTest() {
    delete geodb;
  }

  GeoDB* getdb() {
    return geodb;
  }
};

const std::string GeoDBTest::kDefaultDbName = test::TmpDir() + "/geodb_test";
Options GeoDBTest::options = Options();

//插入、获取和移除
TEST_F(GeoDBTest, SimpleTest) {
  GeoPosition pos1(100, 101);
  std::string id1("id1");
  std::string value1("value1");

//将第一个对象插入数据库
  GeoObject obj1(pos1, id1, value1);
  Status status = getdb()->Insert(obj1);
  ASSERT_TRUE(status.ok());

//将第二个对象插入数据库
  GeoPosition pos2(200, 201);
  std::string id2("id2");
  std::string value2 = "value2";
  GeoObject obj2(pos2, id2, value2);
  status = getdb()->Insert(obj2);
  ASSERT_TRUE(status.ok());

//使用位置检索第一个对象
  std::string value;
  status = getdb()->GetByPosition(pos1, Slice(id1), &value);
  ASSERT_TRUE(status.ok());
  ASSERT_EQ(value, value1);

//使用ID检索第一个对象
  GeoObject obj;
  status = getdb()->GetById(Slice(id1), &obj);
  ASSERT_TRUE(status.ok());
  ASSERT_EQ(obj.position.latitude, 100);
  ASSERT_EQ(obj.position.longitude, 101);
  ASSERT_EQ(obj.id.compare(id1), 0);
  ASSERT_EQ(obj.value, value1);

//删除第一个对象
  status = getdb()->Remove(Slice(id1));
  ASSERT_TRUE(status.ok());
  status = getdb()->GetByPosition(pos1, Slice(id1), &value);
  ASSERT_TRUE(status.IsNotFound());
  status = getdb()->GetById(id1, &obj);
  ASSERT_TRUE(status.IsNotFound());

//检查是否仍然可以找到第二个对象
  status = getdb()->GetByPosition(pos2, id2, &value);
  ASSERT_TRUE(status.ok());
  ASSERT_EQ(value, value2);
  status = getdb()->GetById(id2, &obj);
  ASSERT_TRUE(status.ok());
}

//搜索。
//通过http://www.stevenmorse.org/nearest/distance.php验证距离
TEST_F(GeoDBTest, Search) {
  GeoPosition pos1(45, 45);
  std::string id1("mid1");
  std::string value1 = "midvalue1";

//在45度纬度插入对象
  GeoObject obj1(pos1, id1, value1);
  Status status = getdb()->Insert(obj1);
  ASSERT_TRUE(status.ok());

//搜索所有以46度纬度为中心的物体
//半径200公里。我们应该找到一个
//我们前面插入了。
  GeoIterator* iter1 = getdb()->SearchRadial(GeoPosition(46, 46), 200000);
  ASSERT_TRUE(status.ok());
  ASSERT_EQ(iter1->geo_object().value, "midvalue1");
  uint32_t size = 0;
  while (iter1->Valid()) {
    size++;
    iter1->Next();
  }
  ASSERT_EQ(size, 1U);
  delete iter1;

//搜索所有以46度纬度为中心的物体
//半径2公里。应该没有。
  GeoIterator* iter2 = getdb()->SearchRadial(GeoPosition(46, 46), 2);
  ASSERT_TRUE(status.ok());
  ASSERT_FALSE(iter2->Valid());
  delete iter2;
}

}  //命名空间rocksdb

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#else

#include <stdio.h>

int main() {
  fprintf(stderr, "SKIPPED\n");
  return 0;
}

#endif  //！摇滚乐
