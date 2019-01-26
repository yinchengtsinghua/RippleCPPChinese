
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2014，vlad balan（vlad.gm@gmail.com）。版权所有。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//
//该文件实现了Java与C++之间的“桥梁”
//对于rocksdb:：mergeoperator。

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <string>
#include <memory>

#include "include/org_rocksdb_StringAppendOperator.h"
#include "rocksjni/portal.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/statistics.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/table.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/merge_operator.h"
#include "utilities/merge_operators.h"

/*
 *类：org_rocksdb_stringAppendOperator
 *方法：NewSharedStringAppendOperator
 *签字：（）J
 **/

jlong Java_org_rocksdb_StringAppendOperator_newSharedStringAppendOperator
(JNIEnv* env, jclass jclazz) {
  auto* sptr_string_append_op = new std::shared_ptr<rocksdb::MergeOperator>(
    rocksdb::MergeOperators::CreateFromStringId("stringappend"));
  return reinterpret_cast<jlong>(sptr_string_append_op);
}

/*
 *类：org_rocksdb_stringAppendOperator
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_StringAppendOperator_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* sptr_string_append_op =
      reinterpret_cast<std::shared_ptr<rocksdb::MergeOperator>* >(jhandle);
delete sptr_string_append_op;  //删除std:：shared_ptr
}
