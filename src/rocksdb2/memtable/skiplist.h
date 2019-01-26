
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
//线程安全性
//--------------
//
//写操作需要外部同步，很可能是互斥体。
//阅读要求保证滑雪运动员不会被摧毁
//在读取过程中。除此之外，读取进度
//没有任何内部锁定或同步。
//
//Invariants：
//
//（1）在skiplist
//摧毁。因为我们
//从不删除任何跳过列表节点。
//
//（2）除了下一个/上一个指针外，节点的内容是
//节点链接到Skiplist后不可变。
//只有insert（）可以修改列表，并且要小心初始化
//一个节点，并使用发布存储在一个或
//更多的列表。
//
//…上一个与下一个指针排序…
//

#pragma once
#include <assert.h>
#include <atomic>
#include <stdlib.h>
#include "port/port.h"
#include "util/allocator.h"
#include "util/random.h"

namespace rocksdb {

template<typename Key, class Comparator>
class SkipList {
 private:
  struct Node;

 public:
//创建一个新的skiplist对象，该对象将使用“cmp”比较键，
//并将使用“*分配器”分配内存。在中分配的对象
//分配器必须在skiplist对象的生存期内保持分配状态。
  explicit SkipList(Comparator cmp, Allocator* allocator,
                    int32_t max_height = 12, int32_t branching_factor = 4);

//在列表中插入键。
//要求：列表中当前没有与键比较的内容。
  void Insert(const Key& key);

//返回true iff列表中比较等于key的条目。
  bool Contains(const Key& key) const;

//返回小于“key”的估计条目数。
  uint64_t EstimateCount(const Key& key) const;

//跳过列表内容的迭代
  class Iterator {
   public:
//在指定列表上初始化迭代器。
//返回的迭代器无效。
    explicit Iterator(const SkipList* list);

//更改用于此迭代器的基础skiplist
//这使我们能够在不解除分配的情况下不更改迭代器。
//一个旧的，然后分配一个新的
    void SetList(const SkipList* list);

//如果迭代器位于有效节点，则返回true。
    bool Valid() const;

//返回当前位置的键。
//要求：有效（）
    const Key& key() const;

//前进到下一个位置。
//要求：有效（）
    void Next();

//前进到上一个位置。
//要求：有效（）
    void Prev();

//使用大于等于target的键前进到第一个条目
    void Seek(const Key& target);

//使用键<=target返回到最后一个条目
    void SeekForPrev(const Key& target);

//在列表中的第一个条目处定位。
//迭代器的最终状态为valid（）iff list不为空。
    void SeekToFirst();

//在列表中的最后一个条目处定位。
//迭代器的最终状态为valid（）iff list不为空。
    void SeekToLast();

   private:
    const SkipList* list_;
    Node* node_;
//有意复制
  };

 private:
  const uint16_t kMaxHeight_;
  const uint16_t kBranching_;
  const uint32_t kScaledInverseBranching_;

//构造后不变
  Comparator const compare_;
Allocator* const allocator_;    //用于节点分配的分配器

  Node* const head_;

//仅由insert（）修改。被读者种族歧视，但陈旧
//值是可以的。
std::atomic<int> max_height_;  //整个列表的高度

//用于优化顺序插入模式。狡猾的赞成[赞成]
//i到max_u height_u是prev_uu0和prev_u height_的前身
//是上一个的高度。prev_[0]只能等于head before
//插入，在这种情况下，最大高度和前一高度为1。
  Node** prev_;
  int32_t prev_height_;

  inline int GetMaxHeight() const {
    return max_height_.load(std::memory_order_relaxed);
  }

  Node* NewNode(const Key& key, int height);
  int RandomHeight();
  bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }
  bool LessThan(const Key& a, const Key& b) const {
    return (compare_(a, b) < 0);
  }

//如果键大于“n”中存储的数据，则返回“真”
  bool KeyIsAfterNode(const Key& key, Node* n) const;

//返回键大于等于键的最早节点。
//如果没有此类节点，则返回nullptr。
  Node* FindGreaterOrEqual(const Key& key) const;

//返回键<键的最新节点。
//如果没有这样的节点，返回head。
//用指向“level”上一个节点的指针为上一个[级别]填充
//如果prev为非空，则级别为[0..max_height_uu1]。
  Node* FindLessThan(const Key& key, Node** prev = nullptr) const;

//返回列表中的最后一个节点。
//如果列表为空，则返回head。
  Node* FindLast() const;

//不允许复制
  SkipList(const SkipList&);
  void operator=(const SkipList&);
};

