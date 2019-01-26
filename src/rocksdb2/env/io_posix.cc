
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

#ifdef ROCKSDB_LIB_IO_POSIX
#include "env/io_posix.h"
#include <errno.h>
#include <fcntl.h>
#include <algorithm>
#if defined(OS_LINUX)
#include <linux/fs.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef OS_LINUX
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#endif
#include "env/posix_logger.h"
#include "monitoring/iostats_context_imp.h"
#include "port/port.h"
#include "rocksdb/slice.h"
#include "util/coding.h"
#include "util/string_util.h"
#include "util/sync_point.h"

namespace rocksdb {

//Fadvise的包装器，如果平台不支持Fadvise，
//它只返回0。
int Fadvise(int fd, off_t offset, size_t len, int advice) {
#ifdef OS_LINUX
  return posix_fadvise(fd, offset, len, advice);
#else
return 0;  //什么都不做。
#endif
}

namespace {
size_t GetLogicalBufferSize(int __attribute__((__unused__)) fd) {
#ifdef OS_LINUX
  struct stat buf;
  int result = fstat(fd, &buf);
  if (result == -1) {
    return kDefaultPageSize;
  }
  if (major(buf.st_dev) == 0) {
//未命名设备（如非设备安装），保留为空设备号。
///sys/dev/block/中没有条目。返回合理的默认值。
    return kDefaultPageSize;
  }

//读取队列/逻辑块大小不需要特殊权限。
  const int kBufferSize = 100;
  char path[kBufferSize];
  char real_path[PATH_MAX + 1];
  snprintf(path, kBufferSize, "/sys/dev/block/%u:%u", major(buf.st_dev),
           minor(buf.st_dev));
  if (realpath(path, real_path) == nullptr) {
    return kDefaultPageSize;
  }
  std::string device_dir(real_path);
  if (!device_dir.empty() && device_dir.back() == '/') {
    device_dir.pop_back();
  }
//注意：sda3没有“queue/”子目录，只有父sda有它。
//$ls-al'/sys/dev/block/8:3'
//LRWXRWXRWX。1根根0 jun 26 01:38/sys/dev/block/8:3->
//../../block/sda/sda3段
  size_t parent_end = device_dir.rfind('/', device_dir.length() - 1);
  if (parent_end == std::string::npos) {
    return kDefaultPageSize;
  }
  size_t parent_begin = device_dir.rfind('/', parent_end - 1);
  if (parent_begin == std::string::npos) {
    return kDefaultPageSize;
  }
  if (device_dir.substr(parent_begin + 1, parent_end - parent_begin - 1) !=
      "block") {
    device_dir = device_dir.substr(0, parent_end);
  }
  std::string fname = device_dir + "/queue/logical_block_size";
  FILE* fp;
  size_t size = 0;
  fp = fopen(fname.c_str(), "r");
  if (fp != nullptr) {
    char* line = nullptr;
    size_t len = 0;
    if (getline(&line, &len, fp) != -1) {
      sscanf(line, "%zu", &size);
    }
    free(line);
    fclose(fp);
  }
  if (size != 0 && (size & (size - 1)) == 0) {
    return size;
  }
#endif
  return kDefaultPageSize;
}
} //命名空间

/*
 *直接助手
 **/

#ifndef NDEBUG
namespace {

bool IsSectorAligned(const size_t off, size_t sector_size) {
  return off % sector_size == 0;
}

bool IsSectorAligned(const void* ptr, size_t sector_size) {
  return uintptr_t(ptr) % sector_size == 0;
}

}
#endif

/*
 *位置序列文件
 **/

PosixSequentialFile::PosixSequentialFile(const std::string& fname, FILE* file,
                                         int fd, const EnvOptions& options)
    : filename_(fname),
      file_(file),
      fd_(fd),
      use_direct_io_(options.use_direct_reads),
      logical_sector_size_(GetLogicalBufferSize(fd_)) {
  assert(!options.use_direct_reads || !options.use_mmap_reads);
}

PosixSequentialFile::~PosixSequentialFile() {
  if (!use_direct_io()) {
    assert(file_);
    fclose(file_);
  } else {
    assert(fd_);
    close(fd_);
  }
}

Status PosixSequentialFile::Read(size_t n, Slice* result, char* scratch) {
  assert(result != nullptr && !use_direct_io());
  Status s;
  size_t r = 0;
  do {
    r = fread_unlocked(scratch, 1, n, file_);
  } while (r == 0 && ferror(file_) && errno == EINTR);
  *result = Slice(scratch, r);
  if (r < n) {
    if (feof(file_)) {
//如果我们点击文件的结尾，我们会将状态保留为OK。
//我们还清除错误以便继续读取
//如果新数据写入文件
      clearerr(file_);
    } else {
//带错误的部分读取：返回非OK状态
      s = IOError("While reading file sequentially", filename_, errno);
    }
  }
  return s;
}

Status PosixSequentialFile::PositionedRead(uint64_t offset, size_t n,
                                           Slice* result, char* scratch) {
  if (use_direct_io()) {
    assert(IsSectorAligned(offset, GetRequiredBufferAlignment()));
    assert(IsSectorAligned(n, GetRequiredBufferAlignment()));
    assert(IsSectorAligned(scratch, GetRequiredBufferAlignment()));
  }
  Status s;
  ssize_t r = -1;
  size_t left = n;
  char* ptr = scratch;
  assert(use_direct_io());
  while (left > 0) {
    r = pread(fd_, ptr, left, static_cast<off_t>(offset));
    if (r <= 0) {
      if (r == -1 && errno == EINTR) {
        continue;
      }
      break;
    }
    ptr += r;
    offset += r;
    left -= r;
    if (r % static_cast<ssize_t>(GetRequiredBufferAlignment()) != 0) {
//字节读取不会填充扇区。应该只发生在最后
//文件的。
      break;
    }
  }
  if (r < 0) {
//错误：返回非OK状态
    s = IOError(
        "While pread " + ToString(n) + " bytes from offset " + ToString(offset),
        filename_, errno);
  }
  *result = Slice(scratch, (r < 0) ? 0 : n - left);
  return s;
}

Status PosixSequentialFile::Skip(uint64_t n) {
  if (fseek(file_, static_cast<long int>(n), SEEK_CUR)) {
    return IOError("While fseek to skip " + ToString(n) + " bytes", filename_,
                   errno);
  }
  return Status::OK();
}

Status PosixSequentialFile::InvalidateCache(size_t offset, size_t length) {
#ifndef OS_LINUX
  return Status::OK();
#else
  if (!use_direct_io()) {
//免费操作系统页面
    int ret = Fadvise(fd_, offset, length, POSIX_FADV_DONTNEED);
    if (ret != 0) {
      return IOError("While fadvise NotNeeded offset " + ToString(offset) +
                         " len " + ToString(length),
                     filename_, errno);
    }
  }
  return Status::OK();
#endif
}

/*
 *posixrandomaaccess文件
 **/

#if defined(OS_LINUX)
size_t PosixHelper::GetUniqueIdFromFile(int fd, char* id, size_t max_size) {
  if (max_size < kMaxVarint64Length * 3) {
    return 0;
  }

  struct stat buf;
  int result = fstat(fd, &buf);
  assert(result != -1);
  if (result == -1) {
    return 0;
  }

  long version = 0;
  result = ioctl(fd, FS_IOC_GETVERSION, &version);
  TEST_SYNC_POINT_CALLBACK("GetUniqueIdFromFile:FS_IOC_GETVERSION", &result);
  if (result == -1) {
    return 0;
  }
  uint64_t uversion = (uint64_t)version;

  char* rid = id;
  rid = EncodeVarint64(rid, buf.st_dev);
  rid = EncodeVarint64(rid, buf.st_ino);
  rid = EncodeVarint64(rid, uversion);
  assert(rid >= id);
  return static_cast<size_t>(rid - id);
}
#endif

#if defined(OS_MACOSX) || defined(OS_AIX)
size_t PosixHelper::GetUniqueIdFromFile(int fd, char* id, size_t max_size) {
  if (max_size < kMaxVarint64Length * 3) {
    return 0;
  }

  struct stat buf;
  int result = fstat(fd, &buf);
  if (result == -1) {
    return 0;
  }

  char* rid = id;
  rid = EncodeVarint64(rid, buf.st_dev);
  rid = EncodeVarint64(rid, buf.st_ino);
  rid = EncodeVarint64(rid, buf.st_gen);
  assert(rid >= id);
  return static_cast<size_t>(rid - id);
}
#endif
/*
 *posixrandomaaccess文件
 *
 *基于pread（）的随机访问
 **/

PosixRandomAccessFile::PosixRandomAccessFile(const std::string& fname, int fd,
                                             const EnvOptions& options)
    : filename_(fname),
      fd_(fd),
      use_direct_io_(options.use_direct_reads),
      logical_sector_size_(GetLogicalBufferSize(fd_)) {
  assert(!options.use_direct_reads || !options.use_mmap_reads);
  assert(!options.use_mmap_reads || sizeof(void*) < 8);
}

PosixRandomAccessFile::~PosixRandomAccessFile() { close(fd_); }

Status PosixRandomAccessFile::Read(uint64_t offset, size_t n, Slice* result,
                                   char* scratch) const {
  if (use_direct_io()) {
    assert(IsSectorAligned(offset, GetRequiredBufferAlignment()));
    assert(IsSectorAligned(n, GetRequiredBufferAlignment()));
    assert(IsSectorAligned(scratch, GetRequiredBufferAlignment()));
  }
  Status s;
  ssize_t r = -1;
  size_t left = n;
  char* ptr = scratch;
  while (left > 0) {
    r = pread(fd_, ptr, left, static_cast<off_t>(offset));
    if (r <= 0) {
      if (r == -1 && errno == EINTR) {
        continue;
      }
      break;
    }
    ptr += r;
    offset += r;
    left -= r;
    if (use_direct_io() &&
        r % static_cast<ssize_t>(GetRequiredBufferAlignment()) != 0) {
//字节读取不会填充扇区。应该只发生在最后
//文件的。
      break;
    }
  }
  if (r < 0) {
//错误：返回非OK状态
    s = IOError(
        "While pread offset " + ToString(offset) + " len " + ToString(n),
        filename_, errno);
  }
  *result = Slice(scratch, (r < 0) ? 0 : n - left);
  return s;
}

Status PosixRandomAccessFile::Prefetch(uint64_t offset, size_t n) {
  Status s;
  if (!use_direct_io()) {
    ssize_t r = 0;
#ifdef OS_LINUX
    r = readahead(fd_, offset, n);
#endif
#ifdef OS_MACOSX
    radvisory advice;
    advice.ra_offset = static_cast<off_t>(offset);
    advice.ra_count = static_cast<int>(n);
    r = fcntl(fd_, F_RDADVISE, &advice);
#endif
    if (r == -1) {
      s = IOError("While prefetching offset " + ToString(offset) + " len " +
                      ToString(n),
                  filename_, errno);
    }
  }
  return s;
}

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_AIX)
size_t PosixRandomAccessFile::GetUniqueId(char* id, size_t max_size) const {
  return PosixHelper::GetUniqueIdFromFile(fd_, id, max_size);
}
#endif

