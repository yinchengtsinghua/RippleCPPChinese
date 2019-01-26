
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

#include "table/merging_iterator.h"
#include <string>
#include <vector>
#include "db/pinned_iterators_manager.h"
#include "monitoring/perf_context_imp.h"
#include "rocksdb/comparator.h"
#include "rocksdb/iterator.h"
#include "rocksdb/options.h"
#include "table/internal_iterator.h"
#include "table/iter_heap.h"
#include "table/iterator_wrapper.h"
#include "util/arena.h"
#include "util/autovector.h"
#include "util/heap.h"
#include "util/stop_watch.h"
#include "util/sync_point.h"

namespace rocksdb {
//如果这里没有匿名名称空间，我们将使警告失败-wMissing原型
namespace {
typedef BinaryHeap<IteratorWrapper*, MaxIteratorComparator> MergerMaxIterHeap;
typedef BinaryHeap<IteratorWrapper*, MinIteratorComparator> MergerMinIterHeap;
}  //命名空间

const size_t kNumIterReserve = 4;

class MergingIterator : public InternalIterator {
 public:
  MergingIterator(const Comparator* comparator, InternalIterator** children,
                  int n, bool is_arena_mode, bool prefix_seek_mode)
      : is_arena_mode_(is_arena_mode),
        comparator_(comparator),
        current_(nullptr),
        direction_(kForward),
        minHeap_(comparator_),
        prefix_seek_mode_(prefix_seek_mode),
        pinned_iters_mgr_(nullptr) {
    children_.resize(n);
    for (int i = 0; i < n; i++) {
      children_[i].Set(children[i]);
    }
    for (auto& child : children_) {
      if (child.Valid()) {
        minHeap_.push(&child);
      }
    }
    current_ = CurrentForward();
  }

  virtual void AddIterator(InternalIterator* iter) {
    assert(direction_ == kForward);
    children_.emplace_back(iter);
    if (pinned_iters_mgr_) {
      iter->SetPinnedItersMgr(pinned_iters_mgr_);
    }
    auto new_wrapper = children_.back();
    if (new_wrapper.Valid()) {
      minHeap_.push(&new_wrapper);
      current_ = CurrentForward();
    }
  }

  virtual ~MergingIterator() {
    for (auto& child : children_) {
      child.DeleteIter(is_arena_mode_);
    }
  }

  virtual bool Valid() const override { return (current_ != nullptr); }

  virtual void SeekToFirst() override {
    ClearHeaps();
    for (auto& child : children_) {
      child.SeekToFirst();
      if (child.Valid()) {
        minHeap_.push(&child);
      }
    }
    direction_ = kForward;
    current_ = CurrentForward();
  }

  virtual void SeekToLast() override {
    ClearHeaps();
    InitMaxHeap();
    for (auto& child : children_) {
      child.SeekToLast();
      if (child.Valid()) {
        maxHeap_->push(&child);
      }
    }
    direction_ = kReverse;
    current_ = CurrentReverse();
  }

  virtual void Seek(const Slice& target) override {
    ClearHeaps();
    for (auto& child : children_) {
      {
        PERF_TIMER_GUARD(seek_child_seek_time);
        child.Seek(target);
      }
      PERF_COUNTER_ADD(seek_child_seek_count, 1);

      if (child.Valid()) {
        PERF_TIMER_GUARD(seek_min_heap_time);
        minHeap_.push(&child);
      }
    }
    direction_ = kForward;
    {
      PERF_TIMER_GUARD(seek_min_heap_time);
      current_ = CurrentForward();
    }
  }

  virtual void SeekForPrev(const Slice& target) override {
    ClearHeaps();
    InitMaxHeap();

    for (auto& child : children_) {
      {
        PERF_TIMER_GUARD(seek_child_seek_time);
        child.SeekForPrev(target);
      }
      PERF_COUNTER_ADD(seek_child_seek_count, 1);

      if (child.Valid()) {
        PERF_TIMER_GUARD(seek_max_heap_time);
        maxHeap_->push(&child);
      }
    }
    direction_ = kReverse;
    {
      PERF_TIMER_GUARD(seek_max_heap_time);
      current_ = CurrentReverse();
    }
  }

