
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

#include "utilities/blob_db/blob_log_writer.h"

#include <cstdint>
#include <string>
#include "rocksdb/env.h"
#include "util/coding.h"
#include "util/file_reader_writer.h"
#include "utilities/blob_db/blob_log_format.h"

namespace rocksdb {
namespace blob_db {

Writer::Writer(unique_ptr<WritableFileWriter>&& dest, uint64_t log_number,
               uint64_t bpsync, bool use_fs, uint64_t boffset)
    : dest_(std::move(dest)),
      log_number_(log_number),
      block_offset_(boffset),
      bytes_per_sync_(bpsync),
      next_sync_offset_(0),
      use_fsync_(use_fs),
      last_elem_type_(kEtNone) {}

void Writer::Sync() { dest_->Sync(use_fsync_); }

Status Writer::WriteHeader(BlobLogHeader& header) {
  assert(block_offset_ == 0);
  assert(last_elem_type_ == kEtNone);
  std::string str;
  header.EncodeTo(&str);

  Status s = dest_->Append(Slice(str));
  if (s.ok()) {
    block_offset_ += str.size();
    s = dest_->Flush();
  }
  last_elem_type_ = kEtFileHdr;
  return s;
}

Status Writer::AppendFooter(BlobLogFooter& footer) {
  assert(block_offset_ != 0);
  assert(last_elem_type_ == kEtFileHdr || last_elem_type_ == kEtRecord);

  std::string str;
  footer.EncodeTo(&str);

  Status s = dest_->Append(Slice(str));
  if (s.ok()) {
    block_offset_ += str.size();
    s = dest_->Close();
    dest_.reset();
  }

  last_elem_type_ = kEtFileFooter;
  return s;
}

Status Writer::AddRecord(const Slice& key, const Slice& val,
                         uint64_t expiration, uint64_t* key_offset,
                         uint64_t* blob_offset) {
  assert(block_offset_ != 0);
  assert(last_elem_type_ == kEtFileHdr || last_elem_type_ == kEtRecord);

  std::string buf;
  ConstructBlobHeader(&buf, key, val, expiration);

  Status s = EmitPhysicalRecord(buf, key, val, key_offset, blob_offset);
  return s;
}

Status Writer::AddRecord(const Slice& key, const Slice& val,
                         uint64_t* key_offset, uint64_t* blob_offset) {
  assert(block_offset_ != 0);
  assert(last_elem_type_ == kEtFileHdr || last_elem_type_ == kEtRecord);

  std::string buf;
  ConstructBlobHeader(&buf, key, val, 0);

  Status s = EmitPhysicalRecord(buf, key, val, key_offset, blob_offset);
  return s;
}

void Writer::ConstructBlobHeader(std::string* buf, const Slice& key,
                                 const Slice& val, uint64_t expiration) {
  BlobLogRecord record;
  record.key = key;
  record.value = val;
  record.expiration = expiration;
  record.EncodeHeaderTo(buf);
}

Status Writer::EmitPhysicalRecord(const std::string& headerbuf,
                                  const Slice& key, const Slice& val,
                                  uint64_t* key_offset, uint64_t* blob_offset) {
  Status s = dest_->Append(Slice(headerbuf));
  if (s.ok()) {
    s = dest_->Append(key);
  }
  if (s.ok()) {
    s = dest_->Append(val);
  }
  if (s.ok()) {
    s = dest_->Flush();
  }

  *key_offset = block_offset_ + BlobLogRecord::kHeaderSize;
  *blob_offset = *key_offset + key.size();
  block_offset_ = *blob_offset + val.size();
  last_elem_type_ = kEtRecord;
  return s;
}

}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //摇滚乐