void PosixRandomAccessFile::Hint(AccessPattern pattern) {
  if (use_direct_io()) {
    return;
  }
  switch (pattern) {
    case NORMAL:
      Fadvise(fd_, 0, 0, POSIX_FADV_NORMAL);
      break;
    case RANDOM:
      Fadvise(fd_, 0, 0, POSIX_FADV_RANDOM);
      break;
    case SEQUENTIAL:
      Fadvise(fd_, 0, 0, POSIX_FADV_SEQUENTIAL);
      break;
    case WILLNEED:
      Fadvise(fd_, 0, 0, POSIX_FADV_WILLNEED);
      break;
    case DONTNEED:
      Fadvise(fd_, 0, 0, POSIX_FADV_DONTNEED);
      break;
    default:
      assert(false);
      break;
  }
}

Status PosixRandomAccessFile::InvalidateCache(size_t offset, size_t length) {
  if (use_direct_io()) {
    return Status::OK();
  }
#ifndef OS_LINUX
  return Status::OK();
#else
//免费操作系统页面
  int ret = Fadvise(fd_, offset, length, POSIX_FADV_DONTNEED);
  if (ret == 0) {
    return Status::OK();
  }
  return IOError("While fadvise NotNeeded offset " + ToString(offset) +
                     " len " + ToString(length),
                 filename_, errno);
#endif
}

