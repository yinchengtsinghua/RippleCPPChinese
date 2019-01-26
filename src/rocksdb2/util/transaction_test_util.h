
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

#pragma once

#ifndef ROCKSDB_LITE

#include "rocksdb/options.h"
#include "port/port.h"
#include "rocksdb/utilities/optimistic_transaction_db.h"
#include "rocksdb/utilities/transaction_db.h"

namespace rocksdb {

class DB;
class Random64;

//用于压力测试事务的实用程序类。可以用来写很多
//并行事务，然后验证写入的数据是否符合逻辑
//一致的。此类假定输入数据库最初为空。
//
//对TransactionDBInsert（）/optimaticTransactionDBInsert（）的每个调用都将
//在num_组键中增加键的值。不管
//事务是否成功，每个键的值总和
//集合是应该保持相等的不变量。
//
//在调用TransactionDBInsert（）/optimaticTransactionDBInsert（）many之后
//可以调用times，verify（）来验证不变量是否保持不变。
//
//为了并行测试写入事务，多个线程可以创建一个
//使用相同数据库的具有类似参数的RandomTransactionInsert。
class RandomTransactionInserter {
 public:
//num_keys是每组中的键数。
//num_sets是键集的数目。
  explicit RandomTransactionInserter(
      Random64* rand, const WriteOptions& write_options = WriteOptions(),
      const ReadOptions& read_options = ReadOptions(), uint64_t num_keys = 1000,
      uint16_t num_sets = 3);

  ~RandomTransactionInserter();

//使用TransactionDB上的事务在每个集合中增加一个键。
//
//如果事务成功或遇到的任何错误为
//应为（如写冲突）。错误状态可以通过调用
//getLastStatus（）；
  bool TransactionDBInsert(
      TransactionDB* db,
      const TransactionOptions& txn_options = TransactionOptions());

//使用事务在
//优化事务数据库
//
//如果事务成功或遇到的任何错误为
//应为（如写冲突）。错误状态可以通过调用
//getLastStatus（）；
  bool OptimisticTransactionDBInsert(
      OptimisticTransactionDB* db,
      const OptimisticTransactionOptions& txn_options =
          OptimisticTransactionOptions());
//在每个集合中增加一个键，而不使用事务。如果这个函数
//并行调用，则verify（）可能失败。
//
//如果写入成功，则返回true。
//可以通过调用GetLastStatus（）获取错误状态。
  bool DBInsert(DB* db);

//如果不变量为真，则返回OK。
  static Status Verify(DB* db, uint16_t num_sets);

//返回上一个插入操作的状态
  Status GetLastStatus() { return last_status_; }

//返回成功写入的调用数
//事务dbinsert/optimaticTransactiondbinsert/dbinsert
  uint64_t GetSuccessCount() { return success_count_; }

//返回调用的次数
//transactiondinsert/optimatictransactiondinsert/dbinsert没有
//写入任何数据。
  uint64_t GetFailureCount() { return failure_count_; }

 private:
//输入选项
  Random64* rand_;
  const WriteOptions write_options_;
  const ReadOptions read_options_;
  const uint64_t num_keys_;
  const uint16_t num_sets_;

//成功执行的插入批数
  uint64_t success_count_ = 0;

//尝试插入批失败的次数
  uint64_t failure_count_ = 0;

//最近插入操作返回的状态
  Status last_status_;

//优化：重用分配的事务对象。
  Transaction* txn_ = nullptr;
  Transaction* optimistic_txn_ = nullptr;

  std::atomic<int> txn_id_;

  bool DoInsert(DB* db, Transaction* txn, bool is_optimistic);
};

}  //命名空间rocksdb

#endif  //摇滚乐
