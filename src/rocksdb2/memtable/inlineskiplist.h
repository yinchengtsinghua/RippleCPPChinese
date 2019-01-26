
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
//版权所有（c）2011 LevelDB作者。版权所有。使用
//此源代码由可以找到的BSD样式许可证管理
//在许可证文件中。有关参与者的名称，请参阅作者文件。
//
//inlineskiplist是从skiplist（skiplist.h）派生的，但它优化了
//内存布局，要求通过
//跳过列表实例。对于skiplist<const char*的常见情况，
//cmp>这会为每个跳过列表节点保存一个指针，并提供更好的缓存。
//以使用allocateAligned浪费填充为代价的位置
//而不是为密钥分配。未使用的填充将来自
//0到sizeof（void*）-1字节，节省的空间为sizeof（void*）
//字节，因此尽管填充了
//skiplist<const char*，…>。
//
//螺纹安全----------
//
//通过插入写入需要外部同步，很可能是互斥体。
//insertConcurrently可以与reads和
//与其他并发插入。阅读要求保证
//在读取过程中，InlineSkipList不会被销毁。
//除此之外，在没有任何内部锁定或
//同步。
//
//Invariants：
//
//（1）在inlineskiplist
//摧毁。因为我们从来没有
//删除所有跳过列表节点。
//
//（2）除了下一个/上一个指针外，节点的内容是
//在将节点链接到inlineskiplist之后是不可变的。
//只有insert（）修改列表，并小心初始化
//节点和使用发布存储在一个或多个列表中发布节点。
//
//…上一个与下一个指针排序…
//

#pragma once
#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <atomic>
#include "port/port.h"
#include "util/allocator.h"
#include "util/random.h"

namespace rocksdb {

template <class Comparator>
class InlineSkipList {
 private:
  struct Node;
  struct Splice;

 public:
  static const uint16_t kMaxPossibleHeight = 32;

//创建一个新的inlineskiplist对象，该对象将使用“cmp”进行比较
//键，并将使用“*分配器”分配内存。分配的对象
//在分配程序中，必须在
//Skiplist对象。
  explicit InlineSkipList(Comparator cmp, Allocator* allocator,
                          int32_t max_height = 12,
                          int32_t branching_factor = 4);

//分配键和跳过列表节点，返回指向键的指针
//节点的一部分。如果分配器
//线程安全。
  char* AllocateKey(size_t key_size);

//使用分配器分配接头。
  Splice* AllocateSplice();

//在实际键值之后插入由allocatekey分配的键
//已填写。
//
//要求：列表中当前没有与键比较的内容。
//要求：没有对任何插入的并发调用。
  void Insert(const char* key);

//插入由allocatekey分配的带有最后一次插入提示的键
//在跳过列表中的位置。如果提示指向nullptr，则新提示将
//已填充，可在后续调用中使用。
//
//它可以用于优化多个组的工作负载
//，每个键都可能插入到靠近最后一个键的位置
//在同一组中插入了键。一个例子是顺序插入。
//
//要求：列表中当前没有与键比较的内容。
//要求：没有对任何插入的并发调用。
  void InsertWithHint(const char* key, void** hint);

//与insert类似，但不需要外部同步。
  void InsertConcurrently(const char* key);

//将节点插入到跳过列表中。密钥必须由分配
//分配键，然后由调用方填写。如果usecas是真的，
//则不需要外部同步，否则此方法
//不能与任何其他插入同时调用。
//
//不管usecas是否为真，拼接必须属于
//由当前线程独占。如果允许部分拼接，则修复为
//是的，则插入成本摊销为o（log d），其中d是
//从接头到插入键的距离（以
//中间节点的数目）。请注意，这个界限非常适合
//连续插入！如果允许部分拼接修复为假，则
//除非当前键正在
//拼接后立即插入。允许部分拼接修复=
//对于非顺序的情况o（log n），false的运行时间更差，
//但一个更好的常数因子。
  template <bool UseCAS>
  void Insert(const char* key, Splice* splice, bool allow_partial_splice_fix);

//返回true iff列表中比较等于key的条目。
  bool Contains(const char* key) const;

//返回小于“key”的估计条目数。
  uint64_t EstimateCount(const char* key) const;

//验证跳过列表的正确性。
  void TEST_Validate() const;

//跳过列表内容的迭代
  class Iterator {
   public:
//在指定列表上初始化迭代器。
//返回的迭代器无效。
    explicit Iterator(const InlineSkipList* list);

//更改用于此迭代器的基础skiplist
//这使我们能够在不解除分配的情况下不更改迭代器。
//一个旧的，然后分配一个新的
    void SetList(const InlineSkipList* list);

//如果迭代器位于有效节点，则返回true。
    bool Valid() const;

//返回当前位置的键。
//要求：有效（）
    const char* key() const;

//前进到下一个位置。
//要求：有效（）
    void Next();

//前进到上一个位置。
//要求：有效（）
    void Prev();

//使用大于等于target的键前进到第一个条目
    void Seek(const char* target);

//使用键<=target返回到最后一个条目
    void SeekForPrev(const char* target);

//在列表中的第一个条目处定位。
//迭代器的最终状态为valid（）iff list不为空。
    void SeekToFirst();

//在列表中的最后一个条目处定位。
//迭代器的最终状态为valid（）iff list不为空。
    void SeekToLast();

