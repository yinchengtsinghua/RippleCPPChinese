
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

#ifndef ROCKSDB_LITE
#include "memtable/hash_cuckoo_rep.h"

#include <algorithm>
#include <atomic>
#include <limits>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "db/memtable.h"
#include "memtable/skiplist.h"
#include "memtable/stl_wrappers.h"
#include "port/port.h"
#include "rocksdb/memtablerep.h"
#include "util/murmurhash.h"

namespace rocksdb {
namespace {

//布谷鸟路径搜索队列的默认最大大小
static const int kCuckooPathMaxSearchSteps = 100;

struct CuckooStep {
  static const int kNullStep = -1;
//布谷鸟数组中的bucket id。
  int bucket_id_;
//指向上一步的布谷鸟步数组的索引，
//-1如果是开始步骤。
  int prev_step_id_;
//当前步骤的深度。
  unsigned int depth_;

  CuckooStep() : bucket_id_(-1), prev_step_id_(kNullStep), depth_(1) {}

  CuckooStep(CuckooStep&& o) = default;

  CuckooStep& operator=(CuckooStep&& rhs) {
    bucket_id_ = std::move(rhs.bucket_id_);
    prev_step_id_ = std::move(rhs.prev_step_id_);
    depth_ = std::move(rhs.depth_);
    return *this;
  }

  CuckooStep(const CuckooStep&) = delete;
  CuckooStep& operator=(const CuckooStep&) = delete;

  CuckooStep(int bucket_id, int prev_step_id, int depth)
      : bucket_id_(bucket_id), prev_step_id_(prev_step_id), depth_(depth) {}
};

class HashCuckooRep : public MemTableRep {
 public:
  explicit HashCuckooRep(const MemTableRep::KeyComparator& compare,
                         Allocator* allocator, const size_t bucket_count,
                         const unsigned int hash_func_count,
                         const size_t approximate_entry_size)
      : MemTableRep(allocator),
        compare_(compare),
        allocator_(allocator),
        bucket_count_(bucket_count),
        approximate_entry_size_(approximate_entry_size),
        cuckoo_path_max_depth_(kDefaultCuckooPathMaxDepth),
        occupied_count_(0),
        hash_function_count_(hash_func_count),
        backup_table_(nullptr) {
    char* mem = reinterpret_cast<char*>(
        allocator_->Allocate(sizeof(std::atomic<const char*>) * bucket_count_));
    cuckoo_array_ = new (mem) std::atomic<char*>[bucket_count_];
    for (unsigned int bid = 0; bid < bucket_count_; ++bid) {
      cuckoo_array_[bid].store(nullptr, std::memory_order_relaxed);
    }

    cuckoo_path_ = reinterpret_cast<int*>(
        allocator_->Allocate(sizeof(int) * (cuckoo_path_max_depth_ + 1)));
    is_nearly_full_ = false;
  }

//返回false，表示hashcuckoorep不支持合并运算符。
  virtual bool IsMergeOperatorSupported() const override { return false; }

//返回false，表示hashcuckoorep不支持快照。
  virtual bool IsSnapshotSupported() const override { return false; }

//返回true iff集合中比较等于key的条目。
  virtual bool Contains(const char* internal_key) const override;

  virtual ~HashCuckooRep() override {}

//将指定的键（内部_键）插入MEM表。断言
//如果失败
//当前MEM表已包含指定的键。
  virtual void Insert(KeyHandle handle) override;

//此函数返回bucket_count_ux近似_entry_size_u时
//在下列情况下，不允许进一步的写入操作：
//1。当满度达到kMaxfullnes时。
//2。当使用备份表时。
//
//否则，此函数将始终返回0。
  virtual size_t ApproximateMemoryUsage() override {
    if (is_nearly_full_) {
      return bucket_count_ * approximate_entry_size_;
    }
    return 0;
  }

  virtual void Get(const LookupKey& k, void* callback_args,
                   bool (*callback_func)(void* arg,
                                         const char* entry)) override;

