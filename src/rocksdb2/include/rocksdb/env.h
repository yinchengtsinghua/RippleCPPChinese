
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//env是rocksdb实现用于访问的接口
//操作系统功能，如文件系统等调用程序
//打开数据库时可能希望提供自定义env对象
//获得精细的增益控制；例如，速率限制文件系统操作。
//
//所有env实现对来自的并发访问都是安全的。
//多线程，没有任何外部同步。

#ifndef STORAGE_ROCKSDB_INCLUDE_ENV_H_
#define STORAGE_ROCKSDB_INCLUDE_ENV_H_

#include <stdint.h>
#include <cstdarg>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include "rocksdb/status.h"
#include "rocksdb/thread_status.h"

#ifdef _WIN32
//Windows API宏干扰
#undef DeleteFile
#undef GetCurrentTime
#endif

namespace rocksdb {

class FileLock;
class Logger;
class RandomAccessFile;
class SequentialFile;
class Slice;
class WritableFile;
class RandomRWFile;
class Directory;
struct DBOptions;
struct ImmutableDBOptions;
class RateLimiter;
class ThreadStatusUpdater;
struct ThreadStatus;

using std::unique_ptr;
using std::shared_ptr;

const size_t kDefaultPageSize = 4 * 1024;

//打开要读/写的文件时的选项
struct EnvOptions {

//使用默认选项构造
  EnvOptions();

//从选项构造
  explicit EnvOptions(const DBOptions& options);

//如果为真，则使用mmap读取数据
  bool use_mmap_reads = false;

//如果为真，则使用mmap写入数据
  bool use_mmap_writes = true;

//如果为真，则使用o_direct读取数据
  bool use_direct_reads = false;

//如果为真，则使用o_direct写入数据
  bool use_direct_writes = false;

//如果为false，则忽略fallocate（）调用
  bool allow_fallocate = true;

//如果为true，则将fd_cloexec设置为open fd。
  bool set_fd_cloexec = true;

//允许操作系统在文件处于
//写，在背景中。对每个同步字节发出一个请求
//书面的。0关掉它。
//默认值：0
  uint64_t bytes_per_sync = 0;

//如果为真，我们将使用falloc_fl_keep_size标志预分配文件，该标志
//意味着文件大小不会作为预分配的一部分更改。
//如果为false，预分配还将更改文件大小。此选项将
//提高工作负载的性能，在工作负载中，您可以在
//写。默认情况下，对于清单写入，我们将其设置为true，对于
//沃尔写作
  bool fallocate_with_keep_size = true;

//参见dboptions文档
  size_t compaction_readahead_size;

//参见dboptions文档
  size_t random_access_max_buffer_size;

//参见dboptions文档
  size_t writable_file_max_buffer_size = 1024 * 1024;

//如果不是nullptr，则为刷新和压缩启用写入速率限制
  RateLimiter* rate_limiter = nullptr;
};

class Env {
 public:
  struct FileAttributes {
//文件名
    std::string name;

//文件大小（字节）
    uint64_t size_bytes;
  };

  Env() : thread_status_updater_(nullptr) {}

