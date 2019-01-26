
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

#include <cstdint>
#include <memory>
#include <string>

#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/types.h"
#include "utilities/blob_db/blob_log_format.h"

namespace rocksdb {

class WritableFileWriter;

namespace blob_db {

/*
 *writer是blob日志流编写器。它只提供一个附加
 *用于写入BLOB数据的抽象。
 *
 *
 *查看blob_db_format.h查看记录格式的详细信息。
 **/


class Writer {
 public:
//创建一个将数据附加到“*dest”的写入程序。
//“*dest”最初必须为空。
//“*dest”在使用此编写器时必须保持活动状态。
  explicit Writer(std::unique_ptr<WritableFileWriter>&& dest,
                  uint64_t log_number, uint64_t bpsync, bool use_fsync,
                  uint64_t boffset = 0);

  ~Writer() = default;

//不允许复制
  Writer(const Writer&) = delete;
  Writer& operator=(const Writer&) = delete;

  static void ConstructBlobHeader(std::string* buf, const Slice& key,
                                  const Slice& val, uint64_t expiration);

  Status AddRecord(const Slice& key, const Slice& val, uint64_t* key_offset,
                   uint64_t* blob_offset);

  Status AddRecord(const Slice& key, const Slice& val, uint64_t expiration,
                   uint64_t* key_offset, uint64_t* blob_offset);

  Status EmitPhysicalRecord(const std::string& headerbuf, const Slice& key,
                            const Slice& val, uint64_t* key_offset,
                            uint64_t* blob_offset);

  Status AppendFooter(BlobLogFooter& footer);

  Status WriteHeader(BlobLogHeader& header);

  WritableFileWriter* file() { return dest_.get(); }

  const WritableFileWriter* file() const { return dest_.get(); }

  uint64_t get_log_number() const { return log_number_; }

  bool ShouldSync() const { return block_offset_ > next_sync_offset_; }

  void Sync();

  void ResetSyncPointer() { next_sync_offset_ += bytes_per_sync_; }

 private:
  std::unique_ptr<WritableFileWriter> dest_;
  uint64_t log_number_;
uint64_t block_offset_;  //块中的当前偏移量
  uint64_t bytes_per_sync_;
  uint64_t next_sync_offset_;
  bool use_fsync_;

 public:
  enum ElemType { kEtNone, kEtFileHdr, kEtRecord, kEtFileFooter };
  ElemType last_elem_type_;
};

}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //摇滚乐
