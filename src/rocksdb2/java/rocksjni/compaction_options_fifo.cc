
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
//RocksDB：压实选项FIFO。

#include <jni.h>

#include "include/org_rocksdb_CompactionOptionsFIFO.h"
#include "rocksdb/advanced_options.h"

/*
 *类别：ORG搎rocksdb搎CompactionOptions FIFO
 *方法：NewCompactionOptionsFifo
 *签字：（）J
 **/

jlong Java_org_rocksdb_CompactionOptionsFIFO_newCompactionOptionsFIFO(
    JNIEnv* env, jclass jcls) {
  const auto* opt = new rocksdb::CompactionOptionsFIFO();
  return reinterpret_cast<jlong>(opt);
}

/*
 *类别：ORG搎rocksdb搎CompactionOptions FIFO
 *方法：setmaxTablefilesSize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_CompactionOptionsFIFO_setMaxTableFilesSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jmax_table_files_size) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsFIFO*>(jhandle);
  opt->max_table_files_size = static_cast<uint64_t>(jmax_table_files_size);
}

/*
 *类别：ORG搎rocksdb搎CompactionOptions FIFO
 *方法：maxTablefilesSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_CompactionOptionsFIFO_maxTableFilesSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsFIFO*>(jhandle);
  return static_cast<jlong>(opt->max_table_files_size);
}

/*
 *类别：ORG搎rocksdb搎CompactionOptions FIFO
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_CompactionOptionsFIFO_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  delete reinterpret_cast<rocksdb::CompactionOptionsFIFO*>(jhandle);
}