   private:
    const InlineSkipList* list_;
    Node* node_;
//有意复制
  };

 private:
  const uint16_t kMaxHeight_;
  const uint16_t kBranching_;
  const uint32_t kScaledInverseBranching_;

//构造后不变
  Comparator const compare_;
Allocator* const allocator_;  //用于节点分配的分配器

  Node* const head_;

//仅由insert（）修改。被读者种族歧视，但陈旧
//值是可以的。
std::atomic<int> max_height_;  //整个列表的高度

//Seq_Splice_u是用于非并发插入的拼接
//案例。它缓存最近一次发现的上一个和下一个
//非并发插入。
  Splice* seq_splice_;

  inline int GetMaxHeight() const {
    return max_height_.load(std::memory_order_relaxed);
  }

  int RandomHeight();

  Node* AllocateNode(size_t key_size, int height);

  bool Equal(const char* a, const char* b) const {
    return (compare_(a, b) == 0);
  }

  bool LessThan(const char* a, const char* b) const {
    return (compare_(a, b) < 0);
  }

//如果键大于“n”中存储的数据，则返回“真”。零n
//被认为是无限的。N不应该是头。
  bool KeyIsAfterNode(const char* key, Node* n) const;

//返回键大于等于键的最早节点。
//如果没有此类节点，则返回nullptr。
  Node* FindGreaterOrEqual(const char* key) const;

//返回键<键的最新节点。
//如果没有这样的节点，返回head。
//用指向“level”上一个节点的指针为上一个[级别]填充
//如果prev为非空，则级别为[0..max_height_uu1]。
  Node* FindLessThan(const char* key, Node** prev = nullptr) const;

//返回最新的节点，并在底层使用键<键。开始搜索
//从顶层以下的根节点开始。
//用指向“level”上一个节点的指针为上一个[级别]填充
//如果prev为非空，则为[底部\级别..顶部\级别1]中的级别。
  Node* FindLessThan(const char* key, Node** prev, Node* root, int top_level,
                     int bottom_level) const;

//返回列表中的最后一个节点。
//如果列表为空，则返回head。
  Node* FindLast() const;

//遍历列表的单个级别，将上一级设置为最后一级
//键前的节点和键后的第一个节点旁的*出。假设
//该键不在跳过列表中。入职前，应
//指向键之前的节点，键之后应指向
//位于键之后的节点。如果一个好的after，则after应为nullptr
//节点不方便使用。
  void FindSpliceForLevel(const char* key, Node* before, Node* after, int level,
                          Node** out_prev, Node** out_next);

//将拼接级别从最高级别（含）重新计算到
//最低级别（含）。
  void RecomputeSpliceLevels(const char* key, Splice* splice,
                             int recompute_level);

//不允许复制
  InlineSkipList(const InlineSkipList&);
  InlineSkipList& operator=(const InlineSkipList&);
};

//实施细节如下

template <class Comparator>
struct InlineSkipList<Comparator>::Splice {
//拼接的不变量是prev_u[i+1].key<=prev_[i].key<
//next_[i].key<=next_[i+1].key代表所有i。这意味着如果a
//键由上一个和下一个括号括起来，然后用括号括起来
//所有更高级别。不要求上一个下一个（i）=
//下一步（可能是在过去的某个时候，但在干预中）
//或者并发操作可能在中间插入了节点）。
  int height_ = 0;
  Node** prev_;
  Node** next_;
};

//节点数据类型更像是指向自定义托管内存的指针，而不是
//一个传统的C++结构。密钥立即存储在字节中
//结构之后，高度大于1的节点的下一个指针是
//立即存储在结构之前。这避免了需要包括
//减少每个节点内存开销的任何指针或大小数据。
template <class Comparator>
struct InlineSkipList<Comparator>::Node {
//将节点的高度存储在通常用于
//NEXTY〔0〕。这用于将数据从allocatekey传递到insert。
  void StashHeight(const int height) {
    assert(sizeof(int) <= sizeof(next_[0]));
    memcpy(&next_[0], &height, sizeof(int));
  }

//检索传递给stashheight的值。调用后未定义
//去设置下一个或无载波_设置下一个。
  int UnstashHeight() const {
    int rv;
    memcpy(&rv, &next_[0], sizeof(int));
    return rv;
  }

