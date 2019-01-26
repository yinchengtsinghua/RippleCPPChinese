
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
//在许可证文件中找到。有关参与者的名称，请参阅作者文件
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#if defined(OS_LINUX)
#include <linux/fs.h>
#endif
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#ifdef OS_LINUX
#include <sys/statfs.h>
#include <sys/syscall.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <algorithm>
//获取nano时间包括
#if defined(OS_LINUX) || defined(OS_FREEBSD)
#elif defined(__MACH__)
#include <mach/clock.h>
#include <mach/mach.h>
#else
#include <chrono>
#endif
#include <deque>
#include <set>
#include <vector>

#include "env/io_posix.h"
#include "env/posix_logger.h"
#include "monitoring/iostats_context_imp.h"
#include "monitoring/thread_status_updater.h"
#include "port/port.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "util/coding.h"
#include "util/logging.h"
#include "util/random.h"
#include "util/string_util.h"
#include "util/sync_point.h"
#include "util/thread_local.h"
#include "util/threadpool_imp.h"

#if !defined(TMPFS_MAGIC)
#define TMPFS_MAGIC 0x01021994
#endif
#if !defined(XFS_SUPER_MAGIC)
#define XFS_SUPER_MAGIC 0x58465342
#endif
#if !defined(EXT4_SUPER_MAGIC)
#define EXT4_SUPER_MAGIC 0xEF53
#endif

namespace rocksdb {

namespace {

ThreadStatusUpdater* CreateThreadStatusUpdater() {
  return new ThreadStatusUpdater();
}

//锁定的路径名列表
static std::set<std::string> lockedFiles;
static port::Mutex mutex_lockedFiles;

static int LockOrUnlock(const std::string& fname, int fd, bool lock) {
  mutex_lockedFiles.Lock();
  if (lock) {
//如果它已经存在于lockedfiles集中，那么它已经被锁定，
//并使锁定尝试失败。否则，将其插入到锁定的文件中。
//此检查是必需的，因为fcntl（）未检测到锁冲突
//如果FCNTL是由先前获取的同一线程发出的
//这把锁。
    if (lockedFiles.insert(fname).second == false) {
      mutex_lockedFiles.Unlock();
      errno = ENOLCK;
      return -1;
    }
  } else {
//如果我们正在解锁，那么确认我们之前锁定过它，
//它应该已经存在于lockedfiles中。从锁定的文件中删除。
    if (lockedFiles.erase(fname) != 1) {
      mutex_lockedFiles.Unlock();
      errno = ENOLCK;
      return -1;
    }
  }
  errno = 0;
  struct flock f;
  memset(&f, 0, sizeof(f));
  f.l_type = (lock ? F_WRLCK : F_UNLCK);
  f.l_whence = SEEK_SET;
  f.l_start = 0;
f.l_len = 0;        //锁定/解锁整个文件
  int value = fcntl(fd, F_SETLK, &f);
  if (value == -1 && lock) {
//如果锁定时出错，请从lockedfiles中删除路径名
    lockedFiles.erase(fname);
  }
  mutex_lockedFiles.Unlock();
  return value;
}

class PosixFileLock : public FileLock {
 public:
  int fd_;
  std::string filename;
};

class PosixEnv : public Env {
 public:
  PosixEnv();

  virtual ~PosixEnv() {
    for (const auto tid : threads_to_join_) {
      pthread_join(tid, nullptr);
    }
    for (int pool_id = 0; pool_id < Env::Priority::TOTAL; ++pool_id) {
      thread_pools_[pool_id].JoinAllThreads();
    }
//仅当当前env不是时，才删除线程\状态\更新程序\uU
//env:：default（）。这是为了避免免费使用后出错时
//当其他一些子线程
//仍在尝试更新线程状态。
    if (this != Env::Default()) {
      delete thread_status_updater_;
    }
  }

