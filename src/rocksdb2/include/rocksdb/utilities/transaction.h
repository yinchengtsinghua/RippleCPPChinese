
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
#include "rocksdb/status.h"

namespace rocksdb {

class Iterator;
class TransactionDB;
class WriteBatchWithIndex;

using TransactionName = std::string;

using TransactionID = uint64_t;

//在以下情况下向调用方提供setSnapshotNextOperation的通知：
//创建实际快照
class TransactionNotifier {
 public:
  virtual ~TransactionNotifier() {}

//实现此方法以在快照为
//通过SetSnapshotNextOperation请求。
  virtual void SnapshotCreated(const Snapshot* newSnapshot) = 0;
};

//提供begin/commit/rollback事务。
//
//要使用事务，必须首先创建optimaticTransactionDB
//或事务数据库。有关
//更多信息。
//
//要创建事务，请使用[optimatic]TransactionDB:：BeginTransaction（）。
//
//由调用者来同步对此对象的访问。
//
//请参阅examples/transaction\u example.cc了解一些简单的示例。
//
//TODO（Agiardullo）：尚未实现
//-PerfContext统计信息
//-支持使用带有dbwithttl的事务
class Transaction {
 public:
  virtual ~Transaction() {}

//如果事务具有快照集，则该事务将确保
//任何成功写入（或通过getForUpdate（）获取）的密钥都没有
//自快照
//集合。
//如果尚未设置快照，则事务将保证键
//自首次写入（或通过
//getForUpdate（））。
//
//使用setSnapshot（）将在
//由于与
//其他书写。
//
//调用setsnapshot（）对在此函数之前写入的键没有影响
//已被调用。
//
//如果要更改，可以多次调用setSnapshot（）。
//用于此事务中不同操作的快照。
//
//调用setSnapshot不会影响get（）返回的数据版本。
//方法。有关详细信息，请参阅Transaction:：get（）。
  virtual void SetSnapshot() = 0;

//类似于setSnapshot（），但不会更改当前快照
//直到调用了Put/Merge/Delete/GetForUpdate/MultiGetForUpdate。
//通过调用此函数，事务将基本上调用
//在执行下一个write/getforupdate之前，为您设置快照（）。
//
//调用setSnapshotNextOperation（）不会影响快照的内容
//由getSnapshot（）返回，直到执行下一个write/getForUpdate。
//
//创建快照时，通知程序的SnapshotCreated方法将
//调用以便调用方可以访问快照。
//
//这是一个优化，以减少冲突的可能性
//可能发生在调用setSnapshot（）和第一个
//写入/getforupdate操作。这样可以防止以下情况
//竞争条件：
//
//txn1->setSnapshot（）；
//txn2->put（“a”，…）；
//TXN2->提交（）；
//txn1->getforupdate（opts，“a”，…）；//失败！
  virtual void SetSnapshotOnNextOperation(
      std::shared_ptr<TransactionNotifier> notifier = nullptr) = 0;

//返回上次调用setSnapshot（）创建的快照。
//
//必需：返回的快照仅在下次之前有效
//调用setsnapshot（）/setsnapshottonxtsavepoint（），clearsnapshot（）。
//调用或删除事务。
  virtual const Snapshot* GetSnapshot() const = 0;

//清除当前快照（即不会“设置”任何快照）
//
//这将删除当前存在或设置为要创建的任何快照
//在下一个更新操作（setSnapshotNextOperation）上。
//
//调用ClearSnapshot（）对在此函数之前写入的键没有影响
//已被调用。
//
//如果通过getSnapshot（）检索到对快照的引用，则不会
//更长的是有效的，在调用ClearSnapshot（）后应丢弃。
  virtual void ClearSnapshot() = 0;

//为2PC准备当前传输
  virtual Status Prepare() = 0;

//将所有批处理的密钥以原子方式写入数据库。
//
//成功后返回OK。
//
//可以返回db:write（）可能返回的任何错误状态。
//
//如果此事务是由optimaticTransactionDB（）创建的，
//如果事务不能保证，则可能返回status:：busy（）。
//没有写冲突。状态：：TryaGain（）可以返回
//如果memtable历史记录大小不够大
//（请参阅要维护的最大\写入\缓冲区\编号）。
//
//如果此事务是由TransactionDB（）创建的，则状态：：Expired（）。
//如果此事务的生存时间超过
//交易选项。到期。
  virtual Status Commit() = 0;

//放弃此事务中的所有批写入。
  virtual Status Rollback() = 0;

//记录事务的状态，以便将来调用
//RollbackToSavePoint（）。可以多次调用以设置多个保存
//点。
  virtual void SetSavePoint() = 0;

//撤消此事务中的所有操作（Put、Merge、Delete、PutLogData）
//因为最近调用了setsavepoint（）并删除了最新的
//StaveSavePoT（）。
//如果以前没有调用setsavepoint（），则返回status:：notfound（）。
  virtual Status RollbackToSavePoint() = 0;

//此函数与db:：get（）类似，但它也将读取挂起
//此交易记录中的更改。当前，此函数将返回
//状态：：mergeinprogress如果最近写入查询的密钥
//此批处理是合并。
//
//如果未设置read_options.snapshot，则密钥的当前版本将
//阅读。调用setSnapshot（）不会影响数据的版本
//返回。
//
//请注意，设置read_options.snapshot将影响从
//但不会更改从该事务中读取的密钥（密钥
//在此事务中，还不属于任何快照，将提取
//无论如何）。
  virtual Status Get(const ReadOptions& options,
                     ColumnFamilyHandle* column_family, const Slice& key,
                     std::string* value) = 0;

//接收pinnableslice的上述方法的重载
//对于向后兼容，提供了默认实现
  virtual Status Get(const ReadOptions& options,
                     ColumnFamilyHandle* column_family, const Slice& key,
                     PinnableSlice* pinnable_val) {
    assert(pinnable_val != nullptr);
    auto s = Get(options, column_family, key, pinnable_val->GetSelf());
    pinnable_val->PinSelf();
    return s;
  }

