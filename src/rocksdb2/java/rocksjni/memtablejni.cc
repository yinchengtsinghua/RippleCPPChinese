
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
//该文件实现了用于表的Java和C++之间的“桥梁”。

#include "rocksjni/portal.h"
#include "include/org_rocksdb_HashSkipListMemTableConfig.h"
#include "include/org_rocksdb_HashLinkedListMemTableConfig.h"
#include "include/org_rocksdb_VectorMemTableConfig.h"
#include "include/org_rocksdb_SkipListMemTableConfig.h"
#include "rocksdb/memtablerep.h"

/*
 *类：org_rocksdb_hashskiplistmemtableconfig
 *方法：NewMemTableFactoryHandle
 *签字：（JII）J
 **/

jlong Java_org_rocksdb_HashSkipListMemTableConfig_newMemTableFactoryHandle(
    JNIEnv* env, jobject jobj, jlong jbucket_count,
    jint jheight, jint jbranching_factor) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(jbucket_count);
  if (s.ok()) {
    return reinterpret_cast<jlong>(rocksdb::NewHashSkipListRepFactory(
        static_cast<size_t>(jbucket_count),
        static_cast<int32_t>(jheight),
        static_cast<int32_t>(jbranching_factor)));
  }
  rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  return 0;
}

/*
 *类：org_rocksdb_hashlinkedlistmemtableconfig
 *方法：NewMemTableFactoryHandle
 *签字：（Jjizi）J
 **/

jlong Java_org_rocksdb_HashLinkedListMemTableConfig_newMemTableFactoryHandle(
    JNIEnv* env, jobject jobj, jlong jbucket_count, jlong jhuge_page_tlb_size,
    jint jbucket_entries_logging_threshold,
    jboolean jif_log_bucket_dist_when_flash, jint jthreshold_use_skiplist) {
  rocksdb::Status statusBucketCount =
      rocksdb::check_if_jlong_fits_size_t(jbucket_count);
  rocksdb::Status statusHugePageTlb =
      rocksdb::check_if_jlong_fits_size_t(jhuge_page_tlb_size);
  if (statusBucketCount.ok() && statusHugePageTlb.ok()) {
    return reinterpret_cast<jlong>(rocksdb::NewHashLinkListRepFactory(
        static_cast<size_t>(jbucket_count),
        static_cast<size_t>(jhuge_page_tlb_size),
        static_cast<int32_t>(jbucket_entries_logging_threshold),
        static_cast<bool>(jif_log_bucket_dist_when_flash),
        static_cast<int32_t>(jthreshold_use_skiplist)));
  }
  rocksdb::IllegalArgumentExceptionJni::ThrowNew(env,
      !statusBucketCount.ok()?statusBucketCount:statusHugePageTlb);
  return 0;
}

/*
 *类别：org_rocksdb_vectromtableconfig
 *方法：NewMemTableFactoryHandle
 *签字：（j）j
 **/

jlong Java_org_rocksdb_VectorMemTableConfig_newMemTableFactoryHandle(
    JNIEnv* env, jobject jobj, jlong jreserved_size) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(jreserved_size);
  if (s.ok()) {
    return reinterpret_cast<jlong>(new rocksdb::VectorRepFactory(
        static_cast<size_t>(jreserved_size)));
  }
  rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  return 0;
}

/*
 *类：org_rocksdb_skiplistmemtableconfig
 *方法：NewMemTableFactoryHandle0
 *签字：（j）j
 **/

jlong Java_org_rocksdb_SkipListMemTableConfig_newMemTableFactoryHandle0(
    JNIEnv* env, jobject jobj, jlong jlookahead) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(jlookahead);
  if (s.ok()) {
    return reinterpret_cast<jlong>(new rocksdb::SkipListFactory(
        static_cast<size_t>(jlookahead)));
  }
  rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  return 0;
}
