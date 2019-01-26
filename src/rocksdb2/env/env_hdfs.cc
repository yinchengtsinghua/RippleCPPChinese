
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

#include "rocksdb/env.h"
#include "hdfs/env_hdfs.h"

#ifdef USE_HDFS
#ifndef ROCKSDB_HDFS_FILE_C
#define ROCKSDB_HDFS_FILE_C

#include <algorithm>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include "rocksdb/status.h"
#include "util/string_util.h"

#define HDFS_EXISTS 0
#define HDFS_DOESNT_EXIST -1
#define HDFS_SUCCESS 0

//
//此文件定义RockSDB的HDFS环境。它使用libhdfs
//访问HDF的API。由RockSDB的一个实例创建的所有HDFS文件
//将驻留在同一个HDFS集群上。
//

namespace rocksdb {

namespace {

//日志错误消息
static Status IOError(const std::string& context, int err_number) {
  return (err_number == ENOSPC) ?
      Status::NoSpace(context, strerror(err_number)) :
      Status::IOError(context, strerror(err_number));
}

//假设现在有一个全局记录器。它不是线程安全的，
//但不必，因为记录器是在db打开时初始化的。
static Logger* mylog = nullptr;

//用于从HDF读取文件。它实现两种顺序读取
//访问方法以及随机读取访问方法。
class HdfsReadableFile : virtual public SequentialFile,
                         virtual public RandomAccessFile {
 private:
  hdfsFS fileSys_;
  std::string filename_;
  hdfsFile hfile_;

 public:
  HdfsReadableFile(hdfsFS fileSys, const std::string& fname)
      : fileSys_(fileSys), filename_(fname), hfile_(nullptr) {
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsReadableFile opening file %s\n",
                    filename_.c_str());
    hfile_ = hdfsOpenFile(fileSys_, filename_.c_str(), O_RDONLY, 0, 0, 0);
    ROCKS_LOG_DEBUG(mylog,
                    "[hdfs] HdfsReadableFile opened file %s hfile_=0x%p\n",
                    filename_.c_str(), hfile_);
  }

  virtual ~HdfsReadableFile() {
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsReadableFile closing file %s\n",
                    filename_.c_str());
    hdfsCloseFile(fileSys_, hfile_);
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsReadableFile closed file %s\n",
                    filename_.c_str());
    hfile_ = nullptr;
  }

  bool isValid() {
    return hfile_ != nullptr;
  }

//顺序访问，以文件中的当前偏移量读取数据
  virtual Status Read(size_t n, Slice* result, char* scratch) {
    Status s;
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsReadableFile reading %s %ld\n",
                    filename_.c_str(), n);

    char* buffer = scratch;
    size_t total_bytes_read = 0;
    tSize bytes_read = 0;
    tSize remaining_bytes = (tSize)n;

//重复读取总共n个字节，直到遇到错误或EOF
    while (remaining_bytes > 0) {
      bytes_read = hdfsRead(fileSys_, hfile_, buffer, remaining_bytes);
      if (bytes_read <= 0) {
        break;
      }
      assert(bytes_read <= remaining_bytes);

      total_bytes_read += bytes_read;
      remaining_bytes -= bytes_read;
      buffer += bytes_read;
    }
    assert(total_bytes_read <= n);

    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsReadableFile read %s\n",
                    filename_.c_str());

    if (bytes_read < 0) {
      s = IOError(filename_, errno);
    } else {
      *result = Slice(scratch, total_bytes_read);
    }

    return s;
  }

//随机访问，从文件中指定的偏移量读取数据
  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const {
    Status s;
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsReadableFile preading %s\n",
                    filename_.c_str());
    ssize_t bytes_read = hdfsPread(fileSys_, hfile_, offset,
                                   (void*)scratch, (tSize)n);
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsReadableFile pread %s\n",
                    filename_.c_str());
    *result = Slice(scratch, (bytes_read < 0) ? 0 : bytes_read);
    if (bytes_read < 0) {
//错误：返回非OK状态
      s = IOError(filename_, errno);
    }
    return s;
  }

  virtual Status Skip(uint64_t n) {
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsReadableFile skip %s\n",
                    filename_.c_str());
