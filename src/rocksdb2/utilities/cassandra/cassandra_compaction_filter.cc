
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

#include "utilities/cassandra/cassandra_compaction_filter.h"
#include <string>
#include "rocksdb/slice.h"
#include "utilities/cassandra/format.h"


namespace rocksdb {
namespace cassandra {

const char* CassandraCompactionFilter::Name() const {
  return "CassandraCompactionFilter";
}

CompactionFilter::Decision CassandraCompactionFilter::FilterV2(
  int level,
  const Slice& key,
  ValueType value_type,
  const Slice& existing_value,
  std::string* new_value,
  std::string* skip_until) const {

  bool value_changed = false;
  RowValue row_value = RowValue::Deserialize(
    existing_value.data(), existing_value.size());
  RowValue compacted = purge_ttl_on_expiration_ ?
    row_value.PurgeTtl(&value_changed) :
    row_value.ExpireTtl(&value_changed);

  if(compacted.Empty()) {
    return Decision::kRemove;
  }

  if (value_changed) {
    compacted.Serialize(new_value);
    return Decision::kChangeValue;
  }

  return Decision::kKeep;
}

}  //命名空间cassandra
}  //命名空间rocksdb
