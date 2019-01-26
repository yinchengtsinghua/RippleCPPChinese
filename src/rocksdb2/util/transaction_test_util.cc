
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
#ifndef ROCKSDB_LITE

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "util/transaction_test_util.h"

#include <inttypes.h>
#include <string>
#include <thread>

#include "rocksdb/db.h"
#include "rocksdb/utilities/optimistic_transaction_db.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include "util/random.h"
#include "util/string_util.h"

namespace rocksdb {

RandomTransactionInserter::RandomTransactionInserter(
    Random64* rand, const WriteOptions& write_options,
    const ReadOptions& read_options, uint64_t num_keys, uint16_t num_sets)
    : rand_(rand),
      write_options_(write_options),
      read_options_(read_options),
      num_keys_(num_keys),
      num_sets_(num_sets),
      txn_id_(0) {}

RandomTransactionInserter::~RandomTransactionInserter() {
  if (txn_ != nullptr) {
    delete txn_;
  }
  if (optimistic_txn_ != nullptr) {
    delete optimistic_txn_;
  }
}

bool RandomTransactionInserter::TransactionDBInsert(
    TransactionDB* db, const TransactionOptions& txn_options) {
  txn_ = db->BeginTransaction(write_options_, txn_options, txn_);

  return DoInsert(nullptr, txn_, false);
}

bool RandomTransactionInserter::OptimisticTransactionDBInsert(
    OptimisticTransactionDB* db,
    const OptimisticTransactionOptions& txn_options) {
  optimistic_txn_ =
      db->BeginTransaction(write_options_, txn_options, optimistic_txn_);

  return DoInsert(nullptr, optimistic_txn_, true);
}

bool RandomTransactionInserter::DBInsert(DB* db) {
  return DoInsert(db, nullptr, false);
}

bool RandomTransactionInserter::DoInsert(DB* db, Transaction* txn,
                                         bool is_optimistic) {
  Status s;
  WriteBatch batch;
  std::string value;

//选择一个随机数用于增加每组中的一个键
  uint64_t incr = (rand_->Next() % 100) + 1;

  bool unexpected_error = false;

//对于每一组，随机选择一个键并递增
  for (uint8_t i = 0; i < num_sets_; i++) {
    uint64_t int_value = 0;
    char prefix_buf[5];
//前缀“buf”必须足够大，以字符串形式保存uint16

//密钥格式：【设置】【随机】
    std::string rand_key = ToString(rand_->Next() % num_keys_);
    Slice base_key(rand_key);

//适当地填充前缀，这样我们可以在每个集合上迭代
    snprintf(prefix_buf, sizeof(prefix_buf), "%.4u", i + 1);
    std::string full_key = std::string(prefix_buf) + base_key.ToString();
    Slice key(full_key);

    if (txn != nullptr) {
      s = txn->GetForUpdate(read_options_, key, &value);
    } else {
      s = db->Get(read_options_, key, &value);
    }

    if (s.ok()) {
//找到键，分析其值
      int_value = std::stoull(value);

      if (int_value == 0 || int_value == ULONG_MAX) {
        unexpected_error = true;
        fprintf(stderr, "Get returned unexpected value: %s\n", value.c_str());
        s = Status::Corruption();
      }
    } else if (s.IsNotFound()) {
//尚未写入此密钥，因此假定其值为0
      int_value = 0;
      s = Status::OK();
    } else {
//乐观事务不应在此处返回非OK状态。
//非乐观事务可能会返回写联合/超时错误。
      if (is_optimistic || !(s.IsBusy() || s.IsTimedOut() || s.IsTryAgain())) {
        fprintf(stderr, "Get returned an unexpected error: %s\n",
                s.ToString().c_str());
        unexpected_error = true;
      }
      break;
    }

    if (s.ok()) {
//增量键
      std::string sum = ToString(int_value + incr);
      if (txn != nullptr) {
        s = txn->Put(key, sum);
        if (!s.ok()) {
//既然我们做了getforupdate，Put就不会失败。
          fprintf(stderr, "Put returned an unexpected error: %s\n",
                  s.ToString().c_str());
          unexpected_error = true;
        }
      } else {
        batch.Put(key, sum);
      }
    }
  }

  if (s.ok()) {
    if (txn != nullptr) {
      std::hash<std::thread::id> hasher;
      char name[64];
      snprintf(name, 64, "txn%zu-%d", hasher(std::this_thread::get_id()),
               txn_id_++);
      assert(strlen(name) < 64 - 1);
      txn->SetName(name);
      s = txn->Prepare();
      s = txn->Commit();

      if (!s.ok()) {
        if (is_optimistic) {
//乐观事务在提交时可能有写入冲突错误。
//任何其他错误都是意外的。
          if (!(s.IsBusy() || s.IsTimedOut() || s.IsTryAgain())) {
            unexpected_error = true;
          }
        } else {
//非乐观事务只应因到期而失败
//或者写失败。为了测试紫玫瑰，我们不期望
//写入失败。
          if (!s.IsExpired()) {
            unexpected_error = true;
          }
        }

        if (unexpected_error) {
          fprintf(stderr, "Commit returned an unexpected error: %s\n",
                  s.ToString().c_str());
        }
      }

    } else {
      s = db->Write(write_options_, &batch);
      if (!s.ok()) {
        unexpected_error = true;
        fprintf(stderr, "Write returned an unexpected error: %s\n",
                s.ToString().c_str());
      }
    }
  } else {
    if (txn != nullptr) {
      txn->Rollback();
    }
  }

  if (s.ok()) {
    success_count_++;
  } else {
    failure_count_++;
  }

  last_status_ = s;

//如果没有任何意外错误，则返回SUCCESS
  return !unexpected_error;
}

Status RandomTransactionInserter::Verify(DB* db, uint16_t num_sets) {
  uint64_t prev_total = 0;

//对于具有相同前缀的每组键，求和所有值
  for (uint32_t i = 0; i < num_sets; i++) {
    char prefix_buf[6];
    snprintf(prefix_buf, sizeof(prefix_buf), "%.4u", i + 1);
    uint64_t total = 0;

    Iterator* iter = db->NewIterator(ReadOptions());

    for (iter->Seek(Slice(prefix_buf, 4)); iter->Valid(); iter->Next()) {
      Slice key = iter->key();

//到达其他前缀时停止
      if (key.ToString().compare(0, 4, prefix_buf) != 0) {
        break;
      }

      Slice value = iter->value();
      uint64_t int_value = std::stoull(value.ToString());
      if (int_value == 0 || int_value == ULONG_MAX) {
        fprintf(stderr, "Iter returned unexpected value: %s\n",
                value.ToString().c_str());
        return Status::Corruption();
      }

      total += int_value;
    }
    delete iter;

    if (i > 0) {
      if (total != prev_total) {
        fprintf(stderr,
                "RandomTransactionVerify found inconsistent totals. "
                "Set[%" PRIu32 "]: %" PRIu64 ", Set[%" PRIu32 "]: %" PRIu64
                " \n",
                i - 1, prev_total, i, total);
        return Status::Corruption();
      }
    }
    prev_total = total;
  }

  return Status::OK();
}

}  //命名空间rocksdb

#endif  //摇滚乐
