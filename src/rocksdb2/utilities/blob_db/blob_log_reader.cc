
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

#include "utilities/blob_db/blob_log_reader.h"

#include <algorithm>

#include "util/file_reader_writer.h"

namespace rocksdb {
namespace blob_db {

Reader::Reader(std::shared_ptr<Logger> info_log,
               unique_ptr<SequentialFileReader>&& _file)
    : info_log_(info_log), file_(std::move(_file)), buffer_(), next_byte_(0) {}

Status Reader::ReadSlice(uint64_t size, Slice* slice, std::string* buf) {
  buf->reserve(size);
  Status s = file_->Read(size, slice, &(*buf)[0]);
  next_byte_ += size;
  if (!s.ok()) {
    return s;
  }
  if (slice->size() != size) {
    return Status::Corruption("EOF reached while reading record");
  }
  return s;
}

Status Reader::ReadHeader(BlobLogHeader* header) {
  assert(file_.get() != nullptr);
  assert(next_byte_ == 0);
  Status s = ReadSlice(BlobLogHeader::kSize, &buffer_, &backing_store_);
  if (!s.ok()) {
    return s;
  }

  if (buffer_.size() != BlobLogHeader::kSize) {
    return Status::Corruption("EOF reached before file header");
  }

  return header->DecodeFrom(buffer_);
}

Status Reader::ReadRecord(BlobLogRecord* record, ReadLevel level,
                          uint64_t* blob_offset) {
  Status s = ReadSlice(BlobLogRecord::kHeaderSize, &buffer_, &backing_store_);
  if (!s.ok()) {
    return s;
  }
  if (buffer_.size() != BlobLogRecord::kHeaderSize) {
    return Status::Corruption("EOF reached before record header");
  }

  s = record->DecodeHeaderFrom(buffer_);
  if (!s.ok()) {
    return s;
  }

  uint64_t kb_size = record->key_size + record->value_size;
  if (blob_offset != nullptr) {
    *blob_offset = next_byte_ + record->key_size;
  }

  switch (level) {
    case kReadHeader:
      file_->Skip(record->key_size + record->value_size);
      next_byte_ += kb_size;
      break;

    case kReadHeaderKey:
      s = ReadSlice(record->key_size, &record->key, &record->key_buf);
      file_->Skip(record->value_size);
      next_byte_ += record->value_size;
      break;

    case kReadHeaderKeyBlob:
      s = ReadSlice(record->key_size, &record->key, &record->key_buf);
      if (s.ok()) {
        s = ReadSlice(record->value_size, &record->value, &record->value_buf);
      }
      if (s.ok()) {
        s = record->CheckBlobCRC();
      }
      break;
  }
  return s;
}

}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //摇滚乐