/*
 *posixmmapreadable文件
 *
 *基于mmap（）的随机访问
 **/

//base[0，length-1]包含文件的mmap内容。
PosixMmapReadableFile::PosixMmapReadableFile(const int fd,
                                             const std::string& fname,
                                             void* base, size_t length,
                                             const EnvOptions& options)
    : fd_(fd), filename_(fname), mmapped_region_(base), length_(length) {
fd_ = fd_ + 0;  //禁止对已用变量发出警告
  assert(options.use_mmap_reads);
  assert(!options.use_direct_reads);
}

PosixMmapReadableFile::~PosixMmapReadableFile() {
  int ret = munmap(mmapped_region_, length_);
  if (ret != 0) {
    fprintf(stdout, "failed to munmap %p length %" ROCKSDB_PRIszt " \n",
            mmapped_region_, length_);
  }
}

Status PosixMmapReadableFile::Read(uint64_t offset, size_t n, Slice* result,
                                   char* scratch) const {
  Status s;
  if (offset > length_) {
    *result = Slice();
    return IOError("While mmap read offset " + ToString(offset) +
                       " larger than file length " + ToString(length_),
                   filename_, EINVAL);
  } else if (offset + n > length_) {
    n = static_cast<size_t>(length_ - offset);
  }
  *result = Slice(reinterpret_cast<char*>(mmapped_region_) + offset, n);
  return s;
}