  class Iterator : public MemTableRep::Iterator {
    std::shared_ptr<std::vector<const char*>> bucket_;
    std::vector<const char*>::const_iterator mutable cit_;
    const KeyComparator& compare_;
std::string tmp_;  //用于传递到encodekey
    bool mutable sorted_;
    void DoSort() const;

   public:
    explicit Iterator(std::shared_ptr<std::vector<const char*>> bucket,
                      const KeyComparator& compare);

//在指定集合上初始化迭代器。
//返回的迭代器无效。
//显式迭代器（const memtablerep*collection）；
    virtual ~Iterator() override{};

//如果迭代器位于有效节点，则返回true。
    virtual bool Valid() const override;

//返回当前位置的键。
//要求：有效（）
    virtual const char* key() const override;

//前进到下一个位置。
//要求：有效（）
    virtual void Next() override;

//前进到上一个位置。
//要求：有效（）
    virtual void Prev() override;

//使用大于等于target的键前进到第一个条目
    virtual void Seek(const Slice& user_key, const char* memtable_key) override;

//使用键<=target返回到最后一个条目
    virtual void SeekForPrev(const Slice& user_key,
                             const char* memtable_key) override;

//在集合中的第一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToFirst() override;

//在集合中的最后一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToLast() override;
  };

  struct CuckooStepBuffer {
    CuckooStepBuffer() : write_index_(0), read_index_(0) {}
    ~CuckooStepBuffer() {}

    int write_index_;
    int read_index_;
    CuckooStep steps_[kCuckooPathMaxSearchSteps];

    CuckooStep& NextWriteBuffer() { return steps_[write_index_++]; }

    inline const CuckooStep& ReadNext() { return steps_[read_index_++]; }

    inline bool HasNewWrite() { return write_index_ > read_index_; }

    inline void reset() {
      write_index_ = 0;
      read_index_ = 0;
    }

    inline bool IsFull() { return write_index_ >= kCuckooPathMaxSearchSteps; }

//返回已读取的步骤数
    inline int ReadCount() { return read_index_; }

//返回已写入缓冲区的步骤数。
    inline int WriteCount() { return write_index_; }
  };

 private:
  const MemTableRep::KeyComparator& compare_;
//指向分配器的指针，用于分配内存，构造后不可变。
  Allocator* const allocator_;
//哈希表中的哈希桶数。
  const size_t bucket_count_;
//每个入口的近似尺寸
  const size_t approximate_entry_size_;
//布谷鸟路径的最大深度。
  const unsigned int cuckoo_path_max_depth_;
//布谷鸟数组中已被占用的项的当前数目。
  size_t occupied_count_;
//布谷鸟散列中使用的散列函数的当前数目。
  unsigned int hash_function_count_;
//用于处理布谷鸟哈希无法找到的情况的备份memtablerep
//用于插入卖出请求密钥的空桶。
  std::shared_ptr<MemTableRep> backup_table_;
//存储指针的数组，指向实际数据。
  std::atomic<char*>* cuckoo_array_;
//存储布谷鸟路径的缓冲区
  int* cuckoo_path_;
//指示bucket数组是否完整的布尔标志
//到达使当前memtable不可变的点。
  bool is_nearly_full_;

//布谷鸟路径的默认最大深度。
  static const unsigned int kDefaultCuckooPathMaxDepth = 10;