  virtual ~Env();

//返回适合当前操作的默认环境
//系统。成熟的用户可能希望提供自己的环境
//实现而不是依赖于此默认环境。
//
//默认（）的结果属于rocksdb，不能删除。
  static Env* Default();

//用指定的名称创建一个全新的顺序可读文件。
//成功后，将指向新文件的指针存储在*result中，并返回OK。
//失败时将nullptr存储在*result中并返回non ok。如果文件有
//不存在，返回非OK状态。
//
//返回的文件一次只能由一个线程访问。
  virtual Status NewSequentialFile(const std::string& fname,
                                   unique_ptr<SequentialFile>* result,
                                   const EnvOptions& options)
                                   = 0;

//创建一个全新的随机访问只读文件
//指定的名称。成功后，将指向新文件的指针存储在
//*结果并返回OK。失败时将nullptr存储在*result和
//返回非OK。如果文件不存在，则返回一个非OK
//状态。
//
//多个线程可以同时访问返回的文件。
  virtual Status NewRandomAccessFile(const std::string& fname,
                                     unique_ptr<RandomAccessFile>* result,
                                     const EnvOptions& options)
                                     = 0;

//创建一个对象，该对象使用指定的
//姓名。删除具有相同名称的任何现有文件并创建
//新文件。成功后，将指向新文件的指针存储在
//*结果并返回OK。失败时将nullptr存储在*result和
//返回非OK。
//
//返回的文件一次只能由一个线程访问。
  virtual Status NewWritableFile(const std::string& fname,
                                 unique_ptr<WritableFile>* result,
                                 const EnvOptions& options) = 0;

//创建一个对象，该对象使用指定的
//姓名。删除具有相同名称的任何现有文件并创建
//新文件。成功后，将指向新文件的指针存储在
//*结果并返回OK。失败时将nullptr存储在*result和
//返回非OK。
//
//返回的文件一次只能由一个线程访问。
  virtual Status ReopenWritableFile(const std::string& fname,
                                    unique_ptr<WritableFile>* result,
                                    const EnvOptions& options) {
    return Status::NotSupported();
  }

//通过重命名现有文件并将其打开为可写文件来重用该文件。
  virtual Status ReuseWritableFile(const std::string& fname,
                                   const std::string& old_fname,
                                   unique_ptr<WritableFile>* result,
                                   const EnvOptions& options);

//如果文件不存在，则打开“fname”进行随机读写
//将被创建。成功后，将指向新文件的指针存储在
//*结果并返回OK。失败时返回不正常。
//
//返回的文件一次只能由一个线程访问。
  virtual Status NewRandomRWFile(const std::string& fname,
                                 unique_ptr<RandomRWFile>* result,
                                 const EnvOptions& options) {
    return Status::NotSupported("RandomRWFile is not implemented in this Env");
  }

//创建表示目录的对象。如果目录
//不存在。如果目录存在，它将打开目录
//并创建一个新的目录对象。
//
//成功后，将指向新目录的指针存储在
//*结果并返回OK。失败时将nullptr存储在*result和
//返回非OK。
  virtual Status NewDirectory(const std::string& name,
                              unique_ptr<Directory>* result) = 0;

//如果命名文件存在，则返回OK。
//如果命名文件不存在，则找不到，
//调用进程没有权限确定
//此文件是否存在，或者路径是否无效。
//如果遇到IO错误，则为IOERROR
  virtual Status FileExists(const std::string& fname) = 0;

//将指定目录的子目录名存储在*result中。
//这些名称是相对于“dir”的。
//*结果的原始内容将被删除。
//如果“dir”存在且“*result”包含其子级，则返回OK。
//NotFound如果“dir”不存在，则调用进程没有
//访问“dir”的权限，或者如果“dir”无效。
//如果遇到IO错误，则为IOERROR
  virtual Status GetChildren(const std::string& dir,
                             std::vector<std::string>* result) = 0;

//将指定目录的子目录的属性存储在*result中。
//如果实现在迭代文件之前列出了目录
//同时删除文件，删除的文件将从
//结果。
//名称属性是相对于“dir”的。
//*结果的原始内容将被删除。
//如果“dir”存在且“*result”包含其子级，则返回OK。
//NotFound如果“dir”不存在，则调用进程没有
//访问“dir”的权限，或者如果“dir”无效。
//如果遇到IO错误，则为IOERROR
  virtual Status GetChildrenFileAttributes(const std::string& dir,
                                           std::vector<FileAttributes>* result);

//删除命名文件。
  virtual Status DeleteFile(const std::string& fname) = 0;

//创建指定的目录。如果目录存在，则返回错误。
  virtual Status CreateDir(const std::string& dirname) = 0;

//如果缺少，则创建目录。如果存在或成功
//创建。
  virtual Status CreateDirIfMissing(const std::string& dirname) = 0;

//删除指定的目录。
  virtual Status DeleteDir(const std::string& dirname) = 0;

//将fname的大小存储在*文件\大小中。
  virtual Status GetFileSize(const std::string& fname, uint64_t* file_size) = 0;

//将fname的最后修改时间存储在*文件时间中。
  virtual Status GetFileModificationTime(const std::string& fname,
                                         uint64_t* file_mtime) = 0;
//将文件src重命名为target。
  virtual Status RenameFile(const std::string& src,
                            const std::string& target) = 0;

//硬链接文件SRC到目标。
  virtual Status LinkFile(const std::string& src, const std::string& target) {
    return Status::NotSupported("LinkFile is not supported for this Env");
  }

//锁定指定的文件。用于防止同时访问
//通过多个进程访问同一数据库。失败时，将nullptr存储在
//*锁定并返回非OK。
//
//成功时，存储指向表示
//获得锁定*锁定并返回OK。打电话的人应该打电话
//解锁文件（*lock）以释放锁。如果进程退出，
//锁将自动释放。
//
//如果有人已经锁上了锁，立即结束
//失败了。即，此调用不等待现有锁
//走开。
//
//如果命名文件不存在，则可以创建它。
  virtual Status LockFile(const std::string& fname, FileLock** lock) = 0;

//释放先前成功调用lockfile获得的锁。
//要求：成功的lockfile（）调用返回了lock
//要求：锁尚未解锁。
  virtual Status UnlockFile(FileLock* lock) = 0;

//线程池中计划作业的优先级
  enum Priority { BOTTOM, LOW, HIGH, TOTAL };

//在速率限制器计划程序中请求字节的优先级
  enum IOPriority {
    IO_LOW = 0,
    IO_HIGH = 1,
    IO_TOTAL = 2
  };

//安排在后台线程中运行一次“（*function）（arg）”，在
//由pri指定的线程池。默认情况下，作业将变为“低”
//优先级线程池。

//“函数”可以在未指定的线程中运行。多功能
//添加到同一个env可以在不同的线程中并发运行。
//也就是说，调用者不能假定后台工作项是
//序列化。
//调用非计划函数时，非计划函数
//在调度时注册的是以arg作为参数调用的。
  virtual void Schedule(void (*function)(void* arg), void* arg,
                        Priority pri = LOW, void* tag = nullptr,
                        void (*unschedFunction)(void* arg) = 0) = 0;

//安排从队列中删除给定arg的作业（如果不是）
//已经安排好了。调用方应在arg上具有独占锁。
  virtual int UnSchedule(void* arg, Priority pri) { return 0; }

//启动一个新线程，在新线程中调用“函数（arg）”。
//当“函数（arg）”返回时，线程将被破坏。
  virtual void StartThread(void (*function)(void* arg), void* arg) = 0;

//等待StartThread启动的所有线程终止。
  virtual void WaitForJoin() {}

//获取特定线程池的线程池队列长度。
  virtual unsigned int GetThreadPoolQueueLen(Priority pri = LOW) const {
    return 0;
  }

//*路径设置为可用于测试的临时目录。它可能
//或者有许多还没有被创造出来。目录可能不同，也可能不同
//在同一进程的运行之间，但随后的调用将返回
//相同的目录。
  virtual Status GetTestDirectory(std::string* path) = 0;

//创建并返回用于存储信息消息的日志文件。
  virtual Status NewLogger(const std::string& fname,
                           shared_ptr<Logger>* result) = 0;

//返回自某个固定时间点以来的微秒数。
//它通常被用作系统时间，例如在GenericRatelimiter中。
//以及其他地方，因此端口需要返回系统时间才能工作。
  virtual uint64_t NowMicros() = 0;

//返回自某个固定时间点以来的纳秒数。只有
//用于计算一次运行中的时间增量。
//默认实现只依赖于nowmicros。
//在平台特定的实现中，nownanos（）应该返回时间点
//那是单调的。
  virtual uint64_t NowNanos() {
    return NowMicros() * 1000;
  }

//休眠/延迟线程数微秒。
  virtual void SleepForMicroseconds(int micros) = 0;

//获取当前主机名。
  virtual Status GetHostName(char* name, uint64_t len) = 0;

//获取自1970-01-01 00:00:00（UTC）以来的秒数。
//成功时仅覆盖*Unix时间。
  virtual Status GetCurrentTime(int64_t* unix_time) = 0;

//获取此数据库的完整目录名。
  virtual Status GetAbsolutePath(const std::string& db_path,
      std::string* output_path) = 0;

//特定线程池的后台工作线程数
//对于这种环境。”“低”是默认池。
//默认数字：1
  virtual void SetBackgroundThreads(int number, Priority pri = LOW) = 0;
  virtual int GetBackgroundThreads(Priority pri = LOW) = 0;

//扩大特定线程池的后台工作线程数
//对于此环境，如果它小于指定值。”“低”是默认值
//池。
  virtual void IncBackgroundThreadsIfNeeded(int number, Priority pri) = 0;

//降低指定池中线程的IO优先级。
  virtual void LowerThreadPoolIOPriority(Priority pool = LOW) {}

//将自1970年1月1日起的秒数转换为可打印字符串
  virtual std::string TimeToString(uint64_t time) = 0;

//生成可用于标识数据库的唯一ID
  virtual std::string GenerateUniqueId();

//OptimizeForLogWrite将创建一个新的envOptions对象，该对象是
//参数中的envOptions，但已针对读取日志文件进行了优化。
  virtual EnvOptions OptimizeForLogRead(const EnvOptions& env_options) const;

//optimizerManifestLead将创建一个新的envOptions对象，该对象是一个副本
//参数中的envOptions，但为读取清单而优化
//文件夹。
  virtual EnvOptions OptimizeForManifestRead(
      const EnvOptions& env_options) const;

//OptimizeForLogWrite将创建一个新的envOptions对象，该对象是
//参数中的envOptions，但已针对写入日志文件进行了优化。
//默认实现返回同一对象的副本。
  virtual EnvOptions OptimizeForLogWrite(const EnvOptions& env_options,
                                         const DBOptions& db_options) const;
//optimizeformanifestwrite将创建一个新的envOptions对象，该对象是一个副本
//参数中的envOptions，但为写入清单而优化
//文件夹。默认实现返回同一对象的副本。
  virtual EnvOptions OptimizeForManifestWrite(
      const EnvOptions& env_options) const;

//optimizefcompactionTableWrite将创建一个新的envOptions对象，该对象是
//参数中的envOptions的副本，但已为写入而优化
//表文件。
  virtual EnvOptions OptimizeForCompactionTableWrite(
      const EnvOptions& env_options,
      const ImmutableDBOptions& db_options) const;

//optimizeforCompactionTableWrite将创建一个新的envOptions对象，该对象
//是参数中envOptions的副本，但已为读取而优化
//表文件。
  virtual EnvOptions OptimizeForCompactionTableRead(
      const EnvOptions& env_options,
      const ImmutableDBOptions& db_options) const;

//返回属于当前env的所有线程的状态。
  virtual Status GetThreadList(std::vector<ThreadStatus>* thread_list) {
    return Status::NotSupported("Not supported.");
  }

//返回指向ThreadStatesUpdater的指针。此功能将
//在RockSDB内部用于更新螺纹状态和支架
//GetThreadList（）。
  virtual ThreadStatusUpdater* GetThreadStatusUpdater() const {
    return thread_status_updater_;
  }

//返回当前线程的ID。
  virtual uint64_t GetThreadID() const;

