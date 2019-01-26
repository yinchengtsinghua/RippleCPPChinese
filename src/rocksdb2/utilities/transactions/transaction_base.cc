
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

#include "utilities/transactions/transaction_base.h"

#include "db/db_impl.h"
#include "db/column_family.h"
#include "rocksdb/comparator.h"
#include "rocksdb/db.h"
#include "rocksdb/status.h"
#include "util/string_util.h"

namespace rocksdb {

TransactionBaseImpl::TransactionBaseImpl(DB* db,
                                         const WriteOptions& write_options)
    : db_(db),
      dbimpl_(reinterpret_cast<DBImpl*>(db)),
      write_options_(write_options),
      cmp_(GetColumnFamilyUserComparator(db->DefaultColumnFamily())),
      start_time_(db_->GetEnv()->NowMicros()),
      write_batch_(cmp_, 0, true, 0),
      indexing_enabled_(true) {
  assert(dynamic_cast<DBImpl*>(db_) != nullptr);
  log_number_ = 0;
  if (dbimpl_->allow_2pc()) {
    WriteBatchInternal::InsertNoop(write_batch_.GetWriteBatch());
  }
}

TransactionBaseImpl::~TransactionBaseImpl() {
//如果设置了快照，则释放快照
  SetSnapshotInternal(nullptr);
}

void TransactionBaseImpl::Clear() {
  save_points_.reset(nullptr);
  write_batch_.Clear();
  commit_time_batch_.Clear();
  tracked_keys_.clear();
  num_puts_ = 0;
  num_deletes_ = 0;
  num_merges_ = 0;

  if (dbimpl_->allow_2pc()) {
    WriteBatchInternal::InsertNoop(write_batch_.GetWriteBatch());
  }
}

void TransactionBaseImpl::Reinitialize(DB* db,
                                       const WriteOptions& write_options) {
  Clear();
  ClearSnapshot();
  db_ = db;
  name_.clear();
  log_number_ = 0;
  write_options_ = write_options;
  start_time_ = db_->GetEnv()->NowMicros();
  indexing_enabled_ = true;
  cmp_ = GetColumnFamilyUserComparator(db_->DefaultColumnFamily());
}

void TransactionBaseImpl::SetSnapshot() {
  const Snapshot* snapshot = dbimpl_->GetSnapshotForWriteConflictBoundary();
  SetSnapshotInternal(snapshot);
}

void TransactionBaseImpl::SetSnapshotInternal(const Snapshot* snapshot) {
//根据快照需要为快照sharedptr设置自定义删除程序
//发布，不再引用时不删除。
  snapshot_.reset(snapshot, std::bind(&TransactionBaseImpl::ReleaseSnapshot,
                                      this, std::placeholders::_1, db_));
  snapshot_needed_ = false;
  snapshot_notifier_ = nullptr;
}

void TransactionBaseImpl::SetSnapshotOnNextOperation(
    std::shared_ptr<TransactionNotifier> notifier) {
  snapshot_needed_ = true;
  snapshot_notifier_ = notifier;
}

void TransactionBaseImpl::SetSnapshotIfNeeded() {
  if (snapshot_needed_) {
    std::shared_ptr<TransactionNotifier> notifier = snapshot_notifier_;
    SetSnapshot();
    if (notifier != nullptr) {
      notifier->SnapshotCreated(GetSnapshot());
    }
  }
}

Status TransactionBaseImpl::TryLock(ColumnFamilyHandle* column_family,
                                    const SliceParts& key, bool read_only,
                                    bool exclusive, bool untracked) {
  size_t key_size = 0;
  for (int i = 0; i < key.num_parts; ++i) {
    key_size += key.parts[i].size();
  }

  std::string str;
  str.reserve(key_size);

  for (int i = 0; i < key.num_parts; ++i) {
    str.append(key.parts[i].data(), key.parts[i].size());
  }

  return TryLock(column_family, str, read_only, exclusive, untracked);
}

void TransactionBaseImpl::SetSavePoint() {
  if (save_points_ == nullptr) {
    save_points_.reset(new std::stack<TransactionBaseImpl::SavePoint>());
  }
  save_points_->emplace(snapshot_, snapshot_needed_, snapshot_notifier_,
                        num_puts_, num_deletes_, num_merges_);
  write_batch_.SetSavePoint();
}

Status TransactionBaseImpl::RollbackToSavePoint() {
  if (save_points_ != nullptr && save_points_->size() > 0) {
//还原保存的保存点
    TransactionBaseImpl::SavePoint& save_point = save_points_->top();
    snapshot_ = save_point.snapshot_;
    snapshot_needed_ = save_point.snapshot_needed_;
    snapshot_notifier_ = save_point.snapshot_notifier_;
    num_puts_ = save_point.num_puts_;
    num_deletes_ = save_point.num_deletes_;
    num_merges_ = save_point.num_merges_;

//回滚批处理
    Status s = write_batch_.RollbackToSavePoint();
    assert(s.ok());

//回滚自上次保存点以来跟踪的所有键
    const TransactionKeyMap& key_map = save_point.new_keys_;
    for (const auto& key_map_iter : key_map) {
      uint32_t column_family_id = key_map_iter.first;
      auto& keys = key_map_iter.second;

      auto& cf_tracked_keys = tracked_keys_[column_family_id];

      for (const auto& key_iter : keys) {
        const std::string& key = key_iter.first;
        uint32_t num_reads = key_iter.second.num_reads;
        uint32_t num_writes = key_iter.second.num_writes;

        auto tracked_keys_iter = cf_tracked_keys.find(key);
        assert(tracked_keys_iter != cf_tracked_keys.end());

//将此密钥的总读/写次数减少
//自上次保存点后完成的读取/写入操作。
        if (num_reads > 0) {
          assert(tracked_keys_iter->second.num_reads >= num_reads);
          tracked_keys_iter->second.num_reads -= num_reads;
        }
        if (num_writes > 0) {
          assert(tracked_keys_iter->second.num_writes >= num_writes);
          tracked_keys_iter->second.num_writes -= num_writes;
        }
        if (tracked_keys_iter->second.num_reads == 0 &&
            tracked_keys_iter->second.num_writes == 0) {
          tracked_keys_[column_family_id].erase(tracked_keys_iter);
        }
      }
    }

    save_points_->pop();

    return s;
  } else {
    assert(write_batch_.RollbackToSavePoint().IsNotFound());
    return Status::NotFound();
  }
}

Status TransactionBaseImpl::Get(const ReadOptions& read_options,
                                ColumnFamilyHandle* column_family,
                                const Slice& key, std::string* value) {
  assert(value != nullptr);
  PinnableSlice pinnable_val(value);
  assert(!pinnable_val.IsPinned());
  auto s = Get(read_options, column_family, key, &pinnable_val);
  if (s.ok() && pinnable_val.IsPinned()) {
    value->assign(pinnable_val.data(), pinnable_val.size());
}  //else值已分配
  return s;
}

Status TransactionBaseImpl::Get(const ReadOptions& read_options,
                                ColumnFamilyHandle* column_family,
                                const Slice& key, PinnableSlice* pinnable_val) {
  return write_batch_.GetFromBatchAndDB(db_, read_options, column_family, key,
                                        pinnable_val);
}

Status TransactionBaseImpl::GetForUpdate(const ReadOptions& read_options,
                                         ColumnFamilyHandle* column_family,
                                         const Slice& key, std::string* value,
                                         bool exclusive) {
  /*tus s s=trylock（列_family，key，true/*read_only*/，exclusive）；

  如果（s.ok（）&&value！= null pTr）{
    断言（价值！= null pTr）；
    pinnableslice pinnable val（值）；
    断言（！）pinnable_val.ispinned（））；
    S=GET（读取选项、列、系列、键和固定值）；
    if（s.ok（）&&pinnable_val.ispinned（））
      value->assign（pinnable_val.data（），pinnable_val.size（））；
    //其他值已分配
  }
  返回S；
}

状态TransactionBaseImpl:：GetForUpdate（const read options&read_options，
                                         columnFamilyhandle*column_系列，
                                         常量切片和键，
                                         可钉切片*可钉切片
                                         bool独占）
  状态S=Trylock（列_family，key，true/*只读*/, exclusive);


  if (s.ok() && pinnable_val != nullptr) {
    s = Get(read_options, column_family, key, pinnable_val);
  }
  return s;
}