  virtual Status Get(const ReadOptions& options, const Slice& key,
                     std::string* value) = 0;
  virtual Status Get(const ReadOptions& options, const Slice& key,
                     PinnableSlice* pinnable_val) {
    assert(pinnable_val != nullptr);
    auto s = Get(options, key, pinnable_val->GetSelf());
    pinnable_val->PinSelf();
    return s;
  }

  virtual std::vector<Status> MultiGet(
      const ReadOptions& options,
      const std::vector<ColumnFamilyHandle*>& column_family,
      const std::vector<Slice>& keys, std::vector<std::string>* values) = 0;

  virtual std::vector<Status> MultiGet(const ReadOptions& options,
                                       const std::vector<Slice>& keys,
                                       std::vector<std::string>* values) = 0;

//读取此密钥并确保此事务仅
//如果此密钥未写入此外部，则可以提交
//第一次读取后的事务（如果是
//快照在此事务中设置）。事务行为是
//不管密钥是否存在，都是一样的。
//
//注意：当前，此函数将返回状态：：mergeinprogress
//如果对该批中查询的密钥的最新写入是合并。
//
//此函数返回的值与transaction:：get（）类似。
//如果value==nullptr，则此函数不会读取任何数据，但会
//仍然确保此密钥不能由外部写入
//交易。
//
//如果此事务是由optimaticTransaction创建的，则getForUpdate（）。
//可能导致commit（）失败。否则，它可能返回任何错误
//可以由db:：get（）返回。
//
//如果此事务是由TransactionDB创建的，它可以返回
//成功时的状态：：OK（），
//status:：busy（）如果存在写入冲突，
//status:：timedout（）如果无法获取锁，
//如果memtable历史记录大小不够大，则返回status:：tryagain（）。
//（见最大写入缓冲区编号）
//如果无法解析合并操作，则状态：：mergeinprogress（）。
//或其他无法读取此密钥的错误。
  virtual Status GetForUpdate(const ReadOptions& options,
                              ColumnFamilyHandle* column_family,
                              const Slice& key, std::string* value,
                              bool exclusive = true) = 0;

//接收pinnableslice的上述方法的重载
//对于向后兼容，提供了默认实现
  virtual Status GetForUpdate(const ReadOptions& options,
                              ColumnFamilyHandle* column_family,
                              const Slice& key, PinnableSlice* pinnable_val,
                              bool exclusive = true) {
    if (pinnable_val == nullptr) {
      std::string* null_str = nullptr;
      return GetForUpdate(options, key, null_str);
    } else {
      auto s = GetForUpdate(options, key, pinnable_val->GetSelf());
      pinnable_val->PinSelf();
      return s;
    }
  }

  virtual Status GetForUpdate(const ReadOptions& options, const Slice& key,
                              std::string* value, bool exclusive = true) = 0;

  virtual std::vector<Status> MultiGetForUpdate(
      const ReadOptions& options,
      const std::vector<ColumnFamilyHandle*>& column_family,
      const std::vector<Slice>& keys, std::vector<std::string>* values) = 0;

