
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
//调用C++ ROCKSDB:：从Java端测试WreeBooT方法。
#include <memory>

#include "db/memtable.h"
#include "db/write_batch_internal.h"
#include "include/org_rocksdb_WriteBatch.h"
#include "include/org_rocksdb_WriteBatchTest.h"
#include "include/org_rocksdb_WriteBatchTestInternalHelper.h"
#include "include/org_rocksdb_WriteBatch_Handler.h"
#include "options/cf_options.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/status.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/write_buffer_manager.h"
#include "rocksjni/portal.h"
#include "table/scoped_arena_iterator.h"
#include "util/string_util.h"
#include "util/testharness.h"

/*
 *类：org_rocksdb_writebatchtest
 *方法：getContents
 *签字：（j）[b
 **/

jbyteArray Java_org_rocksdb_WriteBatchTest_getContents(
    JNIEnv* env, jclass jclazz, jlong jwb_handle) {
  auto* b = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(b != nullptr);

//TODO:当前以下代码直接从
//db/write_bench_test.cc.它可以在Java中实现一次
//所有必要的组件都可以通过JNIAPI访问。

  rocksdb::InternalKeyComparator cmp(rocksdb::BytewiseComparator());
  auto factory = std::make_shared<rocksdb::SkipListFactory>();
  rocksdb::Options options;
  rocksdb::WriteBufferManager wb(options.db_write_buffer_size);
  options.memtable_factory = factory;
  rocksdb::MemTable* mem = new rocksdb::MemTable(
      cmp, rocksdb::ImmutableCFOptions(options),
      rocksdb::MutableCFOptions(options), &wb, rocksdb::kMaxSequenceNumber,
      /**列_family_id*/）；
  MEM＞Rf（）；
  std：：字符串状态；
  rocksdb:：columnFamilyMemTablesDefault cf_mems_default（mem）；
  RocksDB：：状态S=
      rocksdb:：writebatchinternal:：insertinto（b，&cf_mems_default，nullptr）；
  int计数＝0；
  竞技场；
  rocksdb:：scopedarenaiterator iter（mem->newiterator（
      rocksdb:：readoptions（），&arena））；
  for（iter->seektofirst（）；iter->valid（）；iter->next（））
    rocksdb:：parsedinternalkey-ikey；
    IKI.清除（）；
    bool parsed=rocksdb:：parseInternalKey（iter->key（），&ikey）；
    如果（！）解析）{
      断言（解析）；
    }
    开关（ikey.type）
      case rocksdb:：k类型值：
        state.append（“放置”）；
        state.append（ikey.user_key.toString（））；
        state.append（“，”）；
        state.append（iter->value（）.toString（））；
        state.append（“）”）；
        计数+；
        断裂；
      大小写RocksDB:：ktypeMerge:
        state.append（“合并”）；
        state.append（ikey.user_key.toString（））；
        state.append（“，”）；
        state.append（iter->value（）.toString（））；
        state.append（“）”）；
        计数+；
        断裂；
      case rocksdb:：k类型删除：
        state.append（“删除”）；
        state.append（ikey.user_key.toString（））；
        state.append（“）”）；
        计数+；
        断裂；
      违约：
        断言（假）；
        断裂；
    }
    state.append（“@”）；
    state.append（rocksdb:：numbertoString（ikey.sequence））；
  }
  如果（！）S.O.（））{
    state.append（s.toString（））；
  否则，如果（计数！=rocksdb:：writebatchinternal:：count（b））
    state.append（“countMismatch（）”）；
  }
  删除mem->unref（）；

  jbytearray jstate=env->newbytearray（static_cast<jsize>（state.size（））；
  if（jState==nullptr）
    //引发异常：OutOfMemoryError
    返回null pTR；
  }

  env->setBytearrayRegion（jState，0，static-cast<jSize>（state.size（）），
                          const_cast<jbyte*>（reinterpret_cast<const jbyte*>（state.c_str（））；
  if（env->exceptioncheck（））
    //引发异常：arrayindexoutofboundsException
    env->deleteLocalRef（jState）；
    返回null pTR；
  }

  返回JSTATE；
}

/*
 *类：org_rocksdb_writebatchestInternalHelper
 *方法：设置顺序
 *签字：（JJ）V
 **/

void Java_org_rocksdb_WriteBatchTestInternalHelper_setSequence(
    JNIEnv* env, jclass jclazz, jlong jwb_handle, jlong jsn) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);

  rocksdb::WriteBatchInternal::SetSequence(
      wb, static_cast<rocksdb::SequenceNumber>(jsn));
}

/*
 *类：org_rocksdb_writebatchestInternalHelper
 *方法：顺序
 *签字：（j）j
 **/

jlong Java_org_rocksdb_WriteBatchTestInternalHelper_sequence(
    JNIEnv* env, jclass jclazz, jlong jwb_handle) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);

  return static_cast<jlong>(rocksdb::WriteBatchInternal::Sequence(wb));
}

/*
 *类：org_rocksdb_writebatchestInternalHelper
 *方法：追加
 *签字：（JJ）V
 **/

void Java_org_rocksdb_WriteBatchTestInternalHelper_append(
    JNIEnv* env, jclass jclazz, jlong jwb_handle_1, jlong jwb_handle_2) {
  auto* wb1 = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle_1);
  assert(wb1 != nullptr);
  auto* wb2 = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle_2);
  assert(wb2 != nullptr);

  rocksdb::WriteBatchInternal::Append(wb1, wb2);
}
