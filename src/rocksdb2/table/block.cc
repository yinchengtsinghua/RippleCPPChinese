
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//解码block_builder.cc生成的块。

#include "table/block.h"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "monitoring/perf_context_imp.h"
#include "port/port.h"
#include "port/stack_trace.h"
#include "rocksdb/comparator.h"
#include "table/block_prefix_index.h"
#include "table/format.h"
#include "util/coding.h"
#include "util/logging.h"

namespace rocksdb {

//helper例程：从“p”开始解码下一个块条目，
//存储共享密钥字节数、非共享密钥字节数，
//以及“*共享”、“*非共享”中值的长度，以及
//分别为“*value_length”。不会超出“限制”。
//
//如果检测到任何错误，则返回nullptr。否则，返回
//指向键增量的指针（刚好超过三个解码值）。
static inline const char* DecodeEntry(const char* p, const char* limit,
                                      uint32_t* shared,
                                      uint32_t* non_shared,
                                      uint32_t* value_length) {
  if (limit - p < 3) return nullptr;
  *shared = reinterpret_cast<const unsigned char*>(p)[0];
  *non_shared = reinterpret_cast<const unsigned char*>(p)[1];
  *value_length = reinterpret_cast<const unsigned char*>(p)[2];
  if ((*shared | *non_shared | *value_length) < 128) {
//快速路径：所有三个值都编码在一个字节中
    p += 3;
  } else {
    if ((p = GetVarint32Ptr(p, limit, shared)) == nullptr) return nullptr;
    if ((p = GetVarint32Ptr(p, limit, non_shared)) == nullptr) return nullptr;
    if ((p = GetVarint32Ptr(p, limit, value_length)) == nullptr) return nullptr;
  }

  if (static_cast<uint32_t>(limit - p) < (*non_shared + *value_length)) {
    return nullptr;
  }
  return p;
}

void BlockIter::Next() {
  assert(Valid());
  ParseNextKey();
}

void BlockIter::Prev() {
  assert(Valid());

  assert(prev_entries_idx_ == -1 ||
         static_cast<size_t>(prev_entries_idx_) < prev_entries_.size());
//检查是否可以使用缓存的prev_项
  if (prev_entries_idx_ > 0 &&
      prev_entries_[prev_entries_idx_].offset == current_) {
//读取缓存的cachedpEventry
    prev_entries_idx_--;
    const CachedPrevEntry& current_prev_entry =
        prev_entries_[prev_entries_idx_];

    const char* key_ptr = nullptr;
    if (current_prev_entry.key_ptr != nullptr) {
//密钥不是增量编码并存储在数据块中
      key_ptr = current_prev_entry.key_ptr;
      key_pinned_ = true;
    } else {
//密钥是增量编码的，并存储在前一个条目中。
      key_ptr = prev_entries_keys_buff_.data() + current_prev_entry.key_offset;
      key_pinned_ = false;
    }
    const Slice current_key(key_ptr, current_prev_entry.key_size);

    current_ = current_prev_entry.offset;
    /*_u.setinternalkey（当前_key，false/*复制*/）；
    value_uu=当前_上一个_entry.value；

    返回；
  }

  //清除上一条缓存
  上一个条目-1；
  上一个条目清除（）；
  上一个条目_keys_buff_u.clear（）；

  //向后扫描到当前\u之前的重新启动点
  const uint32_t original=当前值；
  while（getrestartpoint（restart_index_u）>=原始）
    如果（重新启动索引==0）
      //没有更多条目
      当前_uu=重新启动_uu；
      重新启动_index_=num_restarts_u；
      返回；
    }
    重新启动索引
  }

  请参见开始点（重新启动索引）；

  做{
    如果（！）parseNextKey（））
      断裂；
    }
    切片当前_key=key（）；

    if（key_.iskeypined（））
      //键不是增量编码的
      上一个条目放回（current_u，current_key.data（），0，
                                 当前_key.size（），value（））；
    }否则{
      //密钥为增量编码，缓存缓冲区中的解码密钥
      size_t new_key_offset=prev_entries_keys_buff_.size（）；
      prev_entries_keys_buff_u.append（current_key.data（），current_key.size（））；

      上一个条目模板返回（当前\，空指针，新的\键偏移，
                                 当前_key.size（），value（））；
    }
    //循环直到当前条目的结尾碰到原始条目的开头
  while（nextentryoffset（）<原始）；
  prev_entries_idx_u=static_cast<int32_t>（prev_entries_u.size（））-1；
}