Status PosixMmapReadableFile::InvalidateCache(size_t offset, size_t length) {
#ifndef OS_LINUX
  return Status::OK();
#else
//免费操作系统页面
  int ret = Fadvise(fd_, offset, length, POSIX_FADV_DONTNEED);
  if (ret == 0) {
    return Status::OK();
  }
  return IOError("While fadvise not needed. Offset " + ToString(offset) +
                     " len" + ToString(length),
                 filename_, errno);
#endif
}

/*
 ＊PosixMAMAPFILE
 *
 *我们预分配最多一个额外的兆字节，并使用memcpy附加新的
 *数据到文件。这是安全的，因为我们要么适当关闭
 *文件在读取之前，或对于日志文件，读取代码
 *足够了解跳过零后缀。
 **/

Status PosixMmapFile::UnmapCurrentRegion() {
  TEST_KILL_RANDOM("PosixMmapFile::UnmapCurrentRegion:0", rocksdb_kill_odds);
  if (base_ != nullptr) {
    int munmap_status = munmap(base_, limit_ - base_);
    if (munmap_status != 0) {
      return IOError("While munmap", filename_, munmap_status);
    }
    file_offset_ += limit_ - base_;
    base_ = nullptr;
    limit_ = nullptr;
    last_sync_ = nullptr;
    dst_ = nullptr;

//增加我们下次映射的量，但上限为1MB
    if (map_size_ < (1 << 20)) {
      map_size_ *= 2;
    }
  }
  return Status::OK();
}