//实施细节如下
template<typename Key, class Comparator>
struct SkipList<Key, Comparator>::Node {
  explicit Node(const Key& k) : key(k) { }

  Key const key;

//链接的访问器/转换器。用方法包装，这样我们可以
//必要时添加适当的屏障。
  Node* Next(int n) {
    assert(n >= 0);
//使用“获取加载”，以便我们观察完全初始化的
//返回节点的版本。
    return (next_[n].load(std::memory_order_acquire));
  }
  void SetNext(int n, Node* x) {
    assert(n >= 0);
//使用“发布商店”，以便任何阅读此内容的人
//指针观察插入节点的完全初始化版本。
    next_[n].store(x, std::memory_order_release);
  }

//在一些地方没有可以安全使用的屏障变体。
  Node* NoBarrier_Next(int n) {
    assert(n >= 0);
    return next_[n].load(std::memory_order_relaxed);
  }
  void NoBarrier_SetNext(int n, Node* x) {
    assert(n >= 0);
    next_[n].store(x, std::memory_order_relaxed);
  }

 private:
//长度等于节点高度的数组。下一个“0”是最低级别的链接。
  std::atomic<Node*> next_[1];
};

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::NewNode(const Key& key, int height) {
  char* mem = allocator_->AllocateAligned(
      sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1));
  return new (mem) Node(key);
}

template<typename Key, class Comparator>
inline SkipList<Key, Comparator>::Iterator::Iterator(const SkipList* list) {
  SetList(list);
}

template<typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::SetList(const SkipList* list) {
  list_ = list;
  node_ = nullptr;
}

template<typename Key, class Comparator>
inline bool SkipList<Key, Comparator>::Iterator::Valid() const {
  return node_ != nullptr;
}

template<typename Key, class Comparator>
inline const Key& SkipList<Key, Comparator>::Iterator::key() const {
  assert(Valid());
  return node_->key;
}

template<typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Next() {
  assert(Valid());
  node_ = node_->Next(0);
}

