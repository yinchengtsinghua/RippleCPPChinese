
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

#include "rocksdb/utilities/write_batch_with_index.h"

#include <limits>
#include <memory>

#include "db/column_family.h"
#include "db/db_impl.h"
#include "db/merge_context.h"
#include "db/merge_helper.h"
#include "memtable/skiplist.h"
#include "options/db_options.h"
#include "rocksdb/comparator.h"
#include "rocksdb/iterator.h"
#include "util/arena.h"
#include "utilities/write_batch_with_index/write_batch_with_index_internal.h"

namespace rocksdb {

//当方向=前进时
//*当前_Base_u<=>Base_迭代器>Delta_迭代器
//当方向==向后时
//*当前_Base_u<=>Base_Iterator<Delta_Iterator
//总是：
//*等号键<=>基_迭代器==delta _迭代器
class BaseDeltaIterator : public Iterator {
 public:
  BaseDeltaIterator(Iterator* base_iterator, WBWIIterator* delta_iterator,
                    const Comparator* comparator)
      : forward_(true),
        current_at_base_(true),
        equal_keys_(false),
        status_(Status::OK()),
        base_iterator_(base_iterator),
        delta_iterator_(delta_iterator),
        comparator_(comparator) {}

  virtual ~BaseDeltaIterator() {}

  bool Valid() const override {
    return current_at_base_ ? BaseValid() : DeltaValid();
  }

  void SeekToFirst() override {
    forward_ = true;
    base_iterator_->SeekToFirst();
    delta_iterator_->SeekToFirst();
    UpdateCurrent();
  }

  void SeekToLast() override {
    forward_ = false;
    base_iterator_->SeekToLast();
    delta_iterator_->SeekToLast();
    UpdateCurrent();
  }

  void Seek(const Slice& k) override {
    forward_ = true;
    base_iterator_->Seek(k);
    delta_iterator_->Seek(k);
    UpdateCurrent();
  }

  void SeekForPrev(const Slice& k) override {
    forward_ = false;
    base_iterator_->SeekForPrev(k);
    delta_iterator_->SeekForPrev(k);
    UpdateCurrent();
  }

  void Next() override {
    if (!Valid()) {
      status_ = Status::NotSupported("Next() on invalid iterator");
    }

    if (!forward_) {
//需要改变方向
//如果我们的方向是向后的，而我们不相等，我们有两种状态：
//*两个迭代器都有效：我们已经处于良好状态（当前
//缩小显示）
//*只有一个迭代器有效：我们需要推进该迭代器
      forward_ = true;
      equal_keys_ = false;
      if (!BaseValid()) {
        assert(DeltaValid());
        base_iterator_->SeekToFirst();
      } else if (!DeltaValid()) {
        delta_iterator_->SeekToFirst();
      } else if (current_at_base_) {
//将delta从大于base更改为小于
        AdvanceDelta();
      } else {
//将基数从大于delta更改为小于
        AdvanceBase();
      }
      if (DeltaValid() && BaseValid()) {
        if (comparator_->Equal(delta_iterator_->Entry().key,
                               base_iterator_->key())) {
          equal_keys_ = true;
        }
      }
    }
    Advance();
  }

  void Prev() override {
    if (!Valid()) {
      status_ = Status::NotSupported("Prev() on invalid iterator");
    }

    if (forward_) {
//需要改变方向
//如果我们的方向是向后的，而我们不相等，我们有两种状态：
//*两个迭代器都有效：我们已经处于良好状态（当前
//缩小显示）
//*只有一个迭代器有效：我们需要推进该迭代器
      forward_ = false;
      equal_keys_ = false;
      if (!BaseValid()) {
        assert(DeltaValid());
        base_iterator_->SeekToLast();
      } else if (!DeltaValid()) {
        delta_iterator_->SeekToLast();
      } else if (current_at_base_) {
//将delta从“低于基础”更改为“高于基础”
        AdvanceDelta();
      } else {
//将基础从低于delta的高级更改为高于delta的高级
        AdvanceBase();
      }
      if (DeltaValid() && BaseValid()) {
        if (comparator_->Equal(delta_iterator_->Entry().key,
                               base_iterator_->key())) {
          equal_keys_ = true;
        }
      }
    }

    Advance();
  }

