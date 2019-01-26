
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

#include "rocksdb/options.h"
#include "util/coding.h"
#include "util/string_util.h"

namespace rocksdb {
namespace blob_db {

//blobindex是指向blob和blob元数据的指针。索引是
//以valueType:：ktypeBloBindex的形式存储在基数据库中。
//有三种类型的blob索引：
//
//kInlinedTTL：
//+------+------+------+------+
//类型到期值
//+------+------+------+------+
//字符变量64变量大小
//+------+------+------+------+
//
//KBLB:
//+------+------+------+------+------+------+------+
//类型文件号偏移大小压缩
//+------+------+------+------+------+------+------+
//字符变量64变量64变量64字符
//+------+------+------+------+------+------+------+
//
//KBLBTTL：
//+——+——————————+————————+————————+————————+————————+——————+
//类型过期文件号偏移大小压缩
//+——+——————————+————————+————————+————————+————————+——————+
//字符变量64变量64变量64变量64字符
//+——+——————————+————————+————————+————————+————————+——————+
//
//因为我们可以将其存储为普通类型，所以没有Kinlined（不带TTL）类型
//值（即valueType:：ktypeValue）。
class BlobIndex {
 public:
  enum class Type : unsigned char {
    kInlinedTTL = 0,
    kBlob = 1,
    kBlobTTL = 2,
    kUnknown = 3,
  };

  BlobIndex() : type_(Type::kUnknown) {}

  bool IsInlined() const { return type_ == Type::kInlinedTTL; }

  bool HasTTL() const {
    return type_ == Type::kInlinedTTL || type_ == Type::kBlobTTL;
  }

  uint64_t expiration() const {
    assert(HasTTL());
    return expiration_;
  }

  const Slice& value() const {
    assert(IsInlined());
    return value_;
  }

  uint64_t file_number() const {
    assert(!IsInlined());
    return file_number_;
  }

  uint64_t offset() const {
    assert(!IsInlined());
    return offset_;
  }

  uint64_t size() const {
    assert(!IsInlined());
    return size_;
  }

  Status DecodeFrom(Slice slice) {
    static const std::string kErrorMessage = "Error while decoding blob index";
    assert(slice.size() > 0);
    type_ = static_cast<Type>(*slice.data());
    if (type_ >= Type::kUnknown) {
      return Status::Corruption(
          kErrorMessage,
          "Unknown blob index type: " + ToString(static_cast<char>(type_)));
    }
    slice = Slice(slice.data() + 1, slice.size() - 1);
    if (HasTTL()) {
      if (!GetVarint64(&slice, &expiration_)) {
        return Status::Corruption(kErrorMessage, "Corrupted expiration");
      }
    }
    if (IsInlined()) {
      value_ = slice;
    } else {
      if (GetVarint64(&slice, &file_number_) && GetVarint64(&slice, &offset_) &&
          GetVarint64(&slice, &size_) && slice.size() == 1) {
        compression_ = static_cast<CompressionType>(*slice.data());
      } else {
        return Status::Corruption(kErrorMessage, "Corrupted blob offset");
      }
    }
    return Status::OK();
  }

  static void EncodeInlinedTTL(std::string* dst, uint64_t expiration,
                               const Slice& value) {
    assert(dst != nullptr);
    dst->clear();
    dst->reserve(1 + kMaxVarint64Length + value.size());
    dst->push_back(static_cast<char>(Type::kInlinedTTL));
    PutVarint64(dst, expiration);
    dst->append(value.data(), value.size());
  }

  static void EncodeBlob(std::string* dst, uint64_t file_number,
                         uint64_t offset, uint64_t size,
                         CompressionType compression) {
    assert(dst != nullptr);
    dst->clear();
    dst->reserve(kMaxVarint64Length * 3 + 2);
    dst->push_back(static_cast<char>(Type::kBlob));
    PutVarint64(dst, file_number);
    PutVarint64(dst, offset);
    PutVarint64(dst, size);
    dst->push_back(static_cast<char>(compression));
  }

  static void EncodeBlobTTL(std::string* dst, uint64_t expiration,
                            uint64_t file_number, uint64_t offset,
                            uint64_t size, CompressionType compression) {
    assert(dst != nullptr);
    dst->clear();
    dst->reserve(kMaxVarint64Length * 4 + 2);
    dst->push_back(static_cast<char>(Type::kBlobTTL));
    PutVarint64(dst, expiration);
    PutVarint64(dst, file_number);
    PutVarint64(dst, offset);
    PutVarint64(dst, size);
    dst->push_back(static_cast<char>(compression));
  }

 private:
  Type type_ = Type::kUnknown;
  uint64_t expiration_ = 0;
  Slice value_;
  uint64_t file_number_ = 0;
  uint64_t offset_ = 0;
  uint64_t size_ = 0;
  CompressionType compression_ = kNoCompression;
};

}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //摇滚乐