std::vector<Status> TransactionBaseImpl::MultiGet(
    const ReadOptions& read_options,
    const std::vector<ColumnFamilyHandle*>& column_family,
    const std::vector<Slice>& keys, std::vector<std::string>* values) {
  size_t num_keys = keys.size();
  values->resize(num_keys);

  std::vector<Status> stat_list(num_keys);
  for (size_t i = 0; i < num_keys; ++i) {
    std::string* value = values ? &(*values)[i] : nullptr;
    stat_list[i] = Get(read_options, column_family[i], keys[i], value);
  }

  return stat_list;
}

std::vector<Status> TransactionBaseImpl::MultiGetForUpdate(
    const ReadOptions& read_options,
    const std::vector<ColumnFamilyHandle*>& column_family,
    const std::vector<Slice>& keys, std::vector<std::string>* values) {
//不管multiget是否成功，跟踪这些键。
  size_t num_keys = keys.size();
  values->resize(num_keys);

//锁定所有键
  for (size_t i = 0; i < num_keys; ++i) {
    /*tus s s=trylock（列_family[i]，键[i]，真/*只读*/，
                       真/*独家*/);

    if (!s.ok()) {
//如果我们不能锁定所有键，则会使整个multiget失败。
      return std::vector<Status>(num_keys, s);
    }
  }