  void SetFD_CLOEXEC(int fd, const EnvOptions* options) {
    if ((options == nullptr || options->set_fd_cloexec) && fd > 0) {
      fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
    }
  }

  virtual Status NewSequentialFile(const std::string& fname,
                                   unique_ptr<SequentialFile>* result,
                                   const EnvOptions& options) override {
    result->reset();
    int fd = -1;
    int flags = O_RDONLY;
    FILE* file = nullptr;

    if (options.use_direct_reads && !options.use_mmap_reads) {
#ifdef ROCKSDB_LITE
      return Status::IOError(fname, "Direct I/O not supported in RocksDB lite");
#endif  //！摇滚乐
#if !defined(OS_MACOSX) && !defined(OS_OPENBSD) && !defined(OS_SOLARIS)
      flags |= O_DIRECT;
#endif
    }

    do {
      IOSTATS_TIMER_GUARD(open_nanos);
      fd = open(fname.c_str(), flags, 0644);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0) {
      return IOError("While opening a file for sequentially reading", fname,
                     errno);
    }

    SetFD_CLOEXEC(fd, &options);

    if (options.use_direct_reads && !options.use_mmap_reads) {
#ifdef OS_MACOSX
      if (fcntl(fd, F_NOCACHE, 1) == -1) {
        close(fd);
        return IOError("While fcntl NoCache", fname, errno);
      }
#endif
    } else {
      do {
        IOSTATS_TIMER_GUARD(open_nanos);
        file = fdopen(fd, "r");
      } while (file == nullptr && errno == EINTR);
      if (file == nullptr) {
        close(fd);
        return IOError("While opening file for sequentially read", fname,
                       errno);
      }
    }
    result->reset(new PosixSequentialFile(fname, file, fd, options));
    return Status::OK();
  }

  virtual Status NewRandomAccessFile(const std::string& fname,
                                     unique_ptr<RandomAccessFile>* result,
                                     const EnvOptions& options) override {
    result->reset();
    Status s;
    int fd;
    int flags = O_RDONLY;
    if (options.use_direct_reads && !options.use_mmap_reads) {
#ifdef ROCKSDB_LITE
      return Status::IOError(fname, "Direct I/O not supported in RocksDB lite");
#endif  //！摇滚乐
#if !defined(OS_MACOSX) && !defined(OS_OPENBSD) && !defined(OS_SOLARIS)
      flags |= O_DIRECT;
      TEST_SYNC_POINT_CALLBACK("NewRandomAccessFile:O_DIRECT", &flags);
#endif
    }

    do {
      IOSTATS_TIMER_GUARD(open_nanos);
      fd = open(fname.c_str(), flags, 0644);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0) {
      return IOError("While open a file for random read", fname, errno);
    }
    SetFD_CLOEXEC(fd, &options);

    if (options.use_mmap_reads && sizeof(void*) >= 8) {
//使用mmap进行随机读取已被删除，因为它
//当存储速度快时会降低性能。
//当虚拟地址空间足够时使用mmap。
      uint64_t size;
      s = GetFileSize(fname, &size);
      if (s.ok()) {
        void* base = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
        if (base != MAP_FAILED) {
          result->reset(new PosixMmapReadableFile(fd, fname, base,
                                                  size, options));
        } else {
          s = IOError("while mmap file for read", fname, errno);
        }
      }
      close(fd);
    } else {
      if (options.use_direct_reads && !options.use_mmap_reads) {
#ifdef OS_MACOSX
        if (fcntl(fd, F_NOCACHE, 1) == -1) {
          close(fd);
          return IOError("while fcntl NoCache", fname, errno);
        }
#endif
      }
      result->reset(new PosixRandomAccessFile(fname, fd, options));
    }
    return s;
  }