  Slice key() const override {
    return current_at_base_ ? base_iterator_->key()
                            : delta_iterator_->Entry().key;
  }

  Slice value() const override {
    return current_at_base_ ? base_iterator_->value()
                            : delta_iterator_->Entry().value;
  }

  Status status() const override {
    if (!status_.ok()) {
      return status_;
    }
    if (!base_iterator_->status().ok()) {
      return base_iterator_->status();
    }
    return delta_iterator_->status();
  }

 private:
  void AssertInvariants() {
#ifndef NDEBUG
    if (!Valid()) {
      return;
    }
    if (!BaseValid()) {
      assert(!current_at_base_ && delta_iterator_->Valid());
      return;
    }
    if (!DeltaValid()) {
      assert(current_at_base_ && base_iterator_->Valid());
      return;
    }
//我们还不支持这些
    assert(delta_iterator_->Entry().type != kMergeRecord &&
           delta_iterator_->Entry().type != kLogDataRecord);
    int compare = comparator_->Compare(delta_iterator_->Entry().key,
                                       base_iterator_->key());
    if (forward_) {
//电流_at_Base->Compare<0
      assert(!current_at_base_ || compare < 0);
//！电流_at_Base->Compare<=0
      assert(current_at_base_ && compare >= 0);
    } else {
//电流_at_Base->Compare>0
      assert(!current_at_base_ || compare > 0);
//！电流_at_Base->Compare<=0
      assert(current_at_base_ && compare <= 0);
    }
//等号键<=>比较==0
    assert((equal_keys_ || compare != 0) && (!equal_keys_ || compare == 0));
#endif
  }

  void Advance() {
    if (equal_keys_) {
      assert(BaseValid() && DeltaValid());
      AdvanceBase();
      AdvanceDelta();
    } else {
      if (current_at_base_) {
        assert(BaseValid());
        AdvanceBase();
      } else {
        assert(DeltaValid());
        AdvanceDelta();
      }
    }
    UpdateCurrent();
  }

  void AdvanceDelta() {
    if (forward_) {
      delta_iterator_->Next();
    } else {
      delta_iterator_->Prev();
    }
  }
  void AdvanceBase() {
    if (forward_) {
      base_iterator_->Next();
    } else {
      base_iterator_->Prev();
    }
  }
  bool BaseValid() const { return base_iterator_->Valid(); }
  bool DeltaValid() const { return delta_iterator_->Valid(); }
  void UpdateCurrent() {
//抑制假阳性clang分析器警告。
#ifndef __clang_analyzer__
    while (true) {
      WriteEntry delta_entry;
      if (DeltaValid()) {
        delta_entry = delta_iterator_->Entry();
      }
      equal_keys_ = false;
      if (!BaseValid()) {
//底座已完成。
        if (!DeltaValid()) {
//完成了
          return;
        }
        if (delta_entry.type == kDeleteRecord ||
            delta_entry.type == kSingleDeleteRecord) {
          AdvanceDelta();
        } else {
          current_at_base_ = false;
          return;
        }
      } else if (!DeltaValid()) {
//Delta已完成。
        current_at_base_ = true;
        return;
      } else {
        int compare =
            (forward_ ? 1 : -1) *
            comparator_->Compare(delta_entry.key, base_iterator_->key());
if (compare <= 0) {  //大于或等于delta
          if (compare == 0) {
            equal_keys_ = true;
          }
          if (delta_entry.type != kDeleteRecord &&
              delta_entry.type != kSingleDeleteRecord) {
            current_at_base_ = false;
            return;
          }
//Delta不太高级，将被删除。
          AdvanceDelta();
          if (equal_keys_) {
            AdvanceBase();
          }
        } else {
          current_at_base_ = true;
          return;
        }
      }
    }

    AssertInvariants();
#endif  //_uuu-clang_分析仪
  }

