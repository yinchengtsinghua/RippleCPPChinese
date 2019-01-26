
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
//该文件实现了RoSdSDB:：选项之间的Java和C++之间的“桥梁”。

#include <jni.h>
#include "include/org_rocksdb_PlainTableConfig.h"
#include "include/org_rocksdb_BlockBasedTableConfig.h"
#include "rocksdb/table.h"
#include "rocksdb/cache.h"
#include "rocksdb/filter_policy.h"

/*
 *类：org_rocksdb_plaintableconfig
 *方法：NewTableFactoryHandle
 *签字：（IIDIIBZZ）J
 **/

jlong Java_org_rocksdb_PlainTableConfig_newTableFactoryHandle(
    JNIEnv* env, jobject jobj, jint jkey_size, jint jbloom_bits_per_key,
    jdouble jhash_table_ratio, jint jindex_sparseness,
    jint jhuge_page_tlb_size, jbyte jencoding_type,
    jboolean jfull_scan_mode, jboolean jstore_index_in_file) {
  rocksdb::PlainTableOptions options = rocksdb::PlainTableOptions();
  options.user_key_len = jkey_size;
  options.bloom_bits_per_key = jbloom_bits_per_key;
  options.hash_table_ratio = jhash_table_ratio;
  options.index_sparseness = jindex_sparseness;
  options.huge_page_tlb_size = jhuge_page_tlb_size;
  options.encoding_type = static_cast<rocksdb::EncodingType>(
      jencoding_type);
  options.full_scan_mode = jfull_scan_mode;
  options.store_index_in_file = jstore_index_in_file;
  return reinterpret_cast<jlong>(rocksdb::NewPlainTableFactory(options));
}

/*
 *类：org_rocksdb_blockbasedtableconfig
 *方法：NewTableFactoryHandle
 *签名：（Zjijiizzzjibbi）J
 **/

jlong Java_org_rocksdb_BlockBasedTableConfig_newTableFactoryHandle(
    JNIEnv* env, jobject jobj, jboolean no_block_cache, jlong block_cache_size,
    jint block_cache_num_shardbits, jlong block_size, jint block_size_deviation,
    jint block_restart_interval, jboolean whole_key_filtering,
    jlong jfilterPolicy, jboolean cache_index_and_filter_blocks,
    jboolean pin_l0_filter_and_index_blocks_in_cache,
    jboolean hash_index_allow_collision, jlong block_cache_compressed_size,
    jint block_cache_compressd_num_shard_bits, jbyte jchecksum_type,
    jbyte jindex_type, jint jformat_version) {
  rocksdb::BlockBasedTableOptions options;
  options.no_block_cache = no_block_cache;

  if (!no_block_cache && block_cache_size > 0) {
    if (block_cache_num_shardbits > 0) {
      options.block_cache =
          rocksdb::NewLRUCache(block_cache_size, block_cache_num_shardbits);
    } else {
      options.block_cache = rocksdb::NewLRUCache(block_cache_size);
    }
  }
  options.block_size = block_size;
  options.block_size_deviation = block_size_deviation;
  options.block_restart_interval = block_restart_interval;
  options.whole_key_filtering = whole_key_filtering;
  if (jfilterPolicy > 0) {
    std::shared_ptr<rocksdb::FilterPolicy> *pFilterPolicy =
        reinterpret_cast<std::shared_ptr<rocksdb::FilterPolicy> *>(
            jfilterPolicy);
    options.filter_policy = *pFilterPolicy;
  }
  options.cache_index_and_filter_blocks = cache_index_and_filter_blocks;
  options.pin_l0_filter_and_index_blocks_in_cache =
      pin_l0_filter_and_index_blocks_in_cache;
  options.hash_index_allow_collision = hash_index_allow_collision;
  if (block_cache_compressed_size > 0) {
    if (block_cache_compressd_num_shard_bits > 0) {
      options.block_cache =
          rocksdb::NewLRUCache(block_cache_compressed_size,
              block_cache_compressd_num_shard_bits);
    } else {
      options.block_cache = rocksdb::NewLRUCache(block_cache_compressed_size);
    }
  }
  options.checksum = static_cast<rocksdb::ChecksumType>(jchecksum_type);
  options.index_type = static_cast<
      rocksdb::BlockBasedTableOptions::IndexType>(jindex_type);
  options.format_version = jformat_version;

  return reinterpret_cast<jlong>(rocksdb::NewBlockBasedTableFactory(options));
}