  virtual std::vector<Status> MultiGetForUpdate(
      const ReadOptions& options, const std::vector<Slice>& keys,
      std::vector<std::string>* values) = 0;

//返回一个迭代器，该迭代器将在默认情况下对所有键进行迭代
//列族，包括数据库中的键和此中的未提交键
//交易。
//
//设置read_options.snapshot将影响从
//但不会更改从该事务中读取的密钥（密钥
//在此事务中，还不属于任何快照，将提取
//无论如何）。
//
//调用方负责删除返回的迭代器。
//
//返回的迭代器仅在commit（）、rollback（）或之前有效
//调用了RollbackToSavePoint（）。
  virtual Iterator* GetIterator(const ReadOptions& read_options) = 0;

  virtual Iterator* GetIterator(const ReadOptions& read_options,
                                ColumnFamilyHandle* column_family) = 0;

//Put、Merge、Delete和SingleDelete的行为与相应的
//函数，但也将对
//正在写入密钥。
//
//如果此事务是在optimaticTransactionDB上创建的，则
//函数应始终返回状态：：OK（）。
//
//如果此事务是在TransactionDB上创建的，则返回状态
//可以是：
//成功时的状态：：OK（），
//status:：busy（）如果存在写入冲突，
//status:：timedout（）如果无法获取锁，
//如果memtable历史记录大小不够大，则返回status:：tryagain（）。
//（见最大写入缓冲区编号）
//或其他意外失败的错误。
  virtual Status Put(ColumnFamilyHandle* column_family, const Slice& key,
                     const Slice& value) = 0;
  virtual Status Put(const Slice& key, const Slice& value) = 0;
  virtual Status Put(ColumnFamilyHandle* column_family, const SliceParts& key,
                     const SliceParts& value) = 0;
  virtual Status Put(const SliceParts& key, const SliceParts& value) = 0;

  virtual Status Merge(ColumnFamilyHandle* column_family, const Slice& key,
                       const Slice& value) = 0;
  virtual Status Merge(const Slice& key, const Slice& value) = 0;

  virtual Status Delete(ColumnFamilyHandle* column_family,
                        const Slice& key) = 0;
  virtual Status Delete(const Slice& key) = 0;
  virtual Status Delete(ColumnFamilyHandle* column_family,
                        const SliceParts& key) = 0;
  virtual Status Delete(const SliceParts& key) = 0;

  virtual Status SingleDelete(ColumnFamilyHandle* column_family,
                              const Slice& key) = 0;
  virtual Status SingleDelete(const Slice& key) = 0;
  virtual Status SingleDelete(ColumnFamilyHandle* column_family,
                              const SliceParts& key) = 0;
  virtual Status SingleDelete(const SliceParts& key) = 0;

//putunttracked（）将写入要提交的一批操作的Put
//在此交易中。只有当此事务
//成功提交。但与Transaction:：Put（）不同，
//不会对此密钥进行冲突检查。
//
//如果此事务是在TransactionDB上创建的，则此函数将
//仍然获取必要的锁，以确保此写入不会导致
//其他事务中有冲突，可能返回status:：busy（）。
  virtual Status PutUntracked(ColumnFamilyHandle* column_family,
                              const Slice& key, const Slice& value) = 0;
  virtual Status PutUntracked(const Slice& key, const Slice& value) = 0;
  virtual Status PutUntracked(ColumnFamilyHandle* column_family,
                              const SliceParts& key,
                              const SliceParts& value) = 0;
  virtual Status PutUntracked(const SliceParts& key,
                              const SliceParts& value) = 0;

  virtual Status MergeUntracked(ColumnFamilyHandle* column_family,
                                const Slice& key, const Slice& value) = 0;
  virtual Status MergeUntracked(const Slice& key, const Slice& value) = 0;

  virtual Status DeleteUntracked(ColumnFamilyHandle* column_family,
                                 const Slice& key) = 0;

