
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

#include <stack>
#include <string>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/snapshot.h"
#include "rocksdb/status.h"
#include "rocksdb/types.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include "rocksdb/utilities/write_batch_with_index.h"
#include "utilities/transactions/transaction_util.h"

namespace rocksdb {

class TransactionBaseImpl : public Transaction {
 public:
  TransactionBaseImpl(DB* db, const WriteOptions& write_options);

  virtual ~TransactionBaseImpl();

//删除在此事务中排队的挂起操作。
  virtual void Clear();

  void Reinitialize(DB* db, const WriteOptions& write_options);

//在执行Put、Merge、Delete和GetForUpdate之前调用。如果TryLock
//返回non OK，则Put/Merge/Delete/GetForUpdate将失败。
//如果从PutuntRacked、DeleteUntracked或
//被跟踪的
  virtual Status TryLock(ColumnFamilyHandle* column_family, const Slice& key,
                         bool read_only, bool exclusive,
                         bool untracked = false) = 0;

  void SetSavePoint() override;

  Status RollbackToSavePoint() override;

  using Transaction::Get;
  Status Get(const ReadOptions& options, ColumnFamilyHandle* column_family,
             const Slice& key, std::string* value) override;

  Status Get(const ReadOptions& options, ColumnFamilyHandle* column_family,
             const Slice& key, PinnableSlice* value) override;

  Status Get(const ReadOptions& options, const Slice& key,
             std::string* value) override {
    return Get(options, db_->DefaultColumnFamily(), key, value);
  }

  using Transaction::GetForUpdate;
  Status GetForUpdate(const ReadOptions& options,
                      ColumnFamilyHandle* column_family, const Slice& key,
                      std::string* value, bool exclusive) override;

  Status GetForUpdate(const ReadOptions& options,
                      ColumnFamilyHandle* column_family, const Slice& key,
                      PinnableSlice* pinnable_val, bool exclusive) override;

  Status GetForUpdate(const ReadOptions& options, const Slice& key,
                      std::string* value, bool exclusive) override {
    return GetForUpdate(options, db_->DefaultColumnFamily(), key, value,
                        exclusive);
  }

  std::vector<Status> MultiGet(
      const ReadOptions& options,
      const std::vector<ColumnFamilyHandle*>& column_family,
      const std::vector<Slice>& keys,
      std::vector<std::string>* values) override;

  std::vector<Status> MultiGet(const ReadOptions& options,
                               const std::vector<Slice>& keys,
                               std::vector<std::string>* values) override {
    return MultiGet(options, std::vector<ColumnFamilyHandle*>(
                                 keys.size(), db_->DefaultColumnFamily()),
                    keys, values);
  }

  std::vector<Status> MultiGetForUpdate(
      const ReadOptions& options,
      const std::vector<ColumnFamilyHandle*>& column_family,
      const std::vector<Slice>& keys,
      std::vector<std::string>* values) override;

  std::vector<Status> MultiGetForUpdate(
      const ReadOptions& options, const std::vector<Slice>& keys,
      std::vector<std::string>* values) override {
    return MultiGetForUpdate(options,
                             std::vector<ColumnFamilyHandle*>(
                                 keys.size(), db_->DefaultColumnFamily()),
                             keys, values);
  }

  Iterator* GetIterator(const ReadOptions& read_options) override;
  Iterator* GetIterator(const ReadOptions& read_options,
                        ColumnFamilyHandle* column_family) override;

  Status Put(ColumnFamilyHandle* column_family, const Slice& key,
             const Slice& value) override;
  Status Put(const Slice& key, const Slice& value) override {
    return Put(nullptr, key, value);
  }

  Status Put(ColumnFamilyHandle* column_family, const SliceParts& key,
             const SliceParts& value) override;
  Status Put(const SliceParts& key, const SliceParts& value) override {
    return Put(nullptr, key, value);
  }