//从文件获取当前偏移量
    tOffset current = hdfsTell(fileSys_, hfile_);
    if (current < 0) {
      return IOError(filename_, errno);
    }
//在文件中查找新偏移量
    tOffset newoffset = current + n;
    int val = hdfsSeek(fileSys_, hfile_, newoffset);
    if (val < 0) {
      return IOError(filename_, errno);
    }
    return Status::OK();
  }

 private:

//如果在文件结尾，则返回true，否则返回false
  bool feof() {
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsReadableFile feof %s\n",
                    filename_.c_str());
    if (hdfsTell(fileSys_, hfile_) == fileSize()) {
      return true;
    }
    return false;
  }

//文件的当前大小
  tOffset fileSize() {
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsReadableFile fileSize %s\n",
                    filename_.c_str());
    hdfsFileInfo* pFileInfo = hdfsGetPathInfo(fileSys_, filename_.c_str());
    tOffset size = 0L;
    if (pFileInfo != nullptr) {
      size = pFileInfo->mSize;
      hdfsFreeFileInfo(pFileInfo, 1);
    } else {
      throw HdfsFatalException("fileSize on unknown file " + filename_);
    }
    return size;
  }
};

//附加到HDFS中的现有文件。
class HdfsWritableFile: public WritableFile {
 private:
  hdfsFS fileSys_;
  std::string filename_;
  hdfsFile hfile_;

 public:
  HdfsWritableFile(hdfsFS fileSys, const std::string& fname)
      : fileSys_(fileSys), filename_(fname) , hfile_(nullptr) {
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsWritableFile opening %s\n",
                    filename_.c_str());
    hfile_ = hdfsOpenFile(fileSys_, filename_.c_str(), O_WRONLY, 0, 0, 0);
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsWritableFile opened %s\n",
                    filename_.c_str());
    assert(hfile_ != nullptr);
  }
  virtual ~HdfsWritableFile() {
    if (hfile_ != nullptr) {
      ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsWritableFile closing %s\n",
                      filename_.c_str());
      hdfsCloseFile(fileSys_, hfile_);
      ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsWritableFile closed %s\n",
                      filename_.c_str());
      hfile_ = nullptr;
    }
  }

//如果文件创建成功，则返回true。
//否则返回false。
  bool isValid() {
    return hfile_ != nullptr;
  }

//文件名，主要用于调试日志记录。
  const std::string& getName() {
    return filename_;
  }

  virtual Status Append(const Slice& data) {
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsWritableFile Append %s\n",
                    filename_.c_str());
    const char* src = data.data();
    size_t left = data.size();
    size_t ret = hdfsWrite(fileSys_, hfile_, src, left);
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsWritableFile Appended %s\n",
                    filename_.c_str());
    if (ret != left) {
      return IOError(filename_, errno);
    }
    return Status::OK();
  }

  virtual Status Flush() {
    return Status::OK();
  }

  virtual Status Sync() {
    Status s;
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsWritableFile Sync %s\n",
                    filename_.c_str());
    if (hdfsFlush(fileSys_, hfile_) == -1) {
      return IOError(filename_, errno);
    }
    if (hdfsHSync(fileSys_, hfile_) == -1) {
      return IOError(filename_, errno);
    }
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsWritableFile Synced %s\n",
                    filename_.c_str());
    return Status::OK();
  }