  bool forward_;
  bool current_at_base_;
  bool equal_keys_;
  Status status_;
  std::unique_ptr<Iterator> base_iterator_;
  std::unique_ptr<WBWIIterator> delta_iterator_;
const Comparator* comparator_;  //不拥有
};

typedef SkipList<WriteBatchIndexEntry*, const WriteBatchEntryComparator&>
    WriteBatchEntrySkipList;

class WBWIIteratorImpl : public WBWIIterator {
 public:
  WBWIIteratorImpl(uint32_t column_family_id,
                   WriteBatchEntrySkipList* skip_list,
                   const ReadableWriteBatch* write_batch)
      : column_family_id_(column_family_id),
        skip_list_iter_(skip_list),
        write_batch_(write_batch) {}

  virtual ~WBWIIteratorImpl() {}

  virtual bool Valid() const override {
    if (!skip_list_iter_.Valid()) {
      return false;
    }
    const WriteBatchIndexEntry* iter_entry = skip_list_iter_.key();
    return (iter_entry != nullptr &&
            iter_entry->column_family == column_family_id_);
  }

  virtual void SeekToFirst() override {
    WriteBatchIndexEntry search_entry(WriteBatchIndexEntry::kFlagMin,
                                      column_family_id_, 0, 0);
    skip_list_iter_.Seek(&search_entry);
  }

  virtual void SeekToLast() override {
    WriteBatchIndexEntry search_entry(WriteBatchIndexEntry::kFlagMin,
                                      column_family_id_ + 1, 0, 0);
    skip_list_iter_.Seek(&search_entry);
    if (!skip_list_iter_.Valid()) {
      skip_list_iter_.SeekToLast();
    } else {
      skip_list_iter_.Prev();
    }
  }

  virtual void Seek(const Slice& key) override {
    WriteBatchIndexEntry search_entry(&key, column_family_id_);
    skip_list_iter_.Seek(&search_entry);
  }

  virtual void SeekForPrev(const Slice& key) override {
    WriteBatchIndexEntry search_entry(&key, column_family_id_);
    skip_list_iter_.SeekForPrev(&search_entry);
  }

  virtual void Next() override { skip_list_iter_.Next(); }

  virtual void Prev() override { skip_list_iter_.Prev(); }

  virtual WriteEntry Entry() const override {
    WriteEntry ret;
    Slice blob, xid;
    const WriteBatchIndexEntry* iter_entry = skip_list_iter_.key();
//这是用有效的（）来保证的。
    assert(iter_entry != nullptr &&
           iter_entry->column_family == column_family_id_);
    auto s = write_batch_->GetEntryFromDataOffset(
        iter_entry->offset, &ret.type, &ret.key, &ret.value, &blob, &xid);
    assert(s.ok());
    assert(ret.type == kPutRecord || ret.type == kDeleteRecord ||
           ret.type == kSingleDeleteRecord || ret.type == kDeleteRangeRecord ||
           ret.type == kMergeRecord);
    return ret;
  }

  virtual Status status() const override {
//这是内存中的数据结构，因此状态可以为非OK的唯一方法是
//通过内存损坏
    return Status::OK();
  }

  const WriteBatchIndexEntry* GetRawEntry() const {
    return skip_list_iter_.key();
  }