void blockiter:：seek（const slice&target）
  性能计时器保护（阻止搜索纳米）；
  if（data_==nullptr）//尚未初始化
    返回；
  }
  uint32_t索引=0；
  bool ok=假；
  if（前缀_index_）
    OK=PrefixSeek（目标和索引）；
  }否则{
    OK=binaryseek（目标，0，num_重新启动，1，&index）；
  }

  如果（！）好的）{
    返回；
  }
  见起始点（索引）；
  //第一个键的线性搜索（在重新启动块内）>>目标

  当（真）{
    如果（！）parseNextKey（）比较（key_u.getInternalKey（），目标）>=0）
      返回；
    }
  }
}

void blockiter:：seekforprev（const slice&target）
  性能计时器保护（阻止搜索纳米）；
  if（data_==nullptr）//尚未初始化
    返回；
  }
  uint32_t索引=0；
  bool ok=假；
  OK=binaryseek（目标，0，num_重新启动，1，&index）；

  如果（！）好的）{
    返回；
  }
  见起始点（索引）；
  //第一个键的线性搜索（在重新启动块内）>>目标

  while（parseNextKey（）&&compare（key_.getInternalKey（），target）<0）
  }
  如果（！）验证（））{
    SektoStaster（）；
  }否则{
    while（valid（）&&compare（key_.getInternalKey（），target）>0）
      （）；
    }
  }
}

void blockiter:：seektofirst（）
  if（data_==nullptr）//尚未初始化
    返回；
  }
  见起点（0）；
  PARSENETKEY（）；
}

void blockiter:：seektolast（）
  if（data_==nullptr）//尚未初始化
    返回；
  }
  seektorestartpoint（num_restarts_u1）；
  while（parseNextKey（）&&nextentryoffset（）<重启_
    //继续跳过
  }
}

void blockiter:：corruptionerror（）
  当前_uu=重新启动_uu；
  重新启动_index_=num_restarts_u；
  status_=status:：corruption（“块中的错误条目”）；
  KEY..Car（）；
  Value..Car（）；
}

bool blockiter:：parseNextkey（）
  当前_u=nextentryoffset（）；
  const char*p=数据\+当前\；
  const char*limit=data_+restarts_；//重新启动发生在数据之后
  如果（P>=极限）
    //没有要返回的条目。标记为无效。
    当前_uu=重新启动_uu；
    重新启动_index_=num_restarts_u；
    返回错误；
  }

  //解码下一个条目
  uint32共享、非共享、值长度；
  P=解码条目（P、限制、共享、非共享、值长度）；
  if（p==nullptr key_.size（）<shared）
    corruptionerror（）；
    返回错误；
  }否则{
    如果（shared==0）
      //如果此键不与prev键共享任何字节，则不需要
      //对其进行解码，可以直接使用块中的地址。
      key_u.setinternalkey（slice（p，non_shared），false/*复制*/);

      key_pinned_ = true;
    } else {
//此密钥与上一个密钥共享“shared”字节，我们需要对其进行解码
      key_.TrimAppend(shared, p, non_shared);
      key_pinned_ = false;
    }

    if (global_seqno_ != kDisableGlobalSequenceNumber) {
//如果我们正在读取具有全局序列号的文件，我们应该
//期望所有编码的序列号都是零和任何值
//类型为ktypeValue、ktypeMerge或ktypeDelection
      assert(GetInternalKeySeqno(key_.GetInternalKey()) == 0);

      ValueType value_type = ExtractValueType(key_.GetInternalKey());
      assert(value_type == ValueType::kTypeValue ||
             value_type == ValueType::kTypeMerge ||
             value_type == ValueType::kTypeDeletion);

      if (key_pinned_) {
//TODO（tec）：调查更新加载块中的seqno
//而不是直接进行复制和更新。

//我们不能直接使用块中的密钥地址，因为
//我们有一个全局的_Seqno_u，它将覆盖编码的那个。
        key_.OwnKey();
        key_pinned_ = false;
      }

      key_.UpdateInternalKey(global_seqno_, value_type);
    }

    value_ = Slice(p + non_shared, value_length);
    while (restart_index_ + 1 < num_restarts_ &&
           GetRestartPoint(restart_index_ + 1) < current_) {
      ++restart_index_;
    }
    return true;
  }
}

