
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
//
//Endian中性编码：
//*固定长度的数字先用最低有效字节编码
//*此外，我们还支持可变长度“varint”编码
//*字符串以变量格式的长度作为前缀进行编码。

#pragma once
#include <algorithm>
#include <stdint.h>
#include <string.h>
#include <string>

#include "rocksdb/write_batch.h"
#include "port/port.h"

//某些处理器不允许未对齐的内存访问
#if defined(__sparc)
  #define PLATFORM_UNALIGNED_ACCESS_NOT_ALLOWED
#endif

namespace rocksdb {

//以字节为单位的变量的最大长度（64位）。
const unsigned int kMaxVarint64Length = 10;

//标准投入…附加到字符串的例程
extern void PutFixed32(std::string* dst, uint32_t value);
extern void PutFixed64(std::string* dst, uint64_t value);
extern void PutVarint32(std::string* dst, uint32_t value);
extern void PutVarint32Varint32(std::string* dst, uint32_t value1,
                                uint32_t value2);
extern void PutVarint32Varint32Varint32(std::string* dst, uint32_t value1,
                                        uint32_t value2, uint32_t value3);
extern void PutVarint64(std::string* dst, uint64_t value);
extern void PutVarint64Varint64(std::string* dst, uint64_t value1,
                                uint64_t value2);
extern void PutVarint32Varint64(std::string* dst, uint32_t value1,
                                uint64_t value2);
extern void PutVarint32Varint32Varint64(std::string* dst, uint32_t value1,
                                        uint32_t value2, uint64_t value3);
extern void PutLengthPrefixedSlice(std::string* dst, const Slice& value);
extern void PutLengthPrefixedSliceParts(std::string* dst,
                                        const SliceParts& slice_parts);

//标准获取…例程从切片的开头分析值
//并将切片推进到解析值之后。
extern bool GetFixed64(Slice* input, uint64_t* value);
extern bool GetFixed32(Slice* input, uint32_t* value);
extern bool GetVarint32(Slice* input, uint32_t* value);
extern bool GetVarint64(Slice* input, uint64_t* value);
extern bool GetLengthPrefixedSlice(Slice* input, Slice* result);
//此函数假定数据格式正确。
extern Slice GetLengthPrefixedSlice(const char* data);

extern Slice GetSliceUntil(Slice* slice, char delimiter);

//基于指针的getvarint变量…它们要么存储一个值
//在*v中，返回一个指针，刚好超过解析值，或者返回
//出错时为空指针。这些例程只查看范围内的字节
//[P.LimIT-1 ]
extern const char* GetVarint32Ptr(const char* p,const char* limit, uint32_t* v);
extern const char* GetVarint64Ptr(const char* p,const char* limit, uint64_t* v);

//返回varint32或varint64编码“v”的长度
extern int VarintLength(uint64_t v);

//Put的低级版本…直接写入字符缓冲区的
//要求：DST有足够的空间用于写入值
extern void EncodeFixed32(char* dst, uint32_t value);
extern void EncodeFixed64(char* dst, uint64_t value);

//Put的低级版本…直接写入字符缓冲区的
//返回一个指针，刚好超过最后一个写入的字节。
//要求：DST有足够的空间用于写入值
extern char* EncodeVarint32(char* dst, uint32_t value);
extern char* EncodeVarint64(char* dst, uint64_t value);

//GeT的低级版本…直接从字符缓冲区读取的
//没有任何边界检查。

inline uint32_t DecodeFixed32(const char* ptr) {
  if (port::kLittleEndian) {
//加载原始字节
    uint32_t result;
memcpy(&result, ptr, sizeof(result));  //GCC将其优化为普通负载
    return result;
  } else {
    return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0])))
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8)
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16)
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
  }
}

inline uint64_t DecodeFixed64(const char* ptr) {
  if (port::kLittleEndian) {
//加载原始字节
    uint64_t result;
memcpy(&result, ptr, sizeof(result));  //GCC将其优化为普通负载
    return result;
  } else {
    uint64_t lo = DecodeFixed32(ptr);
    uint64_t hi = DecodeFixed32(ptr + 4);
    return (hi << 32) | lo;
  }
}

//供getvarint32ptr的回退路径使用的内部例程
extern const char* GetVarint32PtrFallback(const char* p,
                                          const char* limit,
                                          uint32_t* value);
inline const char* GetVarint32Ptr(const char* p,
                                  const char* limit,
                                  uint32_t* value) {
  if (p < limit) {
    uint32_t result = *(reinterpret_cast<const unsigned char*>(p));
    if ((result & 128) == 0) {
      *value = result;
      return p + 1;
    }
  }
  return GetVarint32PtrFallback(p, limit, value);
}