 private:
  uint32_t column_family_id_;
  WriteBatchEntrySkipList::Iterator skip_list_iter_;
  const ReadableWriteBatch* write_batch_;
};

struct WriteBatchWithIndex::Rep {
  explicit Rep(const Comparator* index_comparator, size_t reserved_bytes = 0,
               size_t max_bytes = 0, bool _overwrite_key = false)
      : write_batch(reserved_bytes, max_bytes),
        comparator(index_comparator, &write_batch),
        skip_list(comparator, &arena),
        overwrite_key(_overwrite_key),
        last_entry_offset(0) {}
  ReadableWriteBatch write_batch;
  WriteBatchEntryComparator comparator;
  Arena arena;
  WriteBatchEntrySkipList skip_list;
  bool overwrite_key;
  size_t last_entry_offset;

//记住内部写入批处理的当前偏移量，该偏移量用作
//下一条记录的起始偏移量。
  void SetLastEntryOffset() { last_entry_offset = write_batch.GetDataSize(); }

//在覆盖模式下，查找同一密钥的现有条目并更新它
//指向当前条目。
//如果找到并更新了密钥，则返回true。
  bool UpdateExistingEntry(ColumnFamilyHandle* column_family, const Slice& key);
  bool UpdateExistingEntryWithCfId(uint32_t column_family_id, const Slice& key);

//将最近的条目添加到更新中。
//在覆盖模式下，如果索引中已经存在键，请更新它。
  void AddOrUpdateIndex(ColumnFamilyHandle* column_family, const Slice& key);
  void AddOrUpdateIndex(const Slice& key);

//分配指向写入批处理中最后一个条目的索引条目，并
//把它放到跳过列表中。
  void AddNewEntry(uint32_t column_family_id);

//清除此批处理中缓冲的所有更新。
  void Clear();
  void ClearIndex();

//通过读取批处理中的所有记录来重新生成索引。
//返回损坏时的非正常状态。
  Status ReBuildIndex();
};

bool WriteBatchWithIndex::Rep::UpdateExistingEntry(
    ColumnFamilyHandle* column_family, const Slice& key) {
  uint32_t cf_id = GetColumnFamilyID(column_family);
  return UpdateExistingEntryWithCfId(cf_id, key);
}

bool WriteBatchWithIndex::Rep::UpdateExistingEntryWithCfId(
    uint32_t column_family_id, const Slice& key) {
  if (!overwrite_key) {
    return false;
  }

  WBWIIteratorImpl iter(column_family_id, &skip_list, &write_batch);
  iter.Seek(key);
  if (!iter.Valid()) {
    return false;
  }
  if (comparator.CompareKey(column_family_id, key, iter.Entry().key) != 0) {
    return false;
  }
  WriteBatchIndexEntry* non_const_entry =
      const_cast<WriteBatchIndexEntry*>(iter.GetRawEntry());
  non_const_entry->offset = last_entry_offset;
  return true;
}

void WriteBatchWithIndex::Rep::AddOrUpdateIndex(
    ColumnFamilyHandle* column_family, const Slice& key) {
  if (!UpdateExistingEntry(column_family, key)) {
    uint32_t cf_id = GetColumnFamilyID(column_family);
    const auto* cf_cmp = GetColumnFamilyUserComparator(column_family);
    if (cf_cmp != nullptr) {
      comparator.SetComparatorForCF(cf_id, cf_cmp);
    }
    AddNewEntry(cf_id);
  }
}

void WriteBatchWithIndex::Rep::AddOrUpdateIndex(const Slice& key) {
  if (!UpdateExistingEntryWithCfId(0, key)) {
    AddNewEntry(0);
  }
}

void WriteBatchWithIndex::Rep::AddNewEntry(uint32_t column_family_id) {
  const std::string& wb_data = write_batch.Data();
  Slice entry_ptr = Slice(wb_data.data() + last_entry_offset,
                          wb_data.size() - last_entry_offset);
//提取密钥
  Slice key;
  bool success __attribute__((__unused__)) =
      ReadKeyFromWriteBatchEntry(&entry_ptr, &key, column_family_id != 0);
  assert(success);

    auto* mem = arena.Allocate(sizeof(WriteBatchIndexEntry));
    auto* index_entry =
        new (mem) WriteBatchIndexEntry(last_entry_offset, column_family_id,
                                       key.data() - wb_data.data(), key.size());
    skip_list.Insert(index_entry);
  }

  void WriteBatchWithIndex::Rep::Clear() {
    write_batch.Clear();
    ClearIndex();
  }

  void WriteBatchWithIndex::Rep::ClearIndex() {
    skip_list.~WriteBatchEntrySkipList();
    arena.~Arena();
    new (&arena) Arena();
    new (&skip_list) WriteBatchEntrySkipList(comparator, &arena);
    last_entry_offset = 0;
  }