//在重新启动数组中进行二进制搜索以查找
//是键小于目标的最后一个重新启动点，
//这意味着下一个重新启动点的密钥大于目标，或者
//键=目标的第一个重新启动点
bool BlockIter::BinarySeek(const Slice& target, uint32_t left, uint32_t right,
                           uint32_t* index) {
  assert(left <= right);

  while (left < right) {
    uint32_t mid = (left + right + 1) / 2;
    uint32_t region_offset = GetRestartPoint(mid);
    uint32_t shared, non_shared, value_length;
    const char* key_ptr = DecodeEntry(data_ + region_offset, data_ + restarts_,
                                      &shared, &non_shared, &value_length);
    if (key_ptr == nullptr || (shared != 0)) {
      CorruptionError();
      return false;
    }
    Slice mid_key(key_ptr, non_shared);
    int cmp = Compare(mid_key, target);
    if (cmp < 0) {
//“Mid”处的键小于“Target”。因此所有
//“mid”之前的块是无趣的。
      left = mid;
    } else if (cmp > 0) {
//“Mid”处的键大于等于“Target”。因此，所有块位于或
//“mid”之后就没意思了。
      right = mid - 1;
    } else {
      left = right = mid;
    }
  }

  *index = left;
  return true;
}

//比较“block_index”块的目标键和块键。
//如果出错，返回-1。
int BlockIter::CompareBlockKey(uint32_t block_index, const Slice& target) {
  uint32_t region_offset = GetRestartPoint(block_index);
  uint32_t shared, non_shared, value_length;
  const char* key_ptr = DecodeEntry(data_ + region_offset, data_ + restarts_,
                                    &shared, &non_shared, &value_length);
  if (key_ptr == nullptr || (shared != 0)) {
    CorruptionError();
return 1;  //返回目标较小
  }
  Slice block_key(key_ptr, non_shared);
  return Compare(block_key, target);
}

//在块中进行二进制搜索以查找第一个块
//用一个大于等于目标的键
bool BlockIter::BinaryBlockIndexSeek(const Slice& target, uint32_t* block_ids,
                                     uint32_t left, uint32_t right,
                                     uint32_t* index) {
  assert(left <= right);
  uint32_t left_bound = left;

  while (left <= right) {
    uint32_t mid = (right + left) / 2;

    int cmp = CompareBlockKey(block_ids[mid], target);
    if (!status_.ok()) {
      return false;
    }
    if (cmp < 0) {
//“Target”处的键大于“Mid”。因此所有
//“中间”之前或之前的块是无趣的。
      left = mid + 1;
    } else {
//“Target”处的键<=“Mid”。因此，所有区块
//“mid”之后就没意思了。
//如果只剩下一个街区，我们就找到了。
      if (left == right) break;
      right = mid;
    }
  }

  if (left == right) {
//以下两种情况之一：
//（1）左边是块ID的第一个
//（2）“Left”和“Left-1”块之间存在块间隙。
//我们可以进一步区分键在块中的情况或键不在块中的情况。
//现有，通过比较目标密钥和上一个密钥
//找到块左侧的块。
    if (block_ids[left] > 0 &&
        (left == left_bound || block_ids[left - 1] != block_ids[left] - 1) &&
        CompareBlockKey(block_ids[left] - 1, target) > 0) {
      current_ = restarts_;
      return false;
    }

    *index = block_ids[left];
    return true;
  } else {
    assert(left > right);
//标记迭代器无效
    current_ = restarts_;
    return false;
  }
}