  CuckooStepBuffer step_buffer_;

//返回根据
  unsigned int GetHash(const Slice& slice, const int hash_func_id) const {
//杂音散列中用于产生不同散列函数的种子。
    static const int kMurmurHashSeeds[HashCuckooRepFactory::kMaxHashCount] = {
        545609244,  1769731426, 763324157,  13099088,   592422103,
        1899789565, 248369300,  1984183468, 1613664382, 1491157517};
    return static_cast<unsigned int>(
        MurmurHash(slice.data(), static_cast<int>(slice.size()),
                   kMurmurHashSeeds[hash_func_id]) %
        bucket_count_);
  }

//布谷鸟路径是一系列桶ID，其中每个ID指向
//布谷鸟阵的位置。这条路径描述了位移序列
//用于存储输入用户指定的所需数据的条目数。
//关键。路径从与
//指定的用户密钥并在布谷鸟数组中的空白处结束。这个
//函数将更新布谷鸟路径。
//
//@如果发现布谷鸟的踪迹，则返回“真”。
  bool FindCuckooPath(const char* internal_key, const Slice& user_key,
                      int* cuckoo_path, size_t* cuckoo_path_length,
                      int initial_hash_id = 0);

//通过检查一个桶中是否有空桶来执行快速插入。
//输入键的可能位置。如果是这样，那么函数将
//如果返回true，则密钥将存储在该空桶中。
//
//此函数是findcuckoopath的一个助手函数，它发现
//布谷鸟之路的第一步。它从第一个计算开始
//输入键的可能位置（并将其存储在bucket-id中）。
//然后，如果其中一个可能的位置是空的，则输入键将
//将存储在该空白空间中，函数将返回true。
//否则，函数将返回false，指示完成搜索
//布谷鸟的道路是必要的。
  bool QuickInsert(const char* internal_key, const Slice& user_key,
                   int bucket_ids[], const int initial_hash_id);

//将指向内部迭代器的指针返回到bucket所在的bucket
//根据用户指定的KeyComparator进行排序。注意
//此函数调用后的任何插入都可能影响
//返回的迭代器。
  virtual MemTableRep::Iterator* GetIterator(Arena* arena) override {
    std::vector<const char*> compact_buckets;
    for (unsigned int bid = 0; bid < bucket_count_; ++bid) {
      const char* bucket = cuckoo_array_[bid].load(std::memory_order_relaxed);
      if (bucket != nullptr) {
        compact_buckets.push_back(bucket);
      }
    }
    MemTableRep* backup_table = backup_table_.get();
    if (backup_table != nullptr) {
      std::unique_ptr<MemTableRep::Iterator> iter(backup_table->GetIterator());
      for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        compact_buckets.push_back(iter->key());
      }
    }
    if (arena == nullptr) {
      return new Iterator(
          std::shared_ptr<std::vector<const char*>>(
              new std::vector<const char*>(std::move(compact_buckets))),
          compare_);
    } else {
      auto mem = arena->AllocateAligned(sizeof(Iterator));
      return new (mem) Iterator(
          std::shared_ptr<std::vector<const char*>>(
              new std::vector<const char*>(std::move(compact_buckets))),
          compare_);
    }
  }
};

void HashCuckooRep::Get(const LookupKey& key, void* callback_args,
                        bool (*callback_func)(void* arg, const char* entry)) {
  Slice user_key = key.user_key();
  for (unsigned int hid = 0; hid < hash_function_count_; ++hid) {
    const char* bucket =
        cuckoo_array_[GetHash(user_key, hid)].load(std::memory_order_acquire);
    if (bucket != nullptr) {
      Slice bucket_user_key = UserKey(bucket);
      if (user_key == bucket_user_key) {
        callback_func(callback_args, bucket);
        break;
      }
    } else {
//as put（）始终存储在
//当我们第一次
//在get（）中找到一个空桶，这意味着一个未命中。
      break;
    }
  }
  MemTableRep* backup_table = backup_table_.get();
  if (backup_table != nullptr) {
    backup_table->Get(key, callback_args, callback_func);
  }
}

void HashCuckooRep::Insert(KeyHandle handle) {
  static const float kMaxFullness = 0.90f;

  auto* key = static_cast<char*>(handle);
  int initial_hash_id = 0;
  size_t cuckoo_path_length = 0;
  auto user_key = UserKey(key);
//寻找布谷鸟之路
  if (FindCuckooPath(key, user_key, cuckoo_path_, &cuckoo_path_length,
                     initial_hash_id) == false) {
//如果是真的，那么即使我们
//已用完所有哈希函数。然后使用备份memtable
//存储这样的键，这将进一步使该MEM表成为
//不变的
    if (backup_table_.get() == nullptr) {
      VectorRepFactory factory(10);
      backup_table_.reset(
          factory.CreateMemTableRep(compare_, allocator_, nullptr, nullptr));
      is_nearly_full_ = true;
    }
    backup_table_->Insert(key);
    return;
  }
//达到此点时，表示插入操作可以成功完成。
  occupied_count_++;
  if (occupied_count_ >= bucket_count_ * kMaxFullness) {
    is_nearly_full_ = true;
  }

//如果布谷鸟路径长度大于1，则执行踢出过程。
  if (cuckoo_path_length == 0) return;

//布谷鸟路径以相反的顺序存储限位路径。
//所以实际执行的是顶出或位移
//以相反的顺序，避免在阅读时出现错误的否定。
//把布谷鸟路径中涉及的每一个键移动到新的
//更换前的位置。
  for (size_t i = 1; i < cuckoo_path_length; ++i) {
    int kicked_out_bid = cuckoo_path_[i - 1];
    int current_bid = cuckoo_path_[i];
//因为我们一次只允许一个作家，所以轻松阅读是安全的。
    cuckoo_array_[kicked_out_bid]
        .store(cuckoo_array_[current_bid].load(std::memory_order_relaxed),
               std::memory_order_release);
  }
  int insert_key_bid = cuckoo_path_[cuckoo_path_length - 1];
  cuckoo_array_[insert_key_bid].store(key, std::memory_order_release);
}

bool HashCuckooRep::Contains(const char* internal_key) const {
  auto user_key = UserKey(internal_key);
  for (unsigned int hid = 0; hid < hash_function_count_; ++hid) {
    const char* stored_key =
        cuckoo_array_[GetHash(user_key, hid)].load(std::memory_order_acquire);
    if (stored_key != nullptr) {
      if (compare_(internal_key, stored_key) == 0) {
        return true;
      }
    }
  }
  return false;
}

bool HashCuckooRep::QuickInsert(const char* internal_key, const Slice& user_key,
                                int bucket_ids[], const int initial_hash_id) {
  int cuckoo_bucket_id = -1;

//以下是：
//0。计算输入键的所有可能位置。
//1。检查是否有一个bucket具有与输入相同的用户密钥。
//2。如果存在这样的桶，则用新的
//插入数据并返回。此步骤还执行重复检查。
//三。如果不存在这样的桶，但存在空桶，则插入
//输入数据。
//4。如果步骤1到3全部失败，则返回false。
  for (unsigned int hid = initial_hash_id; hid < hash_function_count_; ++hid) {
    bucket_ids[hid] = GetHash(user_key, hid);
//因为一次只允许一个看跌期权，这是看跌期权的一部分
//操作，这样我们就可以安全地执行放松负载。
    const char* stored_key =
        cuckoo_array_[bucket_ids[hid]].load(std::memory_order_relaxed);
    if (stored_key == nullptr) {
      if (cuckoo_bucket_id == -1) {
        cuckoo_bucket_id = bucket_ids[hid];
      }
    } else {
      const auto bucket_user_key = UserKey(stored_key);
      if (bucket_user_key.compare(user_key) == 0) {
        cuckoo_bucket_id = bucket_ids[hid];
        break;
      }
    }
  }

  if (cuckoo_bucket_id != -1) {
    cuckoo_array_[cuckoo_bucket_id].store(const_cast<char*>(internal_key),
                                          std::memory_order_release);
    return true;
  }

  return false;
}

//进行预检查，找出最短的布谷鸟路径。杜鹃小径
//是用于插入指定输入键的位移序列。
//
//@如果成功地找到了一个空位或布谷鸟路径，则返回true。
//如果返回值为真，但布谷鸟路径的长度为零，
//则表示空桶或有匹配用户的桶
//找到输入的键，并快速插入。
bool HashCuckooRep::FindCuckooPath(const char* internal_key,
                                   const Slice& user_key, int* cuckoo_path,
                                   size_t* cuckoo_path_length,
                                   const int initial_hash_id) {
  int bucket_ids[HashCuckooRepFactory::kMaxHashCount];
  *cuckoo_path_length = 0;

  if (QuickInsert(internal_key, user_key, bucket_ids, initial_hash_id)) {
    return true;
  }
//如果达到此步骤，则表示：
//1。输入键的任何可能位置都没有空的存储桶。
//2。输入键的任何可能位置都没有相同的用户
//key作为“internal_key”的输入。

//步骤队列的前后索引
  step_buffer_.reset();

  for (unsigned int hid = initial_hash_id; hid < hash_function_count_; ++hid) {
///cuckoostep&current_step=step_queue_uu[front_pos++]
    CuckooStep& current_step = step_buffer_.NextWriteBuffer();
    current_step.bucket_id_ = bucket_ids[hid];
    current_step.prev_step_id_ = CuckooStep::kNullStep;
    current_step.depth_ = 1;
  }

  while (step_buffer_.HasNewWrite()) {
    int step_id = step_buffer_.read_index_;
    const CuckooStep& step = step_buffer_.ReadNext();
//既然这是一个BFS过程，那么第一步就要更深一步
//超过最大允许深度表示所有剩余步骤
//在步骤缓冲区队列中，都将超过最大深度。
//立即返回false，表示找不到空桶
//对于最大允许深度之前的输入键。
    if (step.depth_ >= cuckoo_path_max_depth_) {
      return false;
    }
//同样，我们可以在这里安全地进行无障碍负载，作为电流。
//线程是唯一的编写器。
    Slice bucket_user_key =
        UserKey(cuckoo_array_[step.bucket_id_].load(std::memory_order_relaxed));
    if (step.prev_step_id_ != CuckooStep::kNullStep) {
      if (bucket_user_key == user_key) {
//然后在当前路径中有一个循环，停止发现此路径。
        continue;
      }
    }
//如果当前bucket存储在第n个位置，那么我们只考虑
//它的mth位置，其中m>n。此属性确保所有读取
//如果我们有与查询键关联的数据，将不会错过。
//
//上面语句中的n和m是代码中隐藏和隐藏的起始值。
    unsigned int start_hid = hash_function_count_;
    for (unsigned int hid = 0; hid < hash_function_count_; ++hid) {
      bucket_ids[hid] = GetHash(bucket_user_key, hid);
      if (step.bucket_id_ == bucket_ids[hid]) {
        start_hid = hid;
      }
    }
//必须找到一个当前为“主”的桶。
    assert(start_hid != hash_function_count_);

//探索当前步骤中所有可能的后续步骤。
    for (unsigned int hid = start_hid + 1; hid < hash_function_count_; ++hid) {
      CuckooStep& next_step = step_buffer_.NextWriteBuffer();
      next_step.bucket_id_ = bucket_ids[hid];
      next_step.prev_step_id_ = step_id;
      next_step.depth_ = step.depth_ + 1;
//一旦找到一个空桶，就可以追溯到它之前的所有步骤。
//生成布谷鸟路径。
      if (cuckoo_array_[next_step.bucket_id_].load(std::memory_order_relaxed) ==
          nullptr) {
//把最后一步储存在布谷鸟的小径上。注意布谷鸟之路
//按相反顺序存储步骤。这样我们就可以移动钥匙了
//布谷鸟的路径是把每一把钥匙先存到新的地方
//把它从旧地方移走。此属性确保读取将
//不会因为沿着布谷鸟路径移动钥匙而错过。
        cuckoo_path[(*cuckoo_path_length)++] = next_step.bucket_id_;
        int depth;
        for (depth = step.depth_; depth > 0 && step_id != CuckooStep::kNullStep;
             depth--) {
          const CuckooStep& prev_step = step_buffer_.steps_[step_id];
          cuckoo_path[(*cuckoo_path_length)++] = prev_step.bucket_id_;
          step_id = prev_step.prev_step_id_;
        }
        assert(depth == 0 && step_id == CuckooStep::kNullStep);
        return true;
      }
      if (step_buffer_.IsFull()) {
//如果为真，则达到布谷鸟搜索步骤的最大数目。
        return false;
      }
    }
  }

//尝试了所有可能的路径，但仍然找不到布谷鸟路径
//哪条路通向一个空桶。
  return false;
}

HashCuckooRep::Iterator::Iterator(
    std::shared_ptr<std::vector<const char*>> bucket,
    const KeyComparator& compare)
    : bucket_(bucket),
      cit_(bucket_->end()),
      compare_(compare),
      sorted_(false) {}

void HashCuckooRep::Iterator::DoSort() const {
  if (!sorted_) {
    std::sort(bucket_->begin(), bucket_->end(),
              stl_wrappers::Compare(compare_));
    cit_ = bucket_->begin();
    sorted_ = true;
  }
}

//如果迭代器位于有效节点，则返回true。
bool HashCuckooRep::Iterator::Valid() const {
  DoSort();
  return cit_ != bucket_->end();
}

//返回当前位置的键。
//要求：有效（）
const char* HashCuckooRep::Iterator::key() const {
  assert(Valid());
  return *cit_;
}

//前进到下一个位置。
//要求：有效（）
void HashCuckooRep::Iterator::Next() {
  assert(Valid());
  if (cit_ == bucket_->end()) {
    return;
  }
  ++cit_;
}

//前进到上一个位置。
//要求：有效（）
void HashCuckooRep::Iterator::Prev() {
  assert(Valid());
  if (cit_ == bucket_->begin()) {
//如果试图从第一个元素返回，迭代器应该是
//作废的所以我们把它设置为超过终点。这意味着你可以
//循环处理容器。
    cit_ = bucket_->end();
  } else {
    --cit_;
  }
}

//使用大于等于target的键前进到第一个条目
void HashCuckooRep::Iterator::Seek(const Slice& user_key,
                                   const char* memtable_key) {
  DoSort();
//执行二进制搜索以查找不小于目标值的第一个值
  const char* encoded_key =
      (memtable_key != nullptr) ? memtable_key : EncodeKey(&tmp_, user_key);
  cit_ = std::equal_range(bucket_->begin(), bucket_->end(), encoded_key,
                          [this](const char* a, const char* b) {
                            return compare_(a, b) < 0;
                          }).first;
}

//使用键<=target返回到最后一个条目
void HashCuckooRep::Iterator::SeekForPrev(const Slice& user_key,
                                          const char* memtable_key) {
  assert(false);
}

//在集合中的第一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
void HashCuckooRep::Iterator::SeekToFirst() {
  DoSort();
  cit_ = bucket_->begin();
}

//在集合中的最后一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
void HashCuckooRep::Iterator::SeekToLast() {
  DoSort();
  cit_ = bucket_->end();
  if (bucket_->size() != 0) {
    --cit_;
  }
}

}  //ANOM命名空间