  virtual Status OpenWritableFile(const std::string& fname,
                                  unique_ptr<WritableFile>* result,
                                  const EnvOptions& options,
                                  bool reopen = false) {
    result->reset();
    Status s;
    int fd = -1;
    int flags = (reopen) ? (O_CREAT | O_APPEND) : (O_CREAT | O_TRUNC);
//直接IO模式，带O_direct标志或F_nocahce（Mac OSX）
    if (options.use_direct_writes && !options.use_mmap_writes) {
//注意：我们应该避免在这里附加O_，因为TA有以下错误：
//posix要求打开带有o_append标志的文件
//对pwrite（）写入数据的位置没有影响。
//但是，在Linux上，如果文件是使用o_Append、pwrite（）打开的
//将数据追加到文件结尾，而不管
//抵消。
//更多信息请访问：https://linux.die.net/man/2/pwrite
#ifdef ROCKSDB_LITE
      return Status::IOError(fname, "Direct I/O not supported in RocksDB lite");
#endif  //摇滚乐
      flags |= O_WRONLY;
#if !defined(OS_MACOSX) && !defined(OS_OPENBSD) && !defined(OS_SOLARIS)
      flags |= O_DIRECT;
#endif
      TEST_SYNC_POINT_CALLBACK("NewWritableFile:O_DIRECT", &flags);
    } else if (options.use_mmap_writes) {
//非直接输入输出
      flags |= O_RDWR;
    } else {
      flags |= O_WRONLY;
    }

    do {
      IOSTATS_TIMER_GUARD(open_nanos);
      fd = open(fname.c_str(), flags, 0644);
    } while (fd < 0 && errno == EINTR);

    if (fd < 0) {
      s = IOError("While open a file for appending", fname, errno);
      return s;
    }
    SetFD_CLOEXEC(fd, &options);

    if (options.use_mmap_writes) {
      if (!checkedDiskForMmap_) {
//这将在程序的生命周期内执行一次。
//不要在非ext-3/xfs/tmpfs系统上使用mmapwrite。
        if (!SupportsFastAllocate(fname)) {
          forceMmapOff_ = true;
        }
        checkedDiskForMmap_ = true;
      }
    }
    if (options.use_mmap_writes && !forceMmapOff_) {
      result->reset(new PosixMmapFile(fname, fd, page_size_, options));
    } else if (options.use_direct_writes && !options.use_mmap_writes) {
#ifdef OS_MACOSX
      if (fcntl(fd, F_NOCACHE, 1) == -1) {
        close(fd);
        s = IOError("While fcntl NoCache an opened file for appending", fname,
                    errno);
        return s;
      }
#elif defined(OS_SOLARIS)
      if (directio(fd, DIRECTIO_ON) == -1) {
if (errno != ENOTTY) { //zfs文件系统不支持
          close(fd);
          s = IOError("While calling directio()", fname, errno);
          return s;
        }
      }
#endif
      result->reset(new PosixWritableFile(fname, fd, options));
    } else {
//禁用mmap写入
      EnvOptions no_mmap_writes_options = options;
      no_mmap_writes_options.use_mmap_writes = false;
      result->reset(new PosixWritableFile(fname, fd, no_mmap_writes_options));
    }
    return s;
  }

  virtual Status NewWritableFile(const std::string& fname,
                                 unique_ptr<WritableFile>* result,
                                 const EnvOptions& options) override {
    return OpenWritableFile(fname, result, options, false);
  }

  virtual Status ReopenWritableFile(const std::string& fname,
                                    unique_ptr<WritableFile>* result,
                                    const EnvOptions& options) override {
    return OpenWritableFile(fname, result, options, true);
  }