  const char* Key() const { return reinterpret_cast<const char*>(&next_[1]); }

//链接的访问器/转换器。包装在方法中以便我们可以添加
//必要时设置适当的屏障，并执行必要的
//将节点下的链接存储在内存中的寻址技巧。
  Node* Next(int n) {
    assert(n >= 0);
//使用“获取加载”，以便我们观察完全初始化的
//返回节点的版本。
    return (next_[-n].load(std::memory_order_acquire));
  }

  void SetNext(int n, Node* x) {
    assert(n >= 0);
//使用“发布商店”，以便任何阅读此内容的人
//指针观察插入节点的完全初始化版本。
    next_[-n].store(x, std::memory_order_release);
  }

  bool CASNext(int n, Node* expected, Node* x) {
    assert(n >= 0);
    return next_[-n].compare_exchange_strong(expected, x);
  }

//在一些地方没有可以安全使用的屏障变体。
  Node* NoBarrier_Next(int n) {
    assert(n >= 0);
    return next_[-n].load(std::memory_order_relaxed);
  }

  void NoBarrier_SetNext(int n, Node* x) {
    assert(n >= 0);
    next_[-n].store(x, std::memory_order_relaxed);
  }

//在特定级别的prev后面插入节点。
  void InsertAfter(Node* prev, int level) {
//nobarrier_setnext（）足够了，因为我们将在
//我们在前面发布了一个指向“this”的指针。
    NoBarrier_SetNext(level, prev->NoBarrier_Next(level));
    prev->SetNext(level, this);
  }

