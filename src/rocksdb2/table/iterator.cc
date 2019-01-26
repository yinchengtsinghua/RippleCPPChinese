
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

#include "rocksdb/iterator.h"
#include "table/internal_iterator.h"
#include "table/iterator_wrapper.h"
#include "util/arena.h"

namespace rocksdb {

Cleanable::Cleanable() {
  cleanup_.function = nullptr;
  cleanup_.next = nullptr;
}

Cleanable::~Cleanable() { DoCleanup(); }

Cleanable::Cleanable(Cleanable&& other) {
  *this = std::move(other);
}

Cleanable& Cleanable::operator=(Cleanable&& other) {
  if (this != &other) {
    cleanup_ = other.cleanup_;
    other.cleanup_.function = nullptr;
    other.cleanup_.next = nullptr;
  }
  return *this;
}

//如果整个链表在堆上，我们可以简单地添加一个附加链表。
//将列表链接到另一个。然而，头部是一个嵌入对象，以避免成本
//当cleanable只有一个时，为大多数用例创建对象
//清理工作要做。如果基准显示没有，我们可以把所有的东西都堆起来。
//对绩效的负面影响。
//此外，由于没有指向
//尾巴。我们可以添加尾部指针，但保持它可能会产生负面影响
//一次清除的常见情况下，如果尾部指针不是
//需要。基准可以再次澄清这一点。
//即使没有尾部指针，我们也可以在列表上迭代，找到尾部，以及
//只更新该节点，而不需要逐个插入清理
//一个。但是，当源可清除文件具有一个或一个
//大多数情况下很少进行清理。
//TODO（Myabandeh）：如果列表太长，我们应该维护一个尾部指针
//并有完整的列表（减去必须单独插入的头部）
//立即与目标链接列表合并。
void Cleanable::DelegateCleanupsTo(Cleanable* other) {
  assert(other != nullptr);
  if (cleanup_.function == nullptr) {
    return;
  }
  Cleanup* c = &cleanup_;
  other->RegisterCleanup(c->function, c->arg1, c->arg2);
  c = c->next;
  while (c != nullptr) {
    Cleanup* next = c->next;
    other->RegisterCleanup(c);
    c = next;
  }
  cleanup_.function = nullptr;
  cleanup_.next = nullptr;
}

void Cleanable::RegisterCleanup(Cleanable::Cleanup* c) {
  assert(c != nullptr);
  if (cleanup_.function == nullptr) {
    cleanup_.function = c->function;
    cleanup_.arg1 = c->arg1;
    cleanup_.arg2 = c->arg2;
    delete c;
  } else {
    c->next = cleanup_.next;
    cleanup_.next = c;
  }
}

void Cleanable::RegisterCleanup(CleanupFunction func, void* arg1, void* arg2) {
  assert(func != nullptr);
  Cleanup* c;
  if (cleanup_.function == nullptr) {
    c = &cleanup_;
  } else {
    c = new Cleanup;
    c->next = cleanup_.next;
    cleanup_.next = c;
  }
  c->function = func;
  c->arg1 = arg1;
  c->arg2 = arg2;
}

Status Iterator::GetProperty(std::string prop_name, std::string* prop) {
  if (prop == nullptr) {
    return Status::InvalidArgument("prop is nullptr");
  }
  if (prop_name == "rocksdb.iterator.is-key-pinned") {
    *prop = "0";
    return Status::OK();
  }
  return Status::InvalidArgument("Undentified property.");
}

namespace {
class EmptyIterator : public Iterator {
 public:
  explicit EmptyIterator(const Status& s) : status_(s) { }
  virtual bool Valid() const override { return false; }
  virtual void Seek(const Slice& target) override {}
  virtual void SeekForPrev(const Slice& target) override {}
  virtual void SeekToFirst() override {}
  virtual void SeekToLast() override {}
  virtual void Next() override { assert(false); }
  virtual void Prev() override { assert(false); }
  Slice key() const override {
    assert(false);
    return Slice();
  }
  Slice value() const override {
    assert(false);
    return Slice();
  }
  virtual Status status() const override { return status_; }

 private:
  Status status_;
};

class EmptyInternalIterator : public InternalIterator {
 public:
  explicit EmptyInternalIterator(const Status& s) : status_(s) {}
  virtual bool Valid() const override { return false; }
  virtual void Seek(const Slice& target) override {}
  virtual void SeekForPrev(const Slice& target) override {}
  virtual void SeekToFirst() override {}
  virtual void SeekToLast() override {}
  virtual void Next() override { assert(false); }
  virtual void Prev() override { assert(false); }
  Slice key() const override {
    assert(false);
    return Slice();
  }
  Slice value() const override {
    assert(false);
    return Slice();
  }
  virtual Status status() const override { return status_; }

 private:
  Status status_;
};
}  //命名空间

Iterator* NewEmptyIterator() {
  return new EmptyIterator(Status::OK());
}

Iterator* NewErrorIterator(const Status& status) {
  return new EmptyIterator(status);
}

InternalIterator* NewEmptyInternalIterator() {
  return new EmptyInternalIterator(Status::OK());
}

InternalIterator* NewEmptyInternalIterator(Arena* arena) {
  if (arena == nullptr) {
    return NewEmptyInternalIterator();
  } else {
    auto mem = arena->AllocateAligned(sizeof(EmptyIterator));
    return new (mem) EmptyInternalIterator(Status::OK());
  }
}

InternalIterator* NewErrorInternalIterator(const Status& status) {
  return new EmptyInternalIterator(status);
}

InternalIterator* NewErrorInternalIterator(const Status& status, Arena* arena) {
  if (arena == nullptr) {
    return NewErrorInternalIterator(status);
  } else {
    auto mem = arena->AllocateAligned(sizeof(EmptyIterator));
    return new (mem) EmptyInternalIterator(status);
  }
}

}  //命名空间rocksdb
