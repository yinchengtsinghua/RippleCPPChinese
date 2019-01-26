
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
//RocksDB:压缩选项通用。

#include <jni.h>

#include "include/org_rocksdb_CompactionOptionsUniversal.h"
#include "rocksdb/advanced_options.h"
#include "rocksjni/portal.h"

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：新的影响选项通用
 *签字：（）J
 **/

jlong Java_org_rocksdb_CompactionOptionsUniversal_newCompactionOptionsUniversal(
    JNIEnv* env, jclass jcls) {
  const auto* opt = new rocksdb::CompactionOptionsUniversal();
  return reinterpret_cast<jlong>(opt);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：设置大小比
 *签名：（Ji）V
 **/

void Java_org_rocksdb_CompactionOptionsUniversal_setSizeRatio(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jsize_ratio) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  opt->size_ratio = static_cast<unsigned int>(jsize_ratio);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：sizeratio
 *签字：（j）I
 **/

jint Java_org_rocksdb_CompactionOptionsUniversal_sizeRatio(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  return static_cast<jint>(opt->size_ratio);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：setminmergewidth
 *签名：（Ji）V
 **/

void Java_org_rocksdb_CompactionOptionsUniversal_setMinMergeWidth(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jmin_merge_width) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  opt->min_merge_width = static_cast<unsigned int>(jmin_merge_width);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：MinMergeWidth
 *签字：（j）I
 **/

jint Java_org_rocksdb_CompactionOptionsUniversal_minMergeWidth(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  return static_cast<jint>(opt->min_merge_width);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：setmaxmergewidth
 *签名：（Ji）V
 **/

void Java_org_rocksdb_CompactionOptionsUniversal_setMaxMergeWidth(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jmax_merge_width) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  opt->max_merge_width = static_cast<unsigned int>(jmax_merge_width);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：maxmergewidth
 *签字：（j）I
 **/

jint Java_org_rocksdb_CompactionOptionsUniversal_maxMergeWidth(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  return static_cast<jint>(opt->max_merge_width);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：setmaxsizeamplificationPercent
 *签名：（Ji）V
 **/

void Java_org_rocksdb_CompactionOptionsUniversal_setMaxSizeAmplificationPercent(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jmax_size_amplification_percent) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  opt->max_size_amplification_percent =
      static_cast<unsigned int>(jmax_size_amplification_percent);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：maxsizeamplificationPercent
 *签字：（j）I
 **/

jint Java_org_rocksdb_CompactionOptionsUniversal_maxSizeAmplificationPercent(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  return static_cast<jint>(opt->max_size_amplification_percent);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：设置压缩大小百分比
 *签名：（Ji）V
 **/

void Java_org_rocksdb_CompactionOptionsUniversal_setCompressionSizePercent(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jcompression_size_percent) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  opt->compression_size_percent =
      static_cast<unsigned int>(jcompression_size_percent);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：压缩大小百分比
 *签字：（j）I
 **/

jint Java_org_rocksdb_CompactionOptionsUniversal_compressionSizePercent(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  return static_cast<jint>(opt->compression_size_percent);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：SetTopStyle
 *签字：（JB）V
 **/

void Java_org_rocksdb_CompactionOptionsUniversal_setStopStyle(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jstop_style_value) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  opt->stop_style =
      rocksdb::CompactionStopStyleJni::toCppCompactionStopStyle(
          jstop_style_value); 
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：stopstyle
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_CompactionOptionsUniversal_stopStyle(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  return rocksdb::CompactionStopStyleJni::toJavaCompactionStopStyle(
      opt->stop_style);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：setallowtrivialmove
 *签字：（JZ）V
 **/

void Java_org_rocksdb_CompactionOptionsUniversal_setAllowTrivialMove(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jallow_trivial_move) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  opt->allow_trivial_move = static_cast<bool>(jallow_trivial_move);
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：allowDrivealMove
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_CompactionOptionsUniversal_allowTrivialMove(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
  return opt->allow_trivial_move;
}

/*
 *类别：Org_RocksDB_CompactionOptions Universal
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_CompactionOptionsUniversal_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  delete reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(jhandle);
}
