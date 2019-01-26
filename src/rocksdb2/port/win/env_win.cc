
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

#include "port/win/env_win.h"
#include "port/win/win_thread.h"
#include <algorithm>
#include <ctime>
#include <thread>

#include <errno.h>
#include <process.h> //γ-谷丙转氨酶
#include <io.h> //访问权限
#include <direct.h> //_rmdir，_mkdir，_getcwd
#include <sys/types.h>
#include <sys/stat.h>

#include "rocksdb/env.h"
#include "rocksdb/slice.h"

#include "port/port.h"
#include "port/dirent.h"
#include "port/win/win_logger.h"
#include "port/win/io_win.h"

#include "monitoring/iostats_context_imp.h"

#include "monitoring/thread_status_updater.h"
#include "monitoring/thread_status_util.h"

#include <rpc.h>  //用于生成UUID
#include <windows.h>

namespace rocksdb {

ThreadStatusUpdater* CreateThreadStatusUpdater() {
  return new ThreadStatusUpdater();
}

namespace {

//拉伊把手助手
const auto CloseHandleFunc = [](HANDLE h) { ::CloseHandle(h); };
typedef std::unique_ptr<void, decltype(CloseHandleFunc)> UniqueCloseHandlePtr;

void WinthreadCall(const char* label, std::error_code result) {
  if (0 != result.value()) {
    fprintf(stderr, "pthread %s: %s\n", label, strerror(result.value()));
    abort();
  }
}

}

namespace port {

WinEnvIO::WinEnvIO(Env* hosted_env)
  :   hosted_env_(hosted_env),
      page_size_(4 * 1012),
      allocation_granularity_(page_size_),
      perf_counter_frequency_(0),
      GetSystemTimePreciseAsFileTime_(NULL) {

  SYSTEM_INFO sinfo;
  GetSystemInfo(&sinfo);

  page_size_ = sinfo.dwPageSize;
  allocation_granularity_ = sinfo.dwAllocationGranularity;

  {
    LARGE_INTEGER qpf;
    BOOL ret = QueryPerformanceFrequency(&qpf);
    assert(ret == TRUE);
    perf_counter_frequency_ = qpf.QuadPart;
  }

  HMODULE module = GetModuleHandle("kernel32.dll");
  if (module != NULL) {
    GetSystemTimePreciseAsFileTime_ = (FnGetSystemTimePreciseAsFileTime)GetProcAddress(
      module, "GetSystemTimePreciseAsFileTime");
  }
}

WinEnvIO::~WinEnvIO() {
}

Status WinEnvIO::DeleteFile(const std::string& fname) {
  Status result;

  if (_unlink(fname.c_str())) {
    result = IOError("Failed to delete: " + fname, errno);
  }

  return result;
}

Status WinEnvIO::GetCurrentTime(int64_t* unix_time) {
  time_t time = std::time(nullptr);
  if (time == (time_t)(-1)) {
    return Status::NotSupported("Failed to get time");
  }

  *unix_time = time;
  return Status::OK();
}

Status WinEnvIO::NewSequentialFile(const std::string& fname,
  std::unique_ptr<SequentialFile>* result,
  const EnvOptions& options) {
  Status s;

  result->reset();

//损坏测试需要重命名和删除此类文件
//当它们仍然用另一个手柄打开时。因此我们
//允许共享写入和删除（允许重命名）。
  HANDLE hFile = INVALID_HANDLE_VALUE;

  DWORD fileFlags = FILE_ATTRIBUTE_READONLY;

  if (options.use_direct_reads && !options.use_mmap_reads) {
    fileFlags |= FILE_FLAG_NO_BUFFERING;
  }

  {
    IOSTATS_TIMER_GUARD(open_nanos);
    hFile = CreateFileA(
      fname.c_str(), GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
OPEN_EXISTING,  //原始fopen模式为“rb”
      fileFlags, NULL);
  }

  if (INVALID_HANDLE_VALUE == hFile) {
    auto lastError = GetLastError();
    s = IOErrorFromWindowsError("Failed to open NewSequentialFile" + fname,
      lastError);
  } else {
    result->reset(new WinSequentialFile(fname, hFile, options));
  }
  return s;
}

Status WinEnvIO::NewRandomAccessFile(const std::string& fname,
  std::unique_ptr<RandomAccessFile>* result,
  const EnvOptions& options) {
  result->reset();
  Status s;

//打开文件进行只读随机访问
//由于系统读取的数据太多，随机访问将禁用预读。
  DWORD fileFlags = FILE_ATTRIBUTE_READONLY;

  if (options.use_direct_reads && !options.use_mmap_reads) {
    fileFlags |= FILE_FLAG_NO_BUFFERING;
  } else {
    fileFlags |= FILE_FLAG_RANDOM_ACCESS;
  }

///shared access是通过损坏测试所必需的
//几乎所有的测试都可以工作，除了故障注入
  HANDLE hFile = 0;
  {
    IOSTATS_TIMER_GUARD(open_nanos);
    hFile =
      CreateFileA(fname.c_str(), GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      NULL, OPEN_EXISTING, fileFlags, NULL);
  }

  if (INVALID_HANDLE_VALUE == hFile) {
    auto lastError = GetLastError();
    return IOErrorFromWindowsError(
      "NewRandomAccessFile failed to Create/Open: " + fname, lastError);
  }

  UniqueCloseHandlePtr fileGuard(hFile, CloseHandleFunc);

//小心！这将把整个文件映射到进程地址空间中
  if (options.use_mmap_reads && sizeof(void*) >= 8) {
//当虚拟地址空间足够时使用mmap。
    uint64_t fileSize;

    s = GetFileSize(fname, &fileSize);

    if (s.ok()) {
//不会映射空文件
      if (fileSize == 0) {
        return IOError(
          "NewRandomAccessFile failed to map empty file: " + fname, EINVAL);
      }

      HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY,
0,  //当前长度的整个文件
        0,
NULL);  //映射名称

      if (!hMap) {
        auto lastError = GetLastError();
        return IOErrorFromWindowsError(
          "Failed to create file mapping for NewRandomAccessFile: " + fname,
          lastError);
      }

      UniqueCloseHandlePtr mapGuard(hMap, CloseHandleFunc);

      const void* mapped_region =
        MapViewOfFileEx(hMap, FILE_MAP_READ,
0,  //访问启动的高DWORD
0,  //低dWord
        fileSize,
NULL);  //让操作系统选择映射

      if (!mapped_region) {
        auto lastError = GetLastError();
        return IOErrorFromWindowsError(
          "Failed to MapViewOfFile for NewRandomAccessFile: " + fname,
          lastError);
      }

      result->reset(new WinMmapReadableFile(fname, hFile, hMap, mapped_region,
        fileSize));

      mapGuard.release();
      fileGuard.release();
    }
  } else {
    result->reset(new WinRandomAccessFile(fname, hFile, page_size_, options));
    fileGuard.release();
  }
  return s;
}

Status WinEnvIO::OpenWritableFile(const std::string& fname,
  std::unique_ptr<WritableFile>* result,
  const EnvOptions& options,
  bool reopen) {

  const size_t c_BufferCapacity = 64 * 1024;

  EnvOptions local_options(options);

  result->reset();
  Status s;

  DWORD fileFlags = FILE_ATTRIBUTE_NORMAL;

  if (local_options.use_direct_writes && !local_options.use_mmap_writes) {
    fileFlags = FILE_FLAG_NO_BUFFERING;
  }

//需要访问。我们只想写在这里，但如果我们想记忆
//地图
//文件没有只写模式，因此我们必须创建它
//读/写
//但是，mapviewoffile只指定只写
  DWORD desired_access = GENERIC_WRITE;
  DWORD shared_mode = FILE_SHARE_READ;

  if (local_options.use_mmap_writes) {
    desired_access |= GENERIC_READ;
  }
  else {
//仅为通过测试添加此项（故障注入测试，
//沃尔玛经理测试）。
    shared_mode |= (FILE_SHARE_WRITE | FILE_SHARE_DELETE);
  }

//这将始终截断文件
  DWORD creation_disposition = CREATE_ALWAYS;
  if (reopen) {
    creation_disposition = OPEN_ALWAYS;
  }

  HANDLE hFile = 0;
  {
    IOSTATS_TIMER_GUARD(open_nanos);
    hFile = CreateFileA(
      fname.c_str(),
desired_access,  //期望访问
      shared_mode,
NULL,           //安全属性
creation_disposition,  //posix env说（重新打开）？（o_create_o_append）：o_creat_o_trunc
fileFlags,      //旗帜
NULL);          //模板文件
  }

  if (INVALID_HANDLE_VALUE == hFile) {
    auto lastError = GetLastError();
    return IOErrorFromWindowsError(
      "Failed to create a NewWriteableFile: " + fname, lastError);
  }

//我们将从结尾开始写，附加
  if (reopen) {
    LARGE_INTEGER zero_move;
    zero_move.QuadPart = 0;
    BOOL ret = SetFilePointerEx(hFile, zero_move, NULL, FILE_END);
    if (!ret) {
      auto lastError = GetLastError();
      return IOErrorFromWindowsError(
        "Failed to create a ReopenWritableFile move to the end: " + fname, lastError);
    }
  }

  if (options.use_mmap_writes) {
//我们通常不在SSD上使用mmmapping，因此我们传递内存
//页尺寸
    result->reset(new WinMmapFile(fname, hFile, page_size_,
      allocation_granularity_, local_options));
  } else {
//这里，我们希望缓冲区分配与SSD页面大小一致。
//是它的倍数
    result->reset(new WinWritableFile(fname, hFile, page_size_,
      c_BufferCapacity, local_options));
  }
  return s;
}

Status WinEnvIO::NewRandomRWFile(const std::string & fname,
  std::unique_ptr<RandomRWFile>* result, const EnvOptions & options) {

  Status s;

//打开文件进行只读随机访问
//由于系统读取的数据太多，随机访问将禁用预读。
  DWORD desired_access = GENERIC_READ | GENERIC_WRITE;
  DWORD shared_mode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
DWORD creation_disposition = OPEN_ALWAYS; //必要时创建或打开现有
  DWORD file_flags = FILE_FLAG_RANDOM_ACCESS;

  if (options.use_direct_reads && options.use_direct_writes) {
    file_flags |= FILE_FLAG_NO_BUFFERING;
  }

///shared access是通过损坏测试所必需的
//几乎所有的测试都可以工作，除了故障注入
  HANDLE hFile = 0;
  {
    IOSTATS_TIMER_GUARD(open_nanos);
    hFile =
      CreateFileA(fname.c_str(),
        desired_access,
        shared_mode,
NULL, //安全属性
        creation_disposition,
        file_flags,
        NULL);
  }

  if (INVALID_HANDLE_VALUE == hFile) {
    auto lastError = GetLastError();
    return IOErrorFromWindowsError(
      "NewRandomRWFile failed to Create/Open: " + fname, lastError);
  }

  UniqueCloseHandlePtr fileGuard(hFile, CloseHandleFunc);
  result->reset(new WinRandomRWFile(fname, hFile, page_size_, options));
  fileGuard.release();

  return s;
}

Status WinEnvIO::NewDirectory(const std::string& name,
  std::unique_ptr<Directory>* result) {
  Status s;
//失败时必须为nullptr
  result->reset();
//如果目录不存在，则必须失败
  if (!DirExists(name)) {
    s = IOError("Directory does not exist: " + name, EEXIST);
  } else {
    IOSTATS_TIMER_GUARD(open_nanos);
    result->reset(new WinDirectory);
  }
  return s;
}

Status WinEnvIO::FileExists(const std::string& fname) {
//FYOK＝0
  const int F_OK_ = 0;
  return _access(fname.c_str(), F_OK_) == 0 ? Status::OK()
    : Status::NotFound();
}

Status WinEnvIO::GetChildren(const std::string& dir,
  std::vector<std::string>* result) {

  result->clear();
  std::vector<std::string> output;

  Status status;

  auto CloseDir = [](DIR* p) { closedir(p); };
  std::unique_ptr<DIR, decltype(CloseDir)> dirp(opendir(dir.c_str()),
    CloseDir);

  if (!dirp) {
    switch (errno) {
      case EACCES:
      case ENOENT:
      case ENOTDIR:
        return Status::NotFound();
      default:
        return IOError(dir, errno);
    }
  } else {
    if (result->capacity() > 0) {
      output.reserve(result->capacity());
    }

    struct dirent* ent = readdir(dirp.get());
    while (ent) {
      output.push_back(ent->d_name);
      ent = readdir(dirp.get());
    }
  }

  output.swap(*result);

  return status;
}

Status WinEnvIO::CreateDir(const std::string& name) {
  Status result;

  if (_mkdir(name.c_str()) != 0) {
    auto code = errno;
    result = IOError("Failed to create dir: " + name, code);
  }

  return result;
}

Status  WinEnvIO::CreateDirIfMissing(const std::string& name) {
  Status result;

  if (DirExists(name)) {
    return result;
  }

  if (_mkdir(name.c_str()) != 0) {
    if (errno == EEXIST) {
      result =
        Status::IOError("`" + name + "' exists but is not a directory");
    } else {
      auto code = errno;
      result = IOError("Failed to create dir: " + name, code);
    }
  }

  return result;
}

Status WinEnvIO::DeleteDir(const std::string& name) {
  Status result;
  if (_rmdir(name.c_str()) != 0) {
    auto code = errno;
    result = IOError("Failed to remove dir: " + name, code);
  }
  return result;
}

Status WinEnvIO::GetFileSize(const std::string& fname,
  uint64_t* size) {
  Status s;

  WIN32_FILE_ATTRIBUTE_DATA attrs;
  if (GetFileAttributesExA(fname.c_str(), GetFileExInfoStandard, &attrs)) {
    ULARGE_INTEGER file_size;
    file_size.HighPart = attrs.nFileSizeHigh;
    file_size.LowPart = attrs.nFileSizeLow;
    *size = file_size.QuadPart;
  } else {
    auto lastError = GetLastError();
    s = IOErrorFromWindowsError("Can not get size for: " + fname, lastError);
  }
  return s;
}

uint64_t WinEnvIO::FileTimeToUnixTime(const FILETIME& ftTime) {
  const uint64_t c_FileTimePerSecond = 10000000U;
//Unix Epoch从1970-01-01t00:00:00z开始
//Windows文件时间从1601-01-01T00:00:00Z开始
//因此，我们需要从
//我们从filetime获得的秒数，明显丢失了
//精度
  const uint64_t c_SecondBeforeUnixEpoch = 11644473600U;

  ULARGE_INTEGER li;
  li.HighPart = ftTime.dwHighDateTime;
  li.LowPart = ftTime.dwLowDateTime;

  uint64_t result =
    (li.QuadPart / c_FileTimePerSecond) - c_SecondBeforeUnixEpoch;
  return result;
}

Status WinEnvIO::GetFileModificationTime(const std::string& fname,
  uint64_t* file_mtime) {
  Status s;

  WIN32_FILE_ATTRIBUTE_DATA attrs;
  if (GetFileAttributesExA(fname.c_str(), GetFileExInfoStandard, &attrs)) {
    *file_mtime = FileTimeToUnixTime(attrs.ftLastWriteTime);
  } else {
    auto lastError = GetLastError();
    s = IOErrorFromWindowsError(
      "Can not get file modification time for: " + fname, lastError);
    *file_mtime = 0;
  }

  return s;
}

Status WinEnvIO::RenameFile(const std::string& src,
  const std::string& target) {
  Status result;

//rename（）无法替换Linux上的现有文件
//所以直接使用操作系统API
  if (!MoveFileExA(src.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING)) {
    DWORD lastError = GetLastError();

    std::string text("Failed to rename: ");
    text.append(src).append(" to: ").append(target);

    result = IOErrorFromWindowsError(text, lastError);
  }

  return result;
}

Status WinEnvIO::LinkFile(const std::string& src,
  const std::string& target) {
  Status result;

  if (!CreateHardLinkA(target.c_str(), src.c_str(), NULL)) {
    DWORD lastError = GetLastError();

    std::string text("Failed to link: ");
    text.append(src).append(" to: ").append(target);

    result = IOErrorFromWindowsError(text, lastError);
  }

  return result;
}

Status  WinEnvIO::LockFile(const std::string& lockFname,
  FileLock** lock) {
  assert(lock != nullptr);

  *lock = NULL;
  Status result;

//不共享，这是一个锁定文件
  const DWORD ExclusiveAccessON = 0;

//获取对锁定文件的独占访问权限
//以前，我们将删除设置为关闭，而不是正常的属性
//除了坚持删除的故障注入测试。
  HANDLE hFile = 0;
  {
    IOSTATS_TIMER_GUARD(open_nanos);
    hFile = CreateFileA(lockFname.c_str(), (GENERIC_READ | GENERIC_WRITE),
      ExclusiveAccessON, NULL, CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL, NULL);
  }

  if (INVALID_HANDLE_VALUE == hFile) {
    auto lastError = GetLastError();
    result = IOErrorFromWindowsError(
      "Failed to create lock file: " + lockFname, lastError);
  } else {
    *lock = new WinFileLock(hFile);
  }

  return result;
}

Status WinEnvIO::UnlockFile(FileLock* lock) {
  Status result;

  assert(lock != nullptr);

  delete lock;

  return result;
}

Status WinEnvIO::GetTestDirectory(std::string* result) {
  std::string output;

  const char* env = getenv("TEST_TMPDIR");
  if (env && env[0] != '\0') {
    output = env;
    CreateDir(output);
  } else {
    env = getenv("TMP");

    if (env && env[0] != '\0') {
      output = env;
    } else {
      output = "c:\\tmp";
    }

    CreateDir(output);
  }

  output.append("\\testrocksdb-");
  output.append(std::to_string(_getpid()));

  CreateDir(output);

  output.swap(*result);

  return Status::OK();
}

Status WinEnvIO::NewLogger(const std::string& fname,
  std::shared_ptr<Logger>* result) {
  Status s;

  result->reset();

  HANDLE hFile = 0;
  {
    IOSTATS_TIMER_GUARD(open_nanos);
    hFile = CreateFileA(
      fname.c_str(), GENERIC_WRITE,
FILE_SHARE_READ | FILE_SHARE_DELETE,  //在rocksdb中，日志文件是
//之前已重命名和删除
//他们关门了。这使得
//这样做。
      NULL,
CREATE_ALWAYS,  //原始fopen模式为“w”
      FILE_ATTRIBUTE_NORMAL, NULL);
  }

  if (INVALID_HANDLE_VALUE == hFile) {
    auto lastError = GetLastError();
    s = IOErrorFromWindowsError("Failed to open LogFile" + fname, lastError);
  } else {
    {
//对于日志文件，我们希望设置从现在开始的真正创建时间
//因为系统
//出于某种原因，缓存前一个文件的属性，
//已从重命名
//此名称，因此自动滚动记录器测试失败
      FILETIME ft;
      GetSystemTimeAsFileTime(&ft);
//将创建、上次访问和上次写入时间设置为相同的值
      SetFileTime(hFile, &ft, &ft, &ft);
    }
    result->reset(new WinLogger(&WinEnvThreads::gettid, hosted_env_, hFile));
  }
  return s;
}

uint64_t WinEnvIO::NowMicros() {

  if (GetSystemTimePreciseAsFileTime_ != NULL) {
//Windows上的所有std：：chrono时钟均已返回
//可能重复的值对于某些用途来说不够好。
    const int64_t c_UnixEpochStartTicks = 116444736000000000LL;
    const int64_t c_FtToMicroSec = 10;

//此接口需要返回系统时间，而不是
//只是微秒，因为它经常被用作参数
//到条件变量上的TimedWait（）。
    FILETIME ftSystemTime;
    GetSystemTimePreciseAsFileTime_(&ftSystemTime);

    LARGE_INTEGER li;
    li.LowPart = ftSystemTime.dwLowDateTime;
    li.HighPart = ftSystemTime.dwHighDateTime;
//减去unix epoch start
    li.QuadPart -= c_UnixEpochStartTicks;
//转换为microsecs
    li.QuadPart /= c_FtToMicroSec;
    return li.QuadPart;
  }
  using namespace std::chrono;
  return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

uint64_t WinEnvIO::NowNanos() {
//Windows上的所有std:：chrono时钟的分辨率都相同，只有
//足以维持微秒，但不能达到纳秒
//在Windows 8和Windows 2012服务器上
//可以使用GetSystemTimePreciseAsFileTime（&current_time）
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
//首先转换为纳秒，以避免精度损失
//并除以频率
  li.QuadPart *= std::nano::den;
  li.QuadPart /= perf_counter_frequency_;
  return li.QuadPart;
}

Status WinEnvIO::GetHostName(char* name, uint64_t len) {
  Status s;
  DWORD nSize = static_cast<DWORD>(
    std::min<uint64_t>(len, std::numeric_limits<DWORD>::max()));

  if (!::GetComputerNameA(name, &nSize)) {
    auto lastError = GetLastError();
    s = IOErrorFromWindowsError("GetHostName", lastError);
  } else {
    name[nSize] = 0;
  }

  return s;
}

Status WinEnvIO::GetAbsolutePath(const std::string& db_path,
  std::string* output_path) {
//检查是否已经有绝对路径
//以非点开头，中间有分号
  if ((!db_path.empty() && (db_path[0] == '/' || db_path[0] == '\\')) ||
    (db_path.size() > 2 && db_path[0] != '.' &&
    ((db_path[1] == ':' && db_path[2] == '\\') ||
    (db_path[1] == ':' && db_path[2] == '/')))) {
    *output_path = db_path;
    return Status::OK();
  }

  std::string result;
  result.resize(_MAX_PATH);

  char* ret = _getcwd(&result[0], _MAX_PATH);
  if (ret == nullptr) {
    return Status::IOError("Failed to get current working directory",
      strerror(errno));
  }

  result.resize(strlen(result.data()));

  result.swap(*output_path);
  return Status::OK();
}

std::string WinEnvIO::TimeToString(uint64_t secondsSince1970) {
  std::string result;

  const time_t seconds = secondsSince1970;
  const int maxsize = 64;

  struct tm t;
  errno_t ret = localtime_s(&t, &seconds);

  if (ret) {
    result = std::to_string(seconds);
  } else {
    result.resize(maxsize);
    char* p = &result[0];

    int len = snprintf(p, maxsize, "%04d/%02d/%02d-%02d:%02d:%02d ",
      t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
      t.tm_min, t.tm_sec);
    assert(len > 0);

    result.resize(len);
  }

  return result;
}

EnvOptions WinEnvIO::OptimizeForLogWrite(const EnvOptions& env_options,
  const DBOptions& db_options) const {
  EnvOptions optimized = env_options;
  optimized.bytes_per_sync = db_options.wal_bytes_per_sync;
  optimized.use_mmap_writes = false;
//这是因为我们只刷新未缓冲IO上的整页
//最后的记录不能保证被刷新。
  optimized.use_direct_writes = false;
//todo（icanadi）如果用keep-size休闲会更快，但是它
//中断TransactionLogiater或TallAtlaStrecord单元测试。固定单元
//测试并使其错误
  optimized.fallocate_with_keep_size = true;
  return optimized;
}

EnvOptions WinEnvIO::OptimizeForManifestWrite(
  const EnvOptions& env_options) const {
  EnvOptions optimized = env_options;
  optimized.use_mmap_writes = false;
  optimized.use_direct_writes = false;
  optimized.fallocate_with_keep_size = true;
  return optimized;
}

//如果指定的目录存在并且是目录，则返回true。
bool WinEnvIO::DirExists(const std::string& dname) {
  WIN32_FILE_ATTRIBUTE_DATA attrs;
  if (GetFileAttributesExA(dname.c_str(), GetFileExInfoStandard, &attrs)) {
    return 0 != (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  }
  return false;
}

//////////////////////////////////////////////////
//文氏螺纹

WinEnvThreads::WinEnvThreads(Env* hosted_env) : hosted_env_(hosted_env), thread_pools_(Env::Priority::TOTAL) {

  for (int pool_id = 0; pool_id < Env::Priority::TOTAL; ++pool_id) {
    thread_pools_[pool_id].SetThreadPriority(
      static_cast<Env::Priority>(pool_id));
//这允许以后初始化每个线程的线程本地环境。
    thread_pools_[pool_id].SetHostEnv(hosted_env);
  }
}

WinEnvThreads::~WinEnvThreads() {

  WaitForJoin();

  for (auto& thpool : thread_pools_) {
    thpool.JoinAllThreads();
  }
}

void WinEnvThreads::Schedule(void(*function)(void*), void* arg, Env::Priority pri,
  void* tag, void(*unschedFunction)(void* arg)) {
  assert(pri >= Env::Priority::BOTTOM && pri <= Env::Priority::HIGH);
  thread_pools_[pri].Schedule(function, arg, tag, unschedFunction);
}

int WinEnvThreads::UnSchedule(void* arg, Env::Priority pri) {
  return thread_pools_[pri].UnSchedule(arg);
}

namespace {

  struct StartThreadState {
    void(*user_function)(void*);
    void* arg;
  };

  void* StartThreadWrapper(void* arg) {
    std::unique_ptr<StartThreadState> state(
      reinterpret_cast<StartThreadState*>(arg));
    state->user_function(state->arg);
    return nullptr;
  }

}

void WinEnvThreads::StartThread(void(*function)(void* arg), void* arg) {
  std::unique_ptr<StartThreadState> state(new StartThreadState);
  state->user_function = function;
  state->arg = arg;
  try {

    rocksdb::port::WindowsThread th(&StartThreadWrapper, state.get());
    state.release();

    std::lock_guard<std::mutex> lg(mu_);
    threads_to_join_.push_back(std::move(th));

  } catch (const std::system_error& ex) {
    WinthreadCall("start thread", ex.code());
  }
}

void WinEnvThreads::WaitForJoin() {
  for (auto& th : threads_to_join_) {
    th.join();
  }
  threads_to_join_.clear();
}

unsigned int WinEnvThreads::GetThreadPoolQueueLen(Env::Priority pri) const {
  assert(pri >= Env::Priority::BOTTOM && pri <= Env::Priority::HIGH);
  return thread_pools_[pri].GetQueueLen();
}

uint64_t WinEnvThreads::gettid() {
  uint64_t thread_id = GetCurrentThreadId();
  return thread_id;
}

uint64_t WinEnvThreads::GetThreadID() const { return gettid(); }

void  WinEnvThreads::SleepForMicroseconds(int micros) {
  std::this_thread::sleep_for(std::chrono::microseconds(micros));
}

void WinEnvThreads::SetBackgroundThreads(int num, Env::Priority pri) {
  assert(pri >= Env::Priority::BOTTOM && pri <= Env::Priority::HIGH);
  thread_pools_[pri].SetBackgroundThreads(num);
}

int WinEnvThreads::GetBackgroundThreads(Env::Priority pri) {
  assert(pri >= Env::Priority::BOTTOM && pri <= Env::Priority::HIGH);
  return thread_pools_[pri].GetBackgroundThreads();
}

void WinEnvThreads::IncBackgroundThreadsIfNeeded(int num, Env::Priority pri) {
  assert(pri >= Env::Priority::BOTTOM && pri <= Env::Priority::HIGH);
  thread_pools_[pri].IncBackgroundThreadsIfNeeded(num);
}

//////////////////////////////////////////////
//温尼夫

WinEnv::WinEnv() : winenv_io_(this), winenv_threads_(this) {
//基类的受保护成员
  thread_status_updater_ = CreateThreadStatusUpdater();
}


WinEnv::~WinEnv() {
//删除之前必须连接所有线程
//线程状态更新程序。
  delete thread_status_updater_;
}

Status WinEnv::GetThreadList(
  std::vector<ThreadStatus>* thread_list) {
  assert(thread_status_updater_);
  return thread_status_updater_->GetThreadList(thread_list);
}

Status WinEnv::DeleteFile(const std::string& fname) {
  return winenv_io_.DeleteFile(fname);
}

Status WinEnv::GetCurrentTime(int64_t* unix_time) {
  return winenv_io_.GetCurrentTime(unix_time);
}

Status  WinEnv::NewSequentialFile(const std::string& fname,
  std::unique_ptr<SequentialFile>* result,
  const EnvOptions& options) {
  return winenv_io_.NewSequentialFile(fname, result, options);
}

Status WinEnv::NewRandomAccessFile(const std::string& fname,
  std::unique_ptr<RandomAccessFile>* result,
  const EnvOptions& options) {
  return winenv_io_.NewRandomAccessFile(fname, result, options);
}

Status WinEnv::NewWritableFile(const std::string& fname,
                               std::unique_ptr<WritableFile>* result,
                               const EnvOptions& options) {
  return winenv_io_.OpenWritableFile(fname, result, options, false);
}

Status WinEnv::ReopenWritableFile(const std::string& fname,
    std::unique_ptr<WritableFile>* result, const EnvOptions& options) {
  return winenv_io_.OpenWritableFile(fname, result, options, true);
}

Status WinEnv::NewRandomRWFile(const std::string & fname,
  unique_ptr<RandomRWFile>* result, const EnvOptions & options) {
  return winenv_io_.NewRandomRWFile(fname, result, options);
}

Status WinEnv::NewDirectory(const std::string& name,
  std::unique_ptr<Directory>* result) {
  return winenv_io_.NewDirectory(name, result);
}

Status WinEnv::FileExists(const std::string& fname) {
  return winenv_io_.FileExists(fname);
}

Status WinEnv::GetChildren(const std::string& dir,
  std::vector<std::string>* result) {
  return winenv_io_.GetChildren(dir, result);
}

Status WinEnv::CreateDir(const std::string& name) {
  return winenv_io_.CreateDir(name);
}

Status WinEnv::CreateDirIfMissing(const std::string& name) {
  return winenv_io_.CreateDirIfMissing(name);
}

Status WinEnv::DeleteDir(const std::string& name) {
  return winenv_io_.DeleteDir(name);
}

Status WinEnv::GetFileSize(const std::string& fname,
  uint64_t* size) {
  return winenv_io_.GetFileSize(fname, size);
}

Status  WinEnv::GetFileModificationTime(const std::string& fname,
  uint64_t* file_mtime) {
  return winenv_io_.GetFileModificationTime(fname, file_mtime);
}

Status WinEnv::RenameFile(const std::string& src,
  const std::string& target) {
  return winenv_io_.RenameFile(src, target);
}

Status WinEnv::LinkFile(const std::string& src,
  const std::string& target) {
  return winenv_io_.LinkFile(src, target);
}

Status WinEnv::LockFile(const std::string& lockFname,
  FileLock** lock) {
  return winenv_io_.LockFile(lockFname, lock);
}

Status WinEnv::UnlockFile(FileLock* lock) {
  return winenv_io_.UnlockFile(lock);
}

Status  WinEnv::GetTestDirectory(std::string* result) {
  return winenv_io_.GetTestDirectory(result);
}

Status WinEnv::NewLogger(const std::string& fname,
  std::shared_ptr<Logger>* result) {
  return winenv_io_.NewLogger(fname, result);
}

uint64_t WinEnv::NowMicros() {
  return winenv_io_.NowMicros();
}

uint64_t  WinEnv::NowNanos() {
  return winenv_io_.NowNanos();
}

Status WinEnv::GetHostName(char* name, uint64_t len) {
  return winenv_io_.GetHostName(name, len);
}

Status WinEnv::GetAbsolutePath(const std::string& db_path,
  std::string* output_path) {
  return winenv_io_.GetAbsolutePath(db_path, output_path);
}

std::string WinEnv::TimeToString(uint64_t secondsSince1970) {
  return winenv_io_.TimeToString(secondsSince1970);
}

void  WinEnv::Schedule(void(*function)(void*), void* arg, Env::Priority pri,
  void* tag,
  void(*unschedFunction)(void* arg)) {
  return winenv_threads_.Schedule(function, arg, pri, tag, unschedFunction);
}

int WinEnv::UnSchedule(void* arg, Env::Priority pri) {
  return winenv_threads_.UnSchedule(arg, pri);
}

void WinEnv::StartThread(void(*function)(void* arg), void* arg) {
  return winenv_threads_.StartThread(function, arg);
}

void WinEnv::WaitForJoin() {
  return winenv_threads_.WaitForJoin();
}

unsigned int  WinEnv::GetThreadPoolQueueLen(Env::Priority pri) const {
  return winenv_threads_.GetThreadPoolQueueLen(pri);
}

uint64_t WinEnv::GetThreadID() const {
  return winenv_threads_.GetThreadID();
}

void WinEnv::SleepForMicroseconds(int micros) {
  return winenv_threads_.SleepForMicroseconds(micros);
}

//允许增加工作线程数。
void  WinEnv::SetBackgroundThreads(int num, Env::Priority pri) {
  return winenv_threads_.SetBackgroundThreads(num, pri);
}

int WinEnv::GetBackgroundThreads(Env::Priority pri) {
  return winenv_threads_.GetBackgroundThreads(pri);
}

void  WinEnv::IncBackgroundThreadsIfNeeded(int num, Env::Priority pri) {
  return winenv_threads_.IncBackgroundThreadsIfNeeded(num, pri);
}

EnvOptions WinEnv::OptimizeForLogWrite(const EnvOptions& env_options,
  const DBOptions& db_options) const {
  return winenv_io_.OptimizeForLogWrite(env_options, db_options);
}

EnvOptions WinEnv::OptimizeForManifestWrite(
  const EnvOptions& env_options) const {
  return winenv_io_.OptimizeForManifestWrite(env_options);
}

}  //命名空间端口

std::string Env::GenerateUniqueId() {
  std::string result;

  UUID uuid;
  UuidCreateSequential(&uuid);

  RPC_CSTR rpc_str;
  auto status = UuidToStringA(&uuid, &rpc_str);
  (void)status;
  assert(status == RPC_S_OK);

  result = reinterpret_cast<char*>(rpc_str);

  status = RpcStringFreeA(&rpc_str);
  assert(status == RPC_S_OK);

  return result;
}

}  //命名空间rocksdb