bool BlockIter::PrefixSeek(const Slice& target, uint32_t* index) {
  assert(prefix_index_);
  uint32_t* block_ids = nullptr;
  uint32_t num_blocks = prefix_index_->GetBlocks(target, &block_ids);

  if (num_blocks == 0) {
    current_ = restarts_;
    return false;
  } else  {
    return BinaryBlockIndexSeek(target, block_ids, 0, num_blocks - 1, index);
  }
}

uint32_t Block::NumRestarts() const {
  assert(size_ >= 2*sizeof(uint32_t));
  return DecodeFixed32(data_ + size_ - sizeof(uint32_t));
}

Block::Block(BlockContents&& contents, SequenceNumber _global_seqno,
             size_t read_amp_bytes_per_bit, Statistics* statistics)
    : contents_(std::move(contents)),
      data_(contents_.data.data()),
      size_(contents_.data.size()),
      global_seqno_(_global_seqno) {
  if (size_ < sizeof(uint32_t)) {
size_ = 0;  //错误标记
  } else {
    restart_offset_ =
        static_cast<uint32_t>(size_) - (1 + NumRestarts()) * sizeof(uint32_t);
    if (restart_offset_ > size_ - sizeof(uint32_t)) {
//大小对于numsrestarts（）来说太小，因此
//重新启动环绕的“偏移”。
      size_ = 0;
    }
  }
  if (read_amp_bytes_per_bit != 0 && statistics && size_ != 0) {
    read_amp_bitmap_.reset(new BlockReadAmpBitmap(
        restart_offset_, read_amp_bytes_per_bit, statistics));
  }
}

InternalIterator* Block::NewIterator(const Comparator* cmp, BlockIter* iter,
                                     bool total_order_seek, Statistics* stats) {
  if (size_ < 2*sizeof(uint32_t)) {
    if (iter != nullptr) {
      iter->SetStatus(Status::Corruption("bad block contents"));
      return iter;
    } else {
      return NewErrorInternalIterator(Status::Corruption("bad block contents"));
    }
  }
  const uint32_t num_restarts = NumRestarts();
  if (num_restarts == 0) {
    if (iter != nullptr) {
      iter->SetStatus(Status::OK());
      return iter;
    } else {
      return NewEmptyInternalIterator();
    }
  } else {
    BlockPrefixIndex* prefix_index_ptr =
        total_order_seek ? nullptr : prefix_index_.get();

    if (iter != nullptr) {
      iter->Initialize(cmp, data_, restart_offset_, num_restarts,
                       prefix_index_ptr, global_seqno_, read_amp_bitmap_.get());
    } else {
      iter = new BlockIter(cmp, data_, restart_offset_, num_restarts,
                           prefix_index_ptr, global_seqno_,
                           read_amp_bitmap_.get());
    }

    if (read_amp_bitmap_) {
      if (read_amp_bitmap_->GetStatistics() != stats) {
//数据库更改了统计指针，我们需要通知读取位图
        read_amp_bitmap_->SetStatistics(stats);
      }
    }
  }

  return iter;
}

void Block::SetBlockPrefixIndex(BlockPrefixIndex* prefix_index) {
  prefix_index_.reset(prefix_index);
}

size_t Block::ApproximateMemoryUsage() const {
  size_t usage = usable_size();
  if (prefix_index_) {
    usage += prefix_index_->ApproximateMemoryUsage();
  }
  return usage;
}

}  //命名空间rocksdb
