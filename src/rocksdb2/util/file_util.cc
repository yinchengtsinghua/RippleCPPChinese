
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
#include "util/file_util.h"

#include <string>
#include <algorithm>

#include "rocksdb/env.h"
#include "util/sst_file_manager_impl.h"
#include "util/file_reader_writer.h"

namespace rocksdb {

//用于将文件复制到指定长度的实用程序函数
Status CopyFile(Env* env, const std::string& source,
                const std::string& destination, uint64_t size, bool use_fsync) {
  const EnvOptions soptions;
  Status s;
  unique_ptr<SequentialFileReader> src_reader;
  unique_ptr<WritableFileWriter> dest_writer;

  {
    unique_ptr<SequentialFile> srcfile;
  s = env->NewSequentialFile(source, &srcfile, soptions);
  unique_ptr<WritableFile> destfile;
  if (s.ok()) {
    s = env->NewWritableFile(destination, &destfile, soptions);
  } else {
    return s;
  }

  if (size == 0) {
//默认参数表示复制所有内容
    if (s.ok()) {
      s = env->GetFileSize(source, &size);
    } else {
      return s;
    }
  }
  src_reader.reset(new SequentialFileReader(std::move(srcfile)));
  dest_writer.reset(new WritableFileWriter(std::move(destfile), soptions));
  }

  char buffer[4096];
  Slice slice;
  while (size > 0) {
    size_t bytes_to_read = std::min(sizeof(buffer), static_cast<size_t>(size));
    if (s.ok()) {
      s = src_reader->Read(bytes_to_read, &slice, buffer);
    }
    if (s.ok()) {
      if (slice.size() == 0) {
        return Status::Corruption("file too small");
      }
      s = dest_writer->Append(slice);
    }
    if (!s.ok()) {
      return s;
    }
    size -= slice.size();
  }
  dest_writer->Sync(use_fsync);
  return Status::OK();
}

//用于创建包含所提供内容的文件的实用程序函数
Status CreateFile(Env* env, const std::string& destination,
                  const std::string& contents) {
  const EnvOptions soptions;
  Status s;
  unique_ptr<WritableFileWriter> dest_writer;

  unique_ptr<WritableFile> destfile;
  s = env->NewWritableFile(destination, &destfile, soptions);
  if (!s.ok()) {
    return s;
  }
  dest_writer.reset(new WritableFileWriter(std::move(destfile), soptions));
  return dest_writer->Append(Slice(contents));
}

Status DeleteSSTFile(const ImmutableDBOptions* db_options,
                     const std::string& fname, uint32_t path_id) {
//TODO（tec）：支持多路径ID的sst文件管理器
#ifndef ROCKSDB_LITE
  auto sfm =
      static_cast<SstFileManagerImpl*>(db_options->sst_file_manager.get());
  if (sfm && path_id == 0) {
    return sfm->ScheduleFileDeletion(fname);
  } else {
    return db_options->env->DeleteFile(fname);
  }
#else
//RocksDB-Lite中不支持sstfileManager
  return db_options->env->DeleteFile(fname);
#endif
}

}  //命名空间rocksdb
