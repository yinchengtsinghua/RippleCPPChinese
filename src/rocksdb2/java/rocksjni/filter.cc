
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

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <string>

#include "include/org_rocksdb_Filter.h"
#include "include/org_rocksdb_BloomFilter.h"
#include "rocksjni/portal.h"
#include "rocksdb/filter_policy.h"

/*
 *类别：Org_RocksDB_Bloomfilter
 *方法：CreateBolumFilter
 *签字：（IZ）J
 **/

jlong Java_org_rocksdb_BloomFilter_createNewBloomFilter(
    JNIEnv* env, jclass jcls, jint bits_per_key,
    jboolean use_block_base_builder) {
  auto* sptr_filter =
      new std::shared_ptr<const rocksdb::FilterPolicy>(
          rocksdb::NewBloomFilterPolicy(bits_per_key, use_block_base_builder));
  return reinterpret_cast<jlong>(sptr_filter);
}

/*
 *类别：org_rocksdb_filter
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_Filter_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* handle =
      reinterpret_cast<std::shared_ptr<const rocksdb::FilterPolicy> *>(jhandle);
delete handle;  //删除std:：shared_ptr
}