 protected:
//指向将更新
//每个线程的状态。
  ThreadStatusUpdater* thread_status_updater_;

 private:
//不允许复制
  Env(const Env&);
  void operator=(const Env&);
};

//工厂函数来构造ThreadStatesUpdater。任何Env
//支持getthreadlist（）功能的应在其
//初始化线程\状态\更新程序\的构造函数。
ThreadStatusUpdater* CreateThreadStatusUpdater();

//用于按顺序读取文件的文件抽象
class SequentialFile {
 public:
  SequentialFile() { }
  virtual ~SequentialFile();

//从文件中读取最多“n”个字节。划痕[0..n-1]“可能是
//由这个程序编写。将“*result”设置为
//读取（包括成功读取少于“n”个字节）。
//可以将“*result”设置为指向“scratch[0..n-1]”中的数据，因此
//使用“*result”时，“scratch[0..n-1]”必须有效。
//如果遇到错误，则返回非OK状态。
//
//要求：外部同步
  virtual Status Read(size_t n, Slice* result, char* scratch) = 0;

//跳过文件中的“n”字节。这肯定不是
//读取相同数据的速度较慢，但可能更快。
//
//如果到达文件结尾，跳过将在
//文件，跳过将返回OK。
//
//要求：外部同步
  virtual Status Skip(uint64_t n) = 0;

//指示当前序列文件实现的上层
//使用直接IO。
  virtual bool use_direct_io() const { return false; }

//使用返回的对齐值分配
//用于直接I/O的对齐缓冲区
  virtual size_t GetRequiredBufferAlignment() const { return kDefaultPageSize; }

//删除从偏移量到偏移量+长度的任何类型的数据缓存
//这个文件。如果长度为0，则表示文件结尾。
//如果系统没有缓存文件内容，那么这是一个noop。
  virtual Status InvalidateCache(size_t offset, size_t length) {
    return Status::NotSupported("InvalidateCache not supported.");
  }

//用于直接I/O的定位读取
//如果启用直接I/O，则偏移量、n和划痕应正确对齐。
  virtual Status PositionedRead(uint64_t offset, size_t n, Slice* result,
                                char* scratch) {
    return Status::NotSupported();
  }
};

//随机读取文件内容的文件抽象。
class RandomAccessFile {
 public:

  RandomAccessFile() { }
  virtual ~RandomAccessFile();

//从文件中读取最多“n”个字节，从“offset”开始。
//“scratch[0..n-1]”可由该例程写入。设置“*结果”
//读取的数据（包括如果小于“n”字节
//读取成功）。可以将“*result”设置为指向
//“scratch[0..n-1]”，因此“scratch[0..n-1]”必须在
//使用“*结果”。如果遇到错误，返回一个非OK
//状态。
//
//多线程并发使用安全。
//如果启用直接I/O，则偏移量、n和划痕应正确对齐。
  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const = 0;

//将文件从偏移量n字节开始预读以进行缓存。
  virtual Status Prefetch(uint64_t offset, size_t n) {
    return Status::OK();
  }

//尝试为此文件获取每次相同的唯一ID
//打开文件（打开文件时保持不变）。
//此外，它还试图使这个ID最大为“最大\大小”字节。如果这样的话
//可以创建ID。此函数返回ID的长度并将其放置
//在“id”中；否则，此函数返回0，在这种情况下，“id”
//可能尚未修改。
//
//对于给定环境中的ID，此函数保证两个唯一的ID
//不能通过向其中一个添加任意字节使彼此相等
//他们。也就是说，没有唯一的ID是另一个的前缀。
//
//此函数保证返回的ID不能解释为
//单个变量。
//
//注意：这些ID仅在进程期间有效。
  virtual size_t GetUniqueId(char* id, size_t max_size) const {
return 0; //默认实现以防止出现向后问题
//兼容性。
  };

  enum AccessPattern { NORMAL, RANDOM, SEQUENTIAL, WILLNEED, DONTNEED };

