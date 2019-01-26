
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
//调用C++ ROCKSDB:：EnVoice方法
//从Java端。

#include <jni.h>

#include "include/org_rocksdb_EnvOptions.h"
#include "rocksdb/env.h"

#define ENV_OPTIONS_SET_BOOL(_jhandle, _opt)                \
  reinterpret_cast<rocksdb::EnvOptions *>(_jhandle)->_opt = \
      static_cast<bool>(_opt)

#define ENV_OPTIONS_SET_SIZE_T(_jhandle, _opt)              \
  reinterpret_cast<rocksdb::EnvOptions *>(_jhandle)->_opt = \
      static_cast<size_t>(_opt)

#define ENV_OPTIONS_SET_UINT64_T(_jhandle, _opt)            \
  reinterpret_cast<rocksdb::EnvOptions *>(_jhandle)->_opt = \
      static_cast<uint64_t>(_opt)

#define ENV_OPTIONS_GET(_jhandle, _opt) \
  reinterpret_cast<rocksdb::EnvOptions *>(_jhandle)->_opt

/*
 *课程：组织rocksdb环境选项
 *方法：newenvoptions
 *签字：（）J
 **/

jlong Java_org_rocksdb_EnvOptions_newEnvOptions(JNIEnv *env, jclass jcls) {
  auto *env_opt = new rocksdb::EnvOptions();
  return reinterpret_cast<jlong>(env_opt);
}

/*
 *课程：组织rocksdb环境选项
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_EnvOptions_disposeInternal(JNIEnv *env, jobject jobj,
                                                 jlong jhandle) {
  auto* eo = reinterpret_cast<rocksdb::EnvOptions *>(jhandle);
  assert(eo != nullptr);
  delete eo;
}

/*
 *课程：组织rocksdb环境选项
 *方法：setusedirectreads
 *签字：（JZ）V
 **/

void Java_org_rocksdb_EnvOptions_setUseDirectReads(JNIEnv *env, jobject jobj,
                                                   jlong jhandle,
                                                   jboolean use_direct_reads) {
  ENV_OPTIONS_SET_BOOL(jhandle, use_direct_reads);
}

/*
 *课程：组织rocksdb环境选项
 *方法：使用直接读取
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_EnvOptions_useDirectReads(JNIEnv *env, jobject jobj,
                                                    jlong jhandle) {
  return ENV_OPTIONS_GET(jhandle, use_direct_reads);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setusedirectwrites
 *签字：（JZ）V
 **/

void Java_org_rocksdb_EnvOptions_setUseDirectWrites(
    JNIEnv *env, jobject jobj, jlong jhandle, jboolean use_direct_writes) {
  ENV_OPTIONS_SET_BOOL(jhandle, use_direct_writes);
}

/*
 *课程：组织rocksdb环境选项
 *方法：useDirectWrites
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_EnvOptions_useDirectWrites(JNIEnv *env, jobject jobj,
                                                     jlong jhandle) {
  return ENV_OPTIONS_GET(jhandle, use_direct_writes);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setusemmapreads
 *签字：（JZ）V
 **/

void Java_org_rocksdb_EnvOptions_setUseMmapReads(JNIEnv *env, jobject jobj,
                                                 jlong jhandle,
                                                 jboolean use_mmap_reads) {
  ENV_OPTIONS_SET_BOOL(jhandle, use_mmap_reads);
}

/*
 *课程：组织rocksdb环境选项
 *方法：使用命令
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_EnvOptions_useMmapReads(JNIEnv *env, jobject jobj,
                                                  jlong jhandle) {
  return ENV_OPTIONS_GET(jhandle, use_mmap_reads);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setusemmapwrities
 *签字：（JZ）V
 **/

void Java_org_rocksdb_EnvOptions_setUseMmapWrites(JNIEnv *env, jobject jobj,
                                                  jlong jhandle,
                                                  jboolean use_mmap_writes) {
  ENV_OPTIONS_SET_BOOL(jhandle, use_mmap_writes);
}

/*
 *课程：组织rocksdb环境选项
 *方法：使用mmapwrities
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_EnvOptions_useMmapWrites(JNIEnv *env, jobject jobj,
                                                   jlong jhandle) {
  return ENV_OPTIONS_GET(jhandle, use_mmap_writes);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setallowflocate
 *签字：（JZ）V
 **/

void Java_org_rocksdb_EnvOptions_setAllowFallocate(JNIEnv *env, jobject jobj,
                                                   jlong jhandle,
                                                   jboolean allow_fallocate) {
  ENV_OPTIONS_SET_BOOL(jhandle, allow_fallocate);
}

