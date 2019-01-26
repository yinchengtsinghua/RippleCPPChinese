
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

namespace rocksdb {

enum ColCompressionType {
  kColNoCompression,
  kColRle,
  kColVarint,
  kColRleVarint,
  kColDeltaVarint,
  kColRleDeltaVarint,
  kColDict,
  kColRleDict
};

struct ColDeclaration;

//colbufencoder是一个对列缓冲区进行编码的类。它可以由
//冷裂。每次将列值带入append（）方法
//对列进行编码并将其存储到内部缓冲区中。在所有行之后
//此列已被使用，应调用finish（）以添加头和
//剩余数据。
class ColBufEncoder {
 public:
//读取一列，对数据进行编码并附加到内部缓冲区中。
  virtual size_t Append(const char *buf) = 0;
  virtual ~ColBufEncoder() = 0;
//获取内部列缓冲区。只应在finish（）之后调用。
  const std::string &GetData();
//完成编码。添加标题和剩余数据。
  virtual void Finish() = 0;
//从coldbufencoder中填充coldbufencoder。
  static ColBufEncoder *NewColBufEncoder(const ColDeclaration &col_declaration);

 protected:
  std::string buffer_;
  static inline bool IsRunLength(ColCompressionType type) {
    return type == kColRle || type == kColRleVarint ||
           type == kColRleDeltaVarint || type == kColRleDict;
  }
};

//定长列缓冲区编码器。在定长列缓冲区中，
//列的大小不应超过8个字节。
//支持以下编码：
//varint：变长整数。有关详细信息，请参阅util/coding.h。
//RLE（运行长度编码）：将连续值序列编码为
//[运行值][运行长度]。可与变量组合
//delta：将值与其相邻项一起编码到delta。使用变量
//可能会减少存储字节。可与RLE结合使用。
//字典：使用字典记录块中的所有可能值和
//使用从0开始的ID对它们进行编码。ID被编码为变量。专栏
//字典编码将有一个头来存储所有实际值，
//按字典值排序，数据将替换为
//字典值。可与RLE结合使用。
class FixedLengthColBufEncoder : public ColBufEncoder {
 public:
  explicit FixedLengthColBufEncoder(
      size_t size, ColCompressionType col_compression_type = kColNoCompression,
      bool nullable = false, bool big_endian = false)
      : size_(size),
        col_compression_type_(col_compression_type),
        nullable_(nullable),
        big_endian_(big_endian),
        last_val_(0),
        run_length_(-1),
        run_val_(0) {}

  size_t Append(const char *buf) override;
  void Finish() override;
  ~FixedLengthColBufEncoder() {}

 private:
  size_t size_;
  ColCompressionType col_compression_type_;
//如果设置为真，输入值可以为空（表示为nullptr）。什么时候？
//nullable为true，请在实际值之前再使用一个字节来指示
//当前值为空。
  bool nullable_;
//如果设置为真，输入值将被视为big endian编码。
  bool big_endian_;

//用于编码
  uint64_t last_val_;
  int16_t run_length_;
  uint64_t run_val_;
//映射到存储字典以进行字典编码
  std::unordered_map<uint64_t, uint64_t> dictionary_;
//字典键的矢量。
  std::vector<uint64_t> dict_vec_;
};

//长固定长度列缓冲区是要保留的固定长度缓冲区的变体
//超过8字节的固定长度缓冲区。我们不支持任何特别的
//在longfixedlengthcolbufencoder中的编码方案。
class LongFixedLengthColBufEncoder : public ColBufEncoder {
 public:
  LongFixedLengthColBufEncoder(size_t size, bool nullable)
      : size_(size), nullable_(nullable) {}
  size_t Append(const char *buf) override;
  void Finish() override;

  ~LongFixedLengthColBufEncoder() {}