  virtual void Hint(AccessPattern pattern) {}

//指示当前RandomAccessFile实现的上层
//使用直接IO。
  virtual bool use_direct_io() const { return false; }

//使用返回的对齐值分配
//用于直接I/O的对齐缓冲区
  virtual size_t GetRequiredBufferAlignment() const { return kDefaultPageSize; }

//删除从偏移量到偏移量+长度的任何类型的数据缓存
//这个文件。如果长度为0，则表示文件结尾。
//如果系统没有缓存文件内容，那么这是一个noop。
  virtual Status InvalidateCache(size_t offset, size_t length) {
    return Status::NotSupported("InvalidateCache not supported.");
  }
};

//用于顺序写入的文件抽象。实施
//必须提供缓冲，因为调用方可能附加小片段
//一次到文件。
class WritableFile {
 public:
  WritableFile()
    : last_preallocated_block_(0),
      preallocation_block_size_(0),
      io_priority_(Env::IO_TOTAL) {
  }
  virtual ~WritableFile();

//将数据追加到文件结尾
//注意：WriteAbelFile对象必须支持Append或
//位置已更改，因此用户不能将两者混合。
  virtual Status Append(const Slice& data) = 0;

//将位置数据应用到指定的偏移量。追加后的新EOF
//必须大于上一个EOF。这将在写入
//没有操作系统缓冲区支持，因此必须始终从
//扇区。因此，实现还需要重写最后一个
//部分扇区。
//注意：positionAppend不保证在
//写。可写文件对象必须支持append或
//位置已更改，因此用户不能将两者混合。
//
//positionedAppPend（）只能出现在页面/扇区边界上。为此
//原因，如果最后一次写入是一个不完整的扇区，我们仍然需要倒带
//回到最近的扇区/页面，用任何
//我们需要补充。我们需要保持在我们停止写作的地方。
//
//positionedAppend（）只能写入整个扇区。因此我们必须
//最后一次写入时用零填充，关闭时根据
//我们在上一步中保持的位置。
//
//positionedAppend（）要求传入对齐的缓冲区。准直
//通过getRequiredBufferAlignment（）查询Required
  /*实际状态位置应用（const slice&/*data*/，uint64_t/*offset*/）
    返回状态：：NotSupported（）；
  }

  //需要截断才能将文件修剪为正确的大小
  //关闭前。不可能总是跟踪文件
  //由于整个页面写入而导致的大小。如果调用，则行为未定义
  //后面还有其他写操作。
  虚拟状态截断（uint64_t size）
    返回状态：：OK（）；
  }
  虚拟状态close（）=0；
  虚拟状态flush（）=0；
  虚拟状态同步（）=0；//同步数据

  /*
   *同步数据和/或元数据。
   *默认情况下，仅同步数据。
   *在需要同步的环境中重写此方法
   *元数据。
   **/

  virtual Status Fsync() {
    return Sync();
  }

//如果sync（）和fsync（）可以安全地与append（）同时调用，则为true。
//和FrHuSE（）。
  virtual bool IsSyncThreadSafe() const {
    return false;
  }

//指示当前可写文件实现的上层
//使用直接IO。
  virtual bool use_direct_io() const { return false; }

//使用返回的对齐值分配
//用于直接I/O的对齐缓冲区
  virtual size_t GetRequiredBufferAlignment() const { return kDefaultPageSize; }
  /*
   *如果启用速率限制，则更改速率限制中的优先级。
   *如果未启用速率限制，则此调用无效。
   **/

  virtual void SetIOPriority(Env::IOPriority pri) {
    io_priority_ = pri;
  }

  virtual Env::IOPriority GetIOPriority() { return io_priority_; }

  /*
   *获取文件中有效数据的大小。
   **/

  virtual uint64_t GetFileSize() {
    return 0;
  }

  /*
   *获取并设置写入的默认预分配块大小
   *此文件。如果非零，则将使用allocate扩展
   *如果env
   *实例支持。
   **/

  virtual void SetPreallocationBlockSize(size_t size) {
    preallocation_block_size_ = size;
  }

  virtual void GetPreallocationStatus(size_t* block_size,
                                      size_t* last_allocated_block) {
    *last_allocated_block = last_preallocated_block_;
    *block_size = preallocation_block_size_;
  }

//有关文档，请参阅RandomAccessFile:：GetUniqueID（）。
  virtual size_t GetUniqueId(char* id, size_t max_size) const {
return 0; //默认实现以防止出现向后问题
  }

//删除从偏移量到偏移量+长度的任何类型的数据缓存
//这个文件。如果长度为0，则表示文件结尾。
//如果系统没有缓存文件内容，那么这是一个noop。
//此调用对缓存中的脏页没有影响。
  virtual Status InvalidateCache(size_t offset, size_t length) {
    return Status::NotSupported("InvalidateCache not supported.");
  }

//将文件范围与磁盘同步。
//偏移量是要同步的文件范围的起始字节。
//nbytes指定要同步的范围的长度。
//这要求操作系统启动将缓存数据刷新到磁盘，
//不等待完成。
//默认实现什么也不做。
  virtual Status RangeSync(uint64_t offset, uint64_t nbytes) { return Status::OK(); }

//PrepareWrite执行写入的任何必要准备
//在写入实际发生之前。这允许预先分配
//设备上的空间可以减少文件
//由于过分狂热的文件系统而导致的碎片和/或更少的浪费
//预分配。
  virtual void PrepareWrite(size_t offset, size_t len) {
    if (preallocation_block_size_ == 0) {
      return;
    }
//如果此写入将跨越一个或多个预分配块，
//确定最后一个预分配块需要
//写下这篇文章并分配到这一点。
    const auto block_size = preallocation_block_size_;
    size_t new_last_preallocated_block =
      (offset + len + block_size - 1) / block_size;
    if (new_last_preallocated_block > last_preallocated_block_) {
      size_t num_spanned_blocks =
        new_last_preallocated_block - last_preallocated_block_;
      Allocate(block_size * last_preallocated_block_,
               block_size * num_spanned_blocks);
      last_preallocated_block_ = new_last_preallocated_block;
    }
  }

