
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

#pragma once

#include <set>

#include "table/internal_iterator.h"

namespace rocksdb {

//具有类似于迭代器的缓存接口的内部包装类
//基础迭代器的valid（）和key（）结果。
//这有助于避免虚拟函数调用，并提供更好的
//缓存局部性。
class IteratorWrapper {
 public:
  IteratorWrapper() : iter_(nullptr), valid_(false) {}
  explicit IteratorWrapper(InternalIterator* _iter) : iter_(nullptr) {
    Set(_iter);
  }
  ~IteratorWrapper() {}
  InternalIterator* iter() const { return iter_; }

//将基础迭代器设置为“iter”并返回
//上一个基础迭代器。
  InternalIterator* Set(InternalIterator* _iter) {
    InternalIterator* old_iter = iter_;

    iter_ = _iter;
    if (iter_ == nullptr) {
      valid_ = false;
    } else {
      Update();
    }
    return old_iter;
  }

  void DeleteIter(bool is_arena_mode) {
    if (iter_) {
      if (!is_arena_mode) {
        delete iter_;
      } else {
        iter_->~InternalIterator();
      }
    }
  }

//迭代器接口方法
  bool Valid() const        { return valid_; }
  Slice key() const         { assert(Valid()); return key_; }
  Slice value() const       { assert(Valid()); return iter_->value(); }
//下面的方法需要iter（）！= Null PTR
  Status status() const     { assert(iter_); return iter_->status(); }
  void Next()               { assert(iter_); iter_->Next();        Update(); }
  void Prev()               { assert(iter_); iter_->Prev();        Update(); }
  void Seek(const Slice& k) { assert(iter_); iter_->Seek(k);       Update(); }
  void SeekForPrev(const Slice& k) {
    assert(iter_);
    iter_->SeekForPrev(k);
    Update();
  }
  void SeekToFirst()        { assert(iter_); iter_->SeekToFirst(); Update(); }
  void SeekToLast()         { assert(iter_); iter_->SeekToLast();  Update(); }

  void SetPinnedItersMgr(PinnedIteratorsManager* pinned_iters_mgr) {
    assert(iter_);
    iter_->SetPinnedItersMgr(pinned_iters_mgr);
  }
  bool IsKeyPinned() const {
    assert(Valid());
    return iter_->IsKeyPinned();
  }
  bool IsValuePinned() const {
    assert(Valid());
    return iter_->IsValuePinned();
  }

 private:
  void Update() {
    valid_ = iter_->Valid();
    if (valid_) {
      key_ = iter_->key();
    }
  }

  InternalIterator* iter_;
  bool valid_;
  Slice key_;
};

class Arena;
//返回从竞技场分配的空迭代器（不生成任何内容）。
extern InternalIterator* NewEmptyInternalIterator(Arena* arena);

//返回具有指定状态、已分配竞技场的空迭代器。
extern InternalIterator* NewErrorInternalIterator(const Status& status,
                                                  Arena* arena);

}  //命名空间rocksdb
