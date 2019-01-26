
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
#include "memtable/hash_linklist_rep.h"

#include <algorithm>
#include <atomic>
#include "db/memtable.h"
#include "memtable/skiplist.h"
#include "monitoring/histogram.h"
#include "port/port.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/slice.h"
#include "rocksdb/slice_transform.h"
#include "util/arena.h"
#include "util/murmurhash.h"

namespace rocksdb {
namespace {

typedef const char* Key;
typedef SkipList<Key, const MemTableRep::KeyComparator&> MemtableSkipList;
typedef std::atomic<void*> Pointer;

//用作哈希桶链接列表的头的数据结构。
struct BucketHeader {
  Pointer next;
  std::atomic<uint32_t> num_entries;

  explicit BucketHeader(void* n, uint32_t count)
      : next(n), num_entries(count) {}

  bool IsSkipListBucket() {
    return next.load(std::memory_order_relaxed) == this;
  }

  uint32_t GetNumEntries() const {
    return num_entries.load(std::memory_order_relaxed);
  }

//要求：从单线程插入（）调用
  void IncNumEntries() {
//一次只能有一个线程执行写操作。不需要做原子
//增量。用轻松的加载和存储更新它。
    num_entries.store(GetNumEntries() + 1, std::memory_order_relaxed);
  }
};

//用作哈希桶的跳过列表的头的数据结构。
struct SkipListBucketHeader {
  BucketHeader Counting_header;
  MemtableSkipList skip_list;

  explicit SkipListBucketHeader(const MemTableRep::KeyComparator& cmp,
                                Allocator* allocator, uint32_t count)
: Counting_header(this,  //指向自身以指示标题类型。
                        count),
        skip_list(cmp, allocator) {}
};

struct Node {
//链接的访问器/转换器。用方法包装，这样我们可以
//必要时添加适当的屏障。
  Node* Next() {
//使用“获取加载”，以便我们观察完全初始化的
//返回节点的版本。
    return next_.load(std::memory_order_acquire);
  }
  void SetNext(Node* x) {
//使用“发布商店”，以便任何阅读此内容的人
//指针观察插入节点的完全初始化版本。
    next_.store(x, std::memory_order_release);
  }
//在一些地方没有可以安全使用的屏障变体。
  Node* NoBarrier_Next() {
    return next_.load(std::memory_order_relaxed);
  }

  void NoBarrier_SetNext(Node* x) { next_.store(x, std::memory_order_relaxed); }

//需要放置新的下方，这很好
  Node() {}

 private:
  std::atomic<Node*> next_;

//由于以下原因，禁止复制
  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;

