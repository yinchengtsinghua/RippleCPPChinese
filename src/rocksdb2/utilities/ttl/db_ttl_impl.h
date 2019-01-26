
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#pragma once

#ifndef ROCKSDB_LITE
#include <deque>
#include <string>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/compaction_filter.h"
#include "rocksdb/merge_operator.h"
#include "rocksdb/utilities/utility_db.h"
#include "rocksdb/utilities/db_ttl.h"
#include "db/db_impl.h"

#ifdef _WIN32
//Windows API宏干扰
#undef GetCurrentTime
#endif


namespace rocksdb {

class DBWithTTLImpl : public DBWithTTL {
 public:
  static void SanitizeOptions(int32_t ttl, ColumnFamilyOptions* options,
                              Env* env);

  explicit DBWithTTLImpl(DB* db);

  virtual ~DBWithTTLImpl();

  Status CreateColumnFamilyWithTtl(const ColumnFamilyOptions& options,
                                   const std::string& column_family_name,
                                   ColumnFamilyHandle** handle,
                                   int ttl) override;

  Status CreateColumnFamily(const ColumnFamilyOptions& options,
                            const std::string& column_family_name,
                            ColumnFamilyHandle** handle) override;

  using StackableDB::Put;
  virtual Status Put(const WriteOptions& options,
                     ColumnFamilyHandle* column_family, const Slice& key,
                     const Slice& val) override;

  using StackableDB::Get;
  virtual Status Get(const ReadOptions& options,
                     ColumnFamilyHandle* column_family, const Slice& key,
                     PinnableSlice* value) override;

  using StackableDB::MultiGet;
  virtual std::vector<Status> MultiGet(
      const ReadOptions& options,
      const std::vector<ColumnFamilyHandle*>& column_family,
      const std::vector<Slice>& keys,
      std::vector<std::string>* values) override;

  using StackableDB::KeyMayExist;
  virtual bool KeyMayExist(const ReadOptions& options,
                           ColumnFamilyHandle* column_family, const Slice& key,
                           std::string* value,
                           bool* value_found = nullptr) override;

  using StackableDB::Merge;
  virtual Status Merge(const WriteOptions& options,
                       ColumnFamilyHandle* column_family, const Slice& key,
                       const Slice& value) override;

  virtual Status Write(const WriteOptions& opts, WriteBatch* updates) override;

  using StackableDB::NewIterator;
  virtual Iterator* NewIterator(const ReadOptions& opts,
                                ColumnFamilyHandle* column_family) override;

  virtual DB* GetBaseDB() override { return db_; }

  static bool IsStale(const Slice& value, int32_t ttl, Env* env);

  static Status AppendTS(const Slice& val, std::string* val_with_ts, Env* env);

  static Status SanityCheckTimestamp(const Slice& str);

  static Status StripTS(std::string* str);

  static Status StripTS(PinnableSlice* str);

static const uint32_t kTSLength = sizeof(int32_t);  //时间戳大小

static const int32_t kMinTimestamp = 1368146402;  //2013年9月5日下午5:40 GMT-8

static const int32_t kMaxTimestamp = 2147483647;  //2038年1月18日下午7:14 GMT-8
};

class TtlIterator : public Iterator {

 public:
  explicit TtlIterator(Iterator* iter) : iter_(iter) { assert(iter_); }

  ~TtlIterator() { delete iter_; }

  bool Valid() const override { return iter_->Valid(); }

  void SeekToFirst() override { iter_->SeekToFirst(); }

  void SeekToLast() override { iter_->SeekToLast(); }

  void Seek(const Slice& target) override { iter_->Seek(target); }

  void SeekForPrev(const Slice& target) override { iter_->SeekForPrev(target); }

  void Next() override { iter_->Next(); }

  void Prev() override { iter_->Prev(); }

  Slice key() const override { return iter_->key(); }

  int32_t timestamp() const {
    return DecodeFixed32(iter_->value().data() + iter_->value().size() -
                         DBWithTTLImpl::kTSLength);
  }

  Slice value() const override {
//TODO:像一般迭代器语义一样处理时间戳损坏
    assert(DBWithTTLImpl::SanityCheckTimestamp(iter_->value()).ok());
    Slice trimmed_value = iter_->value();
    trimmed_value.size_ -= DBWithTTLImpl::kTSLength;
    return trimmed_value;
  }

  Status status() const override { return iter_->status(); }

 private:
  Iterator* iter_;
};

class TtlCompactionFilter : public CompactionFilter {
 public:
  TtlCompactionFilter(
      int32_t ttl, Env* env, const CompactionFilter* user_comp_filter,
      std::unique_ptr<const CompactionFilter> user_comp_filter_from_factory =
          nullptr)
      : ttl_(ttl),
        env_(env),
        user_comp_filter_(user_comp_filter),
        user_comp_filter_from_factory_(
            std::move(user_comp_filter_from_factory)) {
//与合并操作符不同，压缩过滤器对于TTL是必需的，因此
//即使用户没有指定任何压缩过滤器，也会调用此函数。
    if (!user_comp_filter_) {
      user_comp_filter_ = user_comp_filter_from_factory_.get();
    }
  }

  virtual bool Filter(int level, const Slice& key, const Slice& old_val,
                      std::string* new_val, bool* value_changed) const
      override {
    if (DBWithTTLImpl::IsStale(old_val, ttl_, env_)) {
      return true;
    }
    if (user_comp_filter_ == nullptr) {
      return false;
    }
    assert(old_val.size() >= DBWithTTLImpl::kTSLength);
    Slice old_val_without_ts(old_val.data(),
                             old_val.size() - DBWithTTLImpl::kTSLength);
    if (user_comp_filter_->Filter(level, key, old_val_without_ts, new_val,
                                  value_changed)) {
      return true;
    }
    if (*value_changed) {
      new_val->append(
          old_val.data() + old_val.size() - DBWithTTLImpl::kTSLength,
          DBWithTTLImpl::kTSLength);
    }
    return false;
  }

