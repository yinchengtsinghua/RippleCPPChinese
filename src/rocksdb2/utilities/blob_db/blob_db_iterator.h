
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

#pragma once
#ifndef ROCKSDB_LITE

#include "rocksdb/iterator.h"
#include "utilities/blob_db/blob_db_impl.h"

namespace rocksdb {
namespace blob_db {

using rocksdb::ManagedSnapshot;

class BlobDBIterator : public Iterator {
 public:
  BlobDBIterator(ManagedSnapshot* snapshot, ArenaWrappedDBIter* iter,
                 BlobDBImpl* blob_db)
      : snapshot_(snapshot), iter_(iter), blob_db_(blob_db) {}

  virtual ~BlobDBIterator() = default;

  bool Valid() const override {
    if (!iter_->Valid()) {
      return false;
    }
    return status_.ok();
  }

  Status status() const override {
    if (!iter_->status().ok()) {
      return iter_->status();
    }
    return status_;
  }

  void SeekToFirst() override {
    iter_->SeekToFirst();
    UpdateBlobValue();
  }

  void SeekToLast() override {
    iter_->SeekToLast();
    UpdateBlobValue();
  }

  void Seek(const Slice& target) override {
    iter_->Seek(target);
    UpdateBlobValue();
  }

  void SeekForPrev(const Slice& target) override {
    iter_->SeekForPrev(target);
    UpdateBlobValue();
  }

  void Next() override {
    assert(Valid());
    iter_->Next();
    UpdateBlobValue();
  }

  void Prev() override {
    assert(Valid());
    iter_->Prev();
    UpdateBlobValue();
  }

  Slice key() const override {
    assert(Valid());
    return iter_->key();
  }

  Slice value() const override {
    assert(Valid());
    if (!iter_->IsBlob()) {
      return iter_->value();
    }
    return value_;
  }

//不支持迭代器：：refresh（）。

 private:
  void UpdateBlobValue() {
    TEST_SYNC_POINT("BlobDBIterator::UpdateBlobValue:Start:1");
    TEST_SYNC_POINT("BlobDBIterator::UpdateBlobValue:Start:2");
    value_.Reset();
    if (iter_->Valid() && iter_->IsBlob()) {
      status_ = blob_db_->GetBlobValue(iter_->key(), iter_->value(), &value_);
    }
  }

  std::unique_ptr<ManagedSnapshot> snapshot_;
  std::unique_ptr<ArenaWrappedDBIter> iter_;
  BlobDBImpl* blob_db_;
  Status status_;
  PinnableSlice value_;
};
}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //！摇滚乐
