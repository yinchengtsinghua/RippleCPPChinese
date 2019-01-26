
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

/*
 *此头文件定义fbsonBuffer和fbsonOutstream类。
 *
 ***输入缓冲区**
 *fbsonBuffer是一个客户输入缓冲区，用于包装原始字符缓冲区。它的
 *对象实例用于在内部创建std:：istream对象。
 *
 ***输出流**
 *fbsonoutstream是一个自定义输出流类，用于包含fbson
 *序列化二进制文件。该类可方便地用于专用化
 *fbsonParser和fbsonWriter。
 *
 *@author tian xia<tianx@fb.com>
 **/


#ifndef FBSON_FBSONSTREAM_H
#define FBSON_FBSONSTREAM_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#if defined OS_WIN && !defined snprintf
#define snprintf _snprintf
#endif

#include <inttypes.h>
#include <iostream>

namespace fbson {

//长度包括符号
#define MAX_INT_DIGITS 11
#define MAX_INT64_DIGITS 20
#define MAX_DOUBLE_DIGITS 23 //1（符号）+16（有效）+1（十进制）+5（指数）

/*
 *fbson对输入缓冲区的实现
 **/

class FbsonInBuffer : public std::streambuf {
 public:
  FbsonInBuffer(const char* str, uint32_t len) {
//这是读取缓冲区，不会更改str
//所以我们使用了constou cast（丑陋！）移除常量
    char* pch(const_cast<char*>(str));
    setg(pch, pch, pch + len);
  }
};

/*
 *fbson对输出流的实现。
 *
 *这是字符缓冲区的包装。默认情况下，缓冲容量为1024
 *字节。如果写操作需要realloc，我们将把缓冲区加倍。
 **/

class FbsonOutStream : public std::ostream {
 public:
  explicit FbsonOutStream(uint32_t capacity = 1024)
      : std::ostream(nullptr),
        head_(nullptr),
        size_(0),
        capacity_(capacity),
        alloc_(true) {
    if (capacity_ == 0) {
      capacity_ = 1024;
    }

    head_ = (char*)malloc(capacity_);
  }

  FbsonOutStream(char* buffer, uint32_t capacity)
      : std::ostream(nullptr),
        head_(buffer),
        size_(0),
        capacity_(capacity),
        alloc_(false) {
    assert(buffer && capacity_ > 0);
  }

  ~FbsonOutStream() {
    if (alloc_) {
      free(head_);
    }
  }

  void put(char c) { write(&c, 1); }

  void write(const char* c_str) { write(c_str, (uint32_t)strlen(c_str)); }

  void write(const char* bytes, uint32_t len) {
    if (len == 0)
      return;

    if (size_ + len > capacity_) {
      realloc(len);
    }

    memcpy(head_ + size_, bytes, len);
    size_ += len;
  }

//将整数写入字符串
  void write(int i) {
//snprintf会自动添加一个空值，所以我们还需要一个字符
    if (size_ + MAX_INT_DIGITS + 1 > capacity_) {
      realloc(MAX_INT_DIGITS + 1);
    }

    int len = snprintf(head_ + size_, MAX_INT_DIGITS + 1, "%d", i);
    assert(len > 0);
    size_ += len;
  }

//将64位整数写入字符串
  void write(int64_t l) {
//snprintf会自动添加一个空值，所以我们还需要一个字符
    if (size_ + MAX_INT64_DIGITS + 1 > capacity_) {
      realloc(MAX_INT64_DIGITS + 1);
    }

    int len = snprintf(head_ + size_, MAX_INT64_DIGITS + 1, "%" PRIi64, l);
    assert(len > 0);
    size_ += len;
  }

//将double写入字符串
  void write(double d) {
//snprintf会自动添加一个空值，所以我们还需要一个字符
    if (size_ + MAX_DOUBLE_DIGITS + 1 > capacity_) {
      realloc(MAX_DOUBLE_DIGITS + 1);
    }

    int len = snprintf(head_ + size_, MAX_DOUBLE_DIGITS + 1, "%.15g", d);
    assert(len > 0);
    size_ += len;
  }

  pos_type tellp() const { return size_; }

  void seekp(pos_type pos) { size_ = (uint32_t)pos; }

  const char* getBuffer() const { return head_; }

  pos_type getSize() const { return tellp(); }

 private:
  void realloc(uint32_t len) {
    assert(capacity_ > 0);

    capacity_ *= 2;
    while (capacity_ < size_ + len) {
      capacity_ *= 2;
    }

    if (alloc_) {
      char* new_buf = (char*)::realloc(head_, capacity_);
      assert(new_buf);
      head_ = new_buf;
    } else {
      char* new_buf = (char*)::malloc(capacity_);
      assert(new_buf);
      memcpy(new_buf, head_, size_);
      head_ = new_buf;
      alloc_ = true;
    }
  }

 private:
  char* head_;
  uint32_t size_;
  uint32_t capacity_;
  bool alloc_;
};

} //命名空间FBSON

#endif //fbson_fbsonstream_h