Status PosixMmapFile::MapNewRegion() {
#ifdef ROCKSDB_FALLOCATE_PRESENT
  assert(base_ == nullptr);
  TEST_KILL_RANDOM("PosixMmapFile::UnmapCurrentRegion:0", rocksdb_kill_odds);
//我们不能和法洛一起休息，保持这里的大小
  if (allow_fallocate_) {
    IOSTATS_TIMER_GUARD(allocate_nanos);
    int alloc_status = fallocate(fd_, 0, file_offset_, map_size_);
    if (alloc_status != 0) {
//回退到位置
      alloc_status = posix_fallocate(fd_, file_offset_, map_size_);
    }
    if (alloc_status != 0) {
      return Status::IOError("Error allocating space to file : " + filename_ +
                             "Error : " + strerror(alloc_status));
    }
  }

  TEST_KILL_RANDOM("PosixMmapFile::Append:1", rocksdb_kill_odds);
  void* ptr = mmap(nullptr, map_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_,
                   file_offset_);
  if (ptr == MAP_FAILED) {
    return Status::IOError("MMap failed on " + filename_);
  }
  TEST_KILL_RANDOM("PosixMmapFile::Append:2", rocksdb_kill_odds);

  base_ = reinterpret_cast<char*>(ptr);
  limit_ = base_ + map_size_;
  dst_ = base_;
  last_sync_ = base_;
  return Status::OK();
#else
  return Status::NotSupported("This platform doesn't support fallocate()");
#endif
}

Status PosixMmapFile::Msync() {
  if (dst_ == last_sync_) {
    return Status::OK();
  }
//查找包含第一页和最后一页的开头
//要同步的字节。
  size_t p1 = TruncateToPageBoundary(last_sync_ - base_);
  size_t p2 = TruncateToPageBoundary(dst_ - base_ - 1);
  last_sync_ = dst_;
  TEST_KILL_RANDOM("PosixMmapFile::Msync:0", rocksdb_kill_odds);
  if (msync(base_ + p1, p2 - p1 + page_size_, MS_SYNC) < 0) {
    return IOError("While msync", filename_, errno);
  }
  return Status::OK();
}

PosixMmapFile::PosixMmapFile(const std::string& fname, int fd, size_t page_size,
                             const EnvOptions& options)
    : filename_(fname),
      fd_(fd),
      page_size_(page_size),
      map_size_(Roundup(65536, page_size)),
      base_(nullptr),
      limit_(nullptr),
      dst_(nullptr),
      last_sync_(nullptr),
      file_offset_(0) {
#ifdef ROCKSDB_FALLOCATE_PRESENT
  allow_fallocate_ = options.allow_fallocate;
  fallocate_with_keep_size_ = options.fallocate_with_keep_size;
#endif
  assert((page_size & (page_size - 1)) == 0);
  assert(options.use_mmap_writes);
  assert(!options.use_direct_writes);
}

PosixMmapFile::~PosixMmapFile() {
  if (fd_ >= 0) {
    PosixMmapFile::Close();
  }
}

Status PosixMmapFile::Append(const Slice& data) {
  const char* src = data.data();
  size_t left = data.size();
  while (left > 0) {
    assert(base_ <= dst_);
    assert(dst_ <= limit_);
    size_t avail = limit_ - dst_;
    if (avail == 0) {
      Status s = UnmapCurrentRegion();
      if (!s.ok()) {
        return s;
      }
      s = MapNewRegion();
      if (!s.ok()) {
        return s;
      }
      TEST_KILL_RANDOM("PosixMmapFile::Append:0", rocksdb_kill_odds);
    }

    size_t n = (left <= avail) ? left : avail;
    assert(dst_);
    memcpy(dst_, src, n);
    dst_ += n;
    src += n;
    left -= n;
  }
  return Status::OK();
}

Status PosixMmapFile::Close() {
  Status s;
  size_t unused = limit_ - dst_;

  s = UnmapCurrentRegion();
  if (!s.ok()) {
    s = IOError("While closing mmapped file", filename_, errno);
  } else if (unused > 0) {
//修剪文件末尾的多余空间
    if (ftruncate(fd_, file_offset_ - unused) < 0) {
      s = IOError("While ftruncating mmaped file", filename_, errno);
    }
  }

  if (close(fd_) < 0) {
    if (s.ok()) {
      s = IOError("While closing mmapped file", filename_, errno);
    }
  }

  fd_ = -1;
  base_ = nullptr;
  limit_ = nullptr;
  return s;
}

Status PosixMmapFile::Flush() { return Status::OK(); }

Status PosixMmapFile::Sync() {
  if (fdatasync(fd_) < 0) {
    return IOError("While fdatasync mmapped file", filename_, errno);
  }

  return Msync();
}

