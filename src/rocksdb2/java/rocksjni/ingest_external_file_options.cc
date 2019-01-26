
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
//rocksdb：：筛选策略。

#include <jni.h>

#include "include/org_rocksdb_IngestExternalFileOptions.h"
#include "rocksdb/options.h"

/*
 *class:org_rocksdb_摄取外部文件选项
 *方法：newIngestExternalFileOptions
 *签字：（）J
 **/

jlong Java_org_rocksdb_IngestExternalFileOptions_newIngestExternalFileOptions__(
    JNIEnv* env, jclass jclazz) {
  auto* options = new rocksdb::IngestExternalFileOptions();
  return reinterpret_cast<jlong>(options);
}

/*
 *class:org_rocksdb_摄取外部文件选项
 *方法：newIngestExternalFileOptions
 *签字：（ZZZZ）J
 **/

jlong Java_org_rocksdb_IngestExternalFileOptions_newIngestExternalFileOptions__ZZZZ(
    JNIEnv* env, jclass jcls, jboolean jmove_files,
    jboolean jsnapshot_consistency, jboolean jallow_global_seqno,
    jboolean jallow_blocking_flush) {
  auto* options = new rocksdb::IngestExternalFileOptions();
  options->move_files = static_cast<bool>(jmove_files);
  options->snapshot_consistency = static_cast<bool>(jsnapshot_consistency);
  options->allow_global_seqno = static_cast<bool>(jallow_global_seqno);
  options->allow_blocking_flush = static_cast<bool>(jallow_blocking_flush);
  return reinterpret_cast<jlong>(options);
}

/*
 *class:org_rocksdb_摄取外部文件选项
 *方法：移动文件
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_IngestExternalFileOptions_moveFiles(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* options =
      reinterpret_cast<rocksdb::IngestExternalFileOptions*>(jhandle);
  return static_cast<jboolean>(options->move_files);
}

/*
 *class:org_rocksdb_摄取外部文件选项
 *方法：setmovefiles
 *签字：（JZ）V
 **/

void Java_org_rocksdb_IngestExternalFileOptions_setMoveFiles(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jmove_files) {
  auto* options =
      reinterpret_cast<rocksdb::IngestExternalFileOptions*>(jhandle);
  options->move_files = static_cast<bool>(jmove_files);
}

/*
 *class:org_rocksdb_摄取外部文件选项
 *方法：SnapshotConsity
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_IngestExternalFileOptions_snapshotConsistency(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* options =
      reinterpret_cast<rocksdb::IngestExternalFileOptions*>(jhandle);
  return static_cast<jboolean>(options->snapshot_consistency);
}

/*
 *class:org_rocksdb_摄取外部文件选项
 *方法：设置快照一致性
 *签字：（JZ）V
 **/

void Java_org_rocksdb_IngestExternalFileOptions_setSnapshotConsistency(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jsnapshot_consistency) {
  auto* options =
      reinterpret_cast<rocksdb::IngestExternalFileOptions*>(jhandle);
  options->snapshot_consistency = static_cast<bool>(jsnapshot_consistency);
}

/*
 *class:org_rocksdb_摄取外部文件选项
 *方法：allowglobalseqno
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_IngestExternalFileOptions_allowGlobalSeqNo(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* options =
      reinterpret_cast<rocksdb::IngestExternalFileOptions*>(jhandle);
  return static_cast<jboolean>(options->allow_global_seqno);
}

/*
 *class:org_rocksdb_摄取外部文件选项
 *方法：setallowglobalseqno
 *签字：（JZ）V
 **/

void Java_org_rocksdb_IngestExternalFileOptions_setAllowGlobalSeqNo(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jallow_global_seqno) {
  auto* options =
      reinterpret_cast<rocksdb::IngestExternalFileOptions*>(jhandle);
  options->allow_global_seqno = static_cast<bool>(jallow_global_seqno);
}

/*
 *class:org_rocksdb_摄取外部文件选项
 *方法：allowblockingflush
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_IngestExternalFileOptions_allowBlockingFlush(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* options =
      reinterpret_cast<rocksdb::IngestExternalFileOptions*>(jhandle);
  return static_cast<jboolean>(options->allow_blocking_flush);
}

/*
 *class:org_rocksdb_摄取外部文件选项
 *方法：setallowblockingflush
 *签字：（JZ）V
 **/

void Java_org_rocksdb_IngestExternalFileOptions_setAllowBlockingFlush(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jallow_blocking_flush) {
  auto* options =
      reinterpret_cast<rocksdb::IngestExternalFileOptions*>(jhandle);
  options->allow_blocking_flush = static_cast<bool>(jallow_blocking_flush);
}

/*
 *class:org_rocksdb_摄取外部文件选项
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_IngestExternalFileOptions_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* options =
      reinterpret_cast<rocksdb::IngestExternalFileOptions*>(jhandle);
  delete options;
}