//hdfslogger使用它将数据写入调试日志文件
  virtual Status Append(const char* src, size_t size) {
    if (hdfsWrite(fileSys_, hfile_, src, size) != (tSize)size) {
      return IOError(filename_, errno);
    }
    return Status::OK();
  }

  virtual Status Close() {
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsWritableFile closing %s\n",
                    filename_.c_str());
    if (hdfsCloseFile(fileSys_, hfile_) != 0) {
      return IOError(filename_, errno);
    }
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsWritableFile closed %s\n",
                    filename_.c_str());
    hfile_ = nullptr;
    return Status::OK();
  }
};

//实现调试日志以驻留在HDFS中的对象。
class HdfsLogger : public Logger {
 private:
  HdfsWritableFile* file_;
uint64_t (*gettid_)();  //返回当前线程的线程ID

 public:
  HdfsLogger(HdfsWritableFile* f, uint64_t (*gettid)())
      : file_(f), gettid_(gettid) {
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsLogger opened %s\n",
                    file_->getName().c_str());
  }

  virtual ~HdfsLogger() {
    ROCKS_LOG_DEBUG(mylog, "[hdfs] HdfsLogger closed %s\n",
                    file_->getName().c_str());
    delete file_;
    if (mylog != nullptr && mylog == this) {
      mylog = nullptr;
    }
  }

  virtual void Logv(const char* format, va_list ap) {
    const uint64_t thread_id = (*gettid_)();

//我们尝试了两次：第一次使用固定大小的堆栈分配缓冲区，
//第二次使用了更大的动态分配缓冲区。
    char buffer[500];
    for (int iter = 0; iter < 2; iter++) {
      char* base;
      int bufsize;
      if (iter == 0) {
        bufsize = sizeof(buffer);
        base = buffer;
      } else {
        bufsize = 30000;
        base = new char[bufsize];
      }
      char* p = base;
      char* limit = base + bufsize;

      struct timeval now_tv;
      gettimeofday(&now_tv, nullptr);
      const time_t seconds = now_tv.tv_sec;
      struct tm t;
      localtime_r(&seconds, &t);
      p += snprintf(p, limit - p,
                    "%04d/%02d/%02d-%02d:%02d:%02d.%06d %llx ",
                    t.tm_year + 1900,
                    t.tm_mon + 1,
                    t.tm_mday,
                    t.tm_hour,
                    t.tm_min,
                    t.tm_sec,
                    static_cast<int>(now_tv.tv_usec),
                    static_cast<long long unsigned int>(thread_id));

//打印邮件
      if (p < limit) {
        va_list backup_ap;
        va_copy(backup_ap, ap);
        p += vsnprintf(p, limit - p, format, backup_ap);
        va_end(backup_ap);
      }

//必要时截断到可用空间
      if (p >= limit) {
        if (iter == 0) {
continue;       //使用较大的缓冲区重试
        } else {
          p = limit - 1;
        }
      }

//必要时添加换行符
      if (p == base || p[-1] != '\n') {
        *p++ = '\n';
      }

      assert(p <= limit);
      file_->Append(base, p-base);
      file_->Flush();
      if (base != buffer) {
        delete[] base;
      }
      break;
    }
  }
};

}  //命名空间

//最后，HDFS环境

const std::string HdfsEnv::kProto = "hdfs://“；
const std::string HdfsEnv::pathsep = "/";

//打开文件进行顺序读取
Status HdfsEnv::NewSequentialFile(const std::string& fname,
                                  unique_ptr<SequentialFile>* result,
                                  const EnvOptions& options) {
  result->reset();
  HdfsReadableFile* f = new HdfsReadableFile(fileSys_, fname);
  if (f == nullptr || !f->isValid()) {
    delete f;
    *result = nullptr;
    return IOError(fname, errno);
  }
  result->reset(dynamic_cast<SequentialFile*>(f));
  return Status::OK();
}

