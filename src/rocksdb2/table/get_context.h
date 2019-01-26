
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
#include <string>
#include "db/merge_context.h"
#include "db/range_del_aggregator.h"
#include "rocksdb/env.h"
#include "rocksdb/types.h"
#include "table/block.h"

namespace rocksdb {
class MergeContext;
class PinnedIteratorsManager;

class GetContext {
 public:
  enum GetState {
    kNotFound,
    kFound,
    kDeleted,
    kCorrupt,
kMerge,  //保存程序包含当前合并结果（操作数）
    kBlobIndex,
  };

  GetContext(const Comparator* ucmp, const MergeOperator* merge_operator,
             Logger* logger, Statistics* statistics, GetState init_state,
             const Slice& user_key, PinnableSlice* value, bool* value_found,
             MergeContext* merge_context, RangeDelAggregator* range_del_agg,
             Env* env, SequenceNumber* seq = nullptr,
             PinnedIteratorsManager* _pinned_iters_mgr = nullptr,
             bool* is_blob_index = nullptr);

  void MarkKeyMayExist();

//记录此键、值和任何元数据（如序列号和
//状态）进入此getContext。
//
//如果需要读取更多的键（由于合并），或
//如果已找到完整值，则返回false。
  bool SaveValue(const ParsedInternalKey& parsed_key, const Slice& value,
                 Cleanable* value_pinner = nullptr);

//前一个函数的简化版本。只有当我们
//你要知道这是一个看跌期权。
  void SaveValue(const Slice& value, SequenceNumber seq);

  GetState State() const { return state_; }

  RangeDelAggregator* range_del_agg() { return range_del_agg_; }

  PinnedIteratorsManager* pinned_iters_mgr() { return pinned_iters_mgr_; }

//如果传递了非空字符串，则所有savevalue调用都将
//已登录到字符串中。然后可以在上重播操作
//另一个具有replayGetContextLog的getContext。
  void SetReplayLog(std::string* replay_log) { replay_log_ = replay_log; }

//我们需要获取这个键的序列号吗？
  bool NeedToReadSequence() const { return (seq_ != nullptr); }

  bool sample() const { return sample_; }

 private:
  const Comparator* ucmp_;
  const MergeOperator* merge_operator_;
//遇到的合并操作；
  Logger* logger_;
  Statistics* statistics_;

  GetState state_;
  Slice user_key_;
  PinnableSlice* pinnable_val_;
bool* value_found_;  //值设置是否正确？由keymayexist使用
  MergeContext* merge_context_;
  RangeDelAggregator* range_del_agg_;
  Env* env_;
//如果找到一个键，seq_u将被设置为最新的sequencenumber
//如果未知，则写入密钥或kmaxSequenceNumber
  SequenceNumber* seq_;
  std::string* replay_log_;
//用于在状态_==getContext:：kmerge时临时固定块
  PinnedIteratorsManager* pinned_iters_mgr_;
  bool sample_;
  bool* is_blob_index_;
};

void replayGetContextLog(const Slice& replay_log, const Slice& user_key,
                         GetContext* get_context,
                         Cleanable* value_pinner = nullptr);

}  //命名空间rocksdb