 public:
  char key[1];
};

//MEM表的内存结构：
//它是一个哈希表，每个bucket指向一个条目、链接列表或
//跳过列表。以便跟踪存储桶中的记录总数以确定
//是否应切换到跳过列表，只添加一个标题以指示
//存储桶中的条目数。
//
//
//+----->空情况1。空桶
//γ
//γ
//+——————————+————+
//next+-->空
//+------+
//+-----+案例2。桶中有一个入口。
//+-+数据下一个指针指向
//+-----+空。所有其他病例
//下一个指针不为空。
//+-----++------+
//＋＋＋
//+-----++->+-----++>+-----++-+->+-----+
//next+——+next+——+next+-->空
//+——+——+——+——+——+——+——++——++——+——+
//+-----+计数
//+-----++------+数据数据
//__
//+-----+案例3。_
//A头+------+++------+
//+-----+指向
//链表。count表示总数
//+-----+此存储桶中的行。
//γ
//+-----++->+------+<-+
//__next+---+
//+-----++-----+案例4。标题指向跳过
//+----+计数列表和下一个指针指向
//+-----++-----+本身，用于区分大小写3或4。
//________
//+-----+跳过桶中用于调试的条目的+-->
//列出数据用途。
//+-->
//+-----+
//+------+
//+----+
//
//我们在更改案例时没有数据竞争，因为：
//（1）当从案例2->3更改时，我们创建一个新的桶头，将
//在不更改原始节点的情况下，先执行单个节点，然后执行
//更换桶指针时释放存储。在这种情况下，读者
//看到桶指针的过时值的人将读取此节点，而
//由于版本存储，读者可以看到正确的值。
//（2）更改案例3->4时，会创建一个新的标题，其中包含跳过列表点。
//在进行获取存储以更改bucket指针之前，返回到数据。
//旧的头和节点永远不会更改，因此任何读卡器都会看到
//其中的现有指针将保证能够迭代到
//链接列表的结尾。
//（3）第3种情况下，标题的下一个指针可能会改变，但它们永远不会相等。
//因此，无论读者看到任何过时或更新的值，它都会
//能够正确区分案例3和4。
//
//我们使用案例2的原因是我们希望使格式更有效
//当桶的利用率相对较低时。如果我们使用案例3
//单入口桶，我们需要为每个入口浪费12个字节，
//这可能会显著降低内存利用率。
class HashLinkListRep : public MemTableRep {
 public:
  HashLinkListRep(const MemTableRep::KeyComparator& compare,
                  Allocator* allocator, const SliceTransform* transform,
                  size_t bucket_size, uint32_t threshold_use_skiplist,
                  size_t huge_page_tlb_size, Logger* logger,
                  int bucket_entries_logging_threshold,
                  bool if_log_bucket_dist_when_flash);

  virtual KeyHandle Allocate(const size_t len, char** buf) override;

  virtual void Insert(KeyHandle handle) override;

  virtual bool Contains(const char* key) const override;

  virtual size_t ApproximateMemoryUsage() override;

  virtual void Get(const LookupKey& k, void* callback_args,
                   bool (*callback_func)(void* arg,
                                         const char* entry)) override;

  virtual ~HashLinkListRep();

  virtual MemTableRep::Iterator* GetIterator(Arena* arena = nullptr) override;

  virtual MemTableRep::Iterator* GetDynamicPrefixIterator(
       Arena* arena = nullptr) override;

 private:
  friend class DynamicIterator;

  size_t bucket_size_;

//将切片（转换为用户键）映射到键共享桶
//同样的转变。
  Pointer* buckets_;

  const uint32_t threshold_use_skiplist_;

//用户提供的转换，其域是用户密钥。
  const SliceTransform* transform_;

  const MemTableRep::KeyComparator& compare_;

  Logger* logger_;
  int bucket_entries_logging_threshold_;
  bool if_log_bucket_dist_when_flash_;

  bool LinkListContains(Node* head, const Slice& key) const;

  SkipListBucketHeader* GetSkipListBucketHeader(Pointer* first_next_pointer)
      const;

  Node* GetLinkListFirstNode(Pointer* first_next_pointer) const;

  Slice GetPrefix(const Slice& internal_key) const {
    return transform_->Transform(ExtractUserKey(internal_key));
  }

  size_t GetHash(const Slice& slice) const {
    return MurmurHash(slice.data(), static_cast<int>(slice.size()), 0) %
           bucket_size_;
  }

  Pointer* GetBucket(size_t i) const {
    return static_cast<Pointer*>(buckets_[i].load(std::memory_order_acquire));
  }

  Pointer* GetBucket(const Slice& slice) const {
    return GetBucket(GetHash(slice));
  }

  bool Equal(const Slice& a, const Key& b) const {
    return (compare_(b, a) == 0);
  }

  bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }

  bool KeyIsAfterNode(const Slice& internal_key, const Node* n) const {
//nullptr n被认为是无限的
    return (n != nullptr) && (compare_(n->key, internal_key) < 0);
  }

  bool KeyIsAfterNode(const Key& key, const Node* n) const {
//nullptr n被认为是无限的
    return (n != nullptr) && (compare_(n->key, key) < 0);
  }