  virtual const char* Name() const override { return "Delete By TTL"; }

 private:
  int32_t ttl_;
  Env* env_;
  const CompactionFilter* user_comp_filter_;
  std::unique_ptr<const CompactionFilter> user_comp_filter_from_factory_;
};

class TtlCompactionFilterFactory : public CompactionFilterFactory {
 public:
  TtlCompactionFilterFactory(
      int32_t ttl, Env* env,
      std::shared_ptr<CompactionFilterFactory> comp_filter_factory)
      : ttl_(ttl), env_(env), user_comp_filter_factory_(comp_filter_factory) {}

  virtual std::unique_ptr<CompactionFilter> CreateCompactionFilter(
      const CompactionFilter::Context& context) override {
    std::unique_ptr<const CompactionFilter> user_comp_filter_from_factory =
        nullptr;
    if (user_comp_filter_factory_) {
      user_comp_filter_from_factory =
          user_comp_filter_factory_->CreateCompactionFilter(context);
    }

    return std::unique_ptr<TtlCompactionFilter>(new TtlCompactionFilter(
        ttl_, env_, nullptr, std::move(user_comp_filter_from_factory)));
  }

  virtual const char* Name() const override {
    return "TtlCompactionFilterFactory";
  }

 private:
  int32_t ttl_;
  Env* env_;
  std::shared_ptr<CompactionFilterFactory> user_comp_filter_factory_;
};

class TtlMergeOperator : public MergeOperator {

 public:
  explicit TtlMergeOperator(const std::shared_ptr<MergeOperator>& merge_op,
                            Env* env)
      : user_merge_op_(merge_op), env_(env) {
    assert(merge_op);
    assert(env);
  }

  virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                           MergeOperationOutput* merge_out) const override {
    const uint32_t ts_len = DBWithTTLImpl::kTSLength;
    if (merge_in.existing_value && merge_in.existing_value->size() < ts_len) {
      ROCKS_LOG_ERROR(merge_in.logger,
                      "Error: Could not remove timestamp from existing value.");
      return false;
    }

//从要传递给用户的每个操作数中提取时间戳
    std::vector<Slice> operands_without_ts;
    for (const auto& operand : merge_in.operand_list) {
      if (operand.size() < ts_len) {
        ROCKS_LOG_ERROR(
            merge_in.logger,
            "Error: Could not remove timestamp from operand value.");
        return false;
      }
      operands_without_ts.push_back(operand);
      operands_without_ts.back().remove_suffix(ts_len);
    }

//应用用户合并运算符（将结果存储在*新值中）
    bool good = true;
    MergeOperationOutput user_merge_out(merge_out->new_value,
                                        merge_out->existing_operand);
    if (merge_in.existing_value) {
      Slice existing_value_without_ts(merge_in.existing_value->data(),
                                      merge_in.existing_value->size() - ts_len);
      good = user_merge_op_->FullMergeV2(
          MergeOperationInput(merge_in.key, &existing_value_without_ts,
                              operands_without_ts, merge_in.logger),
          &user_merge_out);
    } else {
      good = user_merge_op_->FullMergeV2(
          MergeOperationInput(merge_in.key, nullptr, operands_without_ts,
                              merge_in.logger),
          &user_merge_out);
    }

//如果用户合并运算符返回了false，则返回false
    if (!good) {
      return false;
    }

    if (merge_out->existing_operand.data()) {
      merge_out->new_value.assign(merge_out->existing_operand.data(),
                                  merge_out->existing_operand.size());
      merge_out->existing_operand = Slice(nullptr, 0);
    }

//用TTL时间戳增加*新_值
    int64_t curtime;
    if (!env_->GetCurrentTime(&curtime).ok()) {
      ROCKS_LOG_ERROR(
          merge_in.logger,
          "Error: Could not get current time to be attached internally "
          "to the new value.");
      return false;
    } else {
      char ts_string[ts_len];
      EncodeFixed32(ts_string, (int32_t)curtime);
      merge_out->new_value.append(ts_string, ts_len);
      return true;
    }
  }

  virtual bool PartialMergeMulti(const Slice& key,
                                 const std::deque<Slice>& operand_list,
                                 std::string* new_value, Logger* logger) const
      override {
    const uint32_t ts_len = DBWithTTLImpl::kTSLength;
    std::deque<Slice> operands_without_ts;

    for (const auto& operand : operand_list) {
      if (operand.size() < ts_len) {
        ROCKS_LOG_ERROR(logger,
                        "Error: Could not remove timestamp from value.");
        return false;
      }

      operands_without_ts.push_back(
          Slice(operand.data(), operand.size() - ts_len));
    }

//应用用户部分合并运算符（将结果存储在*新值中）
    assert(new_value);
    if (!user_merge_op_->PartialMergeMulti(key, operands_without_ts, new_value,
                                           logger)) {
      return false;
    }

//用TTL时间戳增加*新_值
    int64_t curtime;
    if (!env_->GetCurrentTime(&curtime).ok()) {
      ROCKS_LOG_ERROR(
          logger,
          "Error: Could not get current time to be attached internally "
          "to the new value.");
      return false;
    } else {
      char ts_string[ts_len];
      EncodeFixed32(ts_string, (int32_t)curtime);
      new_value->append(ts_string, ts_len);
      return true;
    }
  }

  virtual const char* Name() const override { return "Merge By TTL"; }

 private:
  std::shared_ptr<MergeOperator> user_merge_op_;
  Env* env_;
};
}
#endif  //摇滚乐