/*
 *课程：组织rocksdb环境选项
 *方法：allowflocate
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_EnvOptions_allowFallocate(JNIEnv *env, jobject jobj,
                                                    jlong jhandle) {
  return ENV_OPTIONS_GET(jhandle, allow_fallocate);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setsetfdcloexec
 *签字：（JZ）V
 **/

void Java_org_rocksdb_EnvOptions_setSetFdCloexec(JNIEnv *env, jobject jobj,
                                                 jlong jhandle,
                                                 jboolean set_fd_cloexec) {
  ENV_OPTIONS_SET_BOOL(jhandle, set_fd_cloexec);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setfdcloexec
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_EnvOptions_setFdCloexec(JNIEnv *env, jobject jobj,
                                                  jlong jhandle) {
  return ENV_OPTIONS_GET(jhandle, set_fd_cloexec);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setbytespersync
 *签字：（JJ）V
 **/

void Java_org_rocksdb_EnvOptions_setBytesPerSync(JNIEnv *env, jobject jobj,
                                                 jlong jhandle,
                                                 jlong bytes_per_sync) {
  ENV_OPTIONS_SET_UINT64_T(jhandle, bytes_per_sync);
}

/*
 *课程：组织rocksdb环境选项
 *方法：bytespersync
 *签字：（j）j
 **/

jlong Java_org_rocksdb_EnvOptions_bytesPerSync(JNIEnv *env, jobject jobj,
                                               jlong jhandle) {
  return ENV_OPTIONS_GET(jhandle, bytes_per_sync);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setfallocatewithkeepsize
 *签字：（JZ）V
 **/

void Java_org_rocksdb_EnvOptions_setFallocateWithKeepSize(
    JNIEnv *env, jobject jobj, jlong jhandle,
    jboolean fallocate_with_keep_size) {
  ENV_OPTIONS_SET_BOOL(jhandle, fallocate_with_keep_size);
}

/*
 *课程：组织rocksdb环境选项
 *方法：使用keepsize进行fallocatewithkeepsize
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_EnvOptions_fallocateWithKeepSize(JNIEnv *env,
                                                           jobject jobj,
                                                           jlong jhandle) {
  return ENV_OPTIONS_GET(jhandle, fallocate_with_keep_size);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setcompactionreadaheadsize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_EnvOptions_setCompactionReadaheadSize(
    JNIEnv *env, jobject jobj, jlong jhandle, jlong compaction_readahead_size) {
  ENV_OPTIONS_SET_SIZE_T(jhandle, compaction_readahead_size);
}

/*
 *课程：组织rocksdb环境选项
 *方法：压缩读取标题大小
 *签字：（j）j
 **/

jlong Java_org_rocksdb_EnvOptions_compactionReadaheadSize(JNIEnv *env,
                                                          jobject jobj,
                                                          jlong jhandle) {
  return ENV_OPTIONS_GET(jhandle, compaction_readahead_size);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setrandomaaccessmaxbuffersize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_EnvOptions_setRandomAccessMaxBufferSize(
    JNIEnv *env, jobject jobj, jlong jhandle,
    jlong random_access_max_buffer_size) {
  ENV_OPTIONS_SET_SIZE_T(jhandle, random_access_max_buffer_size);
}

/*
 *课程：组织rocksdb环境选项
 *方法：RandomAccessMaxBufferSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_EnvOptions_randomAccessMaxBufferSize(JNIEnv *env,
                                                            jobject jobj,
                                                            jlong jhandle) {
  return ENV_OPTIONS_GET(jhandle, random_access_max_buffer_size);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setwritablefilemaxbuffersize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_EnvOptions_setWritableFileMaxBufferSize(
    JNIEnv *env, jobject jobj, jlong jhandle,
    jlong writable_file_max_buffer_size) {
  ENV_OPTIONS_SET_SIZE_T(jhandle, writable_file_max_buffer_size);
}

/*
 *课程：组织rocksdb环境选项
 *方法：WritableFileMaxBufferSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_EnvOptions_writableFileMaxBufferSize(JNIEnv *env,
                                                            jobject jobj,
                                                            jlong jhandle) {
  return ENV_OPTIONS_GET(jhandle, writable_file_max_buffer_size);
}

/*
 *课程：组织rocksdb环境选项
 *方法：setratelimiter
 *签字：（JJ）V
 **/

void Java_org_rocksdb_EnvOptions_setRateLimiter(JNIEnv *env, jobject jobj,
                                                jlong jhandle,
                                                jlong rl_handle) {
  auto* sptr_rate_limiter =
      reinterpret_cast<std::shared_ptr<rocksdb::RateLimiter> *>(rl_handle);
  auto* env_opt = reinterpret_cast<rocksdb::EnvOptions *>(jhandle);
  env_opt->rate_limiter = sptr_rate_limiter->get();
}
