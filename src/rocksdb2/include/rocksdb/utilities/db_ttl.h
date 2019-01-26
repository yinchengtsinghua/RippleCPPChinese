
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

#pragma once
#ifndef ROCKSDB_LITE

#include <string>
#include <vector>

#include "rocksdb/utilities/stackable_db.h"
#include "rocksdb/db.h"

namespace rocksdb {

//支持TTL的数据库。
//
//美国案例：
//当插入的键值为
//打算在非严格的“ttl”时间内从数据库中删除
//因此，这保证插入的键值将保留在
//db for>=ttl时间量，db将努力删除
//在TTL秒后尽快插入键值。
//
//行为：
//以秒为单位接受TTL
//（int32_t）timestamp（creation）后缀为put内部的值
//仅在压缩中删除过期的TTL值：（timestamp+ttl<time\u now）
//get/iterator可能返回过期的条目（压缩尚未对其运行）
//不同打开时可以使用不同的TTL
//示例：t=0时打开1，ttl=4，插入k1、k2，t=2时关闭
//打开2，T=3，TTL=5。现在，k1，k2应该在t大于等于5时删除。
//只读=真在通常的只读模式下打开。压实不会
//已触发（非手动或自动），因此未删除过期条目
//
//制约因素：
//不指定/传递或非正TTL的行为类似于TTL=无穷大
//
//！！！！警告！！！！：
//直接调用db:：open重新打开此API创建的db
//损坏的值（时间戳后缀）将不存在TTL效果
//在第二次打开期间，请始终使用此API打开数据库
//传递带有小正值的TTL时要小心，因为
//可以在很短的时间内删除整个数据库

class DBWithTTL : public StackableDB {
 public:
  virtual Status CreateColumnFamilyWithTtl(
      const ColumnFamilyOptions& options, const std::string& column_family_name,
      ColumnFamilyHandle** handle, int ttl) = 0;

  static Status Open(const Options& options, const std::string& dbname,
                     DBWithTTL** dbptr, int32_t ttl = 0,
                     bool read_only = false);

  static Status Open(const DBOptions& db_options, const std::string& dbname,
                     const std::vector<ColumnFamilyDescriptor>& column_families,
                     std::vector<ColumnFamilyHandle*>* handles,
                     DBWithTTL** dbptr, std::vector<int32_t> ttls,
                     bool read_only = false);

 protected:
  explicit DBWithTTL(DB* db) : StackableDB(db) {}
};

}  //命名空间rocksdb
#endif  //摇滚乐
