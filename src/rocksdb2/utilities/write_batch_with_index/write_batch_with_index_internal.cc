
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

#include "utilities/write_batch_with_index/write_batch_with_index_internal.h"

#include "db/column_family.h"
#include "db/merge_context.h"
#include "db/merge_helper.h"
#include "rocksdb/comparator.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/write_batch_with_index.h"
#include "util/coding.h"
#include "util/string_util.h"

namespace rocksdb {

class Env;
class Logger;
class Statistics;

Status ReadableWriteBatch::GetEntryFromDataOffset(size_t data_offset,
                                                  WriteType* type, Slice* Key,
                                                  Slice* value, Slice* blob,
                                                  Slice* xid) const {
  if (type == nullptr || Key == nullptr || value == nullptr ||
      blob == nullptr || xid == nullptr) {
    return Status::InvalidArgument("Output parameters cannot be null");
  }

  if (data_offset == GetDataSize()) {
//已到达批处理结束。
    return Status::NotFound();
  }

  if (data_offset > GetDataSize()) {
    return Status::InvalidArgument("data offset exceed write batch size");
  }
  Slice input = Slice(rep_.data() + data_offset, rep_.size() - data_offset);
  char tag;
  uint32_t column_family;
  Status s = ReadRecordFromWriteBatch(&input, &tag, &column_family, Key, value,
                                      blob, xid);

  switch (tag) {
    case kTypeColumnFamilyValue:
    case kTypeValue:
      *type = kPutRecord;
      break;
    case kTypeColumnFamilyDeletion:
    case kTypeDeletion:
      *type = kDeleteRecord;
      break;
    case kTypeColumnFamilySingleDeletion:
    case kTypeSingleDeletion:
      *type = kSingleDeleteRecord;
      break;
    case kTypeColumnFamilyRangeDeletion:
    case kTypeRangeDeletion:
      *type = kDeleteRangeRecord;
      break;
    case kTypeColumnFamilyMerge:
    case kTypeMerge:
      *type = kMergeRecord;
      break;
    case kTypeLogData:
      *type = kLogDataRecord;
      break;
    case kTypeBeginPrepareXID:
    case kTypeEndPrepareXID:
    case kTypeCommitXID:
    case kTypeRollbackXID:
      *type = kXIDRecord;
      break;
    default:
      return Status::Corruption("unknown WriteBatch tag");
  }
  return Status::OK();
}

int WriteBatchEntryComparator::operator()(
    const WriteBatchIndexEntry* entry1,
    const WriteBatchIndexEntry* entry2) const {
  if (entry1->column_family > entry2->column_family) {
    return 1;
  } else if (entry1->column_family < entry2->column_family) {
    return -1;
  }

  if (entry1->offset == WriteBatchIndexEntry::kFlagMin) {
    return -1;
  } else if (entry2->offset == WriteBatchIndexEntry::kFlagMin) {
    return 1;
  }

  Slice key1, key2;
  if (entry1->search_key == nullptr) {
    key1 = Slice(write_batch_->Data().data() + entry1->key_offset,
                 entry1->key_size);
  } else {
    key1 = *(entry1->search_key);
  }
  if (entry2->search_key == nullptr) {
    key2 = Slice(write_batch_->Data().data() + entry2->key_offset,
                 entry2->key_size);
  } else {
    key2 = *(entry2->search_key);
  }

  int cmp = CompareKey(entry1->column_family, key1, key2);
  if (cmp != 0) {
    return cmp;
  } else if (entry1->offset > entry2->offset) {
    return 1;
  } else if (entry1->offset < entry2->offset) {
    return -1;
  }
  return 0;
}

int WriteBatchEntryComparator::CompareKey(uint32_t column_family,
                                          const Slice& key1,
                                          const Slice& key2) const {
  if (column_family < cf_comparators_.size() &&
      cf_comparators_[column_family] != nullptr) {
    return cf_comparators_[column_family]->Compare(key1, key2);
  } else {
    return default_comparator_->Compare(key1, key2);
  }
}

WriteBatchWithIndexInternal::Result WriteBatchWithIndexInternal::GetFromBatch(
    const ImmutableDBOptions& immuable_db_options, WriteBatchWithIndex* batch,
    ColumnFamilyHandle* column_family, const Slice& key,
    MergeContext* merge_context, WriteBatchEntryComparator* cmp,
    std::string* value, bool overwrite_key, Status* s) {
  uint32_t cf_id = GetColumnFamilyID(column_family);
  *s = Status::OK();
  WriteBatchWithIndexInternal::Result result =
      WriteBatchWithIndexInternal::Result::kNotFound;

  std::unique_ptr<WBWIIterator> iter =
      std::unique_ptr<WBWIIterator>(batch->NewIterator(column_family));

//我们希望按照写入被添加到
//批处理。因为我们没有反向迭代器，所以必须从结尾处寻找。
//TODO（Agiardullo）：考虑添加对反向迭代的支持
  iter->Seek(key);
  while (iter->Valid()) {
    const WriteEntry entry = iter->Entry();
    if (cmp->CompareKey(cf_id, entry.key, key) != 0) {
      break;
    }

    iter->Next();
  }

  if (!(*s).ok()) {
    return WriteBatchWithIndexInternal::Result::kError;
  }

  if (!iter->Valid()) {
//读取超过结果结尾。在最后一个结果上重新定位。
    iter->SeekToLast();
  } else {
    iter->Prev();
  }

  Slice entry_value;
  while (iter->Valid()) {
    const WriteEntry entry = iter->Entry();
    if (cmp->CompareKey(cf_id, entry.key, key) != 0) {
//意外错误或我们已到达另一个下一个键
      break;
    }

    switch (entry.type) {
      case kPutRecord: {
        result = WriteBatchWithIndexInternal::Result::kFound;
        entry_value = entry.value;
        break;
      }
      case kMergeRecord: {
        result = WriteBatchWithIndexInternal::Result::kMergeInProgress;
        merge_context->PushOperand(entry.value);
        break;
      }
      case kDeleteRecord:
      case kSingleDeleteRecord: {
        result = WriteBatchWithIndexInternal::Result::kDeleted;
        break;
      }
      case kLogDataRecord:
      case kXIDRecord: {
//忽视
        break;
      }
      default: {
        result = WriteBatchWithIndexInternal::Result::kError;
        (*s) = Status::Corruption("Unexpected entry in WriteBatchWithIndex:",
                                  ToString(entry.type));
        break;
      }
    }
    if (result == WriteBatchWithIndexInternal::Result::kFound ||
        result == WriteBatchWithIndexInternal::Result::kDeleted ||
        result == WriteBatchWithIndexInternal::Result::kError) {
//一旦找到Put或Delete，就可以停止迭代
      break;
    }
    if (result == WriteBatchWithIndexInternal::Result::kMergeInProgress &&
        overwrite_key == true) {
//因为我们已经覆盖了密钥，所以我们不知道还有哪些操作
//在这个批处理中，对于这个键，我们不能进行合并来计算
//结果。相反，我们只返回正在进行的合并。
      break;
    }

    iter->Prev();
  }

  if ((*s).ok()) {
    if (result == WriteBatchWithIndexInternal::Result::kFound ||
        result == WriteBatchWithIndexInternal::Result::kDeleted) {
//找到Put或Delete。必要时合并。
      if (merge_context->GetNumOperands() > 0) {
        const MergeOperator* merge_operator;

        if (column_family != nullptr) {
          auto cfh = reinterpret_cast<ColumnFamilyHandleImpl*>(column_family);
          merge_operator = cfh->cfd()->ioptions()->merge_operator;
        } else {
          *s = Status::InvalidArgument("Must provide a column_family");
          result = WriteBatchWithIndexInternal::Result::kError;
          return result;
        }
        Statistics* statistics = immuable_db_options.statistics.get();
        Env* env = immuable_db_options.env;
        Logger* logger = immuable_db_options.info_log.get();

        if (merge_operator) {
          *s = MergeHelper::TimedFullMerge(merge_operator, key, &entry_value,
                                           merge_context->GetOperands(), value,
                                           logger, statistics, env);
        } else {
          *s = Status::InvalidArgument("Options::merge_operator must be set");
        }
        if ((*s).ok()) {
          result = WriteBatchWithIndexInternal::Result::kFound;
        } else {
          result = WriteBatchWithIndexInternal::Result::kError;
        }
} else {  //没有要合并的内容
if (result == WriteBatchWithIndexInternal::Result::kFound) {  //放
          value->assign(entry_value.data(), entry_value.size());
        }
      }
    }
  }

  return result;
}

}  //命名空间rocksdb

#endif  //！摇滚乐