//托多：优化multiget？
  std::vector<Status> stat_list(num_keys);
  for (size_t i = 0; i < num_keys; ++i) {
    std::string* value = values ? &(*values)[i] : nullptr;
    stat_list[i] = Get(read_options, column_family[i], keys[i], value);
  }

  return stat_list;
}

Iterator* TransactionBaseImpl::GetIterator(const ReadOptions& read_options) {
  Iterator* db_iter = db_->NewIterator(read_options);
  assert(db_iter);

  return write_batch_.NewIteratorWithBase(db_iter);
}

Iterator* TransactionBaseImpl::GetIterator(const ReadOptions& read_options,
                                           ColumnFamilyHandle* column_family) {
  Iterator* db_iter = db_->NewIterator(read_options, column_family);
  assert(db_iter);

  return write_batch_.NewIteratorWithBase(column_family, db_iter);
}

Status TransactionBaseImpl::Put(ColumnFamilyHandle* column_family,
                                const Slice& key, const Slice& value) {
  Status s =
      /*锁（列_family，key，false/*只读*/，true/*独占*/）；

  如果（S.O.（））{
    s=getbatchforwrite（）->put（column_family，key，value）；
    如果（S.O.（））{
      NuffysPs++；
    }
  }

  返回S；
}

状态TransactionBaseImpl:：Put（ColumnFamilyHandle*Column_Family，
                                常量切片部件和密钥，
                                const sliceparts&value）
  状态S＝
      Trylock（列_family，key，false/*只读*/, true /* exclusive */);


  if (s.ok()) {
    s = GetBatchForWrite()->Put(column_family, key, value);
    if (s.ok()) {
      num_puts_++;
    }
  }

  return s;
}

Status TransactionBaseImpl::Merge(ColumnFamilyHandle* column_family,
                                  const Slice& key, const Slice& value) {
  Status s =
      /*锁（列_family，key，false/*只读*/，true/*独占*/）；

  如果（S.O.（））{
    s=getbatchforwrite（）->合并（column_family，key，value）；
    如果（S.O.（））{
      NuMyMixeSe++；
    }
  }

  返回S；
}

状态TransactionBaseImpl:：Delete（ColumnFamilyHandle*Column_Family，
                                   常量切片和键）
  状态S＝
      Trylock（列_family，key，false/*只读*/, true /* exclusive */);


  if (s.ok()) {
    s = GetBatchForWrite()->Delete(column_family, key);
    if (s.ok()) {
      num_deletes_++;
    }
  }

  return s;
}

