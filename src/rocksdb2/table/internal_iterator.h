
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

#pragma once

#include <string>
#include "rocksdb/comparator.h"
#include "rocksdb/iterator.h"
#include "rocksdb/status.h"

namespace rocksdb {

class PinnedIteratorsManager;

class InternalIterator : public Cleanable {
 public:
  InternalIterator() {}
  virtual ~InternalIterator() {}

//迭代器要么定位在键/值对上，要么
//无效。如果迭代器有效，则此方法返回true。
  virtual bool Valid() const = 0;

//在震源中的第一个键处定位。迭代器有效（）
//调用iff后，源不为空。
  virtual void SeekToFirst() = 0;

//定位在源中的最后一个键。迭代器是
//在此调用之后，如果源不是空的，则返回valid（）。
  virtual void SeekToLast() = 0;

//在源中的第一个键处定位，该键位于或超过目标
//在调用源包含的iff之后，迭代器是有效的（）。
//到达或超过目标的条目。
  virtual void Seek(const Slice& target) = 0;

//在源中位于目标位置或目标位置之前的第一个键处
//在调用源包含的iff之后，迭代器是有效的（）。
//到达目标或在目标之前的条目。
  virtual void SeekForPrev(const Slice& target) = 0;

//移动到源中的下一个条目。在此调用之后，valid（）是
//如果迭代器未定位在源中的最后一个条目，则返回true。
//要求：有效（）
  virtual void Next() = 0;

//移动到源中的上一个条目。在此调用之后，valid（）是
//如果迭代器未定位在源中的第一个条目，则返回true。
//要求：有效（）
  virtual void Prev() = 0;

//返回当前条目的键。的基础存储
//返回的切片仅在下一次修改
//迭代器。
//要求：有效（）
  virtual Slice key() const = 0;

//返回当前条目的值。的基础存储
//返回的切片仅在下一次修改
//迭代器。
//要求：！（）启动（）
  virtual Slice value() const = 0;

//如果发生错误，请返回。否则返回OK状态。
//如果请求非阻塞IO，而此操作不能
//如果不做一些IO就满意了，那么这将返回status:：incomplete（）。
  virtual Status status() const = 0;

//将pinnediteratorsmanager传递给迭代器，大多数迭代器不传递
//与pinnediteratorsmanager通信，因此默认实现为no op
//但是对于需要与pinnediteratorsmanager通信的迭代器
//他们将实现此函数并使用传递的指针进行通信
//使用PinnediteratorsManager。
  virtual void SetPinnedItersMgr(PinnedIteratorsManager* pinned_iters_mgr) {}

//如果为真，这意味着key（）返回的切片只要
//未调用pinnediteratorsManager:：ReleasePinnedData，并且
//迭代器未被删除。
//
//iskeypined（）保证在以下情况下始终返回true
//-迭代器是用readoptions：：pin_data=true创建的
//-db表是使用blockbasedTableOptions：：使用_delta_编码创建的
//设置为假。
  virtual bool IsKeyPinned() const { return false; }

//如果为真，这意味着value（）返回的切片只要
//未调用pinnediteratorsManager:：ReleasePinnedData，并且
//迭代器未被删除。
  virtual bool IsValuePinned() const { return false; }

  virtual Status GetProperty(std::string prop_name, std::string* prop) {
    return Status::NotSupported("");
  }

 protected:
  void SeekForPrevImpl(const Slice& target, const Comparator* cmp) {
    Seek(target);
    if (!Valid()) {
      SeekToLast();
    }
    while (Valid() && cmp->Compare(target, key()) < 0) {
      Prev();
    }
  }

 private:
//不允许复制
  InternalIterator(const InternalIterator&) = delete;
  InternalIterator& operator=(const InternalIterator&) = delete;
};

//返回一个空的迭代器（不产生任何结果）。
extern InternalIterator* NewEmptyInternalIterator();

//返回具有指定状态的空迭代器。
extern InternalIterator* NewErrorInternalIterator(const Status& status);

}  //命名空间rocksdb