  Status WriteBatchWithIndex::Rep::ReBuildIndex() {
    Status s;

    ClearIndex();

    if (write_batch.Count() == 0) {
//无需重新索引
      return s;
    }

    size_t offset = WriteBatchInternal::GetFirstOffset(&write_batch);

    Slice input(write_batch.Data());
    input.remove_prefix(offset);

//循环遍历rep中的所有条目，并将每个条目添加到索引中
    int found = 0;
    while (s.ok() && !input.empty()) {
      Slice key, value, blob, xid;
uint32_t column_family_id = 0;  //违约
      char tag = 0;

//为调用addNewEntry（）设置当前项的偏移量
      last_entry_offset = input.data() - write_batch.Data().data();

      s = ReadRecordFromWriteBatch(&input, &tag, &column_family_id, &key,
                                   &value, &blob, &xid);
      if (!s.ok()) {
        break;
      }

      switch (tag) {
        case kTypeColumnFamilyValue:
        case kTypeValue:
        case kTypeColumnFamilyDeletion:
        case kTypeDeletion:
        case kTypeColumnFamilySingleDeletion:
        case kTypeSingleDeletion:
        case kTypeColumnFamilyMerge:
        case kTypeMerge:
          found++;
          if (!UpdateExistingEntryWithCfId(column_family_id, key)) {
            AddNewEntry(column_family_id);
          }
          break;
        case kTypeLogData:
        case kTypeBeginPrepareXID:
        case kTypeEndPrepareXID:
        case kTypeCommitXID:
        case kTypeRollbackXID:
        case kTypeNoop:
          break;
        default:
          return Status::Corruption("unknown WriteBatch tag");
      }
    }

    if (s.ok() && found != write_batch.Count()) {
      s = Status::Corruption("WriteBatch has wrong count");
    }

    return s;
  }

  WriteBatchWithIndex::WriteBatchWithIndex(
      const Comparator* default_index_comparator, size_t reserved_bytes,
      bool overwrite_key, size_t max_bytes)
      : rep(new Rep(default_index_comparator, reserved_bytes, max_bytes,
                    overwrite_key)) {}

  WriteBatchWithIndex::~WriteBatchWithIndex() {}

  WriteBatch* WriteBatchWithIndex::GetWriteBatch() { return &rep->write_batch; }

