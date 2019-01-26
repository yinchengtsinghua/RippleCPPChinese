
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
#include "rocksdb/memtablerep.h"

#include <unordered_set>
#include <set>
#include <memory>
#include <algorithm>
#include <type_traits>

#include "util/arena.h"
#include "db/memtable.h"
#include "memtable/stl_wrappers.h"
#include "port/port.h"
#include "util/mutexlock.h"

namespace rocksdb {
namespace {

using namespace stl_wrappers;

class VectorRep : public MemTableRep {
 public:
  VectorRep(const KeyComparator& compare, Allocator* allocator, size_t count);

//在集合中插入键。（调用方将密钥和值打包到
//单个缓冲区并将其作为要插入的参数传入）
//要求：当前没有与键比较等于的内容位于
//收集。
  virtual void Insert(KeyHandle handle) override;

//返回true iff集合中比较等于key的条目。
  virtual bool Contains(const char* key) const override;

  virtual void MarkReadOnly() override;

  virtual size_t ApproximateMemoryUsage() override;

  virtual void Get(const LookupKey& k, void* callback_args,
                   bool (*callback_func)(void* arg,
                                         const char* entry)) override;

  virtual ~VectorRep() override { }

  class Iterator : public MemTableRep::Iterator {
    class VectorRep* vrep_;
    std::shared_ptr<std::vector<const char*>> bucket_;
    std::vector<const char*>::const_iterator mutable cit_;
    const KeyComparator& compare_;
std::string tmp_;       //用于传递到encodekey
    bool mutable sorted_;
    void DoSort() const;
   public:
    explicit Iterator(class VectorRep* vrep,
      std::shared_ptr<std::vector<const char*>> bucket,
      const KeyComparator& compare);

//在指定集合上初始化迭代器。
//返回的迭代器无效。
//显式迭代器（const memtablerep*collection）；
    virtual ~Iterator() override { };

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

//使用键<=target前进到第一个条目
    virtual void SeekForPrev(const Slice& user_key,
                             const char* memtable_key) override;

//在集合中的第一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToFirst() override;

//在集合中的最后一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToLast() override;
  };

//返回此表示形式中键的迭代器。
  virtual MemTableRep::Iterator* GetIterator(Arena* arena) override;