  virtual Status ReuseWritableFile(const std::string& fname,
                                   const std::string& old_fname,
                                   unique_ptr<WritableFile>* result,
                                   const EnvOptions& options) override {
    result->reset();
    Status s;
    int fd = -1;

    int flags = 0;
//直接IO模式，带O_direct标志或F_nocahce（Mac OSX）
    if (options.use_direct_writes && !options.use_mmap_writes) {
#ifdef ROCKSDB_LITE
      return Status::IOError(fname, "Direct I/O not supported in RocksDB lite");
#endif  //！摇滚乐
      flags |= O_WRONLY;
#if !defined(OS_MACOSX) && !defined(OS_OPENBSD) && !defined(OS_SOLARIS)
      flags |= O_DIRECT;
#endif
      TEST_SYNC_POINT_CALLBACK("NewWritableFile:O_DIRECT", &flags);
    } else if (options.use_mmap_writes) {
//MMAP需要O-RDWR模式
      flags |= O_RDWR;
    } else {
      flags |= O_WRONLY;
    }

    do {
      IOSTATS_TIMER_GUARD(open_nanos);
      fd = open(old_fname.c_str(), flags, 0644);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0) {
      s = IOError("while reopen file for write", fname, errno);
      return s;
    }

    SetFD_CLOEXEC(fd, &options);
//重命名到位
    if (rename(old_fname.c_str(), fname.c_str()) != 0) {
      s = IOError("while rename file to " + fname, old_fname, errno);
      close(fd);
      return s;
    }

    if (options.use_mmap_writes) {
      if (!checkedDiskForMmap_) {
//这将在程序的生命周期内执行一次。
//不要在非ext-3/xfs/tmpfs系统上使用mmapwrite。
        if (!SupportsFastAllocate(fname)) {
          forceMmapOff_ = true;
        }
        checkedDiskForMmap_ = true;
      }
    }
    if (options.use_mmap_writes && !forceMmapOff_) {
      result->reset(new PosixMmapFile(fname, fd, page_size_, options));
    } else if (options.use_direct_writes && !options.use_mmap_writes) {
#ifdef OS_MACOSX
      if (fcntl(fd, F_NOCACHE, 1) == -1) {
        close(fd);
        s = IOError("while fcntl NoCache for reopened file for append", fname,
                    errno);
        return s;
      }
#elif defined(OS_SOLARIS)
      if (directio(fd, DIRECTIO_ON) == -1) {
if (errno != ENOTTY) { //zfs文件系统不支持
          close(fd);
          s = IOError("while calling directio()", fname, errno);
          return s;
        }
      }
#endif
      result->reset(new PosixWritableFile(fname, fd, options));
    } else {
//禁用mmap写入
      EnvOptions no_mmap_writes_options = options;
      no_mmap_writes_options.use_mmap_writes = false;
      result->reset(new PosixWritableFile(fname, fd, no_mmap_writes_options));
    }
    return s;

    return s;
  }

  virtual Status NewRandomRWFile(const std::string& fname,
                                 unique_ptr<RandomRWFile>* result,
                                 const EnvOptions& options) override {
    int fd = -1;
    while (fd < 0) {
      IOSTATS_TIMER_GUARD(open_nanos);
      fd = open(fname.c_str(), O_CREAT | O_RDWR, 0644);
      if (fd < 0) {
//打开文件时出错
        if (errno == EINTR) {
          continue;
        }
        return IOError("While open file for random read/write", fname, errno);
      }
    }

    SetFD_CLOEXEC(fd, &options);
    result->reset(new PosixRandomRWFile(fname, fd, options));
    return Status::OK();
  }

  virtual Status NewDirectory(const std::string& name,
                              unique_ptr<Directory>* result) override {
    result->reset();
    int fd;
    {
      IOSTATS_TIMER_GUARD(open_nanos);
      fd = open(name.c_str(), 0);
    }
    if (fd < 0) {
      return IOError("While open directory", name, errno);
    } else {
      result->reset(new PosixDirectory(fd));
    }
    return Status::OK();
  }

  virtual Status FileExists(const std::string& fname) override {
    int result = access(fname.c_str(), F_OK);

    if (result == 0) {
      return Status::OK();
    }

    switch (errno) {
      case EACCES:
      case ELOOP:
      case ENAMETOOLONG:
      case ENOENT:
      case ENOTDIR:
        return Status::NotFound();
      default:
        assert(result == EIO || result == ENOMEM);
        return Status::IOError("Unexpected error(" + ToString(result) +
                               ") accessing file `" + fname + "' ");
    }
  }