template<typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Prev() {
//我们不使用显式的“prev”链接，只搜索
//落在键之前的最后一个节点。
  assert(Valid());
  node_ = list_->FindLessThan(node_->key);
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template<typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Seek(const Key& target) {
  node_ = list_->FindGreaterOrEqual(target);
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::SeekForPrev(
    const Key& target) {
  Seek(target);
  if (!Valid()) {
    SeekToLast();
  }
  while (Valid() && list_->LessThan(target, key())) {
    Prev();
  }
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::SeekToFirst() {
  node_ = list_->head_->Next(0);
}

template<typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::SeekToLast() {
  node_ = list_->FindLast();
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template<typename Key, class Comparator>
int SkipList<Key, Comparator>::RandomHeight() {
  auto rnd = Random::GetTLSInstance();

//以Kbranching的概率1增加高度
  int height = 1;
  while (height < kMaxHeight_ && rnd->Next() < kScaledInverseBranching_) {
    height++;
  }
  assert(height > 0);
  assert(height <= kMaxHeight_);
  return height;
}

template<typename Key, class Comparator>
bool SkipList<Key, Comparator>::KeyIsAfterNode(const Key& key, Node* n) const {
//nullptr n被认为是无限的
  return (n != nullptr) && (compare_(n->key, key) < 0);
}

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::
  FindGreaterOrEqual(const Key& key) const {
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
    assert(x == head_ || next == nullptr || KeyIsAfterNode(next->key, x));
//确保我们在搜索过程中没有超负荷
    assert(x == head_ || KeyIsAfterNode(key, x));
    int cmp = (next == nullptr || next == last_bigger)
        ? 1 : compare_(next->key, key);
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

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::FindLessThan(const Key& key, Node** prev) const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
//key is after（key，last_not_after）绝对是错误的
  Node* last_not_after = nullptr;
  while (true) {
    Node* next = x->Next(level);
    assert(x == head_ || next == nullptr || KeyIsAfterNode(next->key, x));
    assert(x == head_ || KeyIsAfterNode(key, x));
    if (next != last_not_after && KeyIsAfterNode(key, next)) {
//继续在此列表中搜索
      x = next;
    } else {
      if (prev != nullptr) {
        prev[level] = x;
      }
      if (level == 0) {
        return x;
      } else {
//切换到下一个列表，重用keyiusafternode（）结果
        last_not_after = next;
        level--;
      }
    }
  }
}

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::FindLast()
    const {
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

template <typename Key, class Comparator>
uint64_t SkipList<Key, Comparator>::EstimateCount(const Key& key) const {
  uint64_t count = 0;

  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    assert(x == head_ || compare_(x->key, key) < 0);
    Node* next = x->Next(level);
    if (next == nullptr || compare_(next->key, key) >= 0) {
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

template <typename Key, class Comparator>
SkipList<Key, Comparator>::SkipList(const Comparator cmp, Allocator* allocator,
                                    int32_t max_height,
                                    int32_t branching_factor)
    : kMaxHeight_(max_height),
      kBranching_(branching_factor),
      kScaledInverseBranching_((Random::kMaxNext + 1) / kBranching_),
      compare_(cmp),
      allocator_(allocator),
      /*d_uu（newnode（0/*任意键都可以做*/，max_height）），
      （1）
      上一个高度
  断言（max_height>0&&kmaxheight_u==static_cast<uint32_t>（max_height））；
  断言（分支因子>0&&
         Kbranching_==static_cast<uint32_t>（branching_factor））；
  断言（kscaledinversebranching_uu>0）；
  //直接从传入的分配器分配前一个节点*数组。
  //不需要释放prev，因为它的生命周期与
  //整个分配器。
  prev_uu=reinterpret_cast<node**>（）
            分配器_->allocateAlligned（sizeof（node*）*kmaxheight_uu））；
  对于（int i=0；i<kmaxheight_；i++）
    head_u->setnext（i，nullptr）；
    上一个=头部；
  }
}

template<typename key，class comparator>
void skiplist<key，comparator>：：insert（const key&key）
  //顺序插入的快速路径
  如果（！）keyisafternode（key，prev_[0]->nobarrier_next（0））&&
      （prev_[0]=头部键IsAfternode（键，prev_[0]））
    断言（上一个）！=头部（前一个高度UU==1&&GetMaxHeight（）==1））；

    //在此方法之外，prev_[1..max_height_uu]是前一个
    //前一个_uu0]，前一个_u高度_u指前一个_u0。内插
    //prev_u[0..max_height-1]是键的前身。从开关
    //内部的外部状态
    对于（int i=1；i<prev_height_u；i++）
      prev_[i]=上一个_u[0]；
    }
  }否则{
    //TODO（opt）：我们可以使用nobarrier前置搜索作为
    //优化内存\顺序\获取需要的架构
    //同步指令。在x86上没关系
    findlessthan（键，上一个）；
  }

  //我们的数据结构不允许重复插入
  断言（prev_[0]->next（0）==nullptr！等于（键，上一个->下一个（0->键））；

  int height=randomheight（）；
  if（height>getmaxheight（））
    对于（int i=getmaxheight（）；i<height；i++）
      上一个=头部；
    }
    //fprintf（stderr，“将高度从%d更改为%d\n”，max_height_uu height）；

    //可以在不同步的情况下改变最大高度
    //同时读卡器。同时观察
    //max_height的新值将看到
    //来自head_uuu（nullptr）的新级别指针，或在中设置的新值
    //下面的循环。在前一种情况下，读者会
    //立即下降到下一个级别，因为nullptr毕竟排序
    //键。在后一种情况下，读卡器将使用新节点。
    最大高度存储（高度，标准：：内存\顺序\松弛）；
  }

  node*x=newnode（键，高度）；
  对于（int i=0；i<height；i++）
    //nobarrier_setnext（）足够了，因为我们将在
    //我们在prev[i]中发布一个指向“x”的指针。
    x->nobarrier_setnext（i，prev_u[i]->nobarrier_next（i））；
    prev_u[i]->setnext（i，x）；
  }
  PREVI[〔0〕＝X〕；
  上一个高度=高度；
}

template<typename key，class comparator>
bool skiplist<key，comparator>：：contains（const key&key）const_
  node*x=findCreaterRequal（键）；
  如果（X）！=nullptr&&equal（key，x->key））
    回归真实；
  }否则{
    返回错误；
  }
}

//命名空间rocksdb