 private:
  friend class Iterator;
  typedef std::vector<const char*> Bucket;
  std::shared_ptr<Bucket> bucket_;
  mutable port::RWMutex rwlock_;
  bool immutable_;
  bool sorted_;
  const KeyComparator& compare_;
};

void VectorRep::Insert(KeyHandle handle) {
  auto* key = static_cast<char*>(handle);
  WriteLock l(&rwlock_);
  assert(!immutable_);
  bucket_->push_back(key);
}

//返回true iff集合中比较等于key的条目。
bool VectorRep::Contains(const char* key) const {
  ReadLock l(&rwlock_);
  return std::find(bucket_->begin(), bucket_->end(), key) != bucket_->end();
}

void VectorRep::MarkReadOnly() {
  WriteLock l(&rwlock_);
  immutable_ = true;
}

size_t VectorRep::ApproximateMemoryUsage() {
  return
    sizeof(bucket_) + sizeof(*bucket_) +
    bucket_->size() *
    sizeof(
      std::remove_reference<decltype(*bucket_)>::type::value_type
    );
}

VectorRep::VectorRep(const KeyComparator& compare, Allocator* allocator,
                     size_t count)
    : MemTableRep(allocator),
      bucket_(new Bucket()),
      immutable_(false),
      sorted_(false),
      compare_(compare) {
  bucket_.get()->reserve(count);
}

VectorRep::Iterator::Iterator(class VectorRep* vrep,
                   std::shared_ptr<std::vector<const char*>> bucket,
                   const KeyComparator& compare)
: vrep_(vrep),
  bucket_(bucket),
  cit_(bucket_->end()),
  compare_(compare),
  sorted_(false) { }

void VectorRep::Iterator::DoSort() const {
//vrep非空意味着我们正在处理一个不可变的memtable
  if (!sorted_ && vrep_ != nullptr) {
    WriteLock l(&vrep_->rwlock_);
    if (!vrep_->sorted_) {
      std::sort(bucket_->begin(), bucket_->end(), Compare(compare_));
      cit_ = bucket_->begin();
      vrep_->sorted_ = true;
    }
    sorted_ = true;
  }
  if (!sorted_) {
    std::sort(bucket_->begin(), bucket_->end(), Compare(compare_));
    cit_ = bucket_->begin();
    sorted_ = true;
  }
  assert(sorted_);
  assert(vrep_ == nullptr || vrep_->sorted_);
}

//如果迭代器位于有效节点，则返回true。
bool VectorRep::Iterator::Valid() const {
  DoSort();
  return cit_ != bucket_->end();
}

//返回当前位置的键。
//要求：有效（）
const char* VectorRep::Iterator::key() const {
  assert(sorted_);
  return *cit_;
}

//前进到下一个位置。
//要求：有效（）
void VectorRep::Iterator::Next() {
  assert(sorted_);
  if (cit_ == bucket_->end()) {
    return;
  }
  ++cit_;
}

//前进到上一个位置。
//要求：有效（）
void VectorRep::Iterator::Prev() {
  assert(sorted_);
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
void VectorRep::Iterator::Seek(const Slice& user_key,
                               const char* memtable_key) {
  DoSort();
//执行二进制搜索以查找不小于目标值的第一个值
  const char* encoded_key =
      (memtable_key != nullptr) ? memtable_key : EncodeKey(&tmp_, user_key);
  cit_ = std::equal_range(bucket_->begin(),
                          bucket_->end(),
                          encoded_key,
                          [this] (const char* a, const char* b) {
                            return compare_(a, b) < 0;
                          }).first;
}

//使用键<=target前进到第一个条目
void VectorRep::Iterator::SeekForPrev(const Slice& user_key,
                                      const char* memtable_key) {
  assert(false);
}

//在集合中的第一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
void VectorRep::Iterator::SeekToFirst() {
  DoSort();
  cit_ = bucket_->begin();
}

//在集合中的最后一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
void VectorRep::Iterator::SeekToLast() {
  DoSort();
  cit_ = bucket_->end();
  if (bucket_->size() != 0) {
    --cit_;
  }
}

void VectorRep::Get(const LookupKey& k, void* callback_args,
                    bool (*callback_func)(void* arg, const char* entry)) {
  rwlock_.ReadLock();
  VectorRep* vector_rep;
  std::shared_ptr<Bucket> bucket;
  if (immutable_) {
    vector_rep = this;
  } else {
    vector_rep = nullptr;
bucket.reset(new Bucket(*bucket_));  //制作副本
  }
  VectorRep::Iterator iter(vector_rep, immutable_ ? bucket_ : bucket, compare_);
  rwlock_.ReadUnlock();

  for (iter.Seek(k.user_key(), k.memtable_key().data());
       iter.Valid() && callback_func(callback_args, iter.key()); iter.Next()) {
  }
}

MemTableRep::Iterator* VectorRep::GetIterator(Arena* arena) {
  char* mem = nullptr;
  if (arena != nullptr) {
    mem = arena->AllocateAligned(sizeof(Iterator));
  }
  ReadLock l(&rwlock_);
//不要在这里排序。排序将在第一次进行
//在迭代器上执行查找。
  if (immutable_) {
    if (arena == nullptr) {
      return new Iterator(this, bucket_, compare_);
    } else {
      return new (mem) Iterator(this, bucket_, compare_);
    }
  } else {
    std::shared_ptr<Bucket> tmp;
tmp.reset(new Bucket(*bucket_)); //制作副本
    if (arena == nullptr) {
      return new Iterator(nullptr, tmp, compare_);
    } else {
      return new (mem) Iterator(nullptr, tmp, compare_);
    }
  }
}
} //非命名空间

MemTableRep* VectorRepFactory::CreateMemTableRep(
    const MemTableRep::KeyComparator& compare, Allocator* allocator,
    const SliceTransform*, Logger* logger) {
  return new VectorRep(compare, allocator, count_);
}
} //命名空间rocksdb
#endif  //摇滚乐
