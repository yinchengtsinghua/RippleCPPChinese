
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

#include <string>
#include <vector>

#include "rocksdb/comparator.h"
#include "rocksdb/db.h"

namespace rocksdb {

class Transaction;

//具有事务支持的数据库。
//
//参见optimatic_transaction.h和examples/transaction_example.cc

//启动乐观事务时要使用的选项
struct OptimisticTransactionOptions {
//设置set_snapshot=true与调用setSnapshot（）相同。
  bool set_snapshot = false;

//如果数据库有一个非默认的比较器，则应该设置。
//请参见writebatchwithindex构造函数中的注释。
  const Comparator* cmp = BytewiseComparator();
};

class OptimisticTransactionDB {
 public:
//打开类似于db:：open（）的optimaticTransactionDB。
  static Status Open(const Options& options, const std::string& dbname,
                     OptimisticTransactionDB** dbptr);

  static Status Open(const DBOptions& db_options, const std::string& dbname,
                     const std::vector<ColumnFamilyDescriptor>& column_families,
                     std::vector<ColumnFamilyHandle*>* handles,
                     OptimisticTransactionDB** dbptr);

  virtual ~OptimisticTransactionDB() {}

//启动新事务。
//
//调用方负责删除返回的事务，如果没有
//需要更长的时间。
//
//如果旧的_Txn不为空，BeginTransaction将重用此事务
//处理而不是分配新的。这是要避免的优化
//重复创建事务时的额外分配。
  virtual Transaction* BeginTransaction(
      const WriteOptions& write_options,
      const OptimisticTransactionOptions& txn_options =
          OptimisticTransactionOptions(),
      Transaction* old_txn = nullptr) = 0;

//返回已打开的基础数据库
  virtual DB* GetBaseDB() = 0;

 protected:
//要创建optimaticTransactionDB，请调用open（）。
  explicit OptimisticTransactionDB(DB* db) {}
  OptimisticTransactionDB() {}

 private:
//不允许复制
  OptimisticTransactionDB(const OptimisticTransactionDB&);
  void operator=(const OptimisticTransactionDB&);
};

}  //命名空间rocksdb

#endif  //摇滚乐