/*
 *将数据和元数据刷新到稳定存储。
 **/

Status PosixMmapFile::Fsync() {
  if (fsync(fd_) < 0) {
    return IOError("While fsync mmaped file", filename_, errno);
  }

  return Msync();
}

/*
 *获取文件中有效数据的大小。这与
 *文件系统返回的大小，因为我们使用mmap
 *每次按地图大小扩展文件。
 **/

uint64_t PosixMmapFile::GetFileSize() {
  size_t used = dst_ - base_;
  return file_offset_ + used;
}

Status PosixMmapFile::InvalidateCache(size_t offset, size_t length) {
#ifndef OS_LINUX
  return Status::OK();
#else
//免费操作系统页面
  int ret = Fadvise(fd_, offset, length, POSIX_FADV_DONTNEED);
  if (ret == 0) {
    return Status::OK();
  }
  return IOError("While fadvise NotNeeded mmapped file", filename_, errno);
#endif
}

#ifdef ROCKSDB_FALLOCATE_PRESENT
Status PosixMmapFile::Allocate(uint64_t offset, uint64_t len) {
  assert(offset <= std::numeric_limits<off_t>::max());
  assert(len <= std::numeric_limits<off_t>::max());
  TEST_KILL_RANDOM("PosixMmapFile::Allocate:0", rocksdb_kill_odds);
  int alloc_status = 0;
  if (allow_fallocate_) {
    alloc_status = fallocate(
        fd_, fallocate_with_keep_size_ ? FALLOC_FL_KEEP_SIZE : 0,
          static_cast<off_t>(offset), static_cast<off_t>(len));
  }
  if (alloc_status == 0) {
    return Status::OK();
  } else {
    return IOError(
        "While fallocate offset " + ToString(offset) + " len " + ToString(len),
        filename_, errno);
  }
}
#endif

/*
 *posixwritable文件
 *
 *使用posix write将数据写入文件。
 **/

PosixWritableFile::PosixWritableFile(const std::string& fname, int fd,
                                     const EnvOptions& options)
    : filename_(fname),
      use_direct_io_(options.use_direct_writes),
      fd_(fd),
      filesize_(0),
      logical_sector_size_(GetLogicalBufferSize(fd_)) {
#ifdef ROCKSDB_FALLOCATE_PRESENT
  allow_fallocate_ = options.allow_fallocate;
  fallocate_with_keep_size_ = options.fallocate_with_keep_size;
#endif
  assert(!options.use_mmap_writes);
}

PosixWritableFile::~PosixWritableFile() {
  if (fd_ >= 0) {
    PosixWritableFile::Close();
  }
}

Status PosixWritableFile::Append(const Slice& data) {
  if (use_direct_io()) {
    assert(IsSectorAligned(data.size(), GetRequiredBufferAlignment()));
    assert(IsSectorAligned(data.data(), GetRequiredBufferAlignment()));
  }
  const char* src = data.data();
  size_t left = data.size();
  while (left != 0) {
    ssize_t done = write(fd_, src, left);
    if (done < 0) {
      if (errno == EINTR) {
        continue;
      }
      return IOError("While appending to file", filename_, errno);
    }
    left -= done;
    src += done;
  }
  filesize_ += data.size();
  return Status::OK();
}

Status PosixWritableFile::PositionedAppend(const Slice& data, uint64_t offset) {
  if (use_direct_io()) {
    assert(IsSectorAligned(offset, GetRequiredBufferAlignment()));
    assert(IsSectorAligned(data.size(), GetRequiredBufferAlignment()));
    assert(IsSectorAligned(data.data(), GetRequiredBufferAlignment()));
  }
  assert(offset <= std::numeric_limits<off_t>::max());
  const char* src = data.data();
  size_t left = data.size();
  while (left != 0) {
    ssize_t done = pwrite(fd_, src, left, static_cast<off_t>(offset));
    if (done < 0) {
      if (errno == EINTR) {
        continue;
      }
      return IOError("While pwrite to file at offset " + ToString(offset),
                     filename_, errno);
    }
    left -= done;
    offset += done;
    src += done;
  }
  filesize_ = offset;
  return Status::OK();
}

