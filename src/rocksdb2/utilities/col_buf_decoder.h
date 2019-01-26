
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
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "util/coding.h"
#include "utilities/col_buf_encoder.h"

namespace rocksdb {

struct ColDeclaration;

//colbufdecode是一个用于解码列缓冲区的类。它可以由
//冷裂。在开始解码之前，应该调用init（）方法。
//每次在decode（）方法中使用一个列值时。
class ColBufDecoder {
 public:
  virtual ~ColBufDecoder() = 0;
  virtual size_t Init(const char* src) { return 0; }
  virtual size_t Decode(const char* src, char** dest) = 0;
  static ColBufDecoder* NewColBufDecoder(const ColDeclaration& col_declaration);

 protected:
  std::string buffer_;
  static inline bool IsRunLength(ColCompressionType type) {
    return type == kColRle || type == kColRleVarint ||
           type == kColRleDeltaVarint || type == kColRleDict;
  }
};

class FixedLengthColBufDecoder : public ColBufDecoder {
 public:
  explicit FixedLengthColBufDecoder(
      size_t size, ColCompressionType col_compression_type = kColNoCompression,
      bool nullable = false, bool big_endian = false)
      : size_(size),
        col_compression_type_(col_compression_type),
        nullable_(nullable),
        big_endian_(big_endian) {}

  size_t Init(const char* src) override;
  size_t Decode(const char* src, char** dest) override;
  ~FixedLengthColBufDecoder() {}

 private:
  size_t size_;
  ColCompressionType col_compression_type_;
  bool nullable_;
  bool big_endian_;

//用于解码
  std::vector<uint64_t> dict_vec_;
  uint64_t remain_runs_;
  uint64_t run_val_;
  uint64_t last_val_;
};

class LongFixedLengthColBufDecoder : public ColBufDecoder {
 public:
  LongFixedLengthColBufDecoder(size_t size, bool nullable)
      : size_(size), nullable_(nullable) {}

  size_t Decode(const char* src, char** dest) override;
  ~LongFixedLengthColBufDecoder() {}

 private:
  size_t size_;
  bool nullable_;
};

class VariableLengthColBufDecoder : public ColBufDecoder {
 public:
  size_t Decode(const char* src, char** dest) override;
  ~VariableLengthColBufDecoder() {}
};

class VariableChunkColBufDecoder : public VariableLengthColBufDecoder {
 public:
  size_t Init(const char* src) override;
  size_t Decode(const char* src, char** dest) override;
  explicit VariableChunkColBufDecoder(ColCompressionType col_compression_type)
      : col_compression_type_(col_compression_type) {}
  VariableChunkColBufDecoder() : col_compression_type_(kColNoCompression) {}

 private:
  ColCompressionType col_compression_type_;
  std::unordered_map<uint64_t, uint64_t> dictionary_;
  std::vector<uint64_t> dict_vec_;
};

struct KVPairColBufDecoders {
  std::vector<std::unique_ptr<ColBufDecoder>> key_col_bufs;
  std::vector<std::unique_ptr<ColBufDecoder>> value_col_bufs;
  std::unique_ptr<ColBufDecoder> value_checksum_buf;

  explicit KVPairColBufDecoders(const KVPairColDeclarations& kvp_cd) {
    for (auto kcd : *kvp_cd.key_col_declarations) {
      key_col_bufs.emplace_back(
          std::move(ColBufDecoder::NewColBufDecoder(kcd)));
    }
    for (auto vcd : *kvp_cd.value_col_declarations) {
      value_col_bufs.emplace_back(
          std::move(ColBufDecoder::NewColBufDecoder(vcd)));
    }
    value_checksum_buf.reset(
        ColBufDecoder::NewColBufDecoder(*kvp_cd.value_checksum_declaration));
  }
};
}  //命名空间rocksdb
