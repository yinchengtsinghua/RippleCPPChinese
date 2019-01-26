
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
#ifndef ROCKSDB_LITE

#include <vector>
#include "util/testharness.h"
#include "util/testutil.h"
#include "utilities/col_buf_decoder.h"
#include "utilities/col_buf_encoder.h"

namespace rocksdb {

class ColumnAwareEncodingTest : public testing::Test {
 public:
  ColumnAwareEncodingTest() {}

  ~ColumnAwareEncodingTest() {}
};

class ColumnAwareEncodingTestWithSize
    : public ColumnAwareEncodingTest,
      public testing::WithParamInterface<size_t> {
 public:
  ColumnAwareEncodingTestWithSize() {}

  ~ColumnAwareEncodingTestWithSize() {}

  static std::vector<size_t> GetValues() { return {4, 8}; }
};

INSTANTIATE_TEST_CASE_P(
    ColumnAwareEncodingTestWithSize, ColumnAwareEncodingTestWithSize,
    ::testing::ValuesIn(ColumnAwareEncodingTestWithSize::GetValues()));

TEST_P(ColumnAwareEncodingTestWithSize, NoCompressionEncodeDecode) {
  size_t col_size = GetParam();
  std::unique_ptr<ColBufEncoder> col_buf_encoder(
      new FixedLengthColBufEncoder(col_size, kColNoCompression, false, true));
  std::string str_buf;
  uint64_t base_val = 0x0102030405060708;
  uint64_t val = 0;
  memcpy(&val, &base_val, col_size);
  const int row_count = 4;
  for (int i = 0; i < row_count; ++i) {
    str_buf.append(reinterpret_cast<char*>(&val), col_size);
  }
  const char* str_buf_ptr = str_buf.c_str();
  for (int i = 0; i < row_count; ++i) {
    col_buf_encoder->Append(str_buf_ptr);
  }
  col_buf_encoder->Finish();
  const std::string& encoded_data = col_buf_encoder->GetData();
//检查编码字符串长度的正确性
  ASSERT_EQ(row_count * col_size, encoded_data.size());

  const char* encoded_data_ptr = encoded_data.c_str();
  uint64_t expected_encoded_val;
  if (col_size == 8) {
    expected_encoded_val = port::kLittleEndian ? 0x0807060504030201 : 0x0102030405060708;
  } else if (col_size == 4) {
    expected_encoded_val = port::kLittleEndian ? 0x08070605 : 0x0102030400000000;
  }
  uint64_t encoded_val = 0;
  for (int i = 0; i < row_count; ++i) {
    memcpy(&encoded_val, encoded_data_ptr, col_size);
//检查编码值的正确性
    ASSERT_EQ(expected_encoded_val, encoded_val);
    encoded_data_ptr += col_size;
  }

  std::unique_ptr<ColBufDecoder> col_buf_decoder(
      new FixedLengthColBufDecoder(col_size, kColNoCompression, false, true));
  encoded_data_ptr = encoded_data.c_str();
  encoded_data_ptr += col_buf_decoder->Init(encoded_data_ptr);
  char* decoded_data = new char[100];
  char* decoded_data_base = decoded_data;
  for (int i = 0; i < row_count; ++i) {
    encoded_data_ptr +=
        col_buf_decoder->Decode(encoded_data_ptr, &decoded_data);
  }

//检查解码字符串长度的正确性
  ASSERT_EQ(row_count * col_size, decoded_data - decoded_data_base);
  decoded_data = decoded_data_base;
  for (int i = 0; i < row_count; ++i) {
    uint64_t decoded_val;
    decoded_val = 0;
    memcpy(&decoded_val, decoded_data, col_size);
//检查解码值的正确性
    ASSERT_EQ(val, decoded_val);
    decoded_data += col_size;
  }
  delete[] decoded_data_base;
}

TEST_P(ColumnAwareEncodingTestWithSize, RleEncodeDecode) {
  size_t col_size = GetParam();
  std::unique_ptr<ColBufEncoder> col_buf_encoder(
      new FixedLengthColBufEncoder(col_size, kColRle, false, true));
  std::string str_buf;
  uint64_t base_val = 0x0102030405060708;
  uint64_t val = 0;
  memcpy(&val, &base_val, col_size);
  const int row_count = 4;
  for (int i = 0; i < row_count; ++i) {
    str_buf.append(reinterpret_cast<char*>(&val), col_size);
  }
  const char* str_buf_ptr = str_buf.c_str();
  for (int i = 0; i < row_count; ++i) {
    str_buf_ptr += col_buf_encoder->Append(str_buf_ptr);
  }
  col_buf_encoder->Finish();
  const std::string& encoded_data = col_buf_encoder->GetData();
//检查编码字符串长度的正确性
  ASSERT_EQ(col_size + 1, encoded_data.size());

  const char* encoded_data_ptr = encoded_data.c_str();
  uint64_t encoded_val = 0;
  memcpy(&encoded_val, encoded_data_ptr, col_size);
  uint64_t expected_encoded_val;
  if (col_size == 8) {
    expected_encoded_val = port::kLittleEndian ? 0x0807060504030201 : 0x0102030405060708;
  } else if (col_size == 4) {
    expected_encoded_val = port::kLittleEndian ? 0x08070605 : 0x0102030400000000;
  }
//检查编码值的正确性
  ASSERT_EQ(expected_encoded_val, encoded_val);

  std::unique_ptr<ColBufDecoder> col_buf_decoder(
      new FixedLengthColBufDecoder(col_size, kColRle, false, true));
  char* decoded_data = new char[100];
  char* decoded_data_base = decoded_data;
  encoded_data_ptr += col_buf_decoder->Init(encoded_data_ptr);
  for (int i = 0; i < row_count; ++i) {
    encoded_data_ptr +=
        col_buf_decoder->Decode(encoded_data_ptr, &decoded_data);
  }
//检查解码字符串长度的正确性
  ASSERT_EQ(decoded_data - decoded_data_base, row_count * col_size);
  decoded_data = decoded_data_base;
  for (int i = 0; i < row_count; ++i) {
    uint64_t decoded_val;
    decoded_val = 0;
    memcpy(&decoded_val, decoded_data, col_size);
//检查解码值的正确性
    ASSERT_EQ(val, decoded_val);
    decoded_data += col_size;
  }
  delete[] decoded_data_base;
}

TEST_P(ColumnAwareEncodingTestWithSize, DeltaEncodeDecode) {
  size_t col_size = GetParam();
  int row_count = 4;
  std::unique_ptr<ColBufEncoder> col_buf_encoder(
      new FixedLengthColBufEncoder(col_size, kColDeltaVarint, false, true));
  std::string str_buf;
  uint64_t base_val1 = port::kLittleEndian ? 0x0102030405060708 : 0x0807060504030201;
  uint64_t base_val2 = port::kLittleEndian ? 0x0202030405060708 : 0x0807060504030202;
  uint64_t val1 = 0, val2 = 0;
  memcpy(&val1, &base_val1, col_size);
  memcpy(&val2, &base_val2, col_size);
  const char* str_buf_ptr;
  for (int i = 0; i < row_count / 2; ++i) {
    str_buf = std::string(reinterpret_cast<char*>(&val1), col_size);
    str_buf_ptr = str_buf.c_str();
    col_buf_encoder->Append(str_buf_ptr);

    str_buf = std::string(reinterpret_cast<char*>(&val2), col_size);
    str_buf_ptr = str_buf.c_str();
    col_buf_encoder->Append(str_buf_ptr);
  }
  col_buf_encoder->Finish();
  const std::string& encoded_data = col_buf_encoder->GetData();
//检查编码字符串长度
  int varint_len = 0;
  if (col_size == 8) {
    varint_len = 9;
  } else if (col_size == 4) {
    varint_len = port::kLittleEndian ? 5 : 9;
  }
//检查编码字符串长度：第一个值是原始值（val-0），而
//接下来的三个被编码为1、-1、1，所以它们在变量中应该取1字节。
  ASSERT_EQ(varint_len + 3 * 1, encoded_data.size());

  std::unique_ptr<ColBufDecoder> col_buf_decoder(
      new FixedLengthColBufDecoder(col_size, kColDeltaVarint, false, true));
  char* decoded_data = new char[100];
  char* decoded_data_base = decoded_data;
  const char* encoded_data_ptr = encoded_data.c_str();
  encoded_data_ptr += col_buf_decoder->Init(encoded_data_ptr);
  for (int i = 0; i < row_count; ++i) {
    encoded_data_ptr +=
        col_buf_decoder->Decode(encoded_data_ptr, &decoded_data);
  }

//检查解码字符串长度的正确性
  ASSERT_EQ(row_count * col_size, decoded_data - decoded_data_base);
  decoded_data = decoded_data_base;

//检查解码数据的正确性
  for (int i = 0; i < row_count / 2; ++i) {
    uint64_t decoded_val = 0;
    memcpy(&decoded_val, decoded_data, col_size);
    ASSERT_EQ(val1, decoded_val);
    decoded_data += col_size;
    memcpy(&decoded_val, decoded_data, col_size);
    ASSERT_EQ(val2, decoded_val);
    decoded_data += col_size;
  }
  delete[] decoded_data_base;
}

TEST_F(ColumnAwareEncodingTest, ChunkBufEncodeDecode) {
  std::unique_ptr<ColBufEncoder> col_buf_encoder(
      new VariableChunkColBufEncoder(kColDict));
  std::string buf("12345678\377\1\0\0\0\0\0\0\0\376", 18);
  col_buf_encoder->Append(buf.c_str());
  col_buf_encoder->Finish();
  const std::string& encoded_data = col_buf_encoder->GetData();
  const char* str_ptr = encoded_data.c_str();

  std::unique_ptr<ColBufDecoder> col_buf_decoder(
      new VariableChunkColBufDecoder(kColDict));
  str_ptr += col_buf_decoder->Init(str_ptr);
  char* decoded_data = new char[100];
  char* decoded_data_base = decoded_data;
  col_buf_decoder->Decode(str_ptr, &decoded_data);
  for (size_t i = 0; i < buf.size(); ++i) {
    ASSERT_EQ(buf[i], decoded_data_base[i]);
  }
  delete[] decoded_data_base;
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else

#include <cstdio>

int main() {
  fprintf(stderr,
          "SKIPPED as column aware encoding experiment is not enabled in "
          "ROCKSDB_LITE\n");
}
#endif  //摇滚乐
