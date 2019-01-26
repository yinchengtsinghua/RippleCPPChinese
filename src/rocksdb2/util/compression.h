
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
#pragma once

#include <algorithm>
#include <limits>
#include <string>

#include "rocksdb/options.h"
#include "util/coding.h"

#ifdef SNAPPY
#include <snappy.h>
#endif

#ifdef ZLIB
#include <zlib.h>
#endif

#ifdef BZIP2
#include <bzlib.h>
#endif

#if defined(LZ4)
#include <lz4.h>
#include <lz4hc.h>
#endif

#if defined(ZSTD)
#include <zstd.h>
#endif

#if defined(XPRESS)
#include "port/xpress.h"
#endif

namespace rocksdb {

inline bool Snappy_Supported() {
#ifdef SNAPPY
  return true;
#endif
  return false;
}

inline bool Zlib_Supported() {
#ifdef ZLIB
  return true;
#endif
  return false;
}

inline bool BZip2_Supported() {
#ifdef BZIP2
  return true;
#endif
  return false;
}

inline bool LZ4_Supported() {
#ifdef LZ4
  return true;
#endif
  return false;
}

inline bool XPRESS_Supported() {
#ifdef XPRESS
  return true;
#endif
  return false;
}

inline bool ZSTD_Supported() {
#ifdef ZSTD
//zstd格式自0.8.0版起定稿。
  return (ZSTD_versionNumber() >= 800);
#endif
  return false;
}

inline bool ZSTDNotFinal_Supported() {
#ifdef ZSTD
  return true;
#endif
  return false;
}

inline bool CompressionTypeSupported(CompressionType compression_type) {
  switch (compression_type) {
    case kNoCompression:
      return true;
    case kSnappyCompression:
      return Snappy_Supported();
    case kZlibCompression:
      return Zlib_Supported();
    case kBZip2Compression:
      return BZip2_Supported();
    case kLZ4Compression:
      return LZ4_Supported();
    case kLZ4HCCompression:
      return LZ4_Supported();
    case kXpressCompression:
      return XPRESS_Supported();
    case kZSTDNotFinalCompression:
      return ZSTDNotFinal_Supported();
    case kZSTD:
      return ZSTD_Supported();
    default:
      assert(false);
      return false;
  }
}

inline std::string CompressionTypeToString(CompressionType compression_type) {
  switch (compression_type) {
    case kNoCompression:
      return "NoCompression";
    case kSnappyCompression:
      return "Snappy";
    case kZlibCompression:
      return "Zlib";
    case kBZip2Compression:
      return "BZip2";
    case kLZ4Compression:
      return "LZ4";
    case kLZ4HCCompression:
      return "LZ4HC";
    case kXpressCompression:
      return "Xpress";
    case kZSTD:
    case kZSTDNotFinalCompression:
      return "ZSTD";
    default:
      assert(false);
      return "";
  }
}

//压缩格式版本可以有两个值：
//1——bzip2和zlib的解压缩大小不包含在压缩文件中。
//块。此外，LZ4的解压大小编码为与平台相关的
//方式。
//2——zlib、bzip2和lz4将解压后的大小编码为varint32，就在
//压缩块的开始。Snappy格式与版本1相同。

inline bool Snappy_Compress(const CompressionOptions& opts, const char* input,
                            size_t length, ::std::string* output) {
#ifdef SNAPPY
  output->resize(snappy::MaxCompressedLength(length));
  size_t outlen;
  snappy::RawCompress(input, length, &(*output)[0], &outlen);
  output->resize(outlen);
  return true;
#endif

  return false;
}

inline bool Snappy_GetUncompressedLength(const char* input, size_t length,
                                         size_t* result) {
#ifdef SNAPPY
  return snappy::GetUncompressedLength(input, length, result);
#else
  return false;
#endif
}

inline bool Snappy_Uncompress(const char* input, size_t length,
                              char* output) {
#ifdef SNAPPY
  return snappy::RawUncompress(input, length, output);
#else
  return false;
#endif
}

namespace compression {
//返回大小
inline size_t PutDecompressedSizeInfo(std::string* output, uint32_t length) {
  PutVarint32(output, length);
  return output->size();
}

inline bool GetDecompressedSizeInfo(const char** input_data,
                                    size_t* input_length,
                                    uint32_t* output_len) {
  auto new_input_data =
      GetVarint32Ptr(*input_data, *input_data + *input_length, output_len);
  if (new_input_data == nullptr) {
    return false;
  }
  *input_length -= (new_input_data - *input_data);
  *input_data = new_input_data;
  return true;
}
}  //命名空间压缩

//compress_format_version==1--解压缩后的大小不包括在
//块标题
//compress_format_version==2——解压后的大小包含在块中
//变量32格式的标题
//@param compression\u用于预设压缩库的dict data
//字典。
inline bool Zlib_Compress(const CompressionOptions& opts,
                          uint32_t compress_format_version, const char* input,
                          size_t length, ::std::string* output,
                          const Slice& compression_dict = Slice()) {
#ifdef ZLIB
  if (length > std::numeric_limits<uint32_t>::max()) {
//压缩量不能超过4GB
    return false;
  }

  size_t output_header_len = 0;
  if (compress_format_version == 2) {
    output_header_len = compression::PutDecompressedSizeInfo(
        output, static_cast<uint32_t>(length));
  }
//将输出调整为普通数据长度。
//如果压缩实际扩展了数据，这可能不够大。
  output->resize(output_header_len + length);

//memlevel参数指定应该分配多少内存
//内部压缩状态。
//memlevel=1使用最小内存，但速度较慢，压缩比降低。
//memlevel=9使用最大内存以获得最佳速度。
//默认值为8。有关更多详细信息，请参阅zconf.h。
  static const int memLevel = 8;
  z_stream _stream;
  memset(&_stream, 0, sizeof(z_stream));
  int st = deflateInit2(&_stream, opts.level, Z_DEFLATED, opts.window_bits,
                        memLevel, opts.strategy);
  if (st != Z_OK) {
    return false;
  }

  if (compression_dict.size()) {
//初始化压缩库的字典
    st = deflateSetDictionary(
        &_stream, reinterpret_cast<const Bytef*>(compression_dict.data()),
        static_cast<unsigned int>(compression_dict.size()));
    if (st != Z_OK) {
      deflateEnd(&_stream);
      return false;
    }
  }

//压缩输入，并将压缩数据放入输出。
  _stream.next_in = (Bytef *)input;
  _stream.avail_in = static_cast<unsigned int>(length);

//初始化输出大小。
  _stream.avail_out = static_cast<unsigned int>(length);
  _stream.next_out = reinterpret_cast<Bytef*>(&(*output)[output_header_len]);

  bool compressed = false;
  st = deflate(&_stream, Z_FINISH);
  if (st == Z_STREAM_END) {
    compressed = true;
    output->resize(output->size() - _stream.avail_out);
  }
//我们真正关心的唯一返回值是z_stream_end。
//Z_OK表示输出空间不足。这意味着压缩
//大于解压缩后的大小。在这种情况下，压缩失败。

  deflateEnd(&_stream);
  return compressed;
#endif
  return false;
}

//compress_format_version==1--解压缩后的大小不包括在
//块标题
//compress_format_version==2——解压后的大小包含在块中
//变量32格式的标题
//@param compression\u用于预设压缩库的dict data
//字典。
inline char* Zlib_Uncompress(const char* input_data, size_t input_length,
                             int* decompress_size,
                             uint32_t compress_format_version,
                             const Slice& compression_dict = Slice(),
                             int windowBits = -14) {
#ifdef ZLIB
  uint32_t output_len = 0;
  if (compress_format_version == 2) {
    if (!compression::GetDecompressedSizeInfo(&input_data, &input_length,
                                              &output_len)) {
      return nullptr;
    }
  } else {
//假设解压缩后的数据大小为压缩后大小的5倍，但为整数
//到页面大小
    size_t proposed_output_len = ((input_length * 5) & (~(4096 - 1))) + 4096;
    output_len = static_cast<uint32_t>(
        std::min(proposed_output_len,
                 static_cast<size_t>(std::numeric_limits<uint32_t>::max())));
  }

  z_stream _stream;
  memset(&_stream, 0, sizeof(z_stream));

//对于原始充气，窗口位应为-8..-15。
//如果windowbits大于零，它将使用zlib
//头或gzip头。加上32可以自动检测。
  int st = inflateInit2(&_stream,
      windowBits > 0 ? windowBits + 32 : windowBits);
  if (st != Z_OK) {
    return nullptr;
  }

  if (compression_dict.size()) {
//初始化压缩库的字典
    st = inflateSetDictionary(
        &_stream, reinterpret_cast<const Bytef*>(compression_dict.data()),
        static_cast<unsigned int>(compression_dict.size()));
    if (st != Z_OK) {
      return nullptr;
    }
  }

  _stream.next_in = (Bytef *)input_data;
  _stream.avail_in = static_cast<unsigned int>(input_length);

  char* output = new char[output_len];

  _stream.next_out = (Bytef *)output;
  _stream.avail_out = static_cast<unsigned int>(output_len);

  bool done = false;
  while (!done) {
    st = inflate(&_stream, Z_SYNC_FLUSH);
    switch (st) {
      case Z_STREAM_END:
        done = true;
        break;
      case Z_OK: {
//没有输出空间。将输出空间增加20%。
//如果
//压缩格式版本==2
        assert(compress_format_version != 2);
        size_t old_sz = output_len;
        uint32_t output_len_delta = output_len/5;
        output_len += output_len_delta < 10 ? 10 : output_len_delta;
        char* tmp = new char[output_len];
        memcpy(tmp, output, old_sz);
        delete[] output;
        output = tmp;

//设置更多输出。
        _stream.next_out = (Bytef *)(output + old_sz);
        _stream.avail_out = static_cast<unsigned int>(output_len - old_sz);
        break;
      }
      case Z_BUF_ERROR:
      default:
        delete[] output;
        inflateEnd(&_stream);
        return nullptr;
    }
  }

//如果我们对解压缩后的块大小进行编码，就不应该有剩余的字节了
  assert(compress_format_version != 2 || _stream.avail_out == 0);
  *decompress_size = static_cast<int>(output_len - _stream.avail_out);
  inflateEnd(&_stream);
  return output;
#endif

  return nullptr;
}

//compress_format_version==1--解压缩后的大小不包括在
//块标题
//compress_format_version==2——解压后的大小包含在块中
//变量32格式的标题
inline bool BZip2_Compress(const CompressionOptions& opts,
                           uint32_t compress_format_version,
                           const char* input, size_t length,
                           ::std::string* output) {
#ifdef BZIP2
  if (length > std::numeric_limits<uint32_t>::max()) {
//压缩量不能超过4GB
    return false;
  }
  size_t output_header_len = 0;
  if (compress_format_version == 2) {
    output_header_len = compression::PutDecompressedSizeInfo(
        output, static_cast<uint32_t>(length));
  }
//将输出调整为普通数据长度。
//如果压缩实际扩展了数据，这可能不够大。
  output->resize(output_header_len + length);


  bz_stream _stream;
  memset(&_stream, 0, sizeof(bz_stream));

//块大小1为100K。
//0表示静默。
//30是默认的Workfactor
  int st = BZ2_bzCompressInit(&_stream, 1, 0, 30);
  if (st != BZ_OK) {
    return false;
  }

//压缩输入，并将压缩数据放入输出。
  _stream.next_in = (char *)input;
  _stream.avail_in = static_cast<unsigned int>(length);

//初始化输出大小。
  _stream.avail_out = static_cast<unsigned int>(length);
  _stream.next_out = reinterpret_cast<char*>(&(*output)[output_header_len]);

  bool compressed = false;
  st = BZ2_bzCompress(&_stream, BZ_FINISH);
  if (st == BZ_STREAM_END) {
    compressed = true;
    output->resize(output->size() - _stream.avail_out);
  }
//我们真正关心的唯一返回值是bz_stream_end。
//bz_finish_ok表示输出空间不足。这意味着压缩
//大于解压缩后的大小。在这种情况下，压缩失败。

  BZ2_bzCompressEnd(&_stream);
  return compressed;
#endif
  return false;
}

//compress_format_version==1--解压缩后的大小不包括在
//块标题
//compress_format_version==2——解压后的大小包含在块中
//变量32格式的标题
inline char* BZip2_Uncompress(const char* input_data, size_t input_length,
                              int* decompress_size,
                              uint32_t compress_format_version) {
#ifdef BZIP2
  uint32_t output_len = 0;
  if (compress_format_version == 2) {
    if (!compression::GetDecompressedSizeInfo(&input_data, &input_length,
                                              &output_len)) {
      return nullptr;
    }
  } else {
//假设解压缩后的数据大小为压缩后大小的5倍，但为整数
//到下一页大小
    size_t proposed_output_len = ((input_length * 5) & (~(4096 - 1))) + 4096;
    output_len = static_cast<uint32_t>(
        std::min(proposed_output_len,
                 static_cast<size_t>(std::numeric_limits<uint32_t>::max())));
  }

  bz_stream _stream;
  memset(&_stream, 0, sizeof(bz_stream));

  int st = BZ2_bzDecompressInit(&_stream, 0, 0);
  if (st != BZ_OK) {
    return nullptr;
  }

  _stream.next_in = (char *)input_data;
  _stream.avail_in = static_cast<unsigned int>(input_length);

  char* output = new char[output_len];

  _stream.next_out = (char *)output;
  _stream.avail_out = static_cast<unsigned int>(output_len);

  bool done = false;
  while (!done) {
    st = BZ2_bzDecompress(&_stream);
    switch (st) {
      case BZ_STREAM_END:
        done = true;
        break;
      case BZ_OK: {
//没有输出空间。将输出空间增加20%。
//如果
//压缩格式版本==2
        assert(compress_format_version != 2);
        uint32_t old_sz = output_len;
        output_len = output_len * 1.2;
        char* tmp = new char[output_len];
        memcpy(tmp, output, old_sz);
        delete[] output;
        output = tmp;

//设置更多输出。
        _stream.next_out = (char *)(output + old_sz);
        _stream.avail_out = static_cast<unsigned int>(output_len - old_sz);
        break;
      }
      default:
        delete[] output;
        BZ2_bzDecompressEnd(&_stream);
        return nullptr;
    }
  }

//如果我们对解压缩后的块大小进行编码，就不应该有剩余的字节了
  assert(compress_format_version != 2 || _stream.avail_out == 0);
  *decompress_size = static_cast<int>(output_len - _stream.avail_out);
  BZ2_bzDecompressEnd(&_stream);
  return output;
#endif
  return nullptr;
}

//compress_format_version==1--解压缩后的大小包含在
//块头使用memcpy，这使得数据库不可移植）
//compress_format_version==2——解压后的大小包含在块中
//变量32格式的标题
//@param compression\u用于预设压缩库的dict data
//字典。
inline bool LZ4_Compress(const CompressionOptions& opts,
                         uint32_t compress_format_version, const char* input,
                         size_t length, ::std::string* output,
                         const Slice compression_dict = Slice()) {
#ifdef LZ4
  if (length > std::numeric_limits<uint32_t>::max()) {
//压缩量不能超过4GB
    return false;
  }

  size_t output_header_len = 0;
  if (compress_format_version == 2) {
//新编码，使用varint32存储大小信息
    output_header_len = compression::PutDecompressedSizeInfo(
        output, static_cast<uint32_t>(length));
  } else {
//传统编码，这不是真正可移植的（取决于大/小
//迂回性）
    output_header_len = 8;
    output->resize(output_header_len);
    char* p = const_cast<char*>(output->c_str());
    memcpy(p, &length, sizeof(length));
  }
  int compress_bound = LZ4_compressBound(static_cast<int>(length));
  output->resize(static_cast<size_t>(output_header_len + compress_bound));

  int outlen;
#if LZ4_VERSION_NUMBER >= 10400  //R124+
  LZ4_stream_t* stream = LZ4_createStream();
  if (compression_dict.size()) {
    LZ4_loadDict(stream, compression_dict.data(),
                 static_cast<int>(compression_dict.size()));
  }
#if LZ4_VERSION_NUMBER >= 10700  //R129+
  outlen = LZ4_compress_fast_continue(
      stream, input, &(*output)[output_header_len], static_cast<int>(length),
      compress_bound, 1);
#else  //高达R128
  outlen = LZ4_compress_limitedOutput_continue(
      stream, input, &(*output)[output_header_len], static_cast<int>(length),
      compress_bound);
#endif
  LZ4_freeStream(stream);
#else   //高达R123
  outlen = LZ4_compress_limitedOutput(input, &(*output)[output_header_len],
                                      static_cast<int>(length), compress_bound);
#endif  //LZ4_版本号>=10400

  if (outlen == 0) {
    return false;
  }
  output->resize(static_cast<size_t>(output_header_len + outlen));
  return true;
#endif  //LZ4
  return false;
}

//compress_format_version==1--解压缩后的大小包含在
//块头使用memcpy，这使得数据库不可移植）
//compress_format_version==2——解压后的大小包含在块中
//变量32格式的标题
//@param compression\u用于预设压缩库的dict data
//字典。
inline char* LZ4_Uncompress(const char* input_data, size_t input_length,
                            int* decompress_size,
                            uint32_t compress_format_version,
                            const Slice& compression_dict = Slice()) {
#ifdef LZ4
  uint32_t output_len = 0;
  if (compress_format_version == 2) {
//新编码，使用varint32存储大小信息
    if (!compression::GetDecompressedSizeInfo(&input_data, &input_length,
                                              &output_len)) {
      return nullptr;
    }
  } else {
//传统编码，这不是真正可移植的（取决于大/小
//迂回性）
    if (input_length < 8) {
      return nullptr;
    }
    memcpy(&output_len, input_data, sizeof(output_len));
    input_length -= 8;
    input_data += 8;
  }

  char* output = new char[output_len];
#if LZ4_VERSION_NUMBER >= 10400  //R124+
  LZ4_streamDecode_t* stream = LZ4_createStreamDecode();
  if (compression_dict.size()) {
    LZ4_setStreamDecode(stream, compression_dict.data(),
                        static_cast<int>(compression_dict.size()));
  }
  *decompress_size = LZ4_decompress_safe_continue(
      stream, input_data, output, static_cast<int>(input_length),
      static_cast<int>(output_len));
  LZ4_freeStreamDecode(stream);
#else   //高达R123
  *decompress_size =
      LZ4_decompress_safe(input_data, output, static_cast<int>(input_length),
                          static_cast<int>(output_len));
#endif  //LZ4_版本号>=10400

  if (*decompress_size < 0) {
    delete[] output;
    return nullptr;
  }
  assert(*decompress_size == static_cast<int>(output_len));
  return output;
#endif  //LZ4
  return nullptr;
}

//compress_format_version==1--解压缩后的大小包含在
//块头使用memcpy，这使得数据库不可移植）
//compress_format_version==2——解压后的大小包含在块中
//变量32格式的标题
//@param compression\u用于预设压缩库的dict data
//字典。
inline bool LZ4HC_Compress(const CompressionOptions& opts,
                           uint32_t compress_format_version, const char* input,
                           size_t length, ::std::string* output,
                           const Slice& compression_dict = Slice()) {
#ifdef LZ4
  if (length > std::numeric_limits<uint32_t>::max()) {
//压缩量不能超过4GB
    return false;
  }

  size_t output_header_len = 0;
  if (compress_format_version == 2) {
//新编码，使用varint32存储大小信息
    output_header_len = compression::PutDecompressedSizeInfo(
        output, static_cast<uint32_t>(length));
  } else {
//传统编码，这不是真正可移植的（取决于大/小
//迂回性）
    output_header_len = 8;
    output->resize(output_header_len);
    char* p = const_cast<char*>(output->c_str());
    memcpy(p, &length, sizeof(length));
  }
  int compress_bound = LZ4_compressBound(static_cast<int>(length));
  output->resize(static_cast<size_t>(output_header_len + compress_bound));

  int outlen;
#if LZ4_VERSION_NUMBER >= 10400  //R124+
  LZ4_streamHC_t* stream = LZ4_createStreamHC();
  LZ4_resetStreamHC(stream, opts.level);
  const char* compression_dict_data =
      compression_dict.size() > 0 ? compression_dict.data() : nullptr;
  size_t compression_dict_size = compression_dict.size();
  LZ4_loadDictHC(stream, compression_dict_data,
                 static_cast<int>(compression_dict_size));

#if LZ4_VERSION_NUMBER >= 10700  //R129+
  outlen =
      LZ4_compress_HC_continue(stream, input, &(*output)[output_header_len],
                               static_cast<int>(length), compress_bound);
#else   //R124-R128
  outlen = LZ4_compressHC_limitedOutput_continue(
      stream, input, &(*output)[output_header_len], static_cast<int>(length),
      compress_bound);
#endif  //LZ4_版本号>=10700
  LZ4_freeStreamHC(stream);

#elif LZ4_VERSION_MAJOR  //R113-R123
  outlen = LZ4_compressHC2_limitedOutput(input, &(*output)[output_header_len],
                                         static_cast<int>(length),
                                         compress_bound, opts.level);
#else                    //高达R112
  outlen =
      LZ4_compressHC_limitedOutput(input, &(*output)[output_header_len],
                                   static_cast<int>(length), compress_bound);
#endif                   //LZ4_版本号>=10400

  if (outlen == 0) {
    return false;
  }
  output->resize(static_cast<size_t>(output_header_len + outlen));
  return true;
#endif  //LZ4
  return false;
}

inline bool XPRESS_Compress(const char* input, size_t length, std::string* output) {
#ifdef XPRESS
  return port::xpress::Compress(input, length, output);
#endif
  return false;
}

inline char* XPRESS_Uncompress(const char* input_data, size_t input_length,
  int* decompress_size) {
#ifdef XPRESS
  return port::xpress::Decompress(input_data, input_length, decompress_size);
#endif
  return nullptr;
}


//@param compression\u用于预设压缩库的dict data
//字典。
inline bool ZSTD_Compress(const CompressionOptions& opts, const char* input,
                          size_t length, ::std::string* output,
                          const Slice& compression_dict = Slice()) {
#ifdef ZSTD
  if (length > std::numeric_limits<uint32_t>::max()) {
//压缩量不能超过4GB
    return false;
  }

  size_t output_header_len = compression::PutDecompressedSizeInfo(
      output, static_cast<uint32_t>(length));

  size_t compressBound = ZSTD_compressBound(length);
  output->resize(static_cast<size_t>(output_header_len + compressBound));
  size_t outlen;
#if ZSTD_VERSION_NUMBER >= 500  //V0.5.0+
  ZSTD_CCtx* context = ZSTD_createCCtx();
  outlen = ZSTD_compress_usingDict(
      context, &(*output)[output_header_len], compressBound, input, length,
      compression_dict.data(), compression_dict.size(), opts.level);
  ZSTD_freeCCtx(context);
#else  //高达V0.4.x
  outlen = ZSTD_compress(&(*output)[output_header_len], compressBound, input,
                         length, opts.level);
#endif  //zsd_版本号>=500
  if (outlen == 0) {
    return false;
  }
  output->resize(output_header_len + outlen);
  return true;
#endif
  return false;
}

//@param compression\u用于预设压缩库的dict data
//字典。
inline char* ZSTD_Uncompress(const char* input_data, size_t input_length,
                             int* decompress_size,
                             const Slice& compression_dict = Slice()) {
#ifdef ZSTD
  uint32_t output_len = 0;
  if (!compression::GetDecompressedSizeInfo(&input_data, &input_length,
                                            &output_len)) {
    return nullptr;
  }

  char* output = new char[output_len];
  size_t actual_output_length;
#if ZSTD_VERSION_NUMBER >= 500  //V0.5.0+
  ZSTD_DCtx* context = ZSTD_createDCtx();
  actual_output_length = ZSTD_decompress_usingDict(
      context, output, output_len, input_data, input_length,
      compression_dict.data(), compression_dict.size());
  ZSTD_freeDCtx(context);
#else  //高达V0.4.x
  actual_output_length =
      ZSTD_decompress(output, output_len, input_data, input_length);
#endif  //zsd_版本号>=500
  assert(actual_output_length == output_len);
  *decompress_size = static_cast<int>(actual_output_length);
  return output;
#endif
  return nullptr;
}

}  //命名空间rocksdb
