
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2017至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

/*
 *序列化和反序列化整数的助手函数
 *在big endian中转换为字节。
 **/


#pragma once

namespace rocksdb {
namespace cassandra {
namespace {
const int64_t kCharMask = 0xFFLL;
const int32_t kBitsPerByte = 8;
}

template<typename T>
void Serialize(T val, std::string* dest);

template<typename T>
T Deserialize(const char* src, std::size_t offset=0);

//专业化
template<>
inline void Serialize<int8_t>(int8_t t, std::string* dest) {
  dest->append(1, static_cast<char>(t & kCharMask));
}

template<>
inline void Serialize<int32_t>(int32_t t, std::string* dest) {
  for (unsigned long i = 0; i < sizeof(int32_t); i++) {
     dest->append(1, static_cast<char>(
       (t >> (sizeof(int32_t) - 1 - i) * kBitsPerByte) & kCharMask));
  }
}

template<>
inline void Serialize<int64_t>(int64_t t, std::string* dest) {
  for (unsigned long i = 0; i < sizeof(int64_t); i++) {
     dest->append(
       1, static_cast<char>(
         (t >> (sizeof(int64_t) - 1 - i) * kBitsPerByte) & kCharMask));
  }
}

template<>
inline int8_t Deserialize<int8_t>(const char* src, std::size_t offset) {
  return static_cast<int8_t>(src[offset]);
}

template<>
inline int32_t Deserialize<int32_t>(const char* src, std::size_t offset) {
  int32_t result = 0;
  for (unsigned long i = 0; i < sizeof(int32_t); i++) {
    result |= static_cast<int32_t>(static_cast<unsigned char>(src[offset + i]))
        << ((sizeof(int32_t) - 1 - i) * kBitsPerByte);
  }
  return result;
}

template<>
inline int64_t Deserialize<int64_t>(const char* src, std::size_t offset) {
  int64_t result = 0;
  for (unsigned long i = 0; i < sizeof(int64_t); i++) {
    result |= static_cast<int64_t>(static_cast<unsigned char>(src[offset + i]))
        << ((sizeof(int64_t) - 1 - i) * kBitsPerByte);
  }
  return result;
}

} //名字卡桑德达
} //命名空间rocksdb