//打开文件进行随机读取
Status HdfsEnv::NewRandomAccessFile(const std::string& fname,
                                    unique_ptr<RandomAccessFile>* result,
                                    const EnvOptions& options) {
  result->reset();
  HdfsReadableFile* f = new HdfsReadableFile(fileSys_, fname);
  if (f == nullptr || !f->isValid()) {
    delete f;
    *result = nullptr;
    return IOError(fname, errno);
  }
  result->reset(dynamic_cast<RandomAccessFile*>(f));
  return Status::OK();
}

//创建新文件进行写入
Status HdfsEnv::NewWritableFile(const std::string& fname,
                                unique_ptr<WritableFile>* result,
                                const EnvOptions& options) {
  result->reset();
  Status s;
  HdfsWritableFile* f = new HdfsWritableFile(fileSys_, fname);
  if (f == nullptr || !f->isValid()) {
    delete f;
    *result = nullptr;
    return IOError(fname, errno);
  }
  result->reset(dynamic_cast<WritableFile*>(f));
  return Status::OK();
}

class HdfsDirectory : public Directory {
 public:
  explicit HdfsDirectory(int fd) : fd_(fd) {}
  ~HdfsDirectory() {}

  virtual Status Fsync() { return Status::OK(); }

 private:
  int fd_;
};

Status HdfsEnv::NewDirectory(const std::string& name,
                             unique_ptr<Directory>* result) {
  int value = hdfsExists(fileSys_, name.c_str());
  switch (value) {
    case HDFS_EXISTS:
      result->reset(new HdfsDirectory(0));
      return Status::OK();
default:  //如果目录不存在，则失败
      ROCKS_LOG_FATAL(mylog, "NewDirectory hdfsExists call failed");
      throw HdfsFatalException("hdfsExists call failed with error " +
                               ToString(value) + " on path " + name +
                               ".\n");
  }
}

Status HdfsEnv::FileExists(const std::string& fname) {
  int value = hdfsExists(fileSys_, fname.c_str());
  switch (value) {
    case HDFS_EXISTS:
      return Status::OK();
    case HDFS_DOESNT_EXIST:
      return Status::NotFound();
default:  //还有什么应该是个错误
      ROCKS_LOG_FATAL(mylog, "FileExists hdfsExists call failed");
      return Status::IOError("hdfsExists call failed with error " +
                             ToString(value) + " on path " + fname + ".\n");
  }
}

Status HdfsEnv::GetChildren(const std::string& path,
                            std::vector<std::string>* result) {
  int value = hdfsExists(fileSys_, path.c_str());
  switch (value) {
case HDFS_EXISTS: {  //目录存在
    int numEntries = 0;
    hdfsFileInfo* pHdfsFileInfo = 0;
    pHdfsFileInfo = hdfsListDirectory(fileSys_, path.c_str(), &numEntries);
    if (numEntries >= 0) {
      for(int i = 0; i < numEntries; i++) {
        char* pathname = pHdfsFileInfo[i].mName;
        char* filename = std::rindex(pathname, '/');
        if (filename != nullptr) {
          result->push_back(filename+1);
        }
      }
      if (pHdfsFileInfo != nullptr) {
        hdfsFreeFileInfo(pHdfsFileInfo, numEntries);
      }
    } else {
//numEntries<0表示错误
      ROCKS_LOG_FATAL(mylog, "hdfsListDirectory call failed with error ");
      throw HdfsFatalException(
          "hdfsListDirectory call failed negative error.\n");
    }
    break;
  }
case HDFS_DOESNT_EXIST:  //目录不存在，退出
    return Status::NotFound();
default:          //还有什么应该是个错误
    ROCKS_LOG_FATAL(mylog, "GetChildren hdfsExists call failed");
    throw HdfsFatalException("hdfsExists call failed with error " +
                             ToString(value) + ".\n");
  }
  return Status::OK();
}

Status HdfsEnv::DeleteFile(const std::string& fname) {
  if (hdfsDelete(fileSys_, fname.c_str(), 1) == 0) {
    return Status::OK();
  }
  return IOError(fname, errno);
};