  Status Merge(ColumnFamilyHandle* column_family, const Slice& key,
               const Slice& value) override;
  Status Merge(const Slice& key, const Slice& value) override {
    return Merge(nullptr, key, value);
  }

  Status Delete(ColumnFamilyHandle* column_family, const Slice& key) override;
  Status Delete(const Slice& key) override { return Delete(nullptr, key); }
  Status Delete(ColumnFamilyHandle* column_family,
                const SliceParts& key) override;
  Status Delete(const SliceParts& key) override { return Delete(nullptr, key); }

  Status SingleDelete(ColumnFamilyHandle* column_family,
                      const Slice& key) override;
  Status SingleDelete(const Slice& key) override {
    return SingleDelete(nullptr, key);
  }
  Status SingleDelete(ColumnFamilyHandle* column_family,
                      const SliceParts& key) override;
  Status SingleDelete(const SliceParts& key) override {
    return SingleDelete(nullptr, key);
  }

  Status PutUntracked(ColumnFamilyHandle* column_family, const Slice& key,
                      const Slice& value) override;
  Status PutUntracked(const Slice& key, const Slice& value) override {
    return PutUntracked(nullptr, key, value);
  }

  Status PutUntracked(ColumnFamilyHandle* column_family, const SliceParts& key,
                      const SliceParts& value) override;
  Status PutUntracked(const SliceParts& key, const SliceParts& value) override {
    return PutUntracked(nullptr, key, value);
  }

  Status MergeUntracked(ColumnFamilyHandle* column_family, const Slice& key,
                        const Slice& value) override;
  Status MergeUntracked(const Slice& key, const Slice& value) override {
    return MergeUntracked(nullptr, key, value);
  }

  Status DeleteUntracked(ColumnFamilyHandle* column_family,
                         const Slice& key) override;
  Status DeleteUntracked(const Slice& key) override {
    return DeleteUntracked(nullptr, key);
  }
  Status DeleteUntracked(ColumnFamilyHandle* column_family,
                         const SliceParts& key) override;
  Status DeleteUntracked(const SliceParts& key) override {
    return DeleteUntracked(nullptr, key);
  }

  void PutLogData(const Slice& blob) override;

  WriteBatchWithIndex* GetWriteBatch() override;

  /*tual void setlocktimeout（int64_t timeout）override/*什么都不做*/
  }