  virtual Status GetChildren(const std::string& dir,
                             std::vector<std::string>* result) override {
    result->clear();
    DIR* d = opendir(dir.c_str());
    if (d == nullptr) {
      switch (errno) {
        case EACCES:
        case ENOENT:
        case ENOTDIR:
          return Status::NotFound();
        default:
          return IOError("While opendir", dir, errno);
      }
    }
    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr) {
      result->push_back(entry->d_name);
    }
    closedir(d);
    return Status::OK();
  }

  virtual Status DeleteFile(const std::string& fname) override {
    Status result;
    if (unlink(fname.c_str()) != 0) {
      result = IOError("while unlink() file", fname, errno);
    }
    return result;
  };

  virtual Status CreateDir(const std::string& name) override {
    Status result;
    if (mkdir(name.c_str(), 0755) != 0) {
      result = IOError("While mkdir", name, errno);
    }
    return result;
  };

  virtual Status CreateDirIfMissing(const std::string& name) override {
    Status result;
    if (mkdir(name.c_str(), 0755) != 0) {
      if (errno != EEXIST) {
        result = IOError("While mkdir if missing", name, errno);
} else if (!DirExists(name)) { //检查那个名字实际上是
//目录。
//消息来自mkdir
        result = Status::IOError("`"+name+"' exists but is not a directory");
      }
    }
    return result;
  };

  virtual Status DeleteDir(const std::string& name) override {
    Status result;
    if (rmdir(name.c_str()) != 0) {
      result = IOError("file rmdir", name, errno);
    }
    return result;
  };

  virtual Status GetFileSize(const std::string& fname,
                             uint64_t* size) override {
    Status s;
    struct stat sbuf;
    if (stat(fname.c_str(), &sbuf) != 0) {
      *size = 0;
      s = IOError("while stat a file for size", fname, errno);
    } else {
      *size = sbuf.st_size;
    }
    return s;
  }

  virtual Status GetFileModificationTime(const std::string& fname,
                                         uint64_t* file_mtime) override {
    struct stat s;
    if (stat(fname.c_str(), &s) !=0) {
      return IOError("while stat a file for modification time", fname, errno);
    }
    *file_mtime = static_cast<uint64_t>(s.st_mtime);
    return Status::OK();
  }
  virtual Status RenameFile(const std::string& src,
                            const std::string& target) override {
    Status result;
    if (rename(src.c_str(), target.c_str()) != 0) {
      result = IOError("While renaming a file to " + target, src, errno);
    }
    return result;
  }

  virtual Status LinkFile(const std::string& src,
                          const std::string& target) override {
    Status result;
    if (link(src.c_str(), target.c_str()) != 0) {
      if (errno == EXDEV) {
        return Status::NotSupported("No cross FS links allowed");
      }
      result = IOError("while link file to " + target, src, errno);
    }
    return result;
  }

  virtual Status LockFile(const std::string& fname, FileLock** lock) override {
    *lock = nullptr;
    Status result;
    int fd;
    {
      IOSTATS_TIMER_GUARD(open_nanos);
      fd = open(fname.c_str(), O_RDWR | O_CREAT, 0644);
    }
    if (fd < 0) {
      result = IOError("while open a file for lock", fname, errno);
    } else if (LockOrUnlock(fname, fd, true) == -1) {
      result = IOError("While lock file", fname, errno);
      close(fd);
    } else {
      SetFD_CLOEXEC(fd, nullptr);
      PosixFileLock* my_lock = new PosixFileLock;
      my_lock->fd_ = fd;
      my_lock->filename = fname;
      *lock = my_lock;
    }
    return result;
  }