  virtual void Next() override {
    assert(Valid());

//确保所有子项都位于键（）之后。
//如果我们正向前进，它已经
//对所有非当前子级都是正确的，因为当前子级是
//最小的子级和键（）==current_->key（）。
    if (direction_ != kForward) {
//否则，推进非当前子项。我们前进电流
//就在if块之后。
      ClearHeaps();
      for (auto& child : children_) {
        if (&child != current_) {
          child.Seek(key());
          if (child.Valid() && comparator_->Equal(key(), child.key())) {
            child.Next();
          }
        }
        if (child.Valid()) {
          minHeap_.push(&child);
        }
      }
      direction_ = kForward;
//循环将所有非当前子代都高级为>key（），因此当前为\
//应该仍然是最小的键。
      assert(current_ == CurrentForward());
    }

//要使下面的堆修改正确，当前值必须是
//堆的当前顶部。
    assert(current_ == CurrentForward());

//因为当前指向当前记录。向前移动迭代器。
    current_->Next();
    if (current_->Valid()) {
//在上面的next（）调用之后，current仍然有效。呼叫
//替换_Top（）以还原堆属性。当同一个孩子
//迭代器生成一系列键，这很便宜。
      minHeap_.replace_top(current_);
    } else {
//当前值已停止有效，请将其从堆中删除。
      minHeap_.pop();
    }
    current_ = CurrentForward();
  }

  virtual void Prev() override {
    assert(Valid());
//确保所有子项都位于键（）之前。
//如果我们朝相反的方向移动，它已经
//对所有非当前子级都是正确的，因为当前子级是
//最大的子级和键（）==current_->key（）。
    if (direction_ != kReverse) {
//否则，退却非流动儿童。我们撤退了
//就在if块之后。
      ClearHeaps();
      InitMaxHeap();
      for (auto& child : children_) {
        if (&child != current_) {
          if (!prefix_seek_mode_) {
            child.Seek(key());
            if (child.Valid()) {
//子项位于第一个条目>=key（）。后退一步，使其小于键（）。
              TEST_SYNC_POINT_CALLBACK("MergeIterator::Prev:BeforePrev",
                                       &child);
              child.Prev();
            } else {
//子项没有大于等于key（）的项。在最后一个入口的位置。
              TEST_SYNC_POINT("MergeIterator::Prev:BeforeSeekToLast");
              child.SeekToLast();
            }
          } else {
            child.SeekForPrev(key());
            if (child.Valid() && comparator_->Equal(key(), child.key())) {
              child.Prev();
            }
          }
        }
        if (child.Valid()) {
          maxHeap_->push(&child);
        }
      }
      direction_ = kReverse;
      if (!prefix_seek_mode_) {
//注意，我们这里不做断言（current_==currentReverse（））。
//因为有些键可能比搜索键大
//在seek（）和seektolast（）之间插入，使当前不
//等于currentReverse（）。
        current_ = CurrentReverse();
      }
//循环将所有非当前子级都提升为<key（），因此当前
//应该仍然是最小的键。
      assert(current_ == CurrentReverse());
    }

//要使下面的堆修改正确，当前值必须是
//堆的当前顶部。
    assert(current_ == CurrentReverse());

    current_->Prev();
    if (current_->Valid()) {
//在上面的prev（）调用之后，current仍然有效。呼叫
//替换_Top（）以还原堆属性。当同一个孩子
//迭代器生成一系列键，这很便宜。
      maxHeap_->replace_top(current_);
    } else {
//当前值已停止有效，请将其从堆中删除。
      maxHeap_->pop();
    }
    current_ = CurrentReverse();
  }

  virtual Slice key() const override {
    assert(Valid());
    return current_->key();
  }

  virtual Slice value() const override {
    assert(Valid());
    return current_->value();
  }

  virtual Status status() const override {
    Status s;
    for (auto& child : children_) {
      s = child.status();
      if (!s.ok()) {
        break;
      }
    }
    return s;
  }

