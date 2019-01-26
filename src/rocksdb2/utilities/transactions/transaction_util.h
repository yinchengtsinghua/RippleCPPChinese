
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
#include <unordered_map>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/types.h"

namespace rocksdb {

struct TransactionKeyMapInfo {
//与此密钥的此事务相关的最早序列号
  SequenceNumber seq;

  uint32_t num_writes;
  uint32_t num_reads;

  bool exclusive;

  explicit TransactionKeyMapInfo(SequenceNumber seq_no)
      : seq(seq_no), num_writes(0), num_reads(0), exclusive(false) {}
};

using TransactionKeyMap =
    std::unordered_map<uint32_t,
                       std::unordered_map<std::string, TransactionKeyMapInfo>>;

class DBImpl;
struct SuperVersion;
class WriteBatchWithIndex;

class TransactionUtil {
 public:
//验证自此之后数据库中是否没有对此密钥进行写入
//序列号。
//
//如果cache_only为true，则此函数不会尝试读取任何
//SST文件。这将使此函数更有可能
//如果无法确定是否有冲突，则返回错误。
//
//成功时返回OK，如果存在写入冲突或其他错误，则返回Busy
//任何意外错误的状态。
  static Status CheckKeyForConflicts(DBImpl* db_impl,
                                     ColumnFamilyHandle* column_family,
                                     const std::string& key,
                                     SequenceNumber key_seq, bool cache_only);

//对于TransactionKeymap中的每个键、SequenceNumber对，此函数
//将验证自那以后没有对数据库中的密钥进行写操作
//序列号。
//
//成功时返回OK，如果存在写入冲突或其他错误，则返回Busy
//任何意外错误的状态。
//
//必需：只应在写入线程上调用此函数，或者如果
//互斥互斥。
  static Status CheckKeysForConflicts(DBImpl* db_impl,
                                      const TransactionKeyMap& keys,
                                      bool cache_only);

 private:
  static Status CheckKey(DBImpl* db_impl, SuperVersion* sv,
                         SequenceNumber earliest_seq, SequenceNumber key_seq,
                         const std::string& key, bool cache_only);
};

}  //命名空间rocksdb

#endif  //摇滚乐
