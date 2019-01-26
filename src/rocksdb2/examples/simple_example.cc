
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

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

using namespace rocksdb;

std::string kDBPath = "/tmp/rocksdb_simple_example";

int main() {
  DB* db;
  Options options;
//优化RocksDB。这是让RockSDB表现出色的最简单方法
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
//如果数据库尚未存在，则创建它
  options.create_if_missing = true;

//开放数据库
  Status s = DB::Open(options, kDBPath, &db);
  assert(s.ok());

//设置关键值
  s = db->Put(WriteOptions(), "key1", "value");
  assert(s.ok());
  std::string value;
//获取价值
  s = db->Get(ReadOptions(), "key1", &value);
  assert(s.ok());
  assert(value == "value");

//自动应用一组更新
  {
    WriteBatch batch;
    batch.Delete("key1");
    batch.Put("key2", value);
    s = db->Write(WriteOptions(), &batch);
  }

  s = db->Get(ReadOptions(), "key1", &value);
  assert(s.IsNotFound());

  db->Get(ReadOptions(), "key2", &value);
  assert(value == "value");

  {
    PinnableSlice pinnable_val;
    db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
    assert(pinnable_val == "value");
  }

  {
    std::string string_val;
//如果无法固定该值，则会将该值复制到其内部缓冲区。
//施工时可设置内部缓冲。
    PinnableSlice pinnable_val(&string_val);
    db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
    assert(pinnable_val == "value");
//如果该值未固定，则内部缓冲区必须具有该值。
    assert(pinnable_val.IsPinned() || string_val == "value");
  }

  PinnableSlice pinnable_val;
  db->Get(ReadOptions(), db->DefaultColumnFamily(), "key1", &pinnable_val);
  assert(s.IsNotFound());
//每次使用后和每次重新使用前重置pinnableslice
  pinnable_val.Reset();
  db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
  assert(pinnable_val == "value");
  pinnable_val.Reset();
//在此点之后，由pinnable_val指向的切片无效

  delete db;

  return 0;
}