Status TransactionBaseImpl::Delete(ColumnFamilyHandle* column_family,
                                   const SliceParts& key) {
  Status s =
      /*锁（列_family，key，false/*只读*/，true/*独占*/）；

  如果（S.O.（））{
    s=getbatchforwrite（）->删除（column_family，key）；
    如果（S.O.（））{
      NUMLDELETESI+；
    }
  }

  返回S；
}

状态TransactionBaseImpl:：SingleDelete（ColumnFamilyHandle*Column_Family，
                                         常量切片和键）
  状态S＝
      Trylock（列_family，key，false/*只读*/, true /* exclusive */);


  if (s.ok()) {
    s = GetBatchForWrite()->SingleDelete(column_family, key);
    if (s.ok()) {
      num_deletes_++;
    }
  }

  return s;
}

Status TransactionBaseImpl::SingleDelete(ColumnFamilyHandle* column_family,
                                         const SliceParts& key) {
  Status s =
      /*锁（列_family，key，false/*只读*/，true/*独占*/）；

  如果（S.O.（））{
    s=getbatchforwrite（）->singledelete（column_family，key）；
    如果（S.O.（））{
      NUMLDELETESI+；
    }
  }

  返回S；
}

状态TransactionBaseImpl:：Putuntracked（ColumnFamilyHandle*Column_Family，
                                         const slice&key，const slice&value）
  状态S=Trylock（列_family，key，false/*只读*/,

                     /*e/*独占*/，真/*未跟踪*/）；

  如果（S.O.（））{
    s=getbatchforwrite（）->put（column_family，key，value）；
    如果（S.O.（））{
      NuffysPs++；
    }
  }

  返回S；
}

状态TransactionBaseImpl:：Putuntracked（ColumnFamilyHandle*Column_Family，
                                         常量切片部件和密钥，
                                         const sliceparts&value）
  状态S=Trylock（列_family，key，false/*只读*/,

                     /*e/*独占*/，真/*未跟踪*/）；

  如果（S.O.（））{
    s=getbatchforwrite（）->put（column_family，key，value）；
    如果（S.O.（））{
      NuffysPs++；
    }
  }

  返回S；
}

状态TransactionBaseImpl:：MergeUntracked（ColumnFamilyHandle*Column_Family，
                                           常量切片和键，
                                           常量切片和值）
  状态S=Trylock（列_family，key，false/*只读*/,

                     /*e/*独占*/，真/*未跟踪*/）；

  如果（S.O.（））{
    s=getbatchforwrite（）->合并（column_family，key，value）；
    如果（S.O.（））{
      NuMyMixeSe++；
    }
  }

  返回S；
}

状态TransactionBaseImpl:：DeleteUntracked（ColumnFamilyHandle*Column_Family，
                                            常量切片和键）
  状态S=Trylock（列_family，key，false/*只读*/,

                     /*e/*独占*/，真/*未跟踪*/）；

  如果（S.O.（））{
    s=getbatchforwrite（）->删除（column_family，key）；
    如果（S.O.（））{
      NUMLDELETESI+；
    }
  }

  返回S；
}

状态TransactionBaseImpl:：DeleteUntracked（ColumnFamilyHandle*Column_Family，
                                            const sliceparts&key）
  状态S=Trylock（列_family，key，false/*只读*/,

                     /*e/*独占*/，真/*未跟踪*/）；

  如果（S.O.（））{
    s=getbatchforwrite（）->删除（column_family，key）；
    如果（S.O.（））{
      NUMLDELETESI+；
    }
  }

  返回S；
}

void transactionbaseimpl:：putlogdata（const slice&blob）
  写入“批处理”putlogdata（blob）；
}

WriteBatchWithIndex*TransactionBaseImpl:：GetWriteBatch（）
  返回并写入“批处理”
}

uint64_t TransactionBaseImpl:：GetElapsedTime（）常量
  返回（db_->getenv（）->nowmicros（）-开始时间_uu）/1000；
}

