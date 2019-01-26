
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
#pragma once

#ifndef ROCKSDB_LITE

#include <memory>
#include <string>

#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "utilities/blob_db/blob_log_format.h"

namespace rocksdb {

class SequentialFileReader;
class Logger;

namespace blob_db {

/*
 *读卡器是一种通用的日志流读卡器实现。实际工作
 *从设备读取的数据由SequentialFile接口实现。
 *
 *有关文件和记录布局的详细信息，请参阅编写器。
 **/

class Reader {
 public:
  enum ReadLevel {
    kReadHeader,
    kReadHeaderKey,
    kReadHeaderKeyBlob,
  };

//创建将从“*文件”返回日志记录的读卡器。
//在使用此读卡器时，“*文件”必须保持活动状态。
//
//如果“reporter”为非nullptr，则每当有数据
//由于检测到损坏而丢弃。“*记者“必须留下
//在使用此阅读器时进行直播。
//
//如果“校验和”为真，请验证校验和（如果可用）。
//
//读卡器将从物理位置的第一个记录开始读取
//位置>=文件中的初始偏移量。
  Reader(std::shared_ptr<Logger> info_log,
         std::unique_ptr<SequentialFileReader>&& file);

  ~Reader() = default;

//不允许复制
  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;

  Status ReadHeader(BlobLogHeader* header);

//将下一个记录读取到*记录中。如果读取，则返回true
//成功，如果我们到达输入的末尾，则返回false。可以使用
//“刮擦”作为临时存储。*记录中填写的内容
//只有在对其执行下一个变异操作之前有效
//读写器或下一个变异到*刮痕。
//如果blob的offset为非空，则通过它返回blob的offset。
  Status ReadRecord(BlobLogRecord* record, ReadLevel level = kReadHeader,
                    uint64_t* blob_offset = nullptr);

  Status ReadSlice(uint64_t size, Slice* slice, std::string* buf);

  SequentialFileReader* file() { return file_.get(); }

  void ResetNextByte() { next_byte_ = 0; }

  uint64_t GetNextByte() const { return next_byte_; }

  const SequentialFileReader* file_reader() const { return file_.get(); }

 private:
  std::shared_ptr<Logger> info_log_;
  const std::unique_ptr<SequentialFileReader> file_;

  std::string backing_store_;
  Slice buffer_;

//下一个要读取的字节。用于断言正确使用
  uint64_t next_byte_;
};

}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //摇滚乐