 private:
//下一个“0”是最低级别的链接（级别0）。更高的级别是
//先前存储了u，所以级别1位于下一个[-1]。
  std::atomic<Node*> next_[1];
};

template <class Comparator>
inline InlineSkipList<Comparator>::Iterator::Iterator(
    const InlineSkipList* list) {
  SetList(list);
}

template <class Comparator>
inline void InlineSkipList<Comparator>::Iterator::SetList(
    const InlineSkipList* list) {
  list_ = list;
  node_ = nullptr;
}

template <class Comparator>
inline bool InlineSkipList<Comparator>::Iterator::Valid() const {
  return node_ != nullptr;
}

template <class Comparator>
inline const char* InlineSkipList<Comparator>::Iterator::key() const {
  assert(Valid());
  return node_->Key();
}

template <class Comparator>
inline void InlineSkipList<Comparator>::Iterator::Next() {
  assert(Valid());
  node_ = node_->Next(0);
}

template <class Comparator>
inline void InlineSkipList<Comparator>::Iterator::Prev() {
//我们不使用显式的“prev”链接，只搜索
//落在键之前的最后一个节点。
  assert(Valid());
  node_ = list_->FindLessThan(node_->Key());
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template <class Comparator>
inline void InlineSkipList<Comparator>::Iterator::Seek(const char* target) {
  node_ = list_->FindGreaterOrEqual(target);
}

template <class Comparator>
inline void InlineSkipList<Comparator>::Iterator::SeekForPrev(
    const char* target) {
  Seek(target);
  if (!Valid()) {
    SeekToLast();
  }
  while (Valid() && list_->LessThan(target, key())) {
    Prev();
  }
}

template <class Comparator>
inline void InlineSkipList<Comparator>::Iterator::SeekToFirst() {
  node_ = list_->head_->Next(0);
}

template <class Comparator>
inline void InlineSkipList<Comparator>::Iterator::SeekToLast() {
  node_ = list_->FindLast();
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template <class Comparator>
int InlineSkipList<Comparator>::RandomHeight() {
  auto rnd = Random::GetTLSInstance();

//以Kbranching的概率1增加高度
  int height = 1;
  while (height < kMaxHeight_ && height < kMaxPossibleHeight &&
         rnd->Next() < kScaledInverseBranching_) {
    height++;
  }
  assert(height > 0);
  assert(height <= kMaxHeight_);
  assert(height <= kMaxPossibleHeight);
  return height;
}

template <class Comparator>
bool InlineSkipList<Comparator>::KeyIsAfterNode(const char* key,
                                                Node* n) const {
//nullptr n被认为是无限的
  assert(n != head_);
  return (n != nullptr) && (compare_(n->Key(), key) < 0);
}

template <class Comparator>
typename InlineSkipList<Comparator>::Node*
InlineSkipList<Comparator>::FindGreaterOrEqual(const char* key) const {
//注意：看起来我们可以通过实现
//此函数用作findlessthan（key）->next（0），但我们无法
//在平等的基础上提前退出，结果甚至都是不正确的。
//同时插入可能发生在findlessthan（key）之后，但发生在
//我们有机会打电话给下一个（0）。
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  Node* last_bigger = nullptr;
  while (true) {
    Node* next = x->Next(level);
//确保列表已排序
    assert(x == head_ || next == nullptr || KeyIsAfterNode(next->Key(), x));
//确保我们在搜索过程中没有超负荷
    assert(x == head_ || KeyIsAfterNode(key, x));
    int cmp = (next == nullptr || next == last_bigger)
                  ? 1
                  : compare_(next->Key(), key);
    if (cmp == 0 || (cmp > 0 && level == 0)) {
      return next;
    } else if (cmp < 0) {
//继续在此列表中搜索
      x = next;
    } else {
//切换到下一个列表，重新使用比较结果
      last_bigger = next;
      level--;
    }
  }
}

template <class Comparator>
typename InlineSkipList<Comparator>::Node*
InlineSkipList<Comparator>::FindLessThan(const char* key, Node** prev) const {
  return FindLessThan(key, prev, head_, GetMaxHeight(), 0);
}

template <class Comparator>
typename InlineSkipList<Comparator>::Node*
InlineSkipList<Comparator>::FindLessThan(const char* key, Node** prev,
                                         Node* root, int top_level,
                                         int bottom_level) const {
  assert(top_level > bottom_level);
  int level = top_level - 1;
  Node* x = root;
//key is after（key，last_not_after）绝对是错误的
  Node* last_not_after = nullptr;
  while (true) {
    Node* next = x->Next(level);
    assert(x == head_ || next == nullptr || KeyIsAfterNode(next->Key(), x));
    assert(x == head_ || KeyIsAfterNode(key, x));
    if (next != last_not_after && KeyIsAfterNode(key, next)) {
//继续在此列表中搜索
      x = next;
    } else {
      if (prev != nullptr) {
        prev[level] = x;
      }
      if (level == bottom_level) {
        return x;
      } else {
//切换到下一个列表，重用keyisafternode（）结果
        last_not_after = next;
        level--;
      }
    }
  }
}

template <class Comparator>
typename InlineSkipList<Comparator>::Node*
InlineSkipList<Comparator>::FindLast() const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    Node* next = x->Next(level);
    if (next == nullptr) {
      if (level == 0) {
        return x;
      } else {
//切换到下一个列表
        level--;
      }
    } else {
      x = next;
    }
  }
}

template <class Comparator>
uint64_t InlineSkipList<Comparator>::EstimateCount(const char* key) const {
  uint64_t count = 0;

  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    assert(x == head_ || compare_(x->Key(), key) < 0);
    Node* next = x->Next(level);
    if (next == nullptr || compare_(next->Key(), key) >= 0) {
      if (level == 0) {
        return count;
      } else {
//切换到下一个列表
        count *= kBranching_;
        level--;
      }
    } else {
      x = next;
      count++;
    }
  }
}