Status HdfsEnv::CreateDir(const std::string& name) {
  if (hdfsCreateDirectory(fileSys_, name.c_str()) == 0) {
    return Status::OK();
  }
  return IOError(name, errno);
};

Status HdfsEnv::CreateDirIfMissing(const std::string& name) {
  const int value = hdfsExists(fileSys_, name.c_str());
//不是原子的。状态可能会更改b/w hdfsexists和createdir。
  switch (value) {
    case HDFS_EXISTS:
    return Status::OK();
    case HDFS_DOESNT_EXIST:
    return CreateDir(name);
default:  //还有什么应该是个错误
      ROCKS_LOG_FATAL(mylog, "CreateDirIfMissing hdfsExists call failed");
      throw HdfsFatalException("hdfsExists call failed with error " +
                               ToString(value) + ".\n");
  }
};

Status HdfsEnv::DeleteDir(const std::string& name) {
  return DeleteFile(name);
};

Status HdfsEnv::GetFileSize(const std::string& fname, uint64_t* size) {
  *size = 0L;
  hdfsFileInfo* pFileInfo = hdfsGetPathInfo(fileSys_, fname.c_str());
  if (pFileInfo != nullptr) {
    *size = pFileInfo->mSize;
    hdfsFreeFileInfo(pFileInfo, 1);
    return Status::OK();
  }
  return IOError(fname, errno);
}

Status HdfsEnv::GetFileModificationTime(const std::string& fname,
                                        uint64_t* time) {
  hdfsFileInfo* pFileInfo = hdfsGetPathInfo(fileSys_, fname.c_str());
  if (pFileInfo != nullptr) {
    *time = static_cast<uint64_t>(pFileInfo->mLastMod);
    hdfsFreeFileInfo(pFileInfo, 1);
    return Status::OK();
  }
  return IOError(fname, errno);

}

//重命名不是原子的。如果
//目标已存在。因此，我们在尝试
//重命名。
Status HdfsEnv::RenameFile(const std::string& src, const std::string& target) {
  hdfsDelete(fileSys_, target.c_str(), 1);
  if (hdfsRename(fileSys_, src.c_str(), target.c_str()) == 0) {
    return Status::OK();
  }
  return IOError(src, errno);
}

Status HdfsEnv::LockFile(const std::string& fname, FileLock** lock) {
//有一种非常好的方法可以自动检查和创建
//通过libhdfs的文件
  *lock = nullptr;
  return Status::OK();
}

Status HdfsEnv::UnlockFile(FileLock* lock) {
  return Status::OK();
}

Status HdfsEnv::NewLogger(const std::string& fname,
                          shared_ptr<Logger>* result) {
  HdfsWritableFile* f = new HdfsWritableFile(fileSys_, fname);
  if (f == nullptr || !f->isValid()) {
    delete f;
    *result = nullptr;
    return IOError(fname, errno);
  }
  HdfsLogger* h = new HdfsLogger(f, &HdfsEnv::gettid);
  result->reset(h);
  if (mylog == nullptr) {
//mylog=h；//取消对此的注释以获取详细的日志记录
  }
  return Status::OK();
}

//创建HDFS环境的工厂方法
Status NewHdfsEnv(Env** hdfs_env, const std::string& fsname) {
  *hdfs_env = new HdfsEnv(fsname);
  return Status::OK();
}
}  //命名空间rocksdb

#endif //rocksdb_hdfs_文件

#else //使用HDFS

//HDF不可用时使用的虚拟占位符
namespace rocksdb {
 Status HdfsEnv::NewSequentialFile(const std::string& fname,
                                   unique_ptr<SequentialFile>* result,
                                   const EnvOptions& options) {
   return Status::NotSupported("Not compiled with hdfs support");
 }

 Status NewHdfsEnv(Env** hdfs_env, const std::string& fsname) {
   return Status::NotSupported("Not compiled with hdfs support");
 }
}

#endif