uint64_t TransactionBaseImpl:：GetNumPuts（）常量返回num_puts_

uint64_t TransactionBaseImpl:：GetNumDeletes（）常量返回num_删除

uint64_t TransactionBaseImpl:：GetNumMerges（）常量返回Num_Merges_

uint64_t TransactionBaseImpl:：GetNumKeys（）常量
  uint64计数=0；

  //汇总所有列族中的锁定键
  for（const auto&key_map_iter:跟踪的_keys_
    const auto&keys=key_map_iter.second；
    count+=keys.size（）；
  }

  返回计数；
}

void transactionbaseimpl:：trackkey（uint32_t cfh_id，const std:：string&key，
                                   SequenceNumber Seq，Bool只读，
                                   bool独占）
  //更新此事务的所有跟踪键的映射
  trackkey（&tracked_keys_uuu，cfh_id，key，seq，read_only，exclusive）；

  如果（保存点）= null pTr& &！保存_Points_->Empty（））
    //更新此保存点中跟踪键的映射
    trackkey（&save_Points_u->top（）。new_Keys_uu，cfh_id，key，seq，read_only，
             独家）；
  }
}

//向给定的TransactionKeyMap添加键
void transactionbaseimpl:：trackkey（transactionkeymap*key_map，uint32_t cfh_id，
                                   const std：：字符串和键，sequencenumber seq，
                                   bool只读，bool独占）
  自动&cf_key_map=（*key_map）[cfh_id]；
  auto iter=cf_key_map.find（key）；
  if（iter==cf_key_map.end（））
    自动结果=cf_key_map.insert（key，transactionkeymapinfo（seq））；
    iter=结果.first；
  else if（seq<iter->second.seq）
    //现在用之前的序列号跟踪这个键
    iter->second.seq=seq；
  }

  如果（只读）
    iter->second.num_reads++；
  }否则{
    iter->second.num_writes++；
  }
  iter->second.exclusive=exclusive；
}

std:：unique_ptr<transactionkeymap>
TransactionBaseImpl:：GetTrackKeyssinceSavePoint（）
  如果（保存点）= null pTr& &！保存_Points_->Empty（））
    //检查对所有写入的密钥执行的读/写次数
    //从上一个保存点开始，并与读取/写入的总数进行比较
    //对于每个键。
    transactionkeymap*result=new transactionkeymap（）；
    for（const auto&key_map_iter:save_points_u->top（）.new_keys_
      uint32_t column_family_id=key_map_iter.first；
      auto&keys=key_map_iter.second；

      auto&cf_tracked_keys=tracked_keys_uux[column_family_id]；

      对于（const auto&keyiter:keys）
        const std：：string&key=key\iter.first；
        uint32_t num_reads=key_iter.second.num_reads；
        uint32_t num_writes=key_iter.second.num_writes；

        auto total_key_info=cf_tracked_keys.find（key）；
        断言（总钥匙信息！=cf_tracked_keys.end（））；
        断言（total_key_info->second.num_reads>=num_reads）；
        断言（total_key_info->second.num_writes-=num_writes）；

        if（total_key_info->second.num_reads==num_reads&&
            _key_info->second.num_writes==num_writes）
          //对该键的所有读/写操作都是在上一个保存点中完成的。
          bool read_only=（num_writes==0）；
          trackkey（result，column_family_id，key，key_iter.second.seq，
                   只读，key-iter.second.exclusive）；
        }
      }
    }
    返回std:：unique_ptr<transactionkeymap>（result）；
  }

  /没有保存点
  返回null pTR；
}

//获取应用于放置/合并/删除的写入批处理。
/ /
//返回writebatch或writebatchwithindex，具体取决于
//已调用DisableIndexing（）。
WriteBatchBase*TransactionBaseImpl:：GetBatchForWrite（）
  如果（索引启用）
    //使用writebatchwithindex
    返回并写入“批处理”
  }否则{
    //不要使用writebatchwithindex。返回基writebatch。
    返回write_batch_.getwritebatch（）；
  }
}

