
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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "utilities/blob_db/blob_dump_tool.h"
#include <inttypes.h>
#include <stdio.h>
#include <iostream>
#include <memory>
#include <string>
#include "port/port.h"
#include "rocksdb/convenience.h"
#include "rocksdb/env.h"
#include "util/coding.h"
#include "util/string_util.h"

namespace rocksdb {
namespace blob_db {

BlobDumpTool::BlobDumpTool()
    : reader_(nullptr), buffer_(nullptr), buffer_size_(0) {}

Status BlobDumpTool::Run(const std::string& filename, DisplayType show_key,
                         DisplayType show_blob) {
  Status s;
  Env* env = Env::Default();
  s = env->FileExists(filename);
  if (!s.ok()) {
    return s;
  }
  uint64_t file_size = 0;
  s = env->GetFileSize(filename, &file_size);
  if (!s.ok()) {
    return s;
  }
  std::unique_ptr<RandomAccessFile> file;
  s = env->NewRandomAccessFile(filename, &file, EnvOptions());
  if (!s.ok()) {
    return s;
  }
  if (file_size == 0) {
    return Status::Corruption("File is empty.");
  }
  reader_.reset(new RandomAccessFileReader(std::move(file), filename));
  uint64_t offset = 0;
  uint64_t footer_offset = 0;
  s = DumpBlobLogHeader(&offset);
  if (!s.ok()) {
    return s;
  }
  s = DumpBlobLogFooter(file_size, &footer_offset);
  if (!s.ok()) {
    return s;
  }
  if (show_key != DisplayType::kNone) {
    while (offset < footer_offset) {
      s = DumpRecord(show_key, show_blob, &offset);
      if (!s.ok()) {
        return s;
      }
    }
  }
  return s;
}

Status BlobDumpTool::Read(uint64_t offset, size_t size, Slice* result) {
  if (buffer_size_ < size) {
    if (buffer_size_ == 0) {
      buffer_size_ = 4096;
    }
    while (buffer_size_ < size) {
      buffer_size_ *= 2;
    }
    buffer_.reset(new char[buffer_size_]);
  }
  Status s = reader_->Read(offset, size, result, buffer_.get());
  if (!s.ok()) {
    return s;
  }
  if (result->size() != size) {
    return Status::Corruption("Reach the end of the file unexpectedly.");
  }
  return s;
}

Status BlobDumpTool::DumpBlobLogHeader(uint64_t* offset) {
  Slice slice;
  Status s = Read(0, BlobLogHeader::kSize, &slice);
  if (!s.ok()) {
    return s;
  }
  BlobLogHeader header;
  s = header.DecodeFrom(slice);
  if (!s.ok()) {
    return s;
  }
  fprintf(stdout, "Blob log header:\n");
  fprintf(stdout, "  Version          : %" PRIu32 "\n", header.version);
  fprintf(stdout, "  Column Family ID : %" PRIu32 "\n",
          header.column_family_id);
  std::string compression_str;
  if (!GetStringFromCompressionType(&compression_str, header.compression)
           .ok()) {
    compression_str = "Unrecongnized compression type (" +
                      ToString((int)header.compression) + ")";
  }
  fprintf(stdout, "  Compression      : %s\n", compression_str.c_str());
  fprintf(stdout, "  Expiration range : %s\n",
          GetString(header.expiration_range).c_str());
  *offset = BlobLogHeader::kSize;
  return s;
}

Status BlobDumpTool::DumpBlobLogFooter(uint64_t file_size,
                                       uint64_t* footer_offset) {
  auto no_footer = [&]() {
    *footer_offset = file_size;
    fprintf(stdout, "No blob log footer.\n");
    return Status::OK();
  };
  if (file_size < BlobLogHeader::kSize + BlobLogFooter::kSize) {
    return no_footer();
  }
  Slice slice;
  *footer_offset = file_size - BlobLogFooter::kSize;
  Status s = Read(*footer_offset, BlobLogFooter::kSize, &slice);
  if (!s.ok()) {
    return s;
  }
  BlobLogFooter footer;
  s = footer.DecodeFrom(slice);
  if (!s.ok()) {
    return s;
  }
  fprintf(stdout, "Blob log footer:\n");
  fprintf(stdout, "  Blob count       : %" PRIu64 "\n", footer.blob_count);
  fprintf(stdout, "  Expiration Range : %s\n",
          GetString(footer.expiration_range).c_str());
  fprintf(stdout, "  Sequence Range   : %s\n",
          GetString(footer.sequence_range).c_str());
  return s;
}

Status BlobDumpTool::DumpRecord(DisplayType show_key, DisplayType show_blob,
                                uint64_t* offset) {
  fprintf(stdout, "Read record with offset 0x%" PRIx64 " (%" PRIu64 "):\n",
          *offset, *offset);
  Slice slice;
  Status s = Read(*offset, BlobLogRecord::kHeaderSize, &slice);
  if (!s.ok()) {
    return s;
  }
  BlobLogRecord record;
  s = record.DecodeHeaderFrom(slice);
  if (!s.ok()) {
    return s;
  }
  uint64_t key_size = record.key_size;
  uint64_t value_size = record.value_size;
  fprintf(stdout, "  key size   : %" PRIu64 "\n", key_size);
  fprintf(stdout, "  value size : %" PRIu64 "\n", value_size);
  fprintf(stdout, "  expiration : %" PRIu64 "\n", record.expiration);
  *offset += BlobLogRecord::kHeaderSize;
  s = Read(*offset, key_size + value_size, &slice);
  if (!s.ok()) {
    return s;
  }
  if (show_key != DisplayType::kNone) {
    fprintf(stdout, "  key        : ");
    DumpSlice(Slice(slice.data(), key_size), show_key);
    if (show_blob != DisplayType::kNone) {
      fprintf(stdout, "  blob       : ");
      DumpSlice(Slice(slice.data() + key_size, value_size), show_blob);
    }
  }
  *offset += key_size + value_size;
  return s;
}

void BlobDumpTool::DumpSlice(const Slice s, DisplayType type) {
  if (type == DisplayType::kRaw) {
    fprintf(stdout, "%s\n", s.ToString().c_str());
  } else if (type == DisplayType::kHex) {
    /*intf（stdout，“%s\n”，s.toString（true/*hex*/）.c_str（））；
  else if（type==displayType:：kdetail）
    CHARBUF〔100〕；
    对于（尺寸_t i=0；i<s.size（）；i+=16）
      memset（buf，0，sizeof（buf））；
      对于（尺寸_t j=0；j<16&&i+j<s.size（）；j++）
        无符号字符c=s[i+j]；
        snprintf（buf+j*3+15，2，“%x”，c>>4）；
        snprintf（buf+j*3+16，2，“%x”，c&0xf）；
        snprintf（buf+j+65，2，“%c”，（0x20<=c&&c<=0x7e）？C：“。”
      }
      对于（尺寸_t p=0；p<sizeof（buf）-1；p++）
        如果（buf[p]==0）
          BUF[P]＝''；
        }
      }
      fprintf（stdout，“%s\n”，i==0？buf+15：buf）；
    }
  }
}

模板<class t>
std:：string blobdumptool:：getstring（std:：pair<t，t>p）
  如果（p.first==0&&p.second==0）
    返回“零”；
  }
  return“（+toString（p.first）+”，+toString（p.second）+”）；
}

//命名空间blob_db
//命名空间rocksdb

endif//rocksdb_lite
