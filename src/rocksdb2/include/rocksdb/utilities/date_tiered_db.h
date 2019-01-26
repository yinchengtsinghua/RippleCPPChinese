
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

#include <map>
#include <string>
#include <vector>

#include "rocksdb/db.h"

namespace rocksdb {

//日期分层数据库是实现
//使用多列族的简化日期分层压缩策略
//作为时间窗口。
//
//datetiereddb提供了一个类似于db的接口，但它假定用户
//提供最后8个字节的键，这些字节以秒为单位编码为时间戳。数据提成数据库
//指定了一个TTL来声明何时应删除数据。
//
//datetiereddb从标准rocksdb实例中隐藏列族图层。它
//使用多个列族管理时间序列数据，每个列包含
//特定的时间范围。列族按其最大可能值命名
//时间戳。当数据更新到
//所有现有列族的最新时间戳。列的时间范围
//族可以通过“column_family_interval”进行配置。通过这样做，我们
//确保仅在柱族中进行压缩。
//
//为datetiereddb分配了TTL。当列族中的所有数据
//过期（cf_timestamp<=cur_timestamp-ttl），我们直接删除整个
//列族。
//
//TODO（JHLI）：这只是故障诊断码的简化版本。在完整的故障诊断码中，
//时间窗口可以随着时间的推移而合并，以便旧的时间窗口
//更大的时间范围。此外，仅对相邻的SST文件执行压缩
//确保SST文件之间没有时间重叠。

class DateTieredDB {
 public:
//打开名为“dbname”的datetiereddb。
//与db:：open（）类似，创建的数据库对象存储在dbptr中。
//
//可以配置两个参数：`ttl`以指定
//关键字应存在于数据库中，并指定“column_family_interval”
//列族间隔的时间范围。
//
//如果只读设置为true，则打开只读数据库。
//TODO（JHLI）：应使用包含TTL和
//列\族\间隔。
  static Status Open(const Options& options, const std::string& dbname,
                     DateTieredDB** dbptr, int64_t ttl,
                     int64_t column_family_interval, bool read_only = false);

  explicit DateTieredDB() {}

  virtual ~DateTieredDB() {}

//Put方法的包装。类似于db:：put（），但列族
//插入由键中的时间戳决定，即用户的最后8个字节
//关键。如果密钥已过时，则不会插入该密钥。
//
//当客户机在datiereddb中放入一个键值对时，它假定最后8个字节
//的键被编码为时间戳。时间戳是64位有符号整数
//编码为1970-01-01 00:00:00（UTC）以来的秒数（与
//env:：getcurrentTime（））。时间戳应该用big endian编码。
  virtual Status Put(const WriteOptions& options, const Slice& key,
                     const Slice& val) = 0;

//get方法的包装。类似于db:：get（），但列族是确定的
//按键中的时间戳。如果密钥已过时，将找不到该密钥。
  virtual Status Get(const ReadOptions& options, const Slice& key,
                     std::string* value) = 0;

//删除方法的包装。类似于db:：delete（），但列族是
//由键中的时间戳决定。如果键已过时，则返回NotFound
//状态。
  virtual Status Delete(const WriteOptions& options, const Slice& key) = 0;

//keymayexist方法的包装。类似于db:：keymayexist（），但列
//族由键中的时间戳决定。当密钥已存在时返回false
//过时的。
  virtual bool KeyMayExist(const ReadOptions& options, const Slice& key,
                           std::string* value, bool* value_found = nullptr) = 0;

//合并方法的包装。类似于db:：merge（），但列族是
//由键中的时间戳决定。
  virtual Status Merge(const WriteOptions& options, const Slice& key,
                       const Slice& value) = 0;

//创建一个迭代器来隐藏低级细节。此迭代器内部
//合并所有活动时间序列列族的结果。注意
//在所有数据都过时之前，不会删除列族，因此
//迭代器可能访问过时的键值对。
  virtual Iterator* NewIterator(const ReadOptions& opts) = 0;

//显式删除所有键都已过时的列族。这个
//进程也在put（）操作中被非法完成。
  virtual Status DropObsoleteColumnFamilies() = 0;

static const uint64_t kTSLength = sizeof(int64_t);  //时间戳大小
};

}  //命名空间rocksdb
#endif  //摇滚乐
