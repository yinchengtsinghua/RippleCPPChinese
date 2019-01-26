
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

#include <memory>
#include <string>
#include <utility>
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "util/file_reader_writer.h"
#include "utilities/blob_db/blob_log_format.h"

namespace rocksdb {
namespace blob_db {

class BlobDumpTool {
 public:
  enum class DisplayType {
    kNone,
    kRaw,
    kHex,
    kDetail,
  };

  BlobDumpTool();

  Status Run(const std::string& filename, DisplayType key_type,
             DisplayType blob_type);

 private:
  std::unique_ptr<RandomAccessFileReader> reader_;
  std::unique_ptr<char> buffer_;
  size_t buffer_size_;

  Status Read(uint64_t offset, size_t size, Slice* result);
  Status DumpBlobLogHeader(uint64_t* offset);
  Status DumpBlobLogFooter(uint64_t file_size, uint64_t* footer_offset);
  Status DumpRecord(DisplayType show_key, DisplayType show_blob,
                    uint64_t* offset);
  void DumpSlice(const Slice s, DisplayType type);

  template <class T>
  std::string GetString(std::pair<T, T> p);
};

}  //命名空间blob_db
}  //命名空间rocksdb

#endif  //摇滚乐
