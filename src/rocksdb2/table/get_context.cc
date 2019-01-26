
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

#include "table/get_context.h"
#include "db/merge_helper.h"
#include "db/pinned_iterators_manager.h"
#include "monitoring/file_read_sample.h"
#include "monitoring/perf_context_imp.h"
#include "monitoring/statistics.h"
#include "rocksdb/env.h"
#include "rocksdb/merge_operator.h"
#include "rocksdb/statistics.h"

namespace rocksdb {

namespace {

void appendToReplayLog(std::string* replay_log, ValueType type, Slice value) {
#ifndef ROCKSDB_LITE
  if (replay_log) {
    if (replay_log->empty()) {
//优化：在通常情况下，在
//日志，我们分配所需的确切空间量。
      replay_log->reserve(1 + VarintLength(value.size()) + value.size());
    }
    replay_log->push_back(type);
    PutLengthPrefixedSlice(replay_log, value);
  }
#endif  //摇滚乐
}

}  //命名空间

GetContext::GetContext(
    const Comparator* ucmp, const MergeOperator* merge_operator, Logger* logger,
    Statistics* statistics, GetState init_state, const Slice& user_key,
    PinnableSlice* pinnable_val, bool* value_found, MergeContext* merge_context,
    RangeDelAggregator* _range_del_agg, Env* env, SequenceNumber* seq,
    PinnedIteratorsManager* _pinned_iters_mgr, bool* is_blob_index)
    : ucmp_(ucmp),
      merge_operator_(merge_operator),
      logger_(logger),
      statistics_(statistics),
      state_(init_state),
      user_key_(user_key),
      pinnable_val_(pinnable_val),
      value_found_(value_found),
      merge_context_(merge_context),
      range_del_agg_(_range_del_agg),
      env_(env),
      seq_(seq),
      replay_log_(nullptr),
      pinned_iters_mgr_(_pinned_iters_mgr),
      is_blob_index_(is_blob_index) {
  if (seq_) {
    *seq_ = kMaxSequenceNumber;
  }
  sample_ = should_sample_file_read();
}

//当文件/块中
//tablecache/blockcache中可能不存在键。在这
//我们不能保证钥匙不存在也不允许这样做
//IO是确定的。设置status=kfound和value_found=false以使
//调用者知道密钥可能存在，但内存中没有。
void GetContext::MarkKeyMayExist() {
  state_ = kFound;
  if (value_found_ != nullptr) {
    *value_found_ = false;
  }
}

void GetContext::SaveValue(const Slice& value, SequenceNumber seq) {
  assert(state_ == kNotFound);
  appendToReplayLog(replay_log_, kTypeValue, value);

  state_ = kFound;
  if (LIKELY(pinnable_val_ != nullptr)) {
    pinnable_val_->PinSelf(value);
  }
}

bool GetContext::SaveValue(const ParsedInternalKey& parsed_key,
                           const Slice& value, Cleanable* value_pinner) {
  assert((state_ != kMerge && parsed_key.type != kTypeMerge) ||
         merge_context_ != nullptr);
  if (ucmp_->Equal(parsed_key.user_key, user_key_)) {
    appendToReplayLog(replay_log_, parsed_key.type, value);

    if (seq_ != nullptr) {
//如果序列号未初始化，则设置该序列号
      if (*seq_ == kMaxSequenceNumber) {
        *seq_ = parsed_key.sequence;
      }
    }

    auto type = parsed_key.type;
//关键匹配。处理它
    if ((type == kTypeValue || type == kTypeMerge || type == kTypeBlobIndex) &&
        range_del_agg_ != nullptr && range_del_agg_->ShouldDelete(parsed_key)) {
      type = kTypeRangeDeletion;
    }
    switch (type) {
      case kTypeValue:
      case kTypeBlobIndex:
        assert(state_ == kNotFound || state_ == kMerge);
        if (type == kTypeBlobIndex && is_blob_index_ == nullptr) {
//不支持blob值。停下来。
          state_ = kBlobIndex;
          return false;
        }
        if (kNotFound == state_) {
          state_ = kFound;
          if (LIKELY(pinnable_val_ != nullptr)) {
            if (LIKELY(value_pinner != nullptr)) {
//如果为值提供了支持资源，请将其固定
              pinnable_val_->PinSlice(value, value_pinner);
            } else {
//否则复制值
              pinnable_val_->PinSelf(value);
            }
          }
        } else if (kMerge == state_) {
          assert(merge_operator_ != nullptr);
          state_ = kFound;
          if (LIKELY(pinnable_val_ != nullptr)) {
            Status merge_status = MergeHelper::TimedFullMerge(
                merge_operator_, user_key_, &value,
                merge_context_->GetOperands(), pinnable_val_->GetSelf(),
                logger_, statistics_, env_);
            pinnable_val_->PinSelf();
            if (!merge_status.ok()) {
              state_ = kCorrupt;
            }
          }
        }
        if (is_blob_index_ != nullptr) {
          *is_blob_index_ = (type == kTypeBlobIndex);
        }
        return false;

      case kTypeDeletion:
      case kTypeSingleDeletion:
      case kTypeRangeDeletion:
//TODO（noetzli）：一旦合并单个删除，请验证正确性
//支持
        assert(state_ == kNotFound || state_ == kMerge);
        if (kNotFound == state_) {
          state_ = kDeleted;
        } else if (kMerge == state_) {
          state_ = kFound;
          if (LIKELY(pinnable_val_ != nullptr)) {
            Status merge_status = MergeHelper::TimedFullMerge(
                merge_operator_, user_key_, nullptr,
                merge_context_->GetOperands(), pinnable_val_->GetSelf(),
                logger_, statistics_, env_);
            pinnable_val_->PinSelf();
            if (!merge_status.ok()) {
              state_ = kCorrupt;
            }
          }
        }
        return false;

      case kTypeMerge:
        assert(state_ == kNotFound || state_ == kMerge);
        state_ = kMerge;
//例如，值“pinner”不是从plain“table”reader.cc设置的。
        if (pinned_iters_mgr() && pinned_iters_mgr()->PinningEnabled() &&
            value_pinner != nullptr) {
          value_pinner->DelegateCleanupsTo(pinned_iters_mgr());
          /*ge_context_u->pushoperand（value，true/*value_pinned*/）；
        }否则{
          合并_Context_u->PushOperand（值，假）；
        }
        回归真实；

      违约：
        断言（假）；
        断裂；
    }
  }

  //状态“可能已损坏、合并或找不到”
  返回错误；
}

void replayGetContextLog（const slice&replay_log，const slice&user_key，
                         getContext*获取_context，可清理*值_pinner）
ifndef岩石
  slice s=重放日志；
  while（s.size（））
    auto type=static_cast<valuetype>（*s.data（））；
    删除前缀（1）；
    切片值；
    bool ret=getlengthprefixedslice（&s，&value）；
    断言（RET）；
    （无效）RET；

    //由于SequenceNumber未存储且未知，因此我们将使用
    //kmaxSequenceNumber。
    获取_context->savevalue（
        parsedinternalkey（用户密钥，kmaxSequenceNumber，类型），值，
        ValuePyPin）；
  }
其他//岩石
  断言（假）；
endif//rocksdb_lite
}

//命名空间rocksdb
