
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
//rocksdb：：压缩选项。

#include <jni.h>

#include "include/org_rocksdb_CompressionOptions.h"
#include "rocksdb/advanced_options.h"

/*
 *类别：org-rocksdb-u压缩选项
 *方法：新的压缩选项
 *签字：（）J
 **/

jlong Java_org_rocksdb_CompressionOptions_newCompressionOptions(
    JNIEnv* env, jclass jcls) {
  const auto* opt = new rocksdb::CompressionOptions();
  return reinterpret_cast<jlong>(opt);
}

/*
 *类别：org-rocksdb-u压缩选项
 *方法：设置窗口位
 *签名：（Ji）V
 **/

void Java_org_rocksdb_CompressionOptions_setWindowBits(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jwindow_bits) {
  auto* opt = reinterpret_cast<rocksdb::CompressionOptions*>(jhandle);
  opt->window_bits = static_cast<int>(jwindow_bits);
}

/*
 *类别：org-rocksdb-u压缩选项
 *方法：windowbits
 *签字：（j）I
 **/

jint Java_org_rocksdb_CompressionOptions_windowBits(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompressionOptions*>(jhandle);
  return static_cast<jint>(opt->window_bits);
}

/*
 *类别：org-rocksdb-u压缩选项
 *方法：设置级别
 *签名：（Ji）V
 **/

void Java_org_rocksdb_CompressionOptions_setLevel(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jlevel) {
  auto* opt = reinterpret_cast<rocksdb::CompressionOptions*>(jhandle);
  opt->level = static_cast<int>(jlevel);
}

/*
 *类别：org-rocksdb-u压缩选项
 *方法：水平
 *签字：（j）I
 **/

jint Java_org_rocksdb_CompressionOptions_level(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompressionOptions*>(jhandle);
  return static_cast<jint>(opt->level);
}

/*
 *类别：org-rocksdb-u压缩选项
 *方法：设置策略
 *签名：（Ji）V
 **/

void Java_org_rocksdb_CompressionOptions_setStrategy(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jstrategy) {
  auto* opt = reinterpret_cast<rocksdb::CompressionOptions*>(jhandle);
  opt->strategy = static_cast<int>(jstrategy);
}

/*
 *类别：org-rocksdb-u压缩选项
 *方法：策略
 *签字：（j）I
 **/

jint Java_org_rocksdb_CompressionOptions_strategy(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompressionOptions*>(jhandle);
  return static_cast<jint>(opt->strategy);
}

/*
 *类别：org-rocksdb-u压缩选项
 *方法：setMaxDictBytes
 *签名：（Ji）V
 **/

void Java_org_rocksdb_CompressionOptions_setMaxDictBytes(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jmax_dict_bytes) {
  auto* opt = reinterpret_cast<rocksdb::CompressionOptions*>(jhandle);
  opt->max_dict_bytes = static_cast<int>(jmax_dict_bytes);
}

/*
 *类别：org-rocksdb-u压缩选项
 *方法：MaxDictBytes
 *签字：（j）I
 **/

jint Java_org_rocksdb_CompressionOptions_maxDictBytes(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompressionOptions*>(jhandle);
  return static_cast<jint>(opt->max_dict_bytes);
}

/*
 *类别：org-rocksdb-u压缩选项
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_CompressionOptions_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  delete reinterpret_cast<rocksdb::CompressionOptions*>(jhandle);
}
