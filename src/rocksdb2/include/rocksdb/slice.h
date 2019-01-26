
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//slice是一个简单的结构，包含指向某个外部
//存储和大小。切片的用户必须确保切片
//在相应的外部存储
//解除分配。
//
//多个线程可以在一个切片上调用const方法，而不需要
//外部同步，但如果任何线程可以调用
//非常量方法，访问同一切片的所有线程都必须使用
//外部同步。

#ifndef STORAGE_ROCKSDB_INCLUDE_SLICE_H_
#define STORAGE_ROCKSDB_INCLUDE_SLICE_H_

#include <assert.h>
#include <cstdio>
#include <stddef.h>
#include <string.h>
#include <string>

#include "rocksdb/cleanable.h"

namespace rocksdb {

class Slice {
 public:
//创建一个空切片。
  Slice() : data_(""), size_(0) { }

//创建一个引用d[0，n-1]的切片。
  Slice(const char* d, size_t n) : data_(d), size_(n) { }

//创建一个引用“s”内容的切片
  /*隐性的*/
  Slice(const std::string& s) : data_(s.data()), size_(s.size()) { }

//创建引用s[0，strlen（s）-1]的切片
  /*隐性的*/
  Slice(const char* s) : data_(s), size_(strlen(s)) { }

//使用buf作为存储，从sliceparts创建单个切片。
//只要返回的切片存在，buf就必须存在。
  Slice(const struct SliceParts& parts, std::string* buf);

//返回指向引用数据开头的指针
  const char* data() const { return data_; }

//返回引用数据的长度（字节）
  size_t size() const { return size_; }

//返回真iff引用数据的长度为零
  bool empty() const { return size_ == 0; }

//返回引用数据中的第i个字节。
//要求：n<size（））
  char operator[](size_t n) const {
    assert(n < size());
    return data_[n];
  }

//更改此切片以引用空数组
  void clear() { data_ = ""; size_ = 0; }

//从这个切片中删除第一个“n”字节。
  void remove_prefix(size_t n) {
    assert(n <= size());
    data_ += n;
    size_ -= n;
  }

  void remove_suffix(size_t n) {
    assert(n <= size());
    size_ -= n;
  }

//返回包含引用数据副本的字符串。
//当hex为true时，返回长度为hex编码长度（0-9a-f）两倍的字符串。
  std::string ToString(bool hex = false) const;

//将解释为十六进制字符串的当前切片解码为result，
//如果成功，则返回true，如果这不是有效的十六进制字符串
//（例如，不是来自slice:：toString（true））decodeHex返回false。
//此切片的字符数应为偶数0-9a-f
//也接受小写（a-f）
  bool DecodeHex(std::string* result) const;

//三向比较。返回值：
//<0 iff“*this”<“b”，
//==0 iff“*此”==“b”，
//>0 iff“*此”>B”
  int compare(const Slice& b) const;

//返回真iff“x”是“*this”的前缀
  bool starts_with(const Slice& x) const {
    return ((size_ >= x.size_) &&
            (memcmp(data_, x.data_, x.size_) == 0));
  }

  bool ends_with(const Slice& x) const {
    return ((size_ >= x.size_) &&
            (memcmp(data_ + size_ - x.size_, x.data_, x.size_) == 0));
  }

//比较两个切片，并返回第一个字节，其中它们不同
  size_t difference_offset(const Slice& b) const;

//私人：将这些公开供Rocksdbjni访问
  const char* data_;
  size_t size_;

//有意复制
};

/*
 *可以与某些清理任务固定的切片，将在其上运行
 *：：reset（）或对象销毁，以先调用者为准。这个可以用
 *通过让pinnsableslice对象引用数据来避免memcpy
 *锁定在内存中，并在数据被消耗后释放它们。
 **/

class PinnableSlice : public Slice, public Cleanable {
 public:
  PinnableSlice() { buf_ = &self_space_; }
  explicit PinnableSlice(std::string* buf) { buf_ = buf; }

//不允许复制构造函数和复制分配。
  PinnableSlice(PinnableSlice&) = delete;
  PinnableSlice& operator=(PinnableSlice&) = delete;

  inline void PinSlice(const Slice& s, CleanupFunction f, void* arg1,
                       void* arg2) {
    assert(!pinned_);
    pinned_ = true;
    data_ = s.data();
    size_ = s.size();
    RegisterCleanup(f, arg1, arg2);
    assert(pinned_);
  }

  inline void PinSlice(const Slice& s, Cleanable* cleanable) {
    assert(!pinned_);
    pinned_ = true;
    data_ = s.data();
    size_ = s.size();
    cleanable->DelegateCleanupsTo(this);
    assert(pinned_);
  }

  inline void PinSelf(const Slice& slice) {
    assert(!pinned_);
    buf_->assign(slice.data(), slice.size());
    data_ = buf_->data();
    size_ = buf_->size();
    assert(!pinned_);
  }

  inline void PinSelf() {
    assert(!pinned_);
    data_ = buf_->data();
    size_ = buf_->size();
    assert(!pinned_);
  }

  void remove_suffix(size_t n) {
    assert(n <= size());
    if (pinned_) {
      size_ -= n;
    } else {
      buf_->erase(size() - n, n);
      PinSelf();
    }
  }

  void remove_prefix(size_t n) {
assert(0);  //未实施
  }

  void Reset() {
    Cleanable::Reset();
    pinned_ = false;
  }

  inline std::string* GetSelf() { return buf_; }

  inline bool IsPinned() { return pinned_; }

 private:
  friend class PinnableSlice4Test;
  std::string self_space_;
  std::string* buf_;
  bool pinned_ = false;
};

//实际上连接在一起的一组切片。”零件点数
//一系列切片。数组中的元素数为“num_parts”。
struct SliceParts {
  SliceParts(const Slice* _parts, int _num_parts) :
      parts(_parts), num_parts(_num_parts) { }
  SliceParts() : parts(nullptr), num_parts(0) {}

  const Slice* parts;
  int num_parts;
};

inline bool operator==(const Slice& x, const Slice& y) {
  return ((x.size() == y.size()) &&
          (memcmp(x.data(), y.data(), x.size()) == 0));
}

inline bool operator!=(const Slice& x, const Slice& y) {
  return !(x == y);
}

inline int Slice::compare(const Slice& b) const {
  assert(data_ != nullptr && b.data_ != nullptr);
  const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
  int r = memcmp(data_, b.data_, min_len);
  if (r == 0) {
    if (size_ < b.size_) r = -1;
    else if (size_ > b.size_) r = +1;
  }
  return r;
}

inline size_t Slice::difference_offset(const Slice& b) const {
  size_t off = 0;
  const size_t len = (size_ < b.size_) ? size_ : b.size_;
  for (; off < len; off++) {
    if (data_[off] != b.data_[off]) break;
  }
  return off;
}

}  //命名空间rocksdb

#endif  //储物架包括切片