  const snapshot*getsnapshot（）const override_
    返回快照？快照获取（）：nullptr；
  }

  void setSnapshot（）重写；
  void setSnapshotNextOperation（
      std:：shared&ptr<transactionnotifier>notifier=nullptr）override；

  void clearsnapshot（）重写
    快照重置（）；
    Snapshot_needed_uu=false；
    快照通知程序nullptr；
  }

  void disableindexing（）覆盖索引_启用=false；

  void enableIndexing（）重写索引_启用=true；

  uint64_t getElapsedTime（）常量重写；

  uint64_t getnumpts（）常量重写；

  uint64_t getNumDeletes（）常量重写；

  uint64_t getnummerges（）常量重写；

  uint64_t getNumKeys（）常量重写；

  void undogetforupdate（columnfamilyhandle*column_family，
                        const slice&key）覆盖；
  void undogetforupdate（const slice&key）override_
    返回UndogetForUpdate（nullptr，key）；
  }；

  //获取此事务中不能有任何冲突的键列表
  //在其他事务中写入。
  const transactionkeymap&gettrackedkeys（）const返回跟踪的_keys_

  writeOptions*getWriteOptions（）重写返回并写入选项

  void setwriteoptions（const write options&write_options）override_
    写入选项=写入选项；
  }

  //用于快照的内存管理
  void releasesnapshot（const snapshot*快照，db*db）；

  //遍历给定的批，并进行适当的插入。
  //用于恢复后重建准备好的事务。
  状态rebuildfromwritebatch（writebatch*src_batch）override；

  writebatch*getcommitTimeWritebatch（）重写；

 受保护的：
  //向跟踪键列表中添加键。
  / /
  //seqno是最早的seqno此密钥与此事务有关。
  //如果没有为此键写入数据，则应将readonly设置为true
  void trackkey（uint32_t cfh_id，const std:：string&key，sequencenumber seqno，
                bool readonly，bool exclusive）；

  //向给定TransactionKeyMap添加键的helper函数
  静态void trackkey（transactionkeymap*key_map，uint32_t cfh_id，
                       const std：：字符串和键，序列号seqno，
                       bool readonly，bool exclusive）；

  //当UndoGetForUpdate确定此密钥可以解锁时调用。
  虚拟void unlocketforupdate（columnfamilyhandle*column_family，
                                  const slice&key）=0；

  std:：unique_ptr<transactionkeymap>gettrackdkeyssincesavepoint（）；

  //如果调用了setSnapshoTontNextOperation（），则设置快照。
  void setSnapshotifRequired（）；

  dB＊dBi；
  dbimpl*dbimpl_u；

  写选项写选项

  常量比较器

  //以微秒为单位存储构造txn的时间。
  uint64_t开始时间

  //存储由setSnapshot设置的当前快照，如果
  //当前未设置快照。
  std:：shared_ptr<const snapshot>snapshot_uuu；

  //此事务中挂起的各种操作的计数
  uint64_t num_puts_uu0；
  uint64_t num_删除_uu0；
  uint64_t num_merges_uu0；

  结构保存点
    std:：shared_ptr<const snapshot>snapshot_uuu；
    bool快照
    std:：shared_ptr<transactionnotifier>snapshot_notifier；
    uint64_t num_投入；
    uint64_t num_删除\；
    uint64_t num_合并；

    //记录自上次保存点以来跟踪的所有键
    交易密钥映射新密钥

    保存点（std:：shared_ptr<const snapshot>snapshot，需要bool snapshot_，
              std:：shared_ptr<transactionnotifier>snapshot_notifier，
              uint64_t num_放置，uint64_t num_删除，uint64_t num_合并）
        ：快照（快照），
          需要快照（需要快照）
          快照通知程序（快照通知程序）
          num_puts_uuu（num_puts）、
          num_删除（num_删除），
          num_合并_uu（num_合并）
  }；

  //记录在此事务中写入挂起
  writebatchwithindex write_批次；

 私人：
  //提交时要写入的批处理
  写入批处理提交时间批处理

  //在每个保存点保存的快照堆栈。保存的快照可以是
  //如果在调用setsavepoint（）时没有快照，则为nullptr。
  std:：unique_ptr<std:：stack<transactionbaseimpl:：savepoint>>保存点

  //从列_family_id映射到与此相关的键的映射
  / /事务。
  //悲观事务将在添加键之前执行冲突检查
  //通过调用trackkey（）。
  //乐观事务将等待提交时间进行冲突检查。
  事务键映射跟踪的键；

  //如果为true，则将来的Put/Merge/Deletes将在
  //writebatchwithindex。
  //如果为false，则将来的Put/Merge/Deletes将直接插入到
  //基础writebatch，但未在writebatchwithindex中编入索引。
  布尔索引启用

  //已经调用了setSnapshotNextOperation（），但快照尚未调用
  //已复位。
  bool snapshot_needed_uu=false；

  //已经调用了setSnapshotNextOperation（），调用方希望
  //通过TransactionNotifier接口发出的通知
  std:：shared_ptr<transactionnotifier>snapshot_notifier_u=nullptr；

  状态Trylock（columnFamilyhandle*column_family，const sliceparts&key，
                 bool read_only，bool exclusive，bool untracked=false）；

  writeBatchBase*getBatchForWrite（）；

  void setsnapshotinternal（const snapshot*快照）；
}；

//命名空间rocksdb

endif//rocksdb_lite