  virtual Status UnlockFile(FileLock* lock) override {
    PosixFileLock* my_lock = reinterpret_cast<PosixFileLock*>(lock);
    Status result;
    if (LockOrUnlock(my_lock->filename, my_lock->fd_, false) == -1) {
      result = IOError("unlock", my_lock->filename, errno);
    }
    close(my_lock->fd_);
    delete my_lock;
    return result;
  }

  virtual void Schedule(void (*function)(void* arg1), void* arg,
                        Priority pri = LOW, void* tag = nullptr,
                        void (*unschedFunction)(void* arg) = 0) override;

  virtual int UnSchedule(void* arg, Priority pri) override;

  virtual void StartThread(void (*function)(void* arg), void* arg) override;

  virtual void WaitForJoin() override;

  virtual unsigned int GetThreadPoolQueueLen(Priority pri = LOW) const override;

  virtual Status GetTestDirectory(std::string* result) override {
    const char* env = getenv("TEST_TMPDIR");
    if (env && env[0] != '\0') {
      *result = env;
    } else {
      char buf[100];
      snprintf(buf, sizeof(buf), "/tmp/rocksdbtest-%d", int(geteuid()));
      *result = buf;
    }
//目录可能已经存在
    CreateDir(*result);
    return Status::OK();
  }

  virtual Status GetThreadList(
      std::vector<ThreadStatus>* thread_list) override {
    assert(thread_status_updater_);
    return thread_status_updater_->GetThreadList(thread_list);
  }

  static uint64_t gettid(pthread_t tid) {
    uint64_t thread_id = 0;
    memcpy(&thread_id, &tid, std::min(sizeof(thread_id), sizeof(tid)));
    return thread_id;
  }

  static uint64_t gettid() {
    pthread_t tid = pthread_self();
    return gettid(tid);
  }

  virtual uint64_t GetThreadID() const override {
    return gettid(pthread_self());
  }

  virtual Status NewLogger(const std::string& fname,
                           shared_ptr<Logger>* result) override {
    FILE* f;
    {
      IOSTATS_TIMER_GUARD(open_nanos);
      f = fopen(fname.c_str(), "w");
    }
    if (f == nullptr) {
      result->reset();
      return IOError("when fopen a file for new logger", fname, errno);
    } else {
      int fd = fileno(f);
#ifdef ROCKSDB_FALLOCATE_PRESENT
      fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, 4 * 1024);
#endif
      SetFD_CLOEXEC(fd, nullptr);
      result->reset(new PosixLogger(f, &PosixEnv::gettid, this));
      return Status::OK();
    }
  }

  virtual uint64_t NowMicros() override {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
  }

  virtual uint64_t NowNanos() override {
#if defined(OS_LINUX) || defined(OS_FREEBSD) || defined(OS_AIX)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec;
#elif defined(OS_SOLARIS)
    return gethrtime();
#elif defined(__MACH__)
    clock_serv_t cclock;
    mach_timespec_t ts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &ts);
    mach_port_deallocate(mach_task_self(), cclock);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec;
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
       std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
  }

  virtual void SleepForMicroseconds(int micros) override { usleep(micros); }

  virtual Status GetHostName(char* name, uint64_t len) override {
    int ret = gethostname(name, static_cast<size_t>(len));
    if (ret < 0) {
      if (errno == EFAULT || errno == EINVAL)
        return Status::InvalidArgument(strerror(errno));
      else
        return IOError("GetHostName", name, errno);
    }
    return Status::OK();
  }

  virtual Status GetCurrentTime(int64_t* unix_time) override {
    time_t ret = time(nullptr);
    if (ret == (time_t) -1) {
      return IOError("GetCurrentTime", "", errno);
    }
    *unix_time = (int64_t) ret;
    return Status::OK();
  }

  virtual Status GetAbsolutePath(const std::string& db_path,
                                 std::string* output_path) override {
    if (db_path.find('/') == 0) {
      *output_path = db_path;
      return Status::OK();
    }

    char the_path[256];
    char* ret = getcwd(the_path, 256);
    if (ret == nullptr) {
      return Status::IOError(strerror(errno));
    }

    *output_path = ret;
    return Status::OK();
  }

