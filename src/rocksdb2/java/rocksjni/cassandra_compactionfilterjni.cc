
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

#include <jni.h>

#include "include/org_rocksdb_CassandraCompactionFilter.h"
#include "utilities/cassandra/cassandra_compaction_filter.h"

/*
 *类别：Org_RocksDB_CassandraCompactionFilter
 *方法：创建新的cassandracompactionfilter0
 *签字：（）J
 **/

jlong Java_org_rocksdb_CassandraCompactionFilter_createNewCassandraCompactionFilter0(
    JNIEnv* env, jclass jcls, jboolean purge_ttl_on_expiration) {
  auto* compaction_filter =
      new rocksdb::cassandra::CassandraCompactionFilter(purge_ttl_on_expiration);
//将本机句柄设置为本机压缩筛选器
  return reinterpret_cast<jlong>(compaction_filter);
}
