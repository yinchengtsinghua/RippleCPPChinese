
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
//该文件实现Java和C++之间的“桥接”并启用
//调用C++ ROCKSDB:：恢复选项方法
//从Java端。

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <string>

#include "include/org_rocksdb_RestoreOptions.h"
#include "rocksjni/portal.h"
#include "rocksdb/utilities/backupable_db.h"
/*
 *类别：组织rocksdb_restoreoptions
 *方法：NewRestoreOptions
 *签字：（Z）J
 **/

jlong Java_org_rocksdb_RestoreOptions_newRestoreOptions(JNIEnv* env,
    jclass jcls, jboolean keep_log_files) {
  auto* ropt = new rocksdb::RestoreOptions(keep_log_files);
  return reinterpret_cast<jlong>(ropt);
}

/*
 *类别：组织rocksdb_restoreoptions
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_RestoreOptions_disposeInternal(JNIEnv* env, jobject jobj,
    jlong jhandle) {
  auto* ropt = reinterpret_cast<rocksdb::RestoreOptions*>(jhandle);
  assert(ropt);
  delete ropt;
}