  bool KeyIsAfterOrAtNode(const Slice& internal_key, const Node* n) const {
//nullptr n被认为是无限的
    return (n != nullptr) && (compare_(n->key, internal_key) <= 0);
  }

  bool KeyIsAfterOrAtNode(const Key& key, const Node* n) const {
//nullptr n被认为是无限的
    return (n != nullptr) && (compare_(n->key, key) <= 0);
  }

  Node* FindGreaterOrEqualInBucket(Node* head, const Slice& key) const;
  Node* FindLessOrEqualInBucket(Node* head, const Slice& key) const;

  class FullListIterator : public MemTableRep::Iterator {
   public:
    explicit FullListIterator(MemtableSkipList* list, Allocator* allocator)
        : iter_(list), full_list_(list), allocator_(allocator) {}

    virtual ~FullListIterator() {
    }

//如果迭代器位于有效节点，则返回true。
    virtual bool Valid() const override { return iter_.Valid(); }

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
      const char* encoded_key =
          (memtable_key != nullptr) ?
              memtable_key : EncodeKey(&tmp_, internal_key);
      iter_.Seek(encoded_key);
    }

//使用键<=target返回到最后一个条目
    virtual void SeekForPrev(const Slice& internal_key,
                             const char* memtable_key) override {
      const char* encoded_key = (memtable_key != nullptr)
                                    ? memtable_key
                                    : EncodeKey(&tmp_, internal_key);
      iter_.SeekForPrev(encoded_key);
    }

//在集合中的第一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToFirst() override { iter_.SeekToFirst(); }

//在集合中的最后一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToLast() override { iter_.SeekToLast(); }
   private:
    MemtableSkipList::Iterator iter_;
//使用迭代器进行析构函数。
    std::unique_ptr<MemtableSkipList> full_list_;
    std::unique_ptr<Allocator> allocator_;
std::string tmp_;       //用于传递到encodekey
  };

  class LinkListIterator : public MemTableRep::Iterator {
   public:
    explicit LinkListIterator(const HashLinkListRep* const hash_link_list_rep,
                              Node* head)
        : hash_link_list_rep_(hash_link_list_rep),
          head_(head),
          node_(nullptr) {}

    virtual ~LinkListIterator() {}

//如果迭代器位于有效节点，则返回true。
    virtual bool Valid() const override { return node_ != nullptr; }

//返回当前位置的键。
//要求：有效（）
    virtual const char* key() const override {
      assert(Valid());
      return node_->key;
    }

//前进到下一个位置。
//要求：有效（）
    virtual void Next() override {
      assert(Valid());
      node_ = node_->Next();
    }

//前进到上一个位置。
//要求：有效（）
    virtual void Prev() override {
//前缀迭代器不支持总顺序。
//我们只是将迭代器设置为无效状态
      Reset(nullptr);
    }

//使用大于等于target的键前进到第一个条目
    virtual void Seek(const Slice& internal_key,
                      const char* memtable_key) override {
      node_ = hash_link_list_rep_->FindGreaterOrEqualInBucket(head_,
                                                              internal_key);
    }

//使用键<=target返回到最后一个条目
    virtual void SeekForPrev(const Slice& internal_key,
                             const char* memtable_key) override {
//因为我们不支持prev（）。
//我们不支持SeekForRev
      Reset(nullptr);
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

   protected:
    void Reset(Node* head) {
      head_ = head;
      node_ = nullptr;
    }
   private:
    friend class HashLinkListRep;
    const HashLinkListRep* const hash_link_list_rep_;
    Node* head_;
    Node* node_;

    virtual void SeekToHead() {
      node_ = head_;
    }
  };