//预先为文件分配空间。
  virtual Status Allocate(uint64_t offset, uint64_t len) {
    return Status::OK();
  }

 protected:
  size_t preallocation_block_size() { return preallocation_block_size_; }

 private:
  size_t last_preallocated_block_;
  size_t preallocation_block_size_;
//不允许复制
  WritableFile(const WritableFile&);
  void operator=(const WritableFile&);

 protected:
  friend class WritableFileWrapper;
  friend class WritableFileMirror;

  Env::IOPriority io_priority_;
};

//用于随机读写的文件抽象。
class RandomRWFile {
 public:
  RandomRWFile() {}
  virtual ~RandomRWFile() {}

//指示类是否使用直接I/O
//如果为false，则必须将对齐的缓冲区传递给write（）。
  virtual bool use_direct_io() const { return false; }

//使用返回的对齐值分配
//用于直接I/O的对齐缓冲区
  virtual size_t GetRequiredBufferAlignment() const { return kDefaultPageSize; }

//在“offset”处的“data”中写入字节，成功时返回status:：ok（）。
//当使用_Direct_IO（）返回true时，传递对齐的缓冲区。
  virtual Status Write(uint64_t offset, const Slice& data) = 0;

//从offset'offset'开始读取最多'n'个字节，并将它们存储在
//结果，提供的“scratch”大小至少应为“n”。
//返回成功时的状态：：OK（）。
  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const = 0;

  virtual Status Flush() = 0;

  virtual Status Sync() = 0;

  virtual Status Fsync() { return Sync(); }

  virtual Status Close() = 0;

//不允许复制
  RandomRWFile(const RandomRWFile&) = delete;
  RandomRWFile& operator=(const RandomRWFile&) = delete;
};

//Directory对象表示文件和实现的集合
//可以在目录上执行的文件系统操作。
class Directory {
 public:
  virtual ~Directory() {}
//fsync目录。可以从多个线程并发调用。
  virtual Status Fsync() = 0;
};

enum InfoLogLevel : unsigned char {
  DEBUG_LEVEL = 0,
  INFO_LEVEL,
  WARN_LEVEL,
  ERROR_LEVEL,
  FATAL_LEVEL,
  HEADER_LEVEL,
  NUM_INFO_LOG_LEVELS,
};

//用于写入日志消息的接口。
class Logger {
 public:
  size_t kDoNotSupportGetLogFileSize = (std::numeric_limits<size_t>::max)();

  explicit Logger(const InfoLogLevel log_level = InfoLogLevel::INFO_LEVEL)
      : log_level_(log_level) {}
  virtual ~Logger();

//用指定的格式将头写入日志文件
//建议您在
//应用。但这并不是强制执行的。
  virtual void LogHeader(const char* format, va_list ap) {
//默认实现执行简单的信息级日志写入。
//请根据记录器类要求重写。
    Logv(format, ap);
  }

//用指定的格式将条目写入日志文件。
  virtual void Logv(const char* format, va_list ap) = 0;

//使用指定的日志级别将条目写入日志文件
//格式。任何级别低于内部日志级别的日志
//其中*个（请参见@setinfologlevel和@getinfologlevel）将不会
//印刷的。
  virtual void Logv(const InfoLogLevel log_level, const char* format, va_list ap);

  virtual size_t GetLogFileSize() const { return kDoNotSupportGetLogFileSize; }
//刷新到操作系统缓冲区
  virtual void Flush() {}
  virtual InfoLogLevel GetInfoLogLevel() const { return log_level_; }
  virtual void SetInfoLogLevel(const InfoLogLevel log_level) {
    log_level_ = log_level;
  }

 private:
//不允许复制
  Logger(const Logger&);
  void operator=(const Logger&);
  InfoLogLevel log_level_;
};


//标识锁定的文件。
class FileLock {
 public:
  FileLock() { }
  virtual ~FileLock();
 private:
//不允许复制
  FileLock(const FileLock&);
  void operator=(const FileLock&);
};

extern void LogFlush(const shared_ptr<Logger>& info_log);

extern void Log(const InfoLogLevel log_level,
                const shared_ptr<Logger>& info_log, const char* format, ...);

//一组具有不同日志级别的日志函数。
extern void Header(const shared_ptr<Logger>& info_log, const char* format, ...);
extern void Debug(const shared_ptr<Logger>& info_log, const char* format, ...);
extern void Info(const shared_ptr<Logger>& info_log, const char* format, ...);
extern void Warn(const shared_ptr<Logger>& info_log, const char* format, ...);
extern void Error(const shared_ptr<Logger>& info_log, const char* format, ...);
extern void Fatal(const shared_ptr<Logger>& info_log, const char* format, ...);

