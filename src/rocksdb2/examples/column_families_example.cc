
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
#include <cstdio>
#include <string>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

using namespace rocksdb;

std::string kDBPath = "/tmp/rocksdb_column_families_example";

int main() {
//开放数据库
  Options options;
  options.create_if_missing = true;
  DB* db;
  Status s = DB::Open(options, kDBPath, &db);
  assert(s.ok());

//创建列族
  ColumnFamilyHandle* cf;
  s = db->CreateColumnFamily(ColumnFamilyOptions(), "new_cf", &cf);
  assert(s.ok());

//关闭数据库
  delete cf;
  delete db;

//使用两列族打开数据库
  std::vector<ColumnFamilyDescriptor> column_families;
//必须打开默认列族
  column_families.push_back(ColumnFamilyDescriptor(
      kDefaultColumnFamilyName, ColumnFamilyOptions()));
//也打开新的
  column_families.push_back(ColumnFamilyDescriptor(
      "new_cf", ColumnFamilyOptions()));
  std::vector<ColumnFamilyHandle*> handles;
  s = DB::Open(DBOptions(), kDBPath, column_families, &handles, &db);
  assert(s.ok());

//从非默认列族中放置和获取
  s = db->Put(WriteOptions(), handles[1], Slice("key"), Slice("value"));
  assert(s.ok());
  std::string value;
  s = db->Get(ReadOptions(), handles[1], Slice("key"), &value);
  assert(s.ok());

//原子写入
  WriteBatch batch;
  batch.Put(handles[0], Slice("key2"), Slice("value2"));
  batch.Put(handles[1], Slice("key3"), Slice("value3"));
  batch.Delete(handles[0], Slice("key"));
  s = db->Write(WriteOptions(), &batch);
  assert(s.ok());

//删除列族
  s = db->DropColumnFamily(handles[1]);
  assert(s.ok());

//关闭数据库
  for (auto handle : handles) {
    delete handle;
  }
  delete db;

  return 0;
}
