
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2017至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <string>
#include <memory>

#include "include/org_rocksdb_CassandraValueMergeOperator.h"
#include "rocksjni/portal.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/statistics.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/table.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/merge_operator.h"
#include "utilities/cassandra/merge_operator.h"

/*
 *课程：组织rocksdb\u cassandravaluemergeoperator
 *方法：newsharedcasandravaluemergeoperator
 *签字：（）J
 **/

jlong Java_org_rocksdb_CassandraValueMergeOperator_newSharedCassandraValueMergeOperator
(JNIEnv* env, jclass jclazz) {
  auto* sptr_string_append_op = new std::shared_ptr<rocksdb::MergeOperator>(
    rocksdb::CassandraValueMergeOperator::CreateSharedInstance());
  return reinterpret_cast<jlong>(sptr_string_append_op);
}

/*
 *课程：组织rocksdb\u cassandravaluemergeoperator
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_CassandraValueMergeOperator_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* sptr_string_append_op =
      reinterpret_cast<std::shared_ptr<rocksdb::MergeOperator>* >(jhandle);
delete sptr_string_append_op;  //删除std:：shared_ptr
}