template <class Comparator>
InlineSkipList<Comparator>::InlineSkipList(const Comparator cmp,
                                           Allocator* allocator,
                                           int32_t max_height,
                                           int32_t branching_factor)
    : kMaxHeight_(max_height),
      kBranching_(branching_factor),
      kScaledInverseBranching_((Random::kMaxNext + 1) / kBranching_),
      compare_(cmp),
      allocator_(allocator),
      head_(AllocateNode(0, max_height)),
      max_height_(1),
      seq_splice_(AllocateSplice()) {
  assert(max_height > 0 && kMaxHeight_ == static_cast<uint32_t>(max_height));
  assert(branching_factor > 1 &&
         kBranching_ == static_cast<uint32_t>(branching_factor));
  assert(kScaledInverseBranching_ > 0);

  for (int i = 0; i < kMaxHeight_; ++i) {
    head_->SetNext(i, nullptr);
  }
}

template <class Comparator>
char* InlineSkipList<Comparator>::AllocateKey(size_t key_size) {
  return const_cast<char*>(AllocateNode(key_size, RandomHeight())->Key());
}

template <class Comparator>
typename InlineSkipList<Comparator>::Node*
InlineSkipList<Comparator>::AllocateNode(size_t key_size, int height) {
  auto prefix = sizeof(std::atomic<Node*>) * (height - 1);

//前缀是高度的空间-我们存储之前的1个指针
//节点实例（下一个）（高度-1）。- 1）。节点开始于
//raw+前缀，并保留底部模式（0级）跳过列表指针
//NEXTY〔0〕。key_size是键的字节，它在
//节点。
  char* raw = allocator_->AllocateAligned(prefix + sizeof(Node) + key_size);
  Node* x = reinterpret_cast<Node*>(raw + prefix);

//一旦我们将节点链接到跳过列表中，我们实际上不需要
//知道它的高度，因为我们可以隐式地使用
//遍历到H级的节点，以知道H是有效的级别
//对于那个节点。我们需要把高度传递到插入台阶上，
//但是，这样它就可以执行正确的链接。既然我们不是
//利用目前的指针，暂时借钱
//从下一个“0”开始存储。
  x->StashHeight(height);
  return x;
}

template <class Comparator>
typename InlineSkipList<Comparator>::Splice*
InlineSkipList<Comparator>::AllocateSplice() {
//上一个和下一个的大小
  size_t array_size = sizeof(Node*) * (kMaxHeight_ + 1);
  char* raw = allocator_->AllocateAligned(sizeof(Splice) + array_size * 2);
  Splice* splice = reinterpret_cast<Splice*>(raw);
  splice->height_ = 0;
  splice->prev_ = reinterpret_cast<Node**>(raw + sizeof(Splice));
  splice->next_ = reinterpret_cast<Node**>(raw + sizeof(Splice) + array_size);
  return splice;
}

template <class Comparator>
void InlineSkipList<Comparator>::Insert(const char* key) {
  Insert<false>(key, seq_splice_, false);
}

template <class Comparator>
void InlineSkipList<Comparator>::InsertConcurrently(const char* key) {
  Node* prev[kMaxPossibleHeight];
  Node* next[kMaxPossibleHeight];
  Splice splice;
  splice.prev_ = prev;
  splice.next_ = next;
  Insert<true>(key, &splice, false);
}

template <class Comparator>
void InlineSkipList<Comparator>::InsertWithHint(const char* key, void** hint) {
  assert(hint != nullptr);
  Splice* splice = reinterpret_cast<Splice*>(*hint);
  if (splice == nullptr) {
    splice = AllocateSplice();
    *hint = reinterpret_cast<void*>(splice);
  }
  Insert<false>(key, splice, true);
}

template <class Comparator>
void InlineSkipList<Comparator>::FindSpliceForLevel(const char* key,
                                                    Node* before, Node* after,
                                                    int level, Node** out_prev,
                                                    Node** out_next) {
  while (true) {
    Node* next = before->Next(level);
    assert(before == head_ || next == nullptr ||
           KeyIsAfterNode(next->Key(), before));
    assert(before == head_ || KeyIsAfterNode(key, before));
    if (next == after || !KeyIsAfterNode(key, next)) {
//找到它
      *out_prev = before;
      *out_next = next;
      return;
    }
    before = next;
  }
}

template <class Comparator>
void InlineSkipList<Comparator>::RecomputeSpliceLevels(const char* key,
                                                       Splice* splice,
                                                       int recompute_level) {
  assert(recompute_level > 0);
  assert(recompute_level <= splice->height_);
  for (int i = recompute_level - 1; i >= 0; --i) {
    FindSpliceForLevel(key, splice->prev_[i + 1], splice->next_[i + 1], i,
                       &splice->prev_[i], &splice->next_[i]);
  }
}

