
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

#ifndef ROCKSDB_LITE

#pragma once

#include <memory>
#include <string>

#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "rocksdb/table_properties.h"
#include "rocksdb/types.h"

#if defined(__GNUC__) || defined(__clang__)
#define ROCKSDB_DEPRECATED_FUNC __attribute__((__deprecated__))
#elif _WIN32
#define ROCKSDB_DEPRECATED_FUNC __declspec(deprecated)
#endif

namespace rocksdb {

class Comparator;

//外部sstfileinfo包括有关创建的sst文件的信息
//使用sstfilewriter。
struct ExternalSstFileInfo {
  ExternalSstFileInfo() {}
  ExternalSstFileInfo(const std::string& _file_path,
                      const std::string& _smallest_key,
                      const std::string& _largest_key,
                      SequenceNumber _sequence_number, uint64_t _file_size,
                      int32_t _num_entries, int32_t _version)
      : file_path(_file_path),
        smallest_key(_smallest_key),
        largest_key(_largest_key),
        sequence_number(_sequence_number),
        file_size(_file_size),
        num_entries(_num_entries),
        version(_version) {}

std::string file_path;           //外部SST文件路径
std::string smallest_key;        //文件中最小的用户密钥
std::string largest_key;         //文件中最大的用户密钥
SequenceNumber sequence_number;  //文件中所有键的序列号
uint64_t file_size;              //文件大小（字节）
uint64_t num_entries;            //文件中的条目数
int32_t version;                 //文件版本
};

//sstfilewriter用于创建可在以后添加到数据库中的sst文件。
//sstfilewriter生成的文件中的所有键的序列号都将为0。
class SstFileWriter {
 public:
//用户可以通过'column_family'指定生成的文件将
//请注意，传递nullptr意味着
//列_族未知。
//如果invalidate_page_cache设置为true，sstfilewriter将为操作系统提供
//提示：每次向文件写入1MB时，不需要此文件页。
//要使用速率限制器，可以传递比IO总优先级小的IO优先级。
  SstFileWriter(const EnvOptions& env_options, const Options& options,
                ColumnFamilyHandle* column_family = nullptr,
                bool invalidate_page_cache = true,
                Env::IOPriority io_priority = Env::IOPriority::IO_TOTAL)
      : SstFileWriter(env_options, options, options.comparator, column_family,
                      invalidate_page_cache, io_priority) {}

//弃用API
  SstFileWriter(const EnvOptions& env_options, const Options& options,
                const Comparator* user_comparator,
                ColumnFamilyHandle* column_family = nullptr,
                bool invalidate_page_cache = true,
                Env::IOPriority io_priority = Env::IOPriority::IO_TOTAL);

  ~SstFileWriter();

//准备sstfilewriter写入位于“file_path”的文件。
  Status Open(const std::string& file_path);

//向当前打开的文件添加带值的Put键（已弃用）
//要求：根据比较器，键在任何先前添加的键之后。
  ROCKSDB_DEPRECATED_FUNC Status Add(const Slice& user_key, const Slice& value);

//向当前打开的文件添加带值的Put键
//要求：根据比较器，键在任何先前添加的键之后。
  Status Put(const Slice& user_key, const Slice& value);

//将带有值的合并键添加到当前打开的文件中
//要求：根据比较器，键在任何先前添加的键之后。
  Status Merge(const Slice& user_key, const Slice& value);

//向当前打开的文件添加删除键
//要求：根据比较器，键在任何先前添加的键之后。
  Status Delete(const Slice& user_key);

//完成对sst文件的写入并关闭文件。
//
//可以将可选的externalstfileinfo指针传递给函数
//它将填充有关创建的sst文件的信息。
  Status Finish(ExternalSstFileInfo* file_info = nullptr);

//返回当前文件大小。
  uint64_t FileSize();

 private:
  void InvalidatePageCache(bool closing);
  struct Rep;
  std::unique_ptr<Rep> rep_;
};
}  //命名空间rocksdb

#endif  //！摇滚乐