void transactionBaseImpl:：releaseSnapshot（const snapshot*snapshot，db*db）
  如果（快照）！= null pTr）{
    db->releasesnapshot（快照）；
  }
}

void TransactionBaseImpl:：UndogetForUpdate（ColumnFamilyHandle*Column_Family，
                                           常量切片和键）
  uint32_t column_family_id=getColumnFamilyID（column_family）；
  auto&cf_tracked_keys=tracked_keys_uux[column_family_id]；
  std:：string key_str=key.toString（）；
  bool can_decrement=false；
  bool canou unlock uuu attribute_uuuu（（unused））=false；

  如果（保存点）= null pTr& &！保存_Points_->Empty（））
    //检查是否为此保存点中的更新提取了此密钥
    auto&cf_savepoint_keys=save_points_u->top（）。新的_keys_u[column_family_id]；

    auto savepoint？iter=cf？savepoint？keys.find（key？str）；
    如果（保存点文件！=cf_savepoint_keys.end（））
      if（保存点iter->second.num_reads>0）
        保存点“iter”-->second.num“读取”--；
        可以减量=真；

        if（保存点iter->second.num_reads==0&&
            savepoint-iter->second.num_writes==0）
          //此保存点中没有其他GetForUpdates或对此密钥进行写操作
          保存点擦除（保存点）；
          可以解锁=真；
        }
      }
    }
  }否则{
    //未设置保存点
    可以减量=真；
    可以解锁=真；
  }

  //只有当我们能够
  //减少当前保存点中的读取计数，或者如果没有
  //保存点集。
  如果（可以减少）
    auto key_iter=cf_tracked_keys.find（key_str）；

    如果（钥匙）！=cf_tracked_keys.end（））
      if（key-iter->second.num_reads>0）
        键“iter->second.num”读取---

        if（key-iter->second.num_reads==0&&
            键“iter”-->second.num“writes==0）”；
          //此密钥上没有其他GetForUpdates或写入操作
          断言（可以解锁）；
          cf_tracked_keys.erase（按键擦除）；
          UnlockGetForUpdate（列\族，键）；
        }
      }
    }
  }
}

状态TransactionBaseImpl:：RebuildFromWriteBatch（WriteBatch*SRC_batch）
  结构indexedWriteBatchBuilder:public writeBatch:：handler_
    交易*txn_uu；
    DbIMPL*dBi；
    indexedWriteBatchBuilder（事务*txn，dbimpl*db）
        ：txn_uu（txn），db_u（db）
      断言（dynamic_cast<transactionbaseimpl*>（txn_u）！= null pTr）；
    }

    status putcf（uint32_t cf，const slice&key，const slice&val）override_
      返回txn_u->put（db_->getColumnFamilyhandle（cf），key，val）；
    }

    状态删除cf（uint32_t cf，const slice&key）覆盖
      返回txn_u->delete（db_->getColumnFamilyhandle（cf），key）；
    }

    status singledeletecf（uint32_t cf，const slice&key）override_
      返回txn_u->singledelete（db_->getColumnFamilyhandle（cf），key）；
    }

    状态合并cf（uint32_t cf，const slice&key，const slice&val）覆盖
      返回txn_u->merge（db_->getColumnFamilyhandle（cf），key，val）；
    }

    //用于在
    /恢复。批中不应该有任何元标记
    //我们正在处理。
    状态markBeginPrepare（）重写返回状态：：InvalidArgument（）；

    状态标记EndPrepare（const slice&）覆盖
      返回状态：：invalidArgument（）；
    }

    状态标记提交（const slice&）重写
      返回状态：：invalidArgument（）；
    }

    状态标记回滚（const slice&）重写
      返回状态：：invalidArgument（）；
    }
  }；

  indexedWriteBatchBuilder copycat（this，dbimpl_u）；
  返回src_batch->iterate（&copycat）；
}

WriteBatch*TransactionBaseImpl:：getcommitTimeWriteBatch（）
  返回并提交“时间”批处理
}
//命名空间rocksdb

endif//rocksdb_lite
