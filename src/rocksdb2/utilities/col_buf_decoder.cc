
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

#include "utilities/col_buf_decoder.h"
#include <cstring>
#include <string>
#include "port/port.h"

namespace rocksdb {

ColBufDecoder::~ColBufDecoder() {}

namespace {

inline uint64_t EncodeFixed64WithEndian(uint64_t val, bool big_endian,
                                        size_t size) {
  if (big_endian && port::kLittleEndian) {
    val = EndianTransform(val, size);
  } else if (!big_endian && !port::kLittleEndian) {
    val = EndianTransform(val, size);
  }
  return val;
}

}  //命名空间

ColBufDecoder* ColBufDecoder::NewColBufDecoder(
    const ColDeclaration& col_declaration) {
  if (col_declaration.col_type == "FixedLength") {
    return new FixedLengthColBufDecoder(
        col_declaration.size, col_declaration.col_compression_type,
        col_declaration.nullable, col_declaration.big_endian);
  } else if (col_declaration.col_type == "VariableLength") {
    return new VariableLengthColBufDecoder();
  } else if (col_declaration.col_type == "VariableChunk") {
    return new VariableChunkColBufDecoder(col_declaration.col_compression_type);
  } else if (col_declaration.col_type == "LongFixedLength") {
    return new LongFixedLengthColBufDecoder(col_declaration.size,
                                            col_declaration.nullable);
  }
//无法识别的列类型
  return nullptr;
}

namespace {

void ReadVarint64(const char** src_ptr, uint64_t* val_ptr) {
  const char* q = GetVarint64Ptr(*src_ptr, *src_ptr + 10, val_ptr);
  assert(q != nullptr);
  *src_ptr = q;
}
}  //命名空间

size_t FixedLengthColBufDecoder::Init(const char* src) {
  remain_runs_ = 0;
  last_val_ = 0;
//字典初始化
  dict_vec_.clear();
  const char* orig_src = src;
  if (col_compression_type_ == kColDict ||
      col_compression_type_ == kColRleDict) {
    const char* q;
    uint64_t dict_size;
//旁路限制
    q = GetVarint64Ptr(src, src + 10, &dict_size);
    assert(q != nullptr);
    src = q;

    uint64_t dict_key;
    for (uint64_t i = 0; i < dict_size; ++i) {
//旁路限制
      ReadVarint64(&src, &dict_key);

      dict_key = EncodeFixed64WithEndian(dict_key, big_endian_, size_);
      dict_vec_.push_back(dict_key);
    }
  }
  return src - orig_src;
}

size_t FixedLengthColBufDecoder::Decode(const char* src, char** dest) {
  uint64_t read_val = 0;
  const char* orig_src = src;
  const char* src_limit = src + 20;
  if (nullable_) {
    bool not_null;
    not_null = *src;
    src += 1;
    if (!not_null) {
      return 1;
    }
  }
  if (IsRunLength(col_compression_type_)) {
    if (remain_runs_ == 0) {
      const char* q;
      run_val_ = 0;
      if (col_compression_type_ == kColRle) {
        memcpy(&run_val_, src, size_);
        src += size_;
      } else {
        q = GetVarint64Ptr(src, src_limit, &run_val_);
        assert(q != nullptr);
        src = q;
      }

      q = GetVarint64Ptr(src, src_limit, &remain_runs_);
      assert(q != nullptr);
      src = q;

      if (col_compression_type_ != kColRleDeltaVarint &&
          col_compression_type_ != kColRleDict) {
        run_val_ = EncodeFixed64WithEndian(run_val_, big_endian_, size_);
      }
    }
    read_val = run_val_;
  } else {
    if (col_compression_type_ == kColNoCompression) {
      memcpy(&read_val, src, size_);
      src += size_;
    } else {
//假设一列在这里不超过8个字节
      const char* q = GetVarint64Ptr(src, src_limit, &read_val);
      assert(q != nullptr);
      src = q;
    }
    if (col_compression_type_ != kColDeltaVarint &&
        col_compression_type_ != kColDict) {
      read_val = EncodeFixed64WithEndian(read_val, big_endian_, size_);
    }
  }

  uint64_t write_val = read_val;
  if (col_compression_type_ == kColDeltaVarint ||
      col_compression_type_ == kColRleDeltaVarint) {
//不支持64位

    uint64_t mask = (write_val & 1) ? (~uint64_t(0)) : 0;
    int64_t delta = (write_val >> 1) ^ mask;
    write_val = last_val_ + delta;

    uint64_t tmp = write_val;
    write_val = EncodeFixed64WithEndian(write_val, big_endian_, size_);
    last_val_ = tmp;
  } else if (col_compression_type_ == kColRleDict ||
             col_compression_type_ == kColDict) {
    uint64_t dict_val = read_val;
    assert(dict_val < dict_vec_.size());
    write_val = dict_vec_[dict_val];
  }

//dest->append（reinterpret_cast<char*>（&write_val），size_uu）；
  memcpy(*dest, reinterpret_cast<char*>(&write_val), size_);
  *dest += size_;
  if (IsRunLength(col_compression_type_)) {
    --remain_runs_;
  }
  return src - orig_src;
}

size_t LongFixedLengthColBufDecoder::Decode(const char* src, char** dest) {
  if (nullable_) {
    bool not_null;
    not_null = *src;
    src += 1;
    if (!not_null) {
      return 1;
    }
  }
  memcpy(*dest, src, size_);
  *dest += size_;
  return size_ + 1;
}

size_t VariableLengthColBufDecoder::Decode(const char* src, char** dest) {
  uint8_t len;
  len = *src;
  memcpy(dest, reinterpret_cast<char*>(&len), 1);
  *dest += 1;
  src += 1;
  memcpy(*dest, src, len);
  *dest += len;
  return len + 1;
}

size_t VariableChunkColBufDecoder::Init(const char* src) {
//字典初始化
  dict_vec_.clear();
  const char* orig_src = src;
  if (col_compression_type_ == kColDict) {
    const char* q;
    uint64_t dict_size;
//旁路限制
    q = GetVarint64Ptr(src, src + 10, &dict_size);
    assert(q != nullptr);
    src = q;

    uint64_t dict_key;
    for (uint64_t i = 0; i < dict_size; ++i) {
//旁路限制
      ReadVarint64(&src, &dict_key);
      dict_vec_.push_back(dict_key);
    }
  }
  return src - orig_src;
}

size_t VariableChunkColBufDecoder::Decode(const char* src, char** dest) {
  const char* orig_src = src;
  uint64_t size = 0;
  ReadVarint64(&src, &size);
  int64_t full_chunks = size / 8;
  uint64_t chunk_buf;
  size_t chunk_size = 8;
  for (int64_t i = 0; i < full_chunks + 1; ++i) {
    chunk_buf = 0;
    if (i == full_chunks) {
      chunk_size = size % 8;
    }
    if (col_compression_type_ == kColDict) {
      uint64_t dict_val;
      ReadVarint64(&src, &dict_val);
      assert(dict_val < dict_vec_.size());
      chunk_buf = dict_vec_[dict_val];
    } else {
      memcpy(&chunk_buf, src, chunk_size);
      src += chunk_size;
    }
    memcpy(*dest, reinterpret_cast<char*>(&chunk_buf), 8);
    *dest += 8;
    uint8_t mask = ((0xFF - 8) + chunk_size) & 0xFF;
    memcpy(*dest, reinterpret_cast<char*>(&mask), 1);
    *dest += 1;
  }

  return src - orig_src;
}

}  //命名空间rocksdb
