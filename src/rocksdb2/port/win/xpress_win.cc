
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

#include "port/win/xpress_win.h"
#include <windows.h>

#include <cassert>
#include <memory>
#include <limits>
#include <iostream>

#ifdef XPRESS

//把这个放在ifdef下，这样Windows系统就不需要这个了
//仍然可以建造
#include <compressapi.h>

namespace rocksdb {
namespace port {
namespace xpress {

//帮手
namespace {

auto CloseCompressorFun = [](void* h) {
  if (NULL != h) {
    ::CloseCompressor(reinterpret_cast<COMPRESSOR_HANDLE>(h));
  }
};

auto CloseDecompressorFun = [](void* h) {
  if (NULL != h) {
    ::CloseDecompressor(reinterpret_cast<DECOMPRESSOR_HANDLE>(h));
  }
};
}

bool Compress(const char* input, size_t length, std::string* output) {

  assert(input != nullptr);
  assert(output != nullptr);

  if (length == 0) {
    output->clear();
    return true;
  }

  COMPRESS_ALLOCATION_ROUTINES* allocRoutinesPtr = nullptr;

  COMPRESSOR_HANDLE compressor = NULL;

  BOOL success = CreateCompressor(
COMPRESS_ALGORITHM_XPRESS, //压缩算法
allocRoutinesPtr,       //可选分配程序
&compressor);              //把手

  if (!success) {
#ifdef _DEBUG
    std::cerr << "XPRESS: Failed to create Compressor LastError: " <<
      GetLastError() << std::endl;
#endif
    return false;
  }

  std::unique_ptr<void, decltype(CloseCompressorFun)>
    compressorGuard(compressor, CloseCompressorFun);

  SIZE_T compressedBufferSize = 0;

//查询压缩缓冲区大小。
  success = ::Compress(
compressor,                 //压缩机手柄
const_cast<char*>(input),   //输入缓冲器
length,                     //未压缩数据大小
NULL,                       //压缩缓冲器
0,                          //压缩缓冲区大小
&compressedBufferSize);     //压缩数据大小

  if (!success) {

    auto lastError = GetLastError();

    if (lastError != ERROR_INSUFFICIENT_BUFFER) {
#ifdef _DEBUG
      std::cerr <<
        "XPRESS: Failed to estimate compressed buffer size LastError " <<
        lastError << std::endl;
#endif
      return false;
    }
  }

  assert(compressedBufferSize > 0);

  std::string result;
  result.resize(compressedBufferSize);

  SIZE_T compressedDataSize = 0;

//压缩
  success = ::Compress(
compressor,                  //压缩机手柄
const_cast<char*>(input),    //输入缓冲器
length,                      //未压缩数据大小
&result[0],                  //压缩缓冲器
compressedBufferSize,        //压缩缓冲区大小
&compressedDataSize);        //压缩数据大小

  if (!success) {
#ifdef _DEBUG
    std::cerr << "XPRESS: Failed to compress LastError " <<
      GetLastError() << std::endl;
#endif
    return false;
  }

  result.resize(compressedDataSize);
  output->swap(result);

  return true;
}

char* Decompress(const char* input_data, size_t input_length,
  int* decompress_size) {

  assert(input_data != nullptr);
  assert(decompress_size != nullptr);

  if (input_length == 0) {
    return nullptr;
  }

  COMPRESS_ALLOCATION_ROUTINES* allocRoutinesPtr = nullptr;

  DECOMPRESSOR_HANDLE decompressor = NULL;

  BOOL success = CreateDecompressor(
COMPRESS_ALGORITHM_XPRESS, //压缩算法
allocRoutinesPtr,          //可选分配程序
&decompressor);            //把手


  if (!success) {
#ifdef _DEBUG
    std::cerr << "XPRESS: Failed to create Decompressor LastError "
      << GetLastError() << std::endl;
#endif
    return nullptr;
  }

  std::unique_ptr<void, decltype(CloseDecompressorFun)>
    compressorGuard(decompressor, CloseDecompressorFun);

  SIZE_T decompressedBufferSize = 0;

  success = ::Decompress(
decompressor,          //压缩机手柄
const_cast<char*>(input_data),  //压缩数据
input_length,               //压缩数据大小
NULL,                        //缓冲区设置为空
0,                           //缓冲区大小设置为0
&decompressedBufferSize);    //解压缩后的数据大小

  if (!success) {

    auto lastError = GetLastError();

    if (lastError != ERROR_INSUFFICIENT_BUFFER) {
#ifdef _DEBUG
      std::cerr
        << "XPRESS: Failed to estimate decompressed buffer size LastError "
        << lastError << std::endl;
#endif
      return nullptr;
    }
  }

  assert(decompressedBufferSize > 0);

//在Windows上，对于
//输出数据大小参数
//所以我们希望永远不会到这里
  if (decompressedBufferSize > std::numeric_limits<int>::max()) {
    assert(false);
    return nullptr;
  }

//调用方正在使用delete[]解除分配
//因此，我们必须分配新的[]
  std::unique_ptr<char[]> outputBuffer(new char[decompressedBufferSize]);

  SIZE_T decompressedDataSize = 0;

  success = ::Decompress(
    decompressor,
    const_cast<char*>(input_data),
    input_length,
    outputBuffer.get(),
    decompressedBufferSize,
    &decompressedDataSize);

  if (!success) {
#ifdef _DEBUG
    std::cerr <<
      "XPRESS: Failed to decompress LastError " <<
      GetLastError() << std::endl;
#endif
    return nullptr;
  }

  *decompress_size = static_cast<int>(decompressedDataSize);

//将原始缓冲区返回给支持传统的调用程序
  return outputBuffer.release();
}
}
}
}

#endif