//——上述功能的实现
inline void EncodeFixed32(char* buf, uint32_t value) {
  if (port::kLittleEndian) {
    memcpy(buf, &value, sizeof(value));
  } else {
    buf[0] = value & 0xff;
    buf[1] = (value >> 8) & 0xff;
    buf[2] = (value >> 16) & 0xff;
    buf[3] = (value >> 24) & 0xff;
  }
}

inline void EncodeFixed64(char* buf, uint64_t value) {
  if (port::kLittleEndian) {
    memcpy(buf, &value, sizeof(value));
  } else {
    buf[0] = value & 0xff;
    buf[1] = (value >> 8) & 0xff;
    buf[2] = (value >> 16) & 0xff;
    buf[3] = (value >> 24) & 0xff;
    buf[4] = (value >> 32) & 0xff;
    buf[5] = (value >> 40) & 0xff;
    buf[6] = (value >> 48) & 0xff;
    buf[7] = (value >> 56) & 0xff;
  }
}

//拉最后8位并将其转换为字符
inline void PutFixed32(std::string* dst, uint32_t value) {
  if (port::kLittleEndian) {
    dst->append(const_cast<const char*>(reinterpret_cast<char*>(&value)),
      sizeof(value));
  } else {
    char buf[sizeof(value)];
    EncodeFixed32(buf, value);
    dst->append(buf, sizeof(buf));
  }
}

inline void PutFixed64(std::string* dst, uint64_t value) {
  if (port::kLittleEndian) {
    dst->append(const_cast<const char*>(reinterpret_cast<char*>(&value)),
      sizeof(value));
  } else {
    char buf[sizeof(value)];
    EncodeFixed64(buf, value);
    dst->append(buf, sizeof(buf));
  }
}

inline void PutVarint32(std::string* dst, uint32_t v) {
  char buf[5];
  char* ptr = EncodeVarint32(buf, v);
  dst->append(buf, static_cast<size_t>(ptr - buf));
}

inline void PutVarint32Varint32(std::string* dst, uint32_t v1, uint32_t v2) {
  char buf[10];
  char* ptr = EncodeVarint32(buf, v1);
  ptr = EncodeVarint32(ptr, v2);
  dst->append(buf, static_cast<size_t>(ptr - buf));
}

inline void PutVarint32Varint32Varint32(std::string* dst, uint32_t v1,
                                        uint32_t v2, uint32_t v3) {
  char buf[15];
  char* ptr = EncodeVarint32(buf, v1);
  ptr = EncodeVarint32(ptr, v2);
  ptr = EncodeVarint32(ptr, v3);
  dst->append(buf, static_cast<size_t>(ptr - buf));
}

inline char* EncodeVarint64(char* dst, uint64_t v) {
  static const unsigned int B = 128;
  unsigned char* ptr = reinterpret_cast<unsigned char*>(dst);
  while (v >= B) {
    *(ptr++) = (v & (B - 1)) | B;
    v >>= 7;
  }
  *(ptr++) = static_cast<unsigned char>(v);
  return reinterpret_cast<char*>(ptr);
}

inline void PutVarint64(std::string* dst, uint64_t v) {
  char buf[10];
  char* ptr = EncodeVarint64(buf, v);
  dst->append(buf, static_cast<size_t>(ptr - buf));
}

inline void PutVarint64Varint64(std::string* dst, uint64_t v1, uint64_t v2) {
  char buf[20];
  char* ptr = EncodeVarint64(buf, v1);
  ptr = EncodeVarint64(ptr, v2);
  dst->append(buf, static_cast<size_t>(ptr - buf));
}

inline void PutVarint32Varint64(std::string* dst, uint32_t v1, uint64_t v2) {
  char buf[15];
  char* ptr = EncodeVarint32(buf, v1);
  ptr = EncodeVarint64(ptr, v2);
  dst->append(buf, static_cast<size_t>(ptr - buf));
}

inline void PutVarint32Varint32Varint64(std::string* dst, uint32_t v1,
                                        uint32_t v2, uint64_t v3) {
  char buf[20];
  char* ptr = EncodeVarint32(buf, v1);
  ptr = EncodeVarint32(ptr, v2);
  ptr = EncodeVarint64(ptr, v3);
  dst->append(buf, static_cast<size_t>(ptr - buf));
}

inline void PutLengthPrefixedSlice(std::string* dst, const Slice& value) {
  PutVarint32(dst, static_cast<uint32_t>(value.size()));
  dst->append(value.data(), value.size());
}