//如果info_log不是nullptr，则将指定的数据记录到*info_log。
//默认的信息日志级别是信息日志级别：：信息级别。
extern void Log(const shared_ptr<Logger>& info_log, const char* format, ...)
#   if defined(__GNUC__) || defined(__clang__)
    __attribute__((__format__ (__printf__, 2, 3)))
#   endif
    ;

extern void LogFlush(Logger *info_log);

extern void Log(const InfoLogLevel log_level, Logger* info_log,
                const char* format, ...);

//默认的信息日志级别是信息日志级别：：信息级别。
extern void Log(Logger* info_log, const char* format, ...)
#   if defined(__GNUC__) || defined(__clang__)
    __attribute__((__format__ (__printf__, 2, 3)))
#   endif
    ;

//一组具有不同日志级别的日志函数。
extern void Header(Logger* info_log, const char* format, ...);
extern void Debug(Logger* info_log, const char* format, ...);
extern void Info(Logger* info_log, const char* format, ...);
extern void Warn(Logger* info_log, const char* format, ...);
extern void Error(Logger* info_log, const char* format, ...);
extern void Fatal(Logger* info_log, const char* format, ...);

//实用程序例程：将“数据”写入命名文件。
extern Status WriteStringToFile(Env* env, const Slice& data,
                                const std::string& fname,
                                bool should_sync = false);

//实用程序例程：将命名文件的内容读取到*数据中
extern Status ReadFileToString(Env* env, const std::string& fname,
                               std::string* data);

//将所有调用转发给另一个env的env实现。
//对于希望覆盖
//另一个环境的功能。
class EnvWrapper : public Env {
 public:
//初始化将所有调用委托给*T的envWrapper
  explicit EnvWrapper(Env* t) : target_(t) { }
  ~EnvWrapper() override;

//返回此env将所有调用转发到的目标
  Env* target() const { return target_; }

//下面的文本是将所有方法转发到target（）的样板文件。
  Status NewSequentialFile(const std::string& f, unique_ptr<SequentialFile>* r,
                           const EnvOptions& options) override {
    return target_->NewSequentialFile(f, r, options);
  }
  Status NewRandomAccessFile(const std::string& f,
                             unique_ptr<RandomAccessFile>* r,
                             const EnvOptions& options) override {
    return target_->NewRandomAccessFile(f, r, options);
  }
  Status NewWritableFile(const std::string& f, unique_ptr<WritableFile>* r,
                         const EnvOptions& options) override {
    return target_->NewWritableFile(f, r, options);
  }
  Status ReopenWritableFile(const std::string& fname,
                            unique_ptr<WritableFile>* result,
                            const EnvOptions& options) override {
    return target_->ReopenWritableFile(fname, result, options);
  }
  Status ReuseWritableFile(const std::string& fname,
                           const std::string& old_fname,
                           unique_ptr<WritableFile>* r,
                           const EnvOptions& options) override {
    return target_->ReuseWritableFile(fname, old_fname, r, options);
  }
  Status NewRandomRWFile(const std::string& fname,
                         unique_ptr<RandomRWFile>* result,
                         const EnvOptions& options) override {
    return target_->NewRandomRWFile(fname, result, options);
  }
  Status NewDirectory(const std::string& name,
                      unique_ptr<Directory>* result) override {
    return target_->NewDirectory(name, result);
  }
  Status FileExists(const std::string& f) override {
    return target_->FileExists(f);
  }
  Status GetChildren(const std::string& dir,
                     std::vector<std::string>* r) override {
    return target_->GetChildren(dir, r);
  }
  Status GetChildrenFileAttributes(
      const std::string& dir, std::vector<FileAttributes>* result) override {
    return target_->GetChildrenFileAttributes(dir, result);
  }
  Status DeleteFile(const std::string& f) override {
    return target_->DeleteFile(f);
  }
  Status CreateDir(const std::string& d) override {
    return target_->CreateDir(d);
  }
  Status CreateDirIfMissing(const std::string& d) override {
    return target_->CreateDirIfMissing(d);
  }
  Status DeleteDir(const std::string& d) override {
    return target_->DeleteDir(d);
  }
  Status GetFileSize(const std::string& f, uint64_t* s) override {
    return target_->GetFileSize(f, s);
  }

  Status GetFileModificationTime(const std::string& fname,
                                 uint64_t* file_mtime) override {
    return target_->GetFileModificationTime(fname, file_mtime);
  }

  Status RenameFile(const std::string& s, const std::string& t) override {
    return target_->RenameFile(s, t);
  }

  Status LinkFile(const std::string& s, const std::string& t) override {
    return target_->LinkFile(s, t);
  }

  Status LockFile(const std::string& f, FileLock** l) override {
    return target_->LockFile(f, l);
  }

  Status UnlockFile(FileLock* l) override { return target_->UnlockFile(l); }

  void Schedule(void (*f)(void* arg), void* a, Priority pri,
                void* tag = nullptr, void (*u)(void* arg) = 0) override {
    return target_->Schedule(f, a, pri, tag, u);
  }

  int UnSchedule(void* tag, Priority pri) override {
    return target_->UnSchedule(tag, pri);
  }

