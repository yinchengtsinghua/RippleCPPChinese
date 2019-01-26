
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
//
//版权所有（c）2012 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#include "table/block_based_filter_block.h"
#include <algorithm>

#include "db/dbformat.h"
#include "monitoring/perf_context_imp.h"
#include "rocksdb/filter_policy.h"
#include "util/coding.h"
#include "util/string_util.h"

namespace rocksdb {

namespace {

void AppendItem(std::string* props, const std::string& key,
                const std::string& value) {
  char cspace = ' ';
  std::string value_str("");
  size_t i = 0;
  const size_t dataLength = 64;
  const size_t tabLength = 2;
  const size_t offLength = 16;

  value_str.append(&value[i], std::min(size_t(dataLength), value.size()));
  i += dataLength;
  while (i < value.size()) {
    value_str.append("\n");
    value_str.append(offLength, cspace);
    value_str.append(&value[i], std::min(size_t(dataLength), value.size() - i));
    i += dataLength;
  }

  std::string result("");
  if (key.size() < (offLength - tabLength))
    result.append(size_t((offLength - tabLength)) - key.size(), cspace);
  result.append(key);

  props->append(result + ": " + value_str + "\n");
}

template <class TKey>
void AppendItem(std::string* props, const TKey& key, const std::string& value) {
  std::string key_str = rocksdb::ToString(key);
  AppendItem(props, key_str, value);
}
}  //命名空间


//有关过滤块格式的说明，请参阅doc/table_format.txt。

//每2KB数据生成一个新的筛选器
static const size_t kFilterBaseLg = 11;
static const size_t kFilterBase = 1 << kFilterBaseLg;

BlockBasedFilterBlockBuilder::BlockBasedFilterBlockBuilder(
    const SliceTransform* prefix_extractor,
    const BlockBasedTableOptions& table_opt)
    : policy_(table_opt.filter_policy.get()),
      prefix_extractor_(prefix_extractor),
      whole_key_filtering_(table_opt.whole_key_filtering),
      prev_prefix_start_(0),
      prev_prefix_size_(0) {
  assert(policy_);
}

void BlockBasedFilterBlockBuilder::StartBlock(uint64_t block_offset) {
  uint64_t filter_index = (block_offset / kFilterBase);
  assert(filter_index >= filter_offsets_.size());
  while (filter_index > filter_offsets_.size()) {
    GenerateFilter();
  }
}

void BlockBasedFilterBlockBuilder::Add(const Slice& key) {
  if (prefix_extractor_ && prefix_extractor_->InDomain(key)) {
    AddPrefix(key);
  }

  if (whole_key_filtering_) {
    AddKey(key);
  }
}

//如果需要，添加筛选键
inline void BlockBasedFilterBlockBuilder::AddKey(const Slice& key) {
  start_.push_back(entries_.size());
  entries_.append(key.data(), key.size());
}

//如果需要，添加前缀到筛选器
inline void BlockBasedFilterBlockBuilder::AddPrefix(const Slice& key) {
//获取最近添加项的切片
  Slice prev;
  if (prev_prefix_size_ > 0) {
    prev = Slice(entries_.data() + prev_prefix_start_, prev_prefix_size_);
  }

  Slice prefix = prefix_extractor_->Transform(key);
//仅当前缀与前一个前缀不同时才插入前缀。
  if (prev.size() == 0 || prefix != prev) {
    start_.push_back(entries_.size());
    prev_prefix_start_ = entries_.size();
    prev_prefix_size_ = prefix.size();
    entries_.append(prefix.data(), prefix.size());
  }
}

Slice BlockBasedFilterBlockBuilder::Finish(const BlockHandle& tmp,
                                           Status* status) {
//在这个例子中，我们忽略blockhandle
  *status = Status::OK();
  if (!start_.empty()) {
    GenerateFilter();
  }

//附加每个过滤器偏移的数组
  const uint32_t array_offset = static_cast<uint32_t>(result_.size());
  for (size_t i = 0; i < filter_offsets_.size(); i++) {
    PutFixed32(&result_, filter_offsets_[i]);
  }

  PutFixed32(&result_, array_offset);
result_.push_back(kFilterBaseLg);  //在结果中保存编码参数
  return Slice(result_);
}

void BlockBasedFilterBlockBuilder::GenerateFilter() {
  const size_t num_entries = start_.size();
  if (num_entries == 0) {
//如果没有此筛选器的键，则为快速路径
    filter_offsets_.push_back(static_cast<uint32_t>(result_.size()));
    return;
  }

//从扁平的键结构创建键列表
start_.push_back(entries_.size());  //简化长度计算
  tmp_entries_.resize(num_entries);
  for (size_t i = 0; i < num_entries; i++) {
    const char* base = entries_.data() + start_[i];
    size_t length = start_[i + 1] - start_[i];
    tmp_entries_[i] = Slice(base, length);
  }

//为当前键集生成筛选器并附加到结果_uuu。
  filter_offsets_.push_back(static_cast<uint32_t>(result_.size()));
  policy_->CreateFilter(&tmp_entries_[0], static_cast<int>(num_entries),
                        &result_);

  tmp_entries_.clear();
  entries_.clear();
  start_.clear();
  prev_prefix_start_ = 0;
  prev_prefix_size_ = 0;
}

BlockBasedFilterBlockReader::BlockBasedFilterBlockReader(
    const SliceTransform* prefix_extractor,
    const BlockBasedTableOptions& table_opt, bool _whole_key_filtering,
    BlockContents&& contents, Statistics* stats)
    : FilterBlockReader(contents.data.size(), stats, _whole_key_filtering),
      policy_(table_opt.filter_policy.get()),
      prefix_extractor_(prefix_extractor),
      data_(nullptr),
      offset_(nullptr),
      num_(0),
      base_lg_(0),
      contents_(std::move(contents)) {
  assert(policy_);
  size_t n = contents_.data.size();
if (n < 5) return;  //基极lg_1字节，偏移阵列起始4字节
  base_lg_ = contents_.data[n - 1];
  uint32_t last_word = DecodeFixed32(contents_.data.data() + n - 5);
  if (last_word > n - 5) return;
  data_ = contents_.data.data();
  offset_ = data_ + last_word;
  num_ = (n - 5 - last_word) / 4;
}

bool BlockBasedFilterBlockReader::KeyMayMatch(
    const Slice& key, uint64_t block_offset, const bool no_io,
    const Slice* const const_ikey_ptr) {
  assert(block_offset != kNotValid);
  if (!whole_key_filtering_) {
    return true;
  }
  return MayMatch(key, block_offset);
}

bool BlockBasedFilterBlockReader::PrefixMayMatch(
    const Slice& prefix, uint64_t block_offset, const bool no_io,
    const Slice* const const_ikey_ptr) {
  assert(block_offset != kNotValid);
  if (!prefix_extractor_) {
    return true;
  }
  return MayMatch(prefix, block_offset);
}

bool BlockBasedFilterBlockReader::MayMatch(const Slice& entry,
                                           uint64_t block_offset) {
  uint64_t index = block_offset >> base_lg_;
  if (index < num_) {
    uint32_t start = DecodeFixed32(offset_ + index * 4);
    uint32_t limit = DecodeFixed32(offset_ + index * 4 + 4);
    if (start <= limit && limit <= (uint32_t)(offset_ - data_)) {
      Slice filter = Slice(data_ + start, limit - start);
      bool const may_match = policy_->KeyMayMatch(entry, filter);
      if (may_match) {
        PERF_COUNTER_ADD(bloom_sst_hit_count, 1);
        return true;
      } else {
        PERF_COUNTER_ADD(bloom_sst_miss_count, 1);
        return false;
      }
    } else if (start == limit) {
//空筛选器与任何条目都不匹配
      return false;
    }
  }
return true;  //错误被视为潜在匹配
}

size_t BlockBasedFilterBlockReader::ApproximateMemoryUsage() const {
  return num_ * 4 + 5 + (offset_ - data_);
}

std::string BlockBasedFilterBlockReader::ToString() const {
  std::string result, filter_meta;
  result.reserve(1024);

  std::string s_bo("Block offset"), s_hd("Hex dump"), s_fb("# filter blocks");
  AppendItem(&result, s_fb, rocksdb::ToString(num_));
  AppendItem(&result, s_bo, s_hd);

  for (size_t index = 0; index < num_; index++) {
    uint32_t start = DecodeFixed32(offset_ + index * 4);
    uint32_t limit = DecodeFixed32(offset_ + index * 4 + 4);

    if (start != limit) {
      result.append(" filter block # " + rocksdb::ToString(index + 1) + "\n");
      Slice filter = Slice(data_ + start, limit - start);
      AppendItem(&result, start, filter.ToString(true));
    }
  }
  return result;
}
}  //命名空间rocksdb