template <class Comparator>
template <bool UseCAS>
void InlineSkipList<Comparator>::Insert(const char* key, Splice* splice,
                                        bool allow_partial_splice_fix) {
  Node* x = reinterpret_cast<Node*>(const_cast<char*>(key)) - 1;
  int height = x->UnstashHeight();
  assert(height >= 1 && height <= kMaxHeight_);

  int max_height = max_height_.load(std::memory_order_relaxed);
  while (height > max_height) {
    if (max_height_.compare_exchange_weak(max_height, height)) {
//已成功更新
      max_height = height;
      break;
    }
//否则重试，可能因为其他人退出循环
//增加它
  }
  assert(max_height <= kMaxPossibleHeight);

  int recompute_height = 0;
  if (splice->height_ < max_height) {
//从未使用拼接，或从那时起最大高度增加。
//最后使用。我们可以在后一种情况下修复它，但是
//这很棘手。
    splice->prev_[max_height] = head_;
    splice->next_[max_height] = nullptr;
    splice->height_ = max_height;
    recompute_height = max_height;
  } else {
//拼接是一个有效的适当高度拼接，它将一些
//键，但它是用括号括起来的吗？我们需要验证它并且
//重新计算拼接的一部分（0级..重新计算高度-1）
//这是所有级别的超集，不包含新键。
//有几种选择是合理的，因为我们必须平衡工作
//根据验证拼接所需的额外比较进行保存。
//
//一种策略是重新计算所有原始拼接高度，如果
//底层没有包围。悲观地认为
//我们要么得到一个完美的拼接命中（按顺序递增
//插入）或没有位置。
//
//另一个策略是沿着拼接层走，直到找到
//用括号括住键的级别。这个策略让拼接
//其他情况的提示帮助：它将插入从O（log n）转换为
//o（log d），其中d是键与键之间的节点数。
//生成了拼接和当前插入（辅助插入
//新键是在拼接之前还是之后）。如果你有
//一种使用键前缀直接映射到最近的键的方法。
//从O（sqrt（n））拼接中拼接出来，我们进行拼接，以便拼接
//也可以在阅读过程中用作提示，然后我们以oshman的
//以及shavit的skiptrie，它有O（log log n）查找和插入
//（与跳过列表的O（log n）比较）。
//
//我们通过允许部分拼接来控制悲观策略。
//一个好的策略可能是对Seq_Splice_，
//乐观如果来电者真的去提供
//接缝
    while (recompute_height < max_height) {
      if (splice->prev_[recompute_height]->Next(recompute_height) !=
          splice->next_[recompute_height]) {
//接头在这一层不紧，一定有一些插入物
//对此
//未更新拼接的位置。我们可能只是一点点
//陈腐的，但如果
//接头很陈旧，需要O（N）来固定。我们还没有用过
//上任何一个
//我们的比较预算，所以即使我们
//悲观的
//我们成功的机会。
        ++recompute_height;
      } else if (splice->prev_[recompute_height] != head_ &&
                 !KeyIsAfterNode(key, splice->prev_[recompute_height])) {
//键来自拼接前
        if (allow_partial_splice_fix) {
//跳过具有相同节点的所有级别，而不进行更多比较
          Node* bad = splice->prev_[recompute_height];
          while (splice->prev_[recompute_height] == bad) {
            ++recompute_height;
          }
        } else {
//我们悲观，重新计算一切
          recompute_height = max_height;
        }
      } else if (KeyIsAfterNode(key, splice->next_[recompute_height])) {
//键来自拼接后
        if (allow_partial_splice_fix) {
          Node* bad = splice->next_[recompute_height];
          while (splice->next_[recompute_height] == bad) {
            ++recompute_height;
          }
        } else {
          recompute_height = max_height;
        }
      } else {
//这个关卡的钥匙，我们赢了！
        break;
      }
    }
  }
  assert(recompute_height <= max_height);
  if (recompute_height > 0) {
    RecomputeSpliceLevels(key, splice, recompute_height);
  }

  bool splice_is_valid = true;
  if (UseCAS) {
    for (int i = 0; i < height; ++i) {
      while (true) {
        assert(splice->next_[i] == nullptr ||
               compare_(x->Key(), splice->next_[i]->Key()) < 0);
        assert(splice->prev_[i] == head_ ||
               compare_(splice->prev_[i]->Key(), x->Key()) < 0);
        x->NoBarrier_SetNext(i, splice->next_[i]);
        if (splice->prev_[i]->CASNext(i, splice->next_[i], x)) {
//成功
          break;
        }
//cas失败，我们需要重新计算prev和next。不太可能
//当我们重做
//搜索，因为很多节点不太可能
//在上一个[I]和下一个[I]之间插入。没有使用的意义
//下一个提示是“我”，因为我们知道它已经过时了。
        FindSpliceForLevel(key, splice->prev_[i], nullptr, i, &splice->prev_[i],
                           &splice->next_[i]);

//既然我们缩小了一级的范围，我们可能
//违反了I和I-1之间的拼接约束。确保
//我们下次重新计算整件事。
        if (i > 0) {
          splice_is_valid = false;
        }
      }
    }
  } else {
    for (int i = 0; i < height; ++i) {
      if (i >= recompute_height &&
          splice->prev_[i]->Next(i) != splice->next_[i]) {
        FindSpliceForLevel(key, splice->prev_[i], nullptr, i, &splice->prev_[i],
                           &splice->next_[i]);
      }
      assert(splice->next_[i] == nullptr ||
             compare_(x->Key(), splice->next_[i]->Key()) < 0);
      assert(splice->prev_[i] == head_ ||
             compare_(splice->prev_[i]->Key(), x->Key()) < 0);
      assert(splice->prev_[i]->Next(i) == splice->next_[i]);
      x->NoBarrier_SetNext(i, splice->next_[i]);
      splice->prev_[i]->SetNext(i, x);
    }
  }
  if (splice_is_valid) {
    for (int i = 0; i < height; ++i) {
      splice->prev_[i] = x;
    }
    assert(splice->prev_[splice->height_] == head_);
    assert(splice->next_[splice->height_] == nullptr);
    for (int i = 0; i < splice->height_; ++i) {
      assert(splice->next_[i] == nullptr ||
             compare_(key, splice->next_[i]->Key()) < 0);
      assert(splice->prev_[i] == head_ ||
             compare_(splice->prev_[i]->Key(), key) <= 0);
      assert(splice->prev_[i + 1] == splice->prev_[i] ||
             splice->prev_[i + 1] == head_ ||
             compare_(splice->prev_[i + 1]->Key(), splice->prev_[i]->Key()) <
                 0);
      assert(splice->next_[i + 1] == splice->next_[i] ||
             splice->next_[i + 1] == nullptr ||
             compare_(splice->next_[i]->Key(), splice->next_[i + 1]->Key()) <
                 0);
    }
  } else {
    splice->height_ = 0;
  }
}

