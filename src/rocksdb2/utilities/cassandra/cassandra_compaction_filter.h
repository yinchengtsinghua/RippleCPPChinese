
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
#include <string>
#include "rocksdb/compaction_filter.h"
#include "rocksdb/slice.h"

namespace rocksdb {
namespace cassandra {

/*
 *压缩过滤器，用于删除带TTL的过期Cassandra数据。
 *如果“到期时清除TTL”选项设置为“真，过期数据”
 *将直接清除。否则将转换过期的数据
 *首先是墓碑，然后在GC宽限期之后最终移除。
 *`purge_ttl_on_expiration`只应在所有
 *写入具有相同的TTL设置，否则可能会使旧数据返回。
 **/

class CassandraCompactionFilter : public CompactionFilter {
public:
  explicit CassandraCompactionFilter(bool purge_ttl_on_expiration)
    : purge_ttl_on_expiration_(purge_ttl_on_expiration) {}

  const char* Name() const override;
  virtual Decision FilterV2(int level,
                            const Slice& key,
                            ValueType value_type,
                            const Slice& existing_value,
                            std::string* new_value,
                            std::string* skip_until) const override;

private:
  bool purge_ttl_on_expiration_;
};
}  //命名空间cassandra
}  //命名空间rocksdb