  void StartThread(void (*f)(void*), void* a) override {
    return target_->StartThread(f, a);
  }
  void WaitForJoin() override { return target_->WaitForJoin(); }
  unsigned int GetThreadPoolQueueLen(Priority pri = LOW) const override {
    return target_->GetThreadPoolQueueLen(pri);
  }
  Status GetTestDirectory(std::string* path) override {
    return target_->GetTestDirectory(path);
  }
  Status NewLogger(const std::string& fname,
                   shared_ptr<Logger>* result) override {
    return target_->NewLogger(fname, result);
  }
  uint64_t NowMicros() override { return target_->NowMicros(); }

  void SleepForMicroseconds(int micros) override {
    target_->SleepForMicroseconds(micros);
  }
  Status GetHostName(char* name, uint64_t len) override {
    return target_->GetHostName(name, len);
  }
  Status GetCurrentTime(int64_t* unix_time) override {
    return target_->GetCurrentTime(unix_time);
  }
  Status GetAbsolutePath(const std::string& db_path,
                         std::string* output_path) override {
    return target_->GetAbsolutePath(db_path, output_path);
  }
  void SetBackgroundThreads(int num, Priority pri) override {
    return target_->SetBackgroundThreads(num, pri);
  }
  int GetBackgroundThreads(Priority pri) override {
    return target_->GetBackgroundThreads(pri);
  }

  void IncBackgroundThreadsIfNeeded(int num, Priority pri) override {
    return target_->IncBackgroundThreadsIfNeeded(num, pri);
  }

  void LowerThreadPoolIOPriority(Priority pool = LOW) override {
    target_->LowerThreadPoolIOPriority(pool);
  }

  std::string TimeToString(uint64_t time) override {
    return target_->TimeToString(time);
  }

  Status GetThreadList(std::vector<ThreadStatus>* thread_list) override {
    return target_->GetThreadList(thread_list);
  }

  ThreadStatusUpdater* GetThreadStatusUpdater() const override {
    return target_->GetThreadStatusUpdater();
  }

  uint64_t GetThreadID() const override {
    return target_->GetThreadID();
  }

  std::string GenerateUniqueId() override {
    return target_->GenerateUniqueId();
  }

 private:
  Env* target_;
};

//将所有调用转发给另一个调用的可写文件的实现。
//写入文件。对于希望覆盖
//其他可写入文件的功能。
//它声明为可写文件的朋友，允许将呼叫转发到
//受保护的虚拟方法。
class WritableFileWrapper : public WritableFile {
 public:
  explicit WritableFileWrapper(WritableFile* t) : target_(t) { }

  Status Append(const Slice& data) override { return target_->Append(data); }
  Status PositionedAppend(const Slice& data, uint64_t offset) override {
    return target_->PositionedAppend(data, offset);
  }
  Status Truncate(uint64_t size) override { return target_->Truncate(size); }
  Status Close() override { return target_->Close(); }
  Status Flush() override { return target_->Flush(); }
  Status Sync() override { return target_->Sync(); }
  Status Fsync() override { return target_->Fsync(); }
  bool IsSyncThreadSafe() const override { return target_->IsSyncThreadSafe(); }
  void SetIOPriority(Env::IOPriority pri) override {
    target_->SetIOPriority(pri);
  }
  Env::IOPriority GetIOPriority() override { return target_->GetIOPriority(); }
  uint64_t GetFileSize() override { return target_->GetFileSize(); }
  void GetPreallocationStatus(size_t* block_size,
                              size_t* last_allocated_block) override {
    target_->GetPreallocationStatus(block_size, last_allocated_block);
  }
  size_t GetUniqueId(char* id, size_t max_size) const override {
    return target_->GetUniqueId(id, max_size);
  }
  Status InvalidateCache(size_t offset, size_t length) override {
    return target_->InvalidateCache(offset, length);
  }

  void SetPreallocationBlockSize(size_t size) override {
    target_->SetPreallocationBlockSize(size);
  }
  void PrepareWrite(size_t offset, size_t len) override {
    target_->PrepareWrite(offset, len);
  }

 protected:
  Status Allocate(uint64_t offset, uint64_t len) override {
    return target_->Allocate(offset, len);
  }
  Status RangeSync(uint64_t offset, uint64_t nbytes) override {
    return target_->RangeSync(offset, nbytes);
  }

 private:
  WritableFile* target_;
};

//返回将其数据存储在内存和委托中的新环境
//所有非文件存储任务到基环境。调用方必须删除结果
//当它不再需要时。
//*使用结果时，基本环境必须保持活动状态。
Env* NewMemEnv(Env* base_env);

//返回用于HDFS环境的新环境。
//这是hdfs/env_hdfs.h中声明的hdfs env的工厂方法
Status NewHdfsEnv(Env** hdfs_env, const std::string& fsname);

//返回一个新环境，该环境测量文件系统的函数调用时间
//操作，将结果报告给PerfContext中的变量。
//这是在utilities/env_timed.cc中定义的timed env的工厂方法。
Env* NewTimedEnv(Env* base_env);

}  //命名空间rocksdb

#endif  //储存室