  class DynamicIterator : public HashLinkListRep::LinkListIterator {
   public:
    explicit DynamicIterator(HashLinkListRep& memtable_rep)
        : HashLinkListRep::LinkListIterator(&memtable_rep, nullptr),
          memtable_rep_(memtable_rep) {}

//使用大于等于target的键前进到第一个条目
    virtual void Seek(const Slice& k, const char* memtable_key) override {
      auto transformed = memtable_rep_.GetPrefix(k);
      auto* bucket = memtable_rep_.GetBucket(transformed);

      SkipListBucketHeader* skip_list_header =
          memtable_rep_.GetSkipListBucketHeader(bucket);
      if (skip_list_header != nullptr) {
//该存储桶被组织为跳过列表
        if (!skip_list_iter_) {
          skip_list_iter_.reset(
              new MemtableSkipList::Iterator(&skip_list_header->skip_list));
        } else {
          skip_list_iter_->SetList(&skip_list_header->skip_list);
        }
        if (memtable_key != nullptr) {
          skip_list_iter_->Seek(memtable_key);
        } else {
          IterKey encoded_key;
          encoded_key.EncodeLengthPrefixedKey(k);
          skip_list_iter_->Seek(encoded_key.GetUserKey().data());
        }
      } else {
//存储桶被组织为一个链接列表
        skip_list_iter_.reset();
        Reset(memtable_rep_.GetLinkListFirstNode(bucket));
        HashLinkListRep::LinkListIterator::Seek(k, memtable_key);
      }
    }

    virtual bool Valid() const override {
      if (skip_list_iter_) {
        return skip_list_iter_->Valid();
      }
      return HashLinkListRep::LinkListIterator::Valid();
    }

    virtual const char* key() const override {
      if (skip_list_iter_) {
        return skip_list_iter_->key();
      }
      return HashLinkListRep::LinkListIterator::key();
    }

    virtual void Next() override {
      if (skip_list_iter_) {
        skip_list_iter_->Next();
      } else {
        HashLinkListRep::LinkListIterator::Next();
      }
    }

   private:
//基础memtable
    const HashLinkListRep& memtable_rep_;
    std::unique_ptr<MemtableSkipList::Iterator> skip_list_iter_;
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
    virtual void Seek(const Slice& user_key,
                      const char* memtable_key) override {}
    virtual void SeekForPrev(const Slice& user_key,
                             const char* memtable_key) override {}
    virtual void SeekToFirst() override {}
    virtual void SeekToLast() override {}