Status PosixWritableFile::Truncate(uint64_t size) {
  Status s;
  int r = ftruncate(fd_, size);
  if (r < 0) {
    s = IOError("While ftruncate file to size " + ToString(size), filename_,
                errno);
  } else {
    filesize_ = size;
  }
  return s;
}

Status PosixWritableFile::Close() {
  Status s;

  size_t block_size;
  size_t last_allocated_block;
  GetPreallocationStatus(&block_size, &last_allocated_block);
  if (last_allocated_block > 0) {
//修剪文件末尾预先分配的额外空间
//注（Ljin）：我们可能不希望将故障视为IOERROR，
//但最好记录这些错误。
    int dummy __attribute__((unused));
    dummy = ftruncate(fd_, filesize_);
#if defined(ROCKSDB_FALLOCATE_PRESENT) && !defined(TRAVIS)
//在某些文件系统中，如果
//新文件大小小于当前大小。调用fallocate
//使用falloc_fl_punch_hole标志明确释放这些未使用的
//阻碍。至少在以下位置支持falloc_fl_punch_孔
//文件系统：
//XFS（从Linux 2.6.38开始）
//ext4（从Linux 3.0开始）
//btrfs（从Linux 3.7开始）
//tmpfs（从Linux 3.5开始）
//我们忽略错误，因为此操作的失败不会影响
//正确性。
//travis-此代码不适用于travis文件系统。
//falloc fl_keep_size选项预计不会更改大小。
//文件，但它确实如此。简单的战略报告将显示这一点。
//当我们与Travis CI团队合作，以确定这是否是
//码头工人/AUF的怪癖，我们将对此发表评论。
    struct stat file_stats;
    int result = fstat(fd_, &file_stats);
//在ftruncate之后，我们检查ftruncate是否具有正确的行为。
//如果不是，我们应该用法洛打洞
    if (result == 0 &&
        (file_stats.st_size + file_stats.st_blksize - 1) /
            file_stats.st_blksize !=
        file_stats.st_blocks / (file_stats.st_blksize / 512)) {
      IOSTATS_TIMER_GUARD(allocate_nanos);
      if (allow_fallocate_) {
        fallocate(fd_, FALLOC_FL_KEEP_SIZE | FALLOC_FL_PUNCH_HOLE, filesize_,
                  block_size * last_allocated_block - filesize_);
      }
    }
#endif
  }

  if (close(fd_) < 0) {
    s = IOError("While closing file after writing", filename_, errno);
  }
  fd_ = -1;
  return s;
}

//将缓存数据写入OS缓存
Status PosixWritableFile::Flush() { return Status::OK(); }

Status PosixWritableFile::Sync() {
  if (fdatasync(fd_) < 0) {
    return IOError("While fdatasync", filename_, errno);
  }
  return Status::OK();
}

Status PosixWritableFile::Fsync() {
  if (fsync(fd_) < 0) {
    return IOError("While fsync", filename_, errno);
  }
  return Status::OK();
}

bool PosixWritableFile::IsSyncThreadSafe() const { return true; }

uint64_t PosixWritableFile::GetFileSize() { return filesize_; }

Status PosixWritableFile::InvalidateCache(size_t offset, size_t length) {
  if (use_direct_io()) {
    return Status::OK();
  }
#ifndef OS_LINUX
  return Status::OK();
#else
//免费操作系统页面
  int ret = Fadvise(fd_, offset, length, POSIX_FADV_DONTNEED);
  if (ret == 0) {
    return Status::OK();
  }
  return IOError("While fadvise NotNeeded", filename_, errno);
#endif
}