  virtual Status DeleteUntracked(const Slice& key) = 0;
  virtual Status DeleteUntracked(ColumnFamilyHandle* column_family,
                                 const SliceParts& key) = 0;
  virtual Status DeleteUntracked(const SliceParts& key) = 0;

//类似于writebatch:：putlogdata
  virtual void PutLogData(const Slice& blob) = 0;

//默认情况下，所有放置/合并/删除操作都将在
//使get/getforupdate/get迭代器可以搜索这些
//钥匙。
//
//如果调用方不想获取即将写入的密钥，
//他们可能希望避免将索引作为性能优化。
//调用disableindexing（）将关闭所有将来的索引
//执行Put/Merge/Delete操作，直到调用EnableIndexing（）。
//
//如果在调用DisableIndexing之后放置/合并/删除了某个键，则
//通过get/getforupdate/get迭代器获取，获取的结果是
//未定义。
  virtual void DisableIndexing() = 0;
  virtual void EnableIndexing() = 0;

//返回此事务正在跟踪的非重复键的数目。
//如果此事务是由TransactionIDB创建的，则这是
//当前被此事务锁定的密钥。
//如果此事务是由optimaticTransactionDB创建的，则这是
//提交时需要检查冲突的键数。
  virtual uint64_t GetNumKeys() const = 0;

//返回已应用于此的放置/删除/合并数
//迄今为止的交易。
  virtual uint64_t GetNumPuts() const = 0;
  virtual uint64_t GetNumDeletes() const = 0;
  virtual uint64_t GetNumMerges() const = 0;

//返回自该事务开始以来经过的时间（毫秒）。
  virtual uint64_t GetElapsedTime() const = 0;

//获取包含所有待处理更改的基础写入批
//坚信的。
//
//注意：您不应该直接从批处理中写入或删除任何内容，并且
//只应使用事务类中的函数
//写入此事务。
  virtual WriteBatchWithIndex* GetWriteBatch() = 0;

//更改的TransactionOptions.Lock_超时值（毫秒）
//本次交易。
//对optimatictransactions没有影响。
  virtual void SetLockTimeout(int64_t timeout) = 0;

//返回将在提交（）期间使用的WriteOptions。
  virtual WriteOptions* GetWriteOptions() = 0;

//重置将在提交（）期间使用的WriteOptions。
  virtual void SetWriteOptions(const WriteOptions& write_options) = 0;

//如果以前在此事务中使用
//GetForUpdate/MultiGetForUpdate（），调用UndoGetForUpdate将告诉您
//它不再需要执行任何冲突检查的事务
//这个钥匙。
//
//如果通过getForUpdate/multyGetForUpdate（）获取了n次密钥，
//只有当它也被称为n时，UndogetForUpdate才会有效果。
//时代。如果此密钥已写入此事务中，
//UndoGetForUpdate（）将不起作用。
//
//如果在getForUpdate（）之后调用了setSavePoint（），
//UndoGetForUpdate（）将不起任何作用。
//
//如果此事务是由optimaticTransactionDB创建的，
//调用UndogetForUpdate会影响是否选中此键冲突
//在提交的时间。
//如果此事务是由TransactionDB创建的，
//调用UndogetForUpdate可能会释放此密钥的所有保留锁。
  virtual void UndoGetForUpdate(ColumnFamilyHandle* column_family,
                                const Slice& key) = 0;
  virtual void UndoGetForUpdate(const Slice& key) = 0;

  virtual Status RebuildFromWriteBatch(WriteBatch* src_batch) = 0;

  virtual WriteBatch* GetCommitTimeWriteBatch() = 0;

  virtual void SetLogNumber(uint64_t log) { log_number_ = log; }

  virtual uint64_t GetLogNumber() const { return log_number_; }

  virtual Status SetName(const TransactionName& name) = 0;

  virtual TransactionName GetName() const { return name_; }

  virtual TransactionID GetID() const { return 0; }

  virtual bool IsDeadlockDetect() const { return false; }

  virtual std::vector<TransactionID> GetWaitingTxns(uint32_t* column_family_id,
                                                    std::string* key) const {
    assert(false);
    return std::vector<TransactionID>();
  }

  enum TransactionState {
    STARTED = 0,
    AWAITING_PREPARE = 1,
    PREPARED = 2,
    AWAITING_COMMIT = 3,
    COMMITED = 4,
    AWAITING_ROLLBACK = 5,
    ROLLEDBACK = 6,
    LOCKS_STOLEN = 7,
  };

  TransactionState GetState() const { return txn_state_; }
  void SetState(TransactionState state) { txn_state_ = state; }

 protected:
  explicit Transaction(const TransactionDB* db) {}
  Transaction() {}

//此txn的准备部分所在的日志
//（用于两阶段提交）
  uint64_t log_number_;
  TransactionName name_;

//事务的执行状态。
  std::atomic<TransactionState> txn_state_;

 private:
//不允许复制
  Transaction(const Transaction&);
  void operator=(const Transaction&);
};

}  //命名空间rocksdb

#endif  //摇滚乐
