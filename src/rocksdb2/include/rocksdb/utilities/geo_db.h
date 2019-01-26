
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
#pragma once
#include <string>
#include <vector>

#include "rocksdb/utilities/stackable_db.h"
#include "rocksdb/status.h"

namespace rocksdb {

//
//设置地理数据库所需的可配置选项
//
struct GeoDBOptions {
//备份信息和错误信息将写入信息日志
//如果非nulpPTR。
//默认值：nullptr
  Logger* info_log;

  explicit GeoDBOptions(Logger* _info_log = nullptr):info_log(_info_log) { }
};

//
//在地球大地水准面上的位置
//
class GeoPosition {
 public:
  double latitude;
  double longitude;

  explicit GeoPosition(double la = 0, double lo = 0) :
    latitude(la), longitude(lo) {
  }
};

//
//大地水准面上对象的描述。它由GPS定位，
//并由ID标识。与此对象关联的值为
//不透明字符串“value”。由唯一ID标识的不同对象
//可以具有与它们关联的相同GPS位置。
//
class GeoObject {
 public:
  GeoPosition position;
  std::string id;
  std::string value;

  GeoObject() {}

  GeoObject(const GeoPosition& pos, const std::string& i,
            const std::string& val) :
    position(pos), id(i), value(val) {
  }
};

class GeoIterator {
 public:
  GeoIterator() = default;
  virtual ~GeoIterator() {}
  virtual void Next() = 0;
  virtual bool Valid() const = 0;
  virtual const GeoObject& geo_object() = 0;
  virtual Status status() const = 0;
};

//
//用geo db堆叠你的数据库，以便获得地理空间支持
//
class GeoDB : public StackableDB {
 public:
//geodbOptions必须与前一个中使用的相同
//数据库的化身
//
//geodb现在拥有指针'db*db'。你不应该删除它，或者
//调用geodb后使用
//geodb（db*db，const geodb选项和选项）：stackabledb（db）
  GeoDB(DB* db, const GeoDBOptions& options) : StackableDB(db) {}
  virtual ~GeoDB() {}

//将新对象插入位置数据库。对象是
//由ID唯一标识。如果已存在具有相同ID的对象
//存在于数据库中，则旧数据库将被新数据库覆盖。
//在此处插入的对象。
  virtual Status Insert(const GeoObject& object) = 0;

//检索位于指定GPS的对象的值
//位置并由“id”标识。
  virtual Status GetByPosition(const GeoPosition& pos,
                               const Slice& id, std::string* value) = 0;

//检索由“id”标识的对象的值。这种方法
//可能比GetByPosition慢
  virtual Status GetById(const Slice& id, GeoObject*  object) = 0;

//删除指定的对象
  virtual Status Remove(const Slice& id) = 0;

//返回循环半径内的项的迭代器
//指定的GPS位置。如果指定了“值的数目”，
//然后迭代器被限制为该数量的对象。
//半径以“米”为单位。
  virtual GeoIterator* SearchRadial(const GeoPosition& pos,
                                    double radius,
                                    int number_of_values = INT_MAX) = 0;
};

}  //命名空间rocksdb
#endif  //摇滚乐