//允许增加工作线程数。
  virtual void SetBackgroundThreads(int num, Priority pri) override {
    assert(pri >= Priority::BOTTOM && pri <= Priority::HIGH);
    thread_pools_[pri].SetBackgroundThreads(num);
  }

  virtual int GetBackgroundThreads(Priority pri) override {
    assert(pri >= Priority::BOTTOM && pri <= Priority::HIGH);
    return thread_pools_[pri].GetBackgroundThreads();
  }

//允许增加工作线程数。
  virtual void IncBackgroundThreadsIfNeeded(int num, Priority pri) override {
    assert(pri >= Priority::BOTTOM && pri <= Priority::HIGH);
    thread_pools_[pri].IncBackgroundThreadsIfNeeded(num);
  }

  virtual void LowerThreadPoolIOPriority(Priority pool = LOW) override {
    assert(pool >= Priority::BOTTOM && pool <= Priority::HIGH);
#ifdef OS_LINUX
    thread_pools_[pool].LowerIOPriority();
#endif
  }

  virtual std::string TimeToString(uint64_t secondsSince1970) override {
    const time_t seconds = (time_t)secondsSince1970;
    struct tm t;
    int maxsize = 64;
    std::string dummy;
    dummy.reserve(maxsize);
    dummy.resize(maxsize);
    char* p = &dummy[0];
    localtime_r(&seconds, &t);
    snprintf(p, maxsize,
             "%04d/%02d/%02d-%02d:%02d:%02d ",
             t.tm_year + 1900,
             t.tm_mon + 1,
             t.tm_mday,
             t.tm_hour,
             t.tm_min,
             t.tm_sec);
    return dummy;
  }

  EnvOptions OptimizeForLogWrite(const EnvOptions& env_options,
                                 const DBOptions& db_options) const override {
    EnvOptions optimized = env_options;
    optimized.use_mmap_writes = false;
    optimized.use_direct_writes = false;
    optimized.bytes_per_sync = db_options.wal_bytes_per_sync;
//todo（icanadi）如果用keep-size休闲会更快，但是它
//中断TransactionLogiater或TallAtlaStrecord单元测试。固定单元
//测试并使其错误
    optimized.fallocate_with_keep_size = true;
    return optimized;
  }

  EnvOptions OptimizeForManifestWrite(
      const EnvOptions& env_options) const override {
    EnvOptions optimized = env_options;
    optimized.use_mmap_writes = false;
    optimized.use_direct_writes = false;
    optimized.fallocate_with_keep_size = true;
    return optimized;
  }

 private:
  bool checkedDiskForMmap_;
bool forceMmapOff_;  //我们是否覆盖env选项？

//如果指定的目录存在并且是目录，则返回true。
  virtual bool DirExists(const std::string& dname) {
    struct stat statbuf;
    if (stat(dname.c_str(), &statbuf) == 0) {
      return S_ISDIR(statbuf.st_mode);
    }
return false; //stat（）失败返回false
  }

  bool SupportsFastAllocate(const std::string& path) {
#ifdef ROCKSDB_FALLOCATE_PRESENT
    struct statfs s;
    if (statfs(path.c_str(), &s)){
      return false;
    }
    switch (s.f_type) {
      case EXT4_SUPER_MAGIC:
        return true;
      case XFS_SUPER_MAGIC:
        return true;
      case TMPFS_MAGIC:
        return true;
      default:
        return false;
    }
#else
    return false;
#endif
  }

  size_t page_size_;

  std::vector<ThreadPoolImpl> thread_pools_;
  pthread_mutex_t mu_;
  std::vector<pthread_t> threads_to_join_;
};

