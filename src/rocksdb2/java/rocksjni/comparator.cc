
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
//该文件实现了Java和C++之间的“桥梁”。
//RocksDB：：比较器。

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <string>
#include <functional>

#include "include/org_rocksdb_AbstractComparator.h"
#include "include/org_rocksdb_Comparator.h"
#include "include/org_rocksdb_DirectComparator.h"
#include "rocksjni/comparatorjnicallback.h"
#include "rocksjni/portal.h"

//<editor fold desc=“org.rocksdb.abstractComparator>

/*
 *类：org-rocksdb-abstractcomparator
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_AbstractComparator_disposeInternal(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* bcjc = reinterpret_cast<rocksdb::BaseComparatorJniCallback*>(handle);
  assert(bcjc != nullptr);
  delete bcjc;
}
//< /编辑器折叠>

//<editor fold desc=“org.rocksdb.comparator>

/*
 *类别：Org_RocksDB_Comparator
 *方法：CreateNewComparator0
 *签字：（）J
 **/

jlong Java_org_rocksdb_Comparator_createNewComparator0(
    JNIEnv* env, jobject jobj, jlong copt_handle) {
  const rocksdb::ComparatorJniCallbackOptions* copt =
    reinterpret_cast<rocksdb::ComparatorJniCallbackOptions*>(copt_handle);
  const rocksdb::ComparatorJniCallback* c =
    new rocksdb::ComparatorJniCallback(env, jobj, copt);
  return reinterpret_cast<jlong>(c);
}
//< /编辑器折叠>

//<editor fold desc=“org.rocksdb.directcomparator>

/*
 *类别：Org_RocksDB_DirectComparator
 *方法：创建新的DirectComparator0
 *签字：（）J
 **/

jlong Java_org_rocksdb_DirectComparator_createNewDirectComparator0(
    JNIEnv* env, jobject jobj, jlong copt_handle) {
  const rocksdb::ComparatorJniCallbackOptions* copt =
    reinterpret_cast<rocksdb::ComparatorJniCallbackOptions*>(copt_handle);
  const rocksdb::DirectComparatorJniCallback* c =
    new rocksdb::DirectComparatorJniCallback(env, jobj, copt);
  return reinterpret_cast<jlong>(c);
}
//< /编辑器折叠>