 private:
  size_t size_;
  bool nullable_;
};

//可变长度列缓冲区保留可变长度列的格式。在
//此格式中，一列由一个字节长度k组成，后跟数据
//k字节长的数据。
class VariableLengthColBufEncoder : public ColBufEncoder {
 public:
  size_t Append(const char *buf) override;
  void Finish() override;

  ~VariableLengthColBufEncoder() {}
};

//可变区块列缓冲区保存可变长度列的另一种格式。
//在这种格式中，一列包含多个数据块，每个数据块
//由8字节长的数据组成，一个字节作为一个掩码来指示
//有更多的数据。如果没有更多的数据，则将掩码设置为0xFF。如果
//块是最后一个块，只有k个有效字节，掩码设置为
//0xFF-（8 -K）。
class VariableChunkColBufEncoder : public VariableLengthColBufEncoder {
 public:
  size_t Append(const char *buf) override;
  void Finish() override;
  explicit VariableChunkColBufEncoder(ColCompressionType col_compression_type)
      : col_compression_type_(col_compression_type) {}
  VariableChunkColBufEncoder() : col_compression_type_(kColNoCompression) {}

 private:
  ColCompressionType col_compression_type_;
//映射到存储字典以进行字典编码
  std::unordered_map<uint64_t, uint64_t> dictionary_;
//字典键的矢量。
  std::vector<uint64_t> dict_vec_;
};

//colderclaration声明列的类型、列感知编码的算法，
//以及其他列数据，如endian和nullability。
struct ColDeclaration {
  explicit ColDeclaration(
      std::string _col_type,
      ColCompressionType _col_compression_type = kColNoCompression,
      size_t _size = 0, bool _nullable = false, bool _big_endian = false)
      : col_type(_col_type),
        col_compression_type(_col_compression_type),
        size(_size),
        nullable(_nullable),
        big_endian(_big_endian) {}
  std::string col_type;
  ColCompressionType col_compression_type;
  size_t size;
  bool nullable;
  bool big_endian;
};

//kvPairColderClarations是一个类，用于保存列的列声明
//关键与价值。
struct KVPairColDeclarations {
  std::vector<ColDeclaration> *key_col_declarations;
  std::vector<ColDeclaration> *value_col_declarations;
  ColDeclaration *value_checksum_declaration;
  KVPairColDeclarations(std::vector<ColDeclaration> *_key_col_declarations,
                        std::vector<ColDeclaration> *_value_col_declarations,
                        ColDeclaration *_value_checksum_declaration)
      : key_col_declarations(_key_col_declarations),
        value_col_declarations(_value_col_declarations),
        value_checksum_declaration(_value_checksum_declaration) {}
};

//与kvPairDeclarations类似，kvPairComBufEncoders用于保存列
//键和值中所有列的缓冲区编码器。
struct KVPairColBufEncoders {
  std::vector<std::unique_ptr<ColBufEncoder>> key_col_bufs;
  std::vector<std::unique_ptr<ColBufEncoder>> value_col_bufs;
  std::unique_ptr<ColBufEncoder> value_checksum_buf;

  explicit KVPairColBufEncoders(const KVPairColDeclarations &kvp_cd) {
    for (auto kcd : *kvp_cd.key_col_declarations) {
      key_col_bufs.emplace_back(
          std::move(ColBufEncoder::NewColBufEncoder(kcd)));
    }
    for (auto vcd : *kvp_cd.value_col_declarations) {
      value_col_bufs.emplace_back(
          std::move(ColBufEncoder::NewColBufEncoder(vcd)));
    }
    value_checksum_buf.reset(
        ColBufEncoder::NewColBufEncoder(*kvp_cd.value_checksum_declaration));
  }

//要调用finish（）的helper函数
  void Finish() {
    for (auto &col_buf : key_col_bufs) {
      col_buf->Finish();
    }
    for (auto &col_buf : value_col_bufs) {
      col_buf->Finish();
    }
    value_checksum_buf->Finish();
  }
};
}  //命名空间rocksdb