#ifdef ROCKSDB_FALLOCATE_PRESENT
Status PosixWritableFile::Allocate(uint64_t offset, uint64_t len) {
  assert(offset <= std::numeric_limits<off_t>::max());
  assert(len <= std::numeric_limits<off_t>::max());
  TEST_KILL_RANDOM("PosixWritableFile::Allocate:0", rocksdb_kill_odds);
  IOSTATS_TIMER_GUARD(allocate_nanos);
  int alloc_status = 0;
  if (allow_fallocate_) {
    alloc_status = fallocate(
        fd_, fallocate_with_keep_size_ ? FALLOC_FL_KEEP_SIZE : 0,
        static_cast<off_t>(offset), static_cast<off_t>(len));
  }
  if (alloc_status == 0) {
    return Status::OK();
  } else {
    return IOError(
        "While fallocate offset " + ToString(offset) + " len " + ToString(len),
        filename_, errno);
  }
}
#endif

#ifdef ROCKSDB_RANGESYNC_PRESENT
Status PosixWritableFile::RangeSync(uint64_t offset, uint64_t nbytes) {
  assert(offset <= std::numeric_limits<off_t>::max());
  assert(nbytes <= std::numeric_limits<off_t>::max());
  if (sync_file_range(fd_, static_cast<off_t>(offset),
      static_cast<off_t>(nbytes), SYNC_FILE_RANGE_WRITE) == 0) {
    return Status::OK();
  } else {
    return IOError("While sync_file_range offset " + ToString(offset) +
                       " bytes " + ToString(nbytes),
                   filename_, errno);
  }
}
#endif

#ifdef OS_LINUX
size_t PosixWritableFile::GetUniqueId(char* id, size_t max_size) const {
  return PosixHelper::GetUniqueIdFromFile(fd_, id, max_size);
}
#endif

/*
 *posixrandomrwfile
 **/


PosixRandomRWFile::PosixRandomRWFile(const std::string& fname, int fd,
                                     const EnvOptions& options)
    : filename_(fname), fd_(fd) {}

PosixRandomRWFile::~PosixRandomRWFile() {
  if (fd_ >= 0) {
    Close();
  }
}

Status PosixRandomRWFile::Write(uint64_t offset, const Slice& data) {
  const char* src = data.data();
  size_t left = data.size();
  while (left != 0) {
    ssize_t done = pwrite(fd_, src, left, offset);
    if (done < 0) {
//写入文件时出错
      if (errno == EINTR) {
//写入被中断，请重试。
        continue;
      }
      return IOError(
          "While write random read/write file at offset " + ToString(offset),
          filename_, errno);
    }

//已写入“done”字节
    left -= done;
    offset += done;
    src += done;
  }

  return Status::OK();
}

Status PosixRandomRWFile::Read(uint64_t offset, size_t n, Slice* result,
                               char* scratch) const {
  size_t left = n;
  char* ptr = scratch;
  while (left > 0) {
    ssize_t done = pread(fd_, ptr, left, offset);
    if (done < 0) {
//读取文件时出错
      if (errno == EINTR) {
//读取被中断，请重试。
        continue;
      }
      return IOError("While reading random read/write file offset " +
                         ToString(offset) + " len " + ToString(n),
                     filename_, errno);
    } else if (done == 0) {
//没什么可看的了
      break;
    }

//读取“完成”字节
    ptr += done;
    offset += done;
    left -= done;
  }

  *result = Slice(scratch, n - left);
  return Status::OK();
}

Status PosixRandomRWFile::Flush() { return Status::OK(); }

Status PosixRandomRWFile::Sync() {
  if (fdatasync(fd_) < 0) {
    return IOError("While fdatasync random read/write file", filename_, errno);
  }
  return Status::OK();
}

Status PosixRandomRWFile::Fsync() {
  if (fsync(fd_) < 0) {
    return IOError("While fsync random read/write file", filename_, errno);
  }
  return Status::OK();
}

Status PosixRandomRWFile::Close() {
  if (close(fd_) < 0) {
    return IOError("While close random read/write file", filename_, errno);
  }
  fd_ = -1;
  return Status::OK();
}

/*
 *位置目录
 **/


PosixDirectory::~PosixDirectory() { close(fd_); }

Status PosixDirectory::Fsync() {
#ifndef OS_AIX
  if (fsync(fd_) == -1) {
    return IOError("While fsync", "a directory", errno);
  }
#endif
  return Status::OK();
}
}  //命名空间rocksdb
#endif