   private:
  };
};

HashLinkListRep::HashLinkListRep(
    const MemTableRep::KeyComparator& compare, Allocator* allocator,
    const SliceTransform* transform, size_t bucket_size,
    uint32_t threshold_use_skiplist, size_t huge_page_tlb_size, Logger* logger,
    int bucket_entries_logging_threshold, bool if_log_bucket_dist_when_flash)
    : MemTableRep(allocator),
      bucket_size_(bucket_size),
//如果小于3，则使用跳过列表的阈值没有意义，因此我们
//强制它至少为3以简化实现。
      threshold_use_skiplist_(std::max(threshold_use_skiplist, 3U)),
      transform_(transform),
      compare_(compare),
      logger_(logger),
      bucket_entries_logging_threshold_(bucket_entries_logging_threshold),
      if_log_bucket_dist_when_flash_(if_log_bucket_dist_when_flash) {
  char* mem = allocator_->AllocateAligned(sizeof(Pointer) * bucket_size,
                                      huge_page_tlb_size, logger);

  buckets_ = new (mem) Pointer[bucket_size];

  for (size_t i = 0; i < bucket_size_; ++i) {
    buckets_[i].store(nullptr, std::memory_order_relaxed);
  }
}

HashLinkListRep::~HashLinkListRep() {
}

KeyHandle HashLinkListRep::Allocate(const size_t len, char** buf) {
  char* mem = allocator_->AllocateAligned(sizeof(Node) + len);
  Node* x = new (mem) Node();
  *buf = x->key;
  return static_cast<void*>(x);
}

SkipListBucketHeader* HashLinkListRep::GetSkipListBucketHeader(
    Pointer* first_next_pointer) const {
  if (first_next_pointer == nullptr) {
    return nullptr;
  }
  if (first_next_pointer->load(std::memory_order_relaxed) == nullptr) {
//单进斗
    return nullptr;
  }
//计数头
  BucketHeader* header = reinterpret_cast<BucketHeader*>(first_next_pointer);
  if (header->IsSkipListBucket()) {
    assert(header->GetNumEntries() > threshold_use_skiplist_);
    auto* skip_list_bucket_header =
        reinterpret_cast<SkipListBucketHeader*>(header);
    assert(skip_list_bucket_header->Counting_header.next.load(
               std::memory_order_relaxed) == header);
    return skip_list_bucket_header;
  }
  assert(header->GetNumEntries() <= threshold_use_skiplist_);
  return nullptr;
}

Node* HashLinkListRep::GetLinkListFirstNode(Pointer* first_next_pointer) const {
  if (first_next_pointer == nullptr) {
    return nullptr;
  }
  if (first_next_pointer->load(std::memory_order_relaxed) == nullptr) {
//单进斗
    return reinterpret_cast<Node*>(first_next_pointer);
  }
//计数头
  BucketHeader* header = reinterpret_cast<BucketHeader*>(first_next_pointer);
  if (!header->IsSkipListBucket()) {
    assert(header->GetNumEntries() <= threshold_use_skiplist_);
    return reinterpret_cast<Node*>(
        header->next.load(std::memory_order_acquire));
  }
  assert(header->GetNumEntries() > threshold_use_skiplist_);
  return nullptr;
}

void HashLinkListRep::Insert(KeyHandle handle) {
  Node* x = static_cast<Node*>(handle);
  assert(!Contains(x->key));
  Slice internal_key = GetLengthPrefixedSlice(x->key);
  auto transformed = GetPrefix(internal_key);
  auto& bucket = buckets_[GetHash(transformed)];
  Pointer* first_next_pointer =
      static_cast<Pointer*>(bucket.load(std::memory_order_relaxed));

  if (first_next_pointer == nullptr) {
//案例1。空桶
//nobarrier_setnext（）足够了，因为我们将在
//我们在前面[i]中发布了一个指向“x”的指针。
    x->NoBarrier_SetNext(nullptr);
    bucket.store(x, std::memory_order_release);
    return;
  }

  BucketHeader* header = nullptr;
  if (first_next_pointer->load(std::memory_order_relaxed) == nullptr) {
//案例2。桶里只有一个入口
//需要转换为计数桶并转到案例4。
    Node* first = reinterpret_cast<Node*>(first_next_pointer);
//需要添加桶头。
//我们必须先把它转换成一个有头的桶，然后再插入
//新节点。否则，我们可能需要更改first的下一个指针。
//在这种情况下，读者可能会看到下一个指针是空的，并且是错误的。
//认为节点是一个桶头。
    auto* mem = allocator_->AllocateAligned(sizeof(BucketHeader));
    header = new (mem) BucketHeader(first, 1);
    bucket.store(header, std::memory_order_release);
  } else {
    header = reinterpret_cast<BucketHeader*>(first_next_pointer);
    if (header->IsSkipListBucket()) {
//案例4。bucket已经是跳过列表
      assert(header->GetNumEntries() > threshold_use_skiplist_);
      auto* skip_list_bucket_header =
          reinterpret_cast<SkipListBucketHeader*>(header);
//一次只能有一个线程执行insert（）。不需要做原子
//增量。
      skip_list_bucket_header->Counting_header.IncNumEntries();
      skip_list_bucket_header->skip_list.Insert(x->key);
      return;
    }
  }

  if (bucket_entries_logging_threshold_ > 0 &&
      header->GetNumEntries() ==
          static_cast<uint32_t>(bucket_entries_logging_threshold_)) {
    Info(logger_, "HashLinkedList bucket %" ROCKSDB_PRIszt
                  " has more than %d "
                  "entries. Key to insert: %s",
         GetHash(transformed), header->GetNumEntries(),
         GetLengthPrefixedSlice(x->key).ToString(true).c_str());
  }

  if (header->GetNumEntries() == threshold_use_skiplist_) {
//案例3。条目数达到阈值，因此需要转换为
//跳过列表。
    LinkListIterator bucket_iter(
        this, reinterpret_cast<Node*>(
                  first_next_pointer->load(std::memory_order_relaxed)));
    auto mem = allocator_->AllocateAligned(sizeof(SkipListBucketHeader));
    SkipListBucketHeader* new_skip_list_header = new (mem)
        SkipListBucketHeader(compare_, allocator_, header->GetNumEntries() + 1);
    auto& skip_list = new_skip_list_header->skip_list;

//将所有当前条目添加到跳过列表
    for (bucket_iter.SeekToHead(); bucket_iter.Valid(); bucket_iter.Next()) {
      skip_list.Insert(bucket_iter.key());
    }

//插入新条目
    skip_list.Insert(x->key);
//设置桶
    bucket.store(new_skip_list_header, std::memory_order_release);
  } else {
//案例5。需要插入到已排序的链接列表而不更改
//标题。
    Node* first =
        reinterpret_cast<Node*>(header->next.load(std::memory_order_relaxed));
    assert(first != nullptr);
//除非bucket需要提前跳过列表，否则提前计数器。
//在这种情况下，我们需要确保以前的计数永远不会超过
//阈值\使用\u skiplist \避免读者转换为错误的格式。
    header->IncNumEntries();

    Node* cur = first;
    Node* prev = nullptr;
    while (true) {
      if (cur == nullptr) {
        break;
      }
      Node* next = cur->Next();
//确保列表已排序。
//如果x指向head_u或next points nullptr，则非常满意。
      assert((cur == first) || (next == nullptr) ||
             KeyIsAfterNode(next->key, cur));
      if (KeyIsAfterNode(internal_key, cur)) {
//继续在此列表中搜索
        prev = cur;
        cur = next;
      } else {
        break;
      }
    }

//我们的数据结构不允许重复插入
    assert(cur == nullptr || !Equal(x->key, cur->key));

//nobarrier_setnext（）足够了，因为我们将在
//我们在前面[i]中发布了一个指向“x”的指针。
    x->NoBarrier_SetNext(cur);

    if (prev) {
      prev->SetNext(x);
    } else {
      header->next.store(static_cast<void*>(x), std::memory_order_release);
    }
  }
}

bool HashLinkListRep::Contains(const char* key) const {
  Slice internal_key = GetLengthPrefixedSlice(key);

  auto transformed = GetPrefix(internal_key);
  auto bucket = GetBucket(transformed);
  if (bucket == nullptr) {
    return false;
  }

  SkipListBucketHeader* skip_list_header = GetSkipListBucketHeader(bucket);
  if (skip_list_header != nullptr) {
    return skip_list_header->skip_list.Contains(key);
  } else {
    return LinkListContains(GetLinkListFirstNode(bucket), internal_key);
  }
}

size_t HashLinkListRep::ApproximateMemoryUsage() {
//内存总是从分配器分配的。
  return 0;
}

void HashLinkListRep::Get(const LookupKey& k, void* callback_args,
                          bool (*callback_func)(void* arg, const char* entry)) {
  auto transformed = transform_->Transform(k.user_key());
  auto bucket = GetBucket(transformed);

  auto* skip_list_header = GetSkipListBucketHeader(bucket);
  if (skip_list_header != nullptr) {
//是跳过列表
    MemtableSkipList::Iterator iter(&skip_list_header->skip_list);
    for (iter.Seek(k.memtable_key().data());
         iter.Valid() && callback_func(callback_args, iter.key());
         iter.Next()) {
    }
  } else {
    auto* link_list_head = GetLinkListFirstNode(bucket);
    if (link_list_head != nullptr) {
      LinkListIterator iter(this, link_list_head);
      for (iter.Seek(k.internal_key(), nullptr);
           iter.Valid() && callback_func(callback_args, iter.key());
           iter.Next()) {
      }
    }
  }
}

MemTableRep::Iterator* HashLinkListRep::GetIterator(Arena* alloc_arena) {
//分配一个与当前使用的竞技场大小相似的新竞技场
  Arena* new_arena = new Arena(allocator_->BlockSize());
  auto list = new MemtableSkipList(compare_, new_arena);
  HistogramImpl keys_per_bucket_hist;

  for (size_t i = 0; i < bucket_size_; ++i) {
    int count = 0;
    auto* bucket = GetBucket(i);
    if (bucket != nullptr) {
      auto* skip_list_header = GetSkipListBucketHeader(bucket);
      if (skip_list_header != nullptr) {
//是跳过列表
        MemtableSkipList::Iterator itr(&skip_list_header->skip_list);
        for (itr.SeekToFirst(); itr.Valid(); itr.Next()) {
          list->Insert(itr.key());
          count++;
        }
      } else {
        auto* link_list_head = GetLinkListFirstNode(bucket);
        if (link_list_head != nullptr) {
          LinkListIterator itr(this, link_list_head);
          for (itr.SeekToHead(); itr.Valid(); itr.Next()) {
            list->Insert(itr.key());
            count++;
          }
        }
      }
    }
    if (if_log_bucket_dist_when_flash_) {
      keys_per_bucket_hist.Add(count);
    }
  }
  if (if_log_bucket_dist_when_flash_ && logger_ != nullptr) {
    Info(logger_, "hashLinkedList Entry distribution among buckets: %s",
         keys_per_bucket_hist.ToString().c_str());
  }

  if (alloc_arena == nullptr) {
    return new FullListIterator(list, new_arena);
  } else {
    auto mem = alloc_arena->AllocateAligned(sizeof(FullListIterator));
    return new (mem) FullListIterator(list, new_arena);
  }
}

MemTableRep::Iterator* HashLinkListRep::GetDynamicPrefixIterator(
    Arena* alloc_arena) {
  if (alloc_arena == nullptr) {
    return new DynamicIterator(*this);
  } else {
    auto mem = alloc_arena->AllocateAligned(sizeof(DynamicIterator));
    return new (mem) DynamicIterator(*this);
  }
}

bool HashLinkListRep::LinkListContains(Node* head,
                                       const Slice& user_key) const {
  Node* x = FindGreaterOrEqualInBucket(head, user_key);
  return (x != nullptr && Equal(user_key, x->key));
}

Node* HashLinkListRep::FindGreaterOrEqualInBucket(Node* head,
                                                  const Slice& key) const {
  Node* x = head;
  while (true) {
    if (x == nullptr) {
      return x;
    }
    Node* next = x->Next();
//确保列表已排序。
//如果x指向head_u或next points nullptr，则非常满意。
    assert((x == head) || (next == nullptr) || KeyIsAfterNode(next->key, x));
    if (KeyIsAfterNode(key, x)) {
//继续在此列表中搜索
      x = next;
    } else {
      break;
    }
  }
  return x;
}

} //非命名空间

MemTableRep* HashLinkListRepFactory::CreateMemTableRep(
    const MemTableRep::KeyComparator& compare, Allocator* allocator,
    const SliceTransform* transform, Logger* logger) {
  return new HashLinkListRep(compare, allocator, transform, bucket_count_,
                             threshold_use_skiplist_, huge_page_tlb_size_,
                             logger, bucket_entries_logging_threshold_,
                             if_log_bucket_dist_when_flash_);
}

MemTableRepFactory* NewHashLinkListRepFactory(
    size_t bucket_count, size_t huge_page_tlb_size,
    int bucket_entries_logging_threshold, bool if_log_bucket_dist_when_flash,
    uint32_t threshold_use_skiplist) {
  return new HashLinkListRepFactory(
      bucket_count, threshold_use_skiplist, huge_page_tlb_size,
      bucket_entries_logging_threshold, if_log_bucket_dist_when_flash);
}

} //命名空间rocksdb
#endif  //摇滚乐