PosixEnv::PosixEnv()
    : checkedDiskForMmap_(false),
      forceMmapOff_(false),
      page_size_(getpagesize()),
      thread_pools_(Priority::TOTAL) {
  ThreadPoolImpl::PthreadCall("mutex_init", pthread_mutex_init(&mu_, nullptr));
  for (int pool_id = 0; pool_id < Env::Priority::TOTAL; ++pool_id) {
    thread_pools_[pool_id].SetThreadPriority(
        static_cast<Env::Priority>(pool_id));
//这允许以后初始化每个线程的线程本地环境。
    thread_pools_[pool_id].SetHostEnv(this);
  }
  thread_status_updater_ = CreateThreadStatusUpdater();
}

void PosixEnv::Schedule(void (*function)(void* arg1), void* arg, Priority pri,
                        void* tag, void (*unschedFunction)(void* arg)) {
  assert(pri >= Priority::BOTTOM && pri <= Priority::HIGH);
  thread_pools_[pri].Schedule(function, arg, tag, unschedFunction);
}

int PosixEnv::UnSchedule(void* arg, Priority pri) {
  return thread_pools_[pri].UnSchedule(arg);
}

unsigned int PosixEnv::GetThreadPoolQueueLen(Priority pri) const {
  assert(pri >= Priority::BOTTOM && pri <= Priority::HIGH);
  return thread_pools_[pri].GetQueueLen();
}

struct StartThreadState {
  void (*user_function)(void*);
  void* arg;
};

static void* StartThreadWrapper(void* arg) {
  StartThreadState* state = reinterpret_cast<StartThreadState*>(arg);
  state->user_function(state->arg);
  delete state;
  return nullptr;
}

void PosixEnv::StartThread(void (*function)(void* arg), void* arg) {
  pthread_t t;
  StartThreadState* state = new StartThreadState;
  state->user_function = function;
  state->arg = arg;
  ThreadPoolImpl::PthreadCall(
      "start thread", pthread_create(&t, nullptr, &StartThreadWrapper, state));
  ThreadPoolImpl::PthreadCall("lock", pthread_mutex_lock(&mu_));
  threads_to_join_.push_back(t);
  ThreadPoolImpl::PthreadCall("unlock", pthread_mutex_unlock(&mu_));
}

void PosixEnv::WaitForJoin() {
  for (const auto tid : threads_to_join_) {
    pthread_join(tid, nullptr);
  }
  threads_to_join_.clear();
}

}  //命名空间

std::string Env::GenerateUniqueId() {
  std::string uuid_file = "/proc/sys/kernel/random/uuid";

  Status s = FileExists(uuid_file);
  if (s.ok()) {
    std::string uuid;
    s = ReadFileToString(this, uuid_file, &uuid);
    if (s.ok()) {
      return uuid;
    }
  }
//无法读取uuid_文件-使用“nanos random”生成uuid
  Random64 r(time(nullptr));
  uint64_t random_uuid_portion =
    r.Uniform(std::numeric_limits<uint64_t>::max());
  uint64_t nanos_uuid_portion = NowNanos();
  char uuid2[200];
  snprintf(uuid2,
           200,
           "%lx-%lx",
           (unsigned long)nanos_uuid_portion,
           (unsigned long)random_uuid_portion);
  return uuid2;
}

//
//默认posix env
//
Env* Env::Default() {
//以下函数调用初始化threadlocalptr的单例
//在静态默认环境之前。这保证了违约
//总是在threadlocalptr单例得到之前被销毁
//作为C++破坏，保证静态变量的破坏。
//与它们的构造顺序相反。
//
//因为静态成员是按相反的顺序销毁的
//他们的结构，在这里打电话保证
//静态posixenv的析构函数将首先执行，然后执行
//threadlocalptr的单例。
  ThreadLocalPtr::InitSingletons();
  static PosixEnv default_env;
  return &default_env;
}

}  //命名空间rocksdb