MemTableRep* HashCuckooRepFactory::CreateMemTableRep(
    const MemTableRep::KeyComparator& compare, Allocator* allocator,
    const SliceTransform* transform, Logger* logger) {
//估计的平均满度。任何关闭哈希的写入性能
//随着MEM表的满度增加而降低。设置K满度
//如果值在0.7左右，则可以更好地避免写入性能下降，同时
//保持高效的内存使用率。
  static const float kFullness = 0.7f;
  size_t pointer_size = sizeof(std::atomic<const char*>);
  assert(write_buffer_size_ >= (average_data_size_ + pointer_size));
  size_t bucket_count =
    static_cast<size_t>(
      (write_buffer_size_ / (average_data_size_ + pointer_size)) / kFullness +
      1);
  unsigned int hash_function_count = hash_function_count_;
  if (hash_function_count < 2) {
    hash_function_count = 2;
  }
  if (hash_function_count > kMaxHashCount) {
    hash_function_count = kMaxHashCount;
  }
  return new HashCuckooRep(compare, allocator, bucket_count,
                           hash_function_count,
                           static_cast<size_t>(
                             (average_data_size_ + pointer_size) / kFullness)
                           );
}

MemTableRepFactory* NewHashCuckooRepFactory(size_t write_buffer_size,
                                            size_t average_data_size,
                                            unsigned int hash_function_count) {
  return new HashCuckooRepFactory(write_buffer_size, average_data_size,
                                  hash_function_count);
}

}  //命名空间rocksdb
#endif  //摇滚乐