  virtual void SetPinnedItersMgr(
      PinnedIteratorsManager* pinned_iters_mgr) override {
    pinned_iters_mgr_ = pinned_iters_mgr;
    for (auto& child : children_) {
      child.SetPinnedItersMgr(pinned_iters_mgr);
    }
  }

  virtual bool IsKeyPinned() const override {
    assert(Valid());
    return pinned_iters_mgr_ && pinned_iters_mgr_->PinningEnabled() &&
           current_->IsKeyPinned();
  }

  virtual bool IsValuePinned() const override {
    assert(Valid());
    return pinned_iters_mgr_ && pinned_iters_mgr_->PinningEnabled() &&
           current_->IsValuePinned();
  }

 private:
//清除两个方向的堆，在更改方向或查找时使用
  void ClearHeaps();
//确保在开始反向时初始化maxheap_u
//方向
  void InitMaxHeap();

  bool is_arena_mode_;
  const Comparator* comparator_;
  autovector<IteratorWrapper, kNumIterReserve> children_;

//缓存的指向具有当前键的子迭代器的指针，如果没有，则为nullptr
//子迭代器有效。这是minheap或maxheap的顶部
//取决于方向。
  IteratorWrapper* current_;
//迭代器移动的方向是什么？
  enum Direction {
    kForward,
    kReverse
  };
  Direction direction_;
  MergerMinIterHeap minHeap_;
  bool prefix_seek_mode_;

//最大堆用于反向迭代，这比
//向前地。懒惰地初始化它以保存内存。
  std::unique_ptr<MergerMaxIterHeap> maxHeap_;
  PinnedIteratorsManager* pinned_iters_mgr_;

  IteratorWrapper* CurrentForward() const {
    assert(direction_ == kForward);
    return !minHeap_.empty() ? minHeap_.top() : nullptr;
  }

  IteratorWrapper* CurrentReverse() const {
    assert(direction_ == kReverse);
    assert(maxHeap_);
    return !maxHeap_->empty() ? maxHeap_->top() : nullptr;
  }
};

void MergingIterator::ClearHeaps() {
  minHeap_.clear();
  if (maxHeap_) {
    maxHeap_->clear();
  }
}

void MergingIterator::InitMaxHeap() {
  if (!maxHeap_) {
    maxHeap_.reset(new MergerMaxIterHeap(comparator_));
  }
}

InternalIterator* NewMergingIterator(const Comparator* cmp,
                                     InternalIterator** list, int n,
                                     Arena* arena, bool prefix_seek_mode) {
  assert(n >= 0);
  if (n == 0) {
    return NewEmptyInternalIterator(arena);
  } else if (n == 1) {
    return list[0];
  } else {
    if (arena == nullptr) {
      return new MergingIterator(cmp, list, n, false, prefix_seek_mode);
    } else {
      auto mem = arena->AllocateAligned(sizeof(MergingIterator));
      return new (mem) MergingIterator(cmp, list, n, true, prefix_seek_mode);
    }
  }
}

MergeIteratorBuilder::MergeIteratorBuilder(const Comparator* comparator,
                                           Arena* a, bool prefix_seek_mode)
    : first_iter(nullptr), use_merging_iter(false), arena(a) {
  auto mem = arena->AllocateAligned(sizeof(MergingIterator));
  merge_iter =
      new (mem) MergingIterator(comparator, nullptr, 0, true, prefix_seek_mode);
}

void MergeIteratorBuilder::AddIterator(InternalIterator* iter) {
  if (!use_merging_iter && first_iter != nullptr) {
    merge_iter->AddIterator(first_iter);
    use_merging_iter = true;
  }
  if (use_merging_iter) {
    merge_iter->AddIterator(iter);
  } else {
    first_iter = iter;
  }
}

InternalIterator* MergeIteratorBuilder::Finish() {
  if (!use_merging_iter) {
    return first_iter;
  } else {
    auto ret = merge_iter;
    merge_iter = nullptr;
    return ret;
  }
}

}  //命名空间rocksdb