  WBWIIterator* WriteBatchWithIndex::NewIterator() {
    return new WBWIIteratorImpl(0, &(rep->skip_list), &rep->write_batch);
}

WBWIIterator* WriteBatchWithIndex::NewIterator(
    ColumnFamilyHandle* column_family) {
  return new WBWIIteratorImpl(GetColumnFamilyID(column_family),
                              &(rep->skip_list), &rep->write_batch);
}

Iterator* WriteBatchWithIndex::NewIteratorWithBase(
    ColumnFamilyHandle* column_family, Iterator* base_iterator) {
  if (rep->overwrite_key == false) {
    assert(false);
    return nullptr;
  }
  return new BaseDeltaIterator(base_iterator, NewIterator(column_family),
                               GetColumnFamilyUserComparator(column_family));
}

Iterator* WriteBatchWithIndex::NewIteratorWithBase(Iterator* base_iterator) {
  if (rep->overwrite_key == false) {
    assert(false);
    return nullptr;
  }
//默认列族的比较器
  return new BaseDeltaIterator(base_iterator, NewIterator(),
                               rep->comparator.default_comparator());
}

Status WriteBatchWithIndex::Put(ColumnFamilyHandle* column_family,
                                const Slice& key, const Slice& value) {
  rep->SetLastEntryOffset();
  auto s = rep->write_batch.Put(column_family, key, value);
  if (s.ok()) {
    rep->AddOrUpdateIndex(column_family, key);
  }
  return s;
}

Status WriteBatchWithIndex::Put(const Slice& key, const Slice& value) {
  rep->SetLastEntryOffset();
  auto s = rep->write_batch.Put(key, value);
  if (s.ok()) {
    rep->AddOrUpdateIndex(key);
  }
  return s;
}

Status WriteBatchWithIndex::Delete(ColumnFamilyHandle* column_family,
                                   const Slice& key) {
  rep->SetLastEntryOffset();
  auto s = rep->write_batch.Delete(column_family, key);
  if (s.ok()) {
    rep->AddOrUpdateIndex(column_family, key);
  }
  return s;
}

Status WriteBatchWithIndex::Delete(const Slice& key) {
  rep->SetLastEntryOffset();
  auto s = rep->write_batch.Delete(key);
  if (s.ok()) {
    rep->AddOrUpdateIndex(key);
  }
  return s;
}

Status WriteBatchWithIndex::SingleDelete(ColumnFamilyHandle* column_family,
                                         const Slice& key) {
  rep->SetLastEntryOffset();
  auto s = rep->write_batch.SingleDelete(column_family, key);
  if (s.ok()) {
    rep->AddOrUpdateIndex(column_family, key);
  }
  return s;
}

Status WriteBatchWithIndex::SingleDelete(const Slice& key) {
  rep->SetLastEntryOffset();
  auto s = rep->write_batch.SingleDelete(key);
  if (s.ok()) {
    rep->AddOrUpdateIndex(key);
  }
  return s;
}

Status WriteBatchWithIndex::DeleteRange(ColumnFamilyHandle* column_family,
                                        const Slice& begin_key,
                                        const Slice& end_key) {
  rep->SetLastEntryOffset();
  auto s = rep->write_batch.DeleteRange(column_family, begin_key, end_key);
  if (s.ok()) {
    rep->AddOrUpdateIndex(column_family, begin_key);
  }
  return s;
}

Status WriteBatchWithIndex::DeleteRange(const Slice& begin_key,
                                        const Slice& end_key) {
  rep->SetLastEntryOffset();
  auto s = rep->write_batch.DeleteRange(begin_key, end_key);
  if (s.ok()) {
    rep->AddOrUpdateIndex(begin_key);
  }
  return s;
}

Status WriteBatchWithIndex::Merge(ColumnFamilyHandle* column_family,
                                  const Slice& key, const Slice& value) {
  rep->SetLastEntryOffset();
  auto s = rep->write_batch.Merge(column_family, key, value);
  if (s.ok()) {
    rep->AddOrUpdateIndex(column_family, key);
  }
  return s;
}

Status WriteBatchWithIndex::Merge(const Slice& key, const Slice& value) {
  rep->SetLastEntryOffset();
  auto s = rep->write_batch.Merge(key, value);
  if (s.ok()) {
    rep->AddOrUpdateIndex(key);
  }
  return s;
}

Status WriteBatchWithIndex::PutLogData(const Slice& blob) {
  return rep->write_batch.PutLogData(blob);
}

void WriteBatchWithIndex::Clear() { rep->Clear(); }

Status WriteBatchWithIndex::GetFromBatch(ColumnFamilyHandle* column_family,
                                         const DBOptions& options,
                                         const Slice& key, std::string* value) {
  Status s;
  MergeContext merge_context;
  const ImmutableDBOptions immuable_db_options(options);

  WriteBatchWithIndexInternal::Result result =
      WriteBatchWithIndexInternal::GetFromBatch(
          immuable_db_options, this, column_family, key, &merge_context,
          &rep->comparator, value, rep->overwrite_key, &s);

  switch (result) {
    case WriteBatchWithIndexInternal::Result::kFound:
    case WriteBatchWithIndexInternal::Result::kError:
//使用返回状态
      break;
    case WriteBatchWithIndexInternal::Result::kDeleted:
    case WriteBatchWithIndexInternal::Result::kNotFound:
      s = Status::NotFound();
      break;
    case WriteBatchWithIndexInternal::Result::kMergeInProgress:
      s = Status::MergeInProgress();
      break;
    default:
      assert(false);
  }

  return s;
}

Status WriteBatchWithIndex::GetFromBatchAndDB(DB* db,
                                              const ReadOptions& read_options,
                                              const Slice& key,
                                              std::string* value) {
  assert(value != nullptr);
  PinnableSlice pinnable_val(value);
  assert(!pinnable_val.IsPinned());
  auto s = GetFromBatchAndDB(db, read_options, db->DefaultColumnFamily(), key,
                             &pinnable_val);
  if (s.ok() && pinnable_val.IsPinned()) {
    value->assign(pinnable_val.data(), pinnable_val.size());
}  //else值已分配
  return s;
}

Status WriteBatchWithIndex::GetFromBatchAndDB(DB* db,
                                              const ReadOptions& read_options,
                                              const Slice& key,
                                              PinnableSlice* pinnable_val) {
  return GetFromBatchAndDB(db, read_options, db->DefaultColumnFamily(), key,
                           pinnable_val);
}

Status WriteBatchWithIndex::GetFromBatchAndDB(DB* db,
                                              const ReadOptions& read_options,
                                              ColumnFamilyHandle* column_family,
                                              const Slice& key,
                                              std::string* value) {
  assert(value != nullptr);
  PinnableSlice pinnable_val(value);
  assert(!pinnable_val.IsPinned());
  auto s =
      GetFromBatchAndDB(db, read_options, column_family, key, &pinnable_val);
  if (s.ok() && pinnable_val.IsPinned()) {
    value->assign(pinnable_val.data(), pinnable_val.size());
}  //else值已分配
  return s;
}

Status WriteBatchWithIndex::GetFromBatchAndDB(DB* db,
                                              const ReadOptions& read_options,
                                              ColumnFamilyHandle* column_family,
                                              const Slice& key,
                                              PinnableSlice* pinnable_val) {
  Status s;
  MergeContext merge_context;
  const ImmutableDBOptions& immuable_db_options =
      reinterpret_cast<DBImpl*>(db)->immutable_db_options();

//因为writebatch的生存期与事务的生存期相同
//我们不能固定它，否则返回的值将不可用
//事务完成后。
  std::string& batch_value = *pinnable_val->GetSelf();
  WriteBatchWithIndexInternal::Result result =
      WriteBatchWithIndexInternal::GetFromBatch(
          immuable_db_options, this, column_family, key, &merge_context,
          &rep->comparator, &batch_value, rep->overwrite_key, &s);

  if (result == WriteBatchWithIndexInternal::Result::kFound) {
    pinnable_val->PinSelf();
    return s;
  }
  if (result == WriteBatchWithIndexInternal::Result::kDeleted) {
    return Status::NotFound();
  }
  if (result == WriteBatchWithIndexInternal::Result::kError) {
    return s;
  }
  if (result == WriteBatchWithIndexInternal::Result::kMergeInProgress &&
      rep->overwrite_key == true) {
//因为我们已经覆盖了密钥，所以我们不知道还有哪些操作
//在这个批处理中，对于这个键，我们不能进行合并来计算
//结果。相反，我们只返回正在进行的合并。
    return Status::MergeInProgress();
  }

  assert(result == WriteBatchWithIndexInternal::Result::kMergeInProgress ||
         result == WriteBatchWithIndexInternal::Result::kNotFound);

//在批处理中找不到键或无法解析合并。尝试DB。
  s = db->Get(read_options, column_family, key, pinnable_val);

if (s.ok() || s.IsNotFound()) {  //数据库获取成功
    if (result == WriteBatchWithIndexInternal::Result::kMergeInProgress) {
//将数据库中的结果与批处理中的合并进行合并
      auto cfh = reinterpret_cast<ColumnFamilyHandleImpl*>(column_family);
      const MergeOperator* merge_operator =
          cfh->cfd()->ioptions()->merge_operator;
      Statistics* statistics = immuable_db_options.statistics.get();
      Env* env = immuable_db_options.env;
      Logger* logger = immuable_db_options.info_log.get();

      Slice* merge_data;
      if (s.ok()) {
        merge_data = pinnable_val;
} else {  //数据库中不存在密钥（s.isNotFound（））
        merge_data = nullptr;
      }

      if (merge_operator) {
        s = MergeHelper::TimedFullMerge(
            merge_operator, key, merge_data, merge_context.GetOperands(),
            pinnable_val->GetSelf(), logger, statistics, env);
        pinnable_val->PinSelf();
      } else {
        s = Status::InvalidArgument("Options::merge_operator must be set");
      }
    }
  }

  return s;
}

void WriteBatchWithIndex::SetSavePoint() { rep->write_batch.SetSavePoint(); }

Status WriteBatchWithIndex::RollbackToSavePoint() {
  Status s = rep->write_batch.RollbackToSavePoint();

  if (s.ok()) {
    s = rep->ReBuildIndex();
  }

  return s;
}

Status WriteBatchWithIndex::PopSavePoint() {
  return rep->write_batch.PopSavePoint();
}

void WriteBatchWithIndex::SetMaxBytes(size_t max_bytes) {
  rep->write_batch.SetMaxBytes(max_bytes);
}

}  //命名空间rocksdb
#endif  //！摇滚乐
