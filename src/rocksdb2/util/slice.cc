
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
//版权所有（c）2012 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#include <algorithm>
#include "rocksdb/slice_transform.h"
#include "rocksdb/slice.h"
#include "util/string_util.h"
#include <stdio.h>

namespace rocksdb {

namespace {

class FixedPrefixTransform : public SliceTransform {
 private:
  size_t prefix_len_;
  std::string name_;

 public:
  explicit FixedPrefixTransform(size_t prefix_len)
      : prefix_len_(prefix_len),
//请注意，如果名称格式的任何部分发生更改，则需要
//为使RocksDBOptionsParser工作而对选项“助手”进行的更改
//为了新的改变。
//TODO（YCHIAN）：在内部移动序列化/反序列化代码
//类实现本身。
        name_("rocksdb.FixedPrefix." + ToString(prefix_len_)) {}

  virtual const char* Name() const override { return name_.c_str(); }

  virtual Slice Transform(const Slice& src) const override {
    assert(InDomain(src));
    return Slice(src.data(), prefix_len_);
  }

  virtual bool InDomain(const Slice& src) const override {
    return (src.size() >= prefix_len_);
  }

  virtual bool InRange(const Slice& dst) const override {
    return (dst.size() == prefix_len_);
  }

  virtual bool SameResultWhenAppended(const Slice& prefix) const override {
    return InDomain(prefix);
  }
};

class CappedPrefixTransform : public SliceTransform {
 private:
  size_t cap_len_;
  std::string name_;

 public:
  explicit CappedPrefixTransform(size_t cap_len)
      : cap_len_(cap_len),
//请注意，如果名称格式的任何部分发生更改，则需要
//为使RocksDBOptionsParser工作而对选项“助手”进行的更改
//为了新的改变。
//TODO（YCHIAN）：在内部移动序列化/反序列化代码
//类实现本身。
        name_("rocksdb.CappedPrefix." + ToString(cap_len_)) {}

  virtual const char* Name() const override { return name_.c_str(); }

  virtual Slice Transform(const Slice& src) const override {
    assert(InDomain(src));
    return Slice(src.data(), std::min(cap_len_, src.size()));
  }

  virtual bool InDomain(const Slice& src) const override { return true; }

  virtual bool InRange(const Slice& dst) const override {
    return (dst.size() <= cap_len_);
  }

  virtual bool SameResultWhenAppended(const Slice& prefix) const override {
    return prefix.size() >= cap_len_;
  }
};

class NoopTransform : public SliceTransform {
 public:
  explicit NoopTransform() { }

  virtual const char* Name() const override { return "rocksdb.Noop"; }

  virtual Slice Transform(const Slice& src) const override { return src; }

  virtual bool InDomain(const Slice& src) const override { return true; }

  virtual bool InRange(const Slice& dst) const override { return true; }

  virtual bool SameResultWhenAppended(const Slice& prefix) const override {
    return false;
  }
};

}

//2个小型内部实用功能，用于高效的十六进制转换
//不需要snprintf，toupper等…
//最初来自wdt/util/encryptionutils.cpp-对于toString（true）/decodeHex：
char toHex(unsigned char v) {
  if (v <= 9) {
    return '0' + v;
  }
  return 'A' + v - 10;
}
//大部分代码用于验证/错误检查
int fromHex(char c) {
//ToupP:
  if (c >= 'a' && c <= 'f') {
c -= ('a' - 'A');  //AKA 0x20
  }
//验证
  if (c < '0' || (c > '9' && (c < 'A' || c > 'F'))) {
return -1;  //无效的非0-9a-f十六进制字符
  }
  if (c <= '9') {
    return c - '0';
  }
  return c - 'A' + 10;
}

Slice::Slice(const SliceParts& parts, std::string* buf) {
  size_t length = 0;
  for (int i = 0; i < parts.num_parts; ++i) {
    length += parts.parts[i].size();
  }
  buf->reserve(length);

  for (int i = 0; i < parts.num_parts; ++i) {
    buf->append(parts.parts[i].data(), parts.parts[i].size());
  }
  data_ = buf->data();
  size_ = buf->size();
}

//返回包含引用数据副本的字符串。
std::string Slice::ToString(bool hex) const {
std::string result;  //RVO/NRVO/移动
  if (hex) {
    result.reserve(2 * size_);
    for (size_t i = 0; i < size_; ++i) {
      unsigned char c = data_[i];
      result.push_back(toHex(c >> 4));
      result.push_back(toHex(c & 0xf));
    }
    return result;
  } else {
    result.assign(data_, size_);
    return result;
  }
}

//最初来自rocksdb/utilities/ldb_Cmd.h
bool Slice::DecodeHex(std::string* result) const {
  std::string::size_type len = size_;
  if (len % 2) {
//十六进制字符串必须是偶数个十六进制数字才能返回完整的字节
    return false;
  }
  if (!result) {
    return false;
  }
  result->clear();
  result->reserve(len / 2);

  for (size_t i = 0; i < len;) {
    int h1 = fromHex(data_[i++]);
    if (h1 < 0) {
      return false;
    }
    int h2 = fromHex(data_[i++]);
    if (h2 < 0) {
      return false;
    }
    result->push_back((h1 << 4) | h2);
  }
  return true;
}

const SliceTransform* NewFixedPrefixTransform(size_t prefix_len) {
  return new FixedPrefixTransform(prefix_len);
}

const SliceTransform* NewCappedPrefixTransform(size_t cap_len) {
  return new CappedPrefixTransform(cap_len);
}

const SliceTransform* NewNoopTransform() {
  return new NoopTransform;
}

}  //命名空间rocksdb