template <class Comparator>
bool InlineSkipList<Comparator>::Contains(const char* key) const {
  Node* x = FindGreaterOrEqual(key);
  if (x != nullptr && Equal(key, x->Key())) {
    return true;
  } else {
    return false;
  }
}

template <class Comparator>
void InlineSkipList<Comparator>::TEST_Validate() const {
//同时在所有级别上进行交互，并验证节点是否出现在
//顺序正确，节点出现在上层，节点也出现在下层。
//水平。
  Node* nodes[kMaxPossibleHeight];
  int max_height = GetMaxHeight();
  for (int i = 0; i < max_height; i++) {
    nodes[i] = head_;
  }
  while (nodes[0] != nullptr) {
    Node* l0_next = nodes[0]->Next(0);
    if (l0_next == nullptr) {
      break;
    }
    assert(nodes[0] == head_ || compare_(nodes[0]->Key(), l0_next->Key()) < 0);
    nodes[0] = l0_next;

    int i = 1;
    while (i < max_height) {
      Node* next = nodes[i]->Next(i);
      if (next == nullptr) {
        break;
      }
      auto cmp = compare_(nodes[0]->Key(), next->Key());
      assert(cmp <= 0);
      if (cmp == 0) {
        assert(next == nodes[0]);
        nodes[i] = next;
      } else {
        break;
      }
      i++;
    }
  }
  for (int i = 1; i < max_height; i++) {
    assert(nodes[i]->Next(i) == nullptr);
  }
}

}  //命名空间rocksdb