inline void PutLengthPrefixedSliceParts(std::string* dst,
                                        const SliceParts& slice_parts) {
  size_t total_bytes = 0;
  for (int i = 0; i < slice_parts.num_parts; ++i) {
    total_bytes += slice_parts.parts[i].size();
  }
  PutVarint32(dst, static_cast<uint32_t>(total_bytes));
  for (int i = 0; i < slice_parts.num_parts; ++i) {
    dst->append(slice_parts.parts[i].data(), slice_parts.parts[i].size());
  }
}

inline int VarintLength(uint64_t v) {
  int len = 1;
  while (v >= 128) {
    v >>= 7;
    len++;
  }
  return len;
}

inline bool GetFixed64(Slice* input, uint64_t* value) {
  if (input->size() < sizeof(uint64_t)) {
    return false;
  }
  *value = DecodeFixed64(input->data());
  input->remove_prefix(sizeof(uint64_t));
  return true;
}

inline bool GetFixed32(Slice* input, uint32_t* value) {
  if (input->size() < sizeof(uint32_t)) {
    return false;
  }
  *value = DecodeFixed32(input->data());
  input->remove_prefix(sizeof(uint32_t));
  return true;
}

inline bool GetVarint32(Slice* input, uint32_t* value) {
  const char* p = input->data();
  const char* limit = p + input->size();
  const char* q = GetVarint32Ptr(p, limit, value);
  if (q == nullptr) {
    return false;
  } else {
    *input = Slice(q, static_cast<size_t>(limit - q));
    return true;
  }
}

inline bool GetVarint64(Slice* input, uint64_t* value) {
  const char* p = input->data();
  const char* limit = p + input->size();
  const char* q = GetVarint64Ptr(p, limit, value);
  if (q == nullptr) {
    return false;
  } else {
    *input = Slice(q, static_cast<size_t>(limit - q));
    return true;
  }
}

//为平台无关的端节点转换提供接口
inline uint64_t EndianTransform(uint64_t input, size_t size) {
  char* pos = reinterpret_cast<char*>(&input);
  uint64_t ret_val = 0;
  for (size_t i = 0; i < size; ++i) {
    ret_val |= (static_cast<uint64_t>(static_cast<unsigned char>(pos[i]))
                << ((size - i - 1) << 3));
  }
  return ret_val;
}

inline bool GetLengthPrefixedSlice(Slice* input, Slice* result) {
  uint32_t len = 0;
  if (GetVarint32(input, &len) && input->size() >= len) {
    *result = Slice(input->data(), len);
    input->remove_prefix(len);
    return true;
  } else {
    return false;
  }
}

inline Slice GetLengthPrefixedSlice(const char* data) {
  uint32_t len = 0;
//+5：我们假设“数据”没有损坏
//无符号字符为7位，uint32_t为32位，需要5个无符号字符
  /*o p=getvarint32ptr（数据，数据+5/*限制*/，&len）；
  返回切片（p，len）；
}

inline slice getsliceuntil（slice*slice，char delimiter）
  uint32_t len=0；
  对于（len=0；len<slice->size（）&&slice->data（）[len]！=分隔符；+len）
    /没有
  }

  slice ret（slice->data（），len）；
  slice->remove_prefix（len+（（len<slice->size（））？1、0）；
  返回RET；
}

模板<class t>
ifdef rocksdb_ubsan_run
如果定义了（clang_）
_uuu属性_uuu（（uu no_sanitize_uuu（“对齐”））
定义的elif
_uuu属性_uuu（（uu no_sanitize_undefined_uuu））。
第二节
第二节
inline void putunaligented（t*内存，常量t&value）
如果定义了（不允许平台未对齐的访问）
  char*nonalinedmory=reinterpret_cast<char*>（内存）；
  memcpy（非对齐的内存，reinterpret_cast<const char*>（&value），sizeof（t））；
否则
  *内存=值；
第二节
}

模板<class t>
ifdef rocksdb_ubsan_run
如果定义了（clang_）
_uuu属性_uuu（（uu no_sanitize_uuu（“对齐”））
定义的elif
_uuu属性_uuu（（uu no_sanitize_undefined_uuu））。
第二节
第二节
inline void getunaligned（const t*内存，t*值）
如果定义了（不允许平台未对齐的访问）
  char*nonalinedmory=reinterpret_cast<char*>（value）；
  memcpy（非对齐的内存，reinterpret_cast<const char*>（memory），sizeof（t））；
否则
  *值=*内存；
第二节
}

//命名空间rocksdb
