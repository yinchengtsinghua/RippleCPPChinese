
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
//这个文件实现了Java和C++之间的“桥梁”。

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/org_rocksdb_Snapshot.h"
#include "rocksdb/db.h"
#include "rocksjni/portal.h"

/*
 *class:org_rocksdb_快照
 *方法：GetSequenceNumber
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Snapshot_getSequenceNumber(JNIEnv* env,
    jobject jobj, jlong jsnapshot_handle) {
  auto* snapshot = reinterpret_cast<rocksdb::Snapshot*>(
      jsnapshot_handle);
  return snapshot->GetSequenceNumber();
}
