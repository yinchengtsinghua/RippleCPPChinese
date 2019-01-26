
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
#include "memtable/hash_skiplist_rep.h"

#include <atomic>

#include "rocksdb/memtablerep.h"
#include "util/arena.h"
#include "rocksdb/slice.h"
#include "rocksdb/slice_transform.h"
#include "port/port.h"
#include "util/murmurhash.h"
#include "db/memtable.h"
#include "memtable/skiplist.h"

namespace rocksdb {
namespace {

class HashSkipListRep : public MemTableRep {
 public:
  HashSkipListRep(const MemTableRep::KeyComparator& compare,
                  Allocator* allocator, const SliceTransform* transform,
                  size_t bucket_size, int32_t skiplist_height,
                  int32_t skiplist_branching_factor);

  virtual void Insert(KeyHandle handle) override;

  virtual bool Contains(const char* key) const override;

  virtual size_t ApproximateMemoryUsage() override;

  virtual void Get(const LookupKey& k, void* callback_args,
                   bool (*callback_func)(void* arg,
                                         const char* entry)) override;

  virtual ~HashSkipListRep();

  virtual MemTableRep::Iterator* GetIterator(Arena* arena = nullptr) override;

  virtual MemTableRep::Iterator* GetDynamicPrefixIterator(
      Arena* arena = nullptr) override;

 private:
  friend class DynamicIterator;
  typedef SkipList<const char*, const MemTableRep::KeyComparator&> Bucket;

  size_t bucket_size_;

  const int32_t skiplist_height_;
  const int32_t skiplist_branching_factor_;

//将切片（转换为用户键）映射到键共享桶
//同样的转变。
  std::atomic<Bucket*>* buckets_;

//用户提供的转换，其域是用户密钥。
  const SliceTransform* transform_;

  const MemTableRep::KeyComparator& compare_;
//构造后不变
  Allocator* const allocator_;

  inline size_t GetHash(const Slice& slice) const {
    return MurmurHash(slice.data(), static_cast<int>(slice.size()), 0) %
           bucket_size_;
  }
  inline Bucket* GetBucket(size_t i) const {
    return buckets_[i].load(std::memory_order_acquire);
  }
  inline Bucket* GetBucket(const Slice& slice) const {
    return GetBucket(GetHash(slice));
  }
//从桶里拿桶来。如果bucket还没有初始化，
//返回前初始化它。
  Bucket* GetInitializedBucket(const Slice& transformed);

  class Iterator : public MemTableRep::Iterator {
   public:
    explicit Iterator(Bucket* list, bool own_list = true,
                      Arena* arena = nullptr)
        : list_(list), iter_(list), own_list_(own_list), arena_(arena) {}

    virtual ~Iterator() {
//如果我们拥有这个列表，我们也应该删除它
      if (own_list_) {
        assert(list_ != nullptr);
        delete list_;
      }
    }

//如果迭代器位于有效节点，则返回true。
    virtual bool Valid() const override {
      return list_ != nullptr && iter_.Valid();
    }

//返回当前位置的键。
//要求：有效（）
    virtual const char* key() const override {
      assert(Valid());
      return iter_.key();
    }

//前进到下一个位置。
//要求：有效（）
    virtual void Next() override {
      assert(Valid());
      iter_.Next();
    }

//前进到上一个位置。
//要求：有效（）
    virtual void Prev() override {
      assert(Valid());
      iter_.Prev();
    }

//使用大于等于target的键前进到第一个条目
    virtual void Seek(const Slice& internal_key,
                      const char* memtable_key) override {
      if (list_ != nullptr) {
        const char* encoded_key =
            (memtable_key != nullptr) ?
                memtable_key : EncodeKey(&tmp_, internal_key);
        iter_.Seek(encoded_key);
      }
    }

//使用键<=target返回到最后一个条目
    virtual void SeekForPrev(const Slice& internal_key,
                             const char* memtable_key) override {
//不支持
      assert(false);
    }

//在集合中的第一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToFirst() override {
      if (list_ != nullptr) {
        iter_.SeekToFirst();
      }
    }

//在集合中的最后一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToLast() override {
      if (list_ != nullptr) {
        iter_.SeekToLast();
      }
    }
   protected:
    void Reset(Bucket* list) {
      if (own_list_) {
        assert(list_ != nullptr);
        delete list_;
      }
      list_ = list;
      iter_.SetList(list);
      own_list_ = false;
    }
   private:
//如果list_u为nullptr，则不应在iter_上调用任何方法
//如果list_u为nullptr，则此迭代器无效（）。
    Bucket* list_;
    Bucket::Iterator iter_;
//这里我们跟踪我们是否拥有列表。如果我们拥有它，我们也
//负责清洁。这是一个穷人的共同点
    bool own_list_;
    std::unique_ptr<Arena> arena_;
std::string tmp_;       //用于传递到encodekey
  };

  class DynamicIterator : public HashSkipListRep::Iterator {
   public:
    explicit DynamicIterator(const HashSkipListRep& memtable_rep)
      : HashSkipListRep::Iterator(nullptr, false),
        memtable_rep_(memtable_rep) {}

//使用大于等于target的键前进到第一个条目
    virtual void Seek(const Slice& k, const char* memtable_key) override {
      auto transformed = memtable_rep_.transform_->Transform(ExtractUserKey(k));
      Reset(memtable_rep_.GetBucket(transformed));
      HashSkipListRep::Iterator::Seek(k, memtable_key);
    }

//在集合中的第一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToFirst() override {
//前缀迭代器不支持总顺序。
//我们只是将迭代器设置为无效状态
      Reset(nullptr);
    }

//在集合中的最后一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToLast() override {
//前缀迭代器不支持总顺序。
//我们只是将迭代器设置为无效状态
      Reset(nullptr);
    }
   private:
//基础memtable
    const HashSkipListRep& memtable_rep_;
  };

  class EmptyIterator : public MemTableRep::Iterator {
//这是在没有桶的情况下使用的。它比
//实例化要在其上迭代的空桶。
   public:
    EmptyIterator() { }
    virtual bool Valid() const override { return false; }
    virtual const char* key() const override {
      assert(false);
      return nullptr;
    }
    virtual void Next() override {}
    virtual void Prev() override {}
    virtual void Seek(const Slice& internal_key,
                      const char* memtable_key) override {}
    virtual void SeekForPrev(const Slice& internal_key,
                             const char* memtable_key) override {}
    virtual void SeekToFirst() override {}
    virtual void SeekToLast() override {}

   private:
  };
};

HashSkipListRep::HashSkipListRep(const MemTableRep::KeyComparator& compare,
                                 Allocator* allocator,
                                 const SliceTransform* transform,
                                 size_t bucket_size, int32_t skiplist_height,
                                 int32_t skiplist_branching_factor)
    : MemTableRep(allocator),
      bucket_size_(bucket_size),
      skiplist_height_(skiplist_height),
      skiplist_branching_factor_(skiplist_branching_factor),
      transform_(transform),
      compare_(compare),
      allocator_(allocator) {
  auto mem = allocator->AllocateAligned(
               sizeof(std::atomic<void*>) * bucket_size);
  buckets_ = new (mem) std::atomic<Bucket*>[bucket_size];

  for (size_t i = 0; i < bucket_size_; ++i) {
    buckets_[i].store(nullptr, std::memory_order_relaxed);
  }
}

HashSkipListRep::~HashSkipListRep() {
}

HashSkipListRep::Bucket* HashSkipListRep::GetInitializedBucket(
    const Slice& transformed) {
  size_t hash = GetHash(transformed);
  auto bucket = GetBucket(hash);
  if (bucket == nullptr) {
    auto addr = allocator_->AllocateAligned(sizeof(Bucket));
    bucket = new (addr) Bucket(compare_, allocator_, skiplist_height_,
                               skiplist_branching_factor_);
    buckets_[hash].store(bucket, std::memory_order_release);
  }
  return bucket;
}

void HashSkipListRep::Insert(KeyHandle handle) {
  auto* key = static_cast<char*>(handle);
  assert(!Contains(key));
  auto transformed = transform_->Transform(UserKey(key));
  auto bucket = GetInitializedBucket(transformed);
  bucket->Insert(key);
}

bool HashSkipListRep::Contains(const char* key) const {
  auto transformed = transform_->Transform(UserKey(key));
  auto bucket = GetBucket(transformed);
  if (bucket == nullptr) {
    return false;
  }
  return bucket->Contains(key);
}

size_t HashSkipListRep::ApproximateMemoryUsage() {
  return 0;
}

void HashSkipListRep::Get(const LookupKey& k, void* callback_args,
                          bool (*callback_func)(void* arg, const char* entry)) {
  auto transformed = transform_->Transform(k.user_key());
  auto bucket = GetBucket(transformed);
  if (bucket != nullptr) {
    Bucket::Iterator iter(bucket);
    for (iter.Seek(k.memtable_key().data());
         iter.Valid() && callback_func(callback_args, iter.key());
         iter.Next()) {
    }
  }
}

MemTableRep::Iterator* HashSkipListRep::GetIterator(Arena* arena) {
//分配一个与当前使用的竞技场大小相似的新竞技场
  Arena* new_arena = new Arena(allocator_->BlockSize());
  auto list = new Bucket(compare_, new_arena);
  for (size_t i = 0; i < bucket_size_; ++i) {
    auto bucket = GetBucket(i);
    if (bucket != nullptr) {
      Bucket::Iterator itr(bucket);
      for (itr.SeekToFirst(); itr.Valid(); itr.Next()) {
        list->Insert(itr.key());
      }
    }
  }
  if (arena == nullptr) {
    return new Iterator(list, true, new_arena);
  } else {
    auto mem = arena->AllocateAligned(sizeof(Iterator));
    return new (mem) Iterator(list, true, new_arena);
  }
}

MemTableRep::Iterator* HashSkipListRep::GetDynamicPrefixIterator(Arena* arena) {
  if (arena == nullptr) {
    return new DynamicIterator(*this);
  } else {
    auto mem = arena->AllocateAligned(sizeof(DynamicIterator));
    return new (mem) DynamicIterator(*this);
  }
}

} //非命名空间

MemTableRep* HashSkipListRepFactory::CreateMemTableRep(
    const MemTableRep::KeyComparator& compare, Allocator* allocator,
    const SliceTransform* transform, Logger* logger) {
  return new HashSkipListRep(compare, allocator, transform, bucket_count_,
                             skiplist_height_, skiplist_branching_factor_);
}

MemTableRepFactory* NewHashSkipListRepFactory(
    size_t bucket_count, int32_t skiplist_height,
    int32_t skiplist_branching_factor) {
  return new HashSkipListRepFactory(bucket_count, skiplist_height,
      skiplist_branching_factor);
}

} //命名空间rocksdb
#endif  //摇滚乐
