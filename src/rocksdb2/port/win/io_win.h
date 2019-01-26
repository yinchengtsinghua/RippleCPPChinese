
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
#pragma once

#include <stdint.h>
#include <mutex>
#include <string>

#include "rocksdb/status.h"
#include "rocksdb/env.h"
#include "util/aligned_buffer.h"

#include <windows.h>


namespace rocksdb {
namespace port {

std::string GetWindowsErrSz(DWORD err);

inline Status IOErrorFromWindowsError(const std::string& context, DWORD err) {
  return ((err == ERROR_HANDLE_DISK_FULL) || (err == ERROR_DISK_FULL))
             ? Status::NoSpace(context, GetWindowsErrSz(err))
             : Status::IOError(context, GetWindowsErrSz(err));
}

inline Status IOErrorFromLastWindowsError(const std::string& context) {
  return IOErrorFromWindowsError(context, GetLastError());
}

inline Status IOError(const std::string& context, int err_number) {
  return (err_number == ENOSPC)
             ? Status::NoSpace(context, strerror(err_number))
             : Status::IOError(context, strerror(err_number));
}

//注意以下两个不设置errno，因为它们仅在此处使用
//文件
//在窗户的把手上，因此，没有必要。正在转换GetLastError（）。
//埃尔诺
//是一件可悲的事
inline int fsync(HANDLE hFile) {
  if (!FlushFileBuffers(hFile)) {
    return -1;
  }

  return 0;
}

SSIZE_T pwrite(HANDLE hFile, const char* src, size_t numBytes, uint64_t offset);

SSIZE_T pread(HANDLE hFile, char* src, size_t numBytes, uint64_t offset);

Status fallocate(const std::string& filename, HANDLE hFile, uint64_t to_size);

Status ftruncate(const std::string& filename, HANDLE hFile, uint64_t toSize);

size_t GetUniqueIdFromFile(HANDLE hFile, char* id, size_t max_size);

class WinFileData {
 protected:
  const std::string filename_;
  HANDLE hFile_;
//如果是真的，发出的I/O将是直接的I/O，缓冲区
//需要对齐（不确定是否有保证
//传入是对齐的）。
  const bool use_direct_io_;

 public:
//我们希望这个类既可用于继承（prive
//或受保护），以及针对公众的遏制措施
  WinFileData(const std::string& filename, HANDLE hFile, bool direct_io)
      : filename_(filename), hFile_(hFile), use_direct_io_(direct_io) {}

  virtual ~WinFileData() { this->CloseFile(); }

  bool CloseFile() {
    bool result = true;

    if (hFile_ != NULL && hFile_ != INVALID_HANDLE_VALUE) {
      result = ::CloseHandle(hFile_);
      assert(result);
      hFile_ = NULL;
    }
    return result;
  }

  const std::string& GetName() const { return filename_; }

  HANDLE GetFileHandle() const { return hFile_; }

  bool use_direct_io() const { return use_direct_io_; }

  WinFileData(const WinFileData&) = delete;
  WinFileData& operator=(const WinFileData&) = delete;
};

class WinSequentialFile : protected WinFileData, public SequentialFile {

//在创建自定义环境时重写行为更改
  virtual SSIZE_T PositionedReadInternal(char* src, size_t numBytes,
    uint64_t offset) const;

public:
  WinSequentialFile(const std::string& fname, HANDLE f,
    const EnvOptions& options);

  ~WinSequentialFile();

  WinSequentialFile(const WinSequentialFile&) = delete;
  WinSequentialFile& operator=(const WinSequentialFile&) = delete;

  virtual Status Read(size_t n, Slice* result, char* scratch) override;
  virtual Status PositionedRead(uint64_t offset, size_t n, Slice* result,
    char* scratch) override;

  virtual Status Skip(uint64_t n) override;

  virtual Status InvalidateCache(size_t offset, size_t length) override;

  virtual bool use_direct_io() const override { return WinFileData::use_direct_io(); }
};

//基于mmap（）的随机访问
class WinMmapReadableFile : private WinFileData, public RandomAccessFile {
  HANDLE hMap_;

  const void* mapped_region_;
  const size_t length_;

 public:
//mapped_region_[0，length-1]包含文件的mmap内容。
  WinMmapReadableFile(const std::string& fileName, HANDLE hFile, HANDLE hMap,
                      const void* mapped_region, size_t length);

  ~WinMmapReadableFile();

  WinMmapReadableFile(const WinMmapReadableFile&) = delete;
  WinMmapReadableFile& operator=(const WinMmapReadableFile&) = delete;

  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const override;

  virtual Status InvalidateCache(size_t offset, size_t length) override;

  virtual size_t GetUniqueId(char* id, size_t max_size) const override;
};

//我们预先分配并使用memcpy附加新的
//数据到文件。这是安全的，因为我们要么适当关闭
//文件在从中读取之前，或者对于日志文件，读取代码
//足够了解跳过零后缀。
class WinMmapFile : private WinFileData, public WritableFile {
 private:
  HANDLE hMap_;

const size_t page_size_;  //我们以页面大小刷新映射视图
//增量。我们可以决定这是不是记忆
//页面大小或SSD页面大小
  const size_t
allocation_granularity_;  //视图必须以这样的粒度开始

size_t reserved_size_;  //预分配尺寸

size_t mapping_size_;  //映射对象的最大大小
//我们想猜测最终的文件大小以最小化重新映射
size_t view_size_;  //一次映射到视图的内存量

char* mapped_begin_;  //必须从与对齐的文件偏移量开始
//分配粒度
  char* mapped_end_;
char* dst_;  //下一个写入位置（在范围[映射的开始，映射的结束]）
char* last_sync_;  //我们在哪里同步的

uint64_t file_offset_;  //文件中映射的\开始\的偏移量

//是否有未同步的写入？
  bool pending_sync_;

//只有在以下情况下才能截断或保留到对齐的扇区大小
//用于使用未缓冲的I/O打开的文件
  Status TruncateFile(uint64_t toSize);

  Status UnmapCurrentRegion();

  Status MapNewRegion();

  virtual Status PreallocateInternal(uint64_t spaceToReserve);

 public:
  WinMmapFile(const std::string& fname, HANDLE hFile, size_t page_size,
              size_t allocation_granularity, const EnvOptions& options);

  ~WinMmapFile();

  WinMmapFile(const WinMmapFile&) = delete;
  WinMmapFile& operator=(const WinMmapFile&) = delete;

  virtual Status Append(const Slice& data) override;

//表示close（）将正确处理截断
//它不需要任何额外的信息
  virtual Status Truncate(uint64_t size) override;

  virtual Status Close() override;

  virtual Status Flush() override;

//仅冲洗数据
  virtual Status Sync() override;

  /*
  *将数据和元数据刷新到稳定存储。
  **/

  virtual Status Fsync() override;

  /*
  *获取文件中有效数据的大小。这与
  *文件系统返回的大小，因为我们使用mmap
  *每次按地图大小扩展文件。
  **/

  virtual uint64_t GetFileSize() override;

  virtual Status InvalidateCache(size_t offset, size_t length) override;

  virtual Status Allocate(uint64_t offset, uint64_t len) override;

  virtual size_t GetUniqueId(char* id, size_t max_size) const override;
};

class WinRandomAccessImpl {
 protected:
  WinFileData* file_base_;
  size_t       alignment_;

//在创建自定义环境时重写行为更改
  virtual SSIZE_T PositionedReadInternal(char* src, size_t numBytes,
                                         uint64_t offset) const;

  WinRandomAccessImpl(WinFileData* file_base, size_t alignment,
                      const EnvOptions& options);

  virtual ~WinRandomAccessImpl() {}

  Status ReadImpl(uint64_t offset, size_t n, Slice* result,
                  char* scratch) const;

  size_t GetAlignment() const { return alignment_; }

 public:

  WinRandomAccessImpl(const WinRandomAccessImpl&) = delete;
  WinRandomAccessImpl& operator=(const WinRandomAccessImpl&) = delete;
};

//基于pread（）的随机访问
class WinRandomAccessFile
    : private WinFileData,
protected WinRandomAccessImpl,  //希望能够覆盖
//位置可怕内部
      public RandomAccessFile {
 public:
  WinRandomAccessFile(const std::string& fname, HANDLE hFile, size_t alignment,
                      const EnvOptions& options);

  ~WinRandomAccessFile();

  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const override;

  virtual size_t GetUniqueId(char* id, size_t max_size) const override;

  virtual bool use_direct_io() const override { return WinFileData::use_direct_io(); }

  virtual Status InvalidateCache(size_t offset, size_t length) override;

  virtual size_t GetRequiredBufferAlignment() const override;
};

//这是一个顺序写入类。它被模仿（像其他人一样）之后
//原始posix类。我们在Windows上添加了对无缓冲I/O的支持
//好
//我们利用原始缓冲区作为对齐缓冲区直接写入
//没有缓冲的文件。
//不需要将提供的缓冲区与物理缓冲区对齐。
//扇区大小（SSD页面大小）和
//所有的setFilePointer（）操作都将与这样的对齐方式一起发生。
//因此，我们总是以扇区/页面大小增量写入驱动器，然后离开
//下一次写入或close（）的尾部，此时我们用零填充。
//无需填充
//缓冲访问。
class WinWritableImpl {
 protected:
  WinFileData* file_data_;
  const uint64_t alignment_;
uint64_t next_write_offset_; //需要，因为Windows不支持o_append
uint64_t reservedsize_;  //我们预留了多少空间

  virtual Status PreallocateInternal(uint64_t spaceToReserve);

  WinWritableImpl(WinFileData* file_data, size_t alignment);

  ~WinWritableImpl() {}

  uint64_t GetAlignement() const { return alignment_; }

  Status AppendImpl(const Slice& data);

//要求按指定的方式对齐数据
//GetRequiredBufferAlignment（）。
  Status PositionedAppendImpl(const Slice& data, uint64_t offset);

  Status TruncateImpl(uint64_t size);

  Status CloseImpl();

  Status SyncImpl();

  uint64_t GetFileNextWriteOffset() {
//现在在这里用书面文件编写器进行双重核算
//当使用非缓冲访问时，这个大小将是错误的
//但是测试实现了它们自己的可写文件，并且不使用
//可写文件包装
//所以我们需要把一个方形的木钉穿过
//这里有个圆孔。
    return next_write_offset_;
  }

  Status AllocateImpl(uint64_t offset, uint64_t len);

 public:
  WinWritableImpl(const WinWritableImpl&) = delete;
  WinWritableImpl& operator=(const WinWritableImpl&) = delete;
};

class WinWritableFile : private WinFileData,
                        protected WinWritableImpl,
                        public WritableFile {
 public:
  WinWritableFile(const std::string& fname, HANDLE hFile, size_t alignment,
                  size_t capacity, const EnvOptions& options);

  ~WinWritableFile();

  virtual Status Append(const Slice& data) override;

//要求按指定的方式对齐数据
//GetRequiredBufferAlignment（）。
  virtual Status PositionedAppend(const Slice& data, uint64_t offset) override;

//需要实现此功能，以便正确截断文件
//缓冲和非缓冲模式时
  virtual Status Truncate(uint64_t size) override;

  virtual Status Close() override;

//将缓存数据写入OS缓存
//现在由可写文件编写器负责
  virtual Status Flush() override;

  virtual Status Sync() override;

  virtual Status Fsync() override;

//指示类是否使用直接I/O
//使用职位申请
  virtual bool use_direct_io() const override;

  virtual size_t GetRequiredBufferAlignment() const override;

  virtual uint64_t GetFileSize() override;

  virtual Status Allocate(uint64_t offset, uint64_t len) override;

  virtual size_t GetUniqueId(char* id, size_t max_size) const override;
};

class WinRandomRWFile : private WinFileData,
                        protected WinRandomAccessImpl,
                        protected WinWritableImpl,
                        public RandomRWFile {
 public:
  WinRandomRWFile(const std::string& fname, HANDLE hFile, size_t alignment,
                  const EnvOptions& options);

  ~WinRandomRWFile() {}

//指示类是否使用直接I/O
//如果为false，则必须将对齐的缓冲区传递给write（）。
  virtual bool use_direct_io() const override;

//使用返回的对齐值分配对齐
//当使用_direct_io（）返回true时，写入（）的缓冲区
  virtual size_t GetRequiredBufferAlignment() const override;

//在“offset”处的“data”中写入字节，成功时返回status:：ok（）。
//当使用_Direct_IO（）返回true时，传递对齐的缓冲区。
  virtual Status Write(uint64_t offset, const Slice& data) override;

//从offset'offset'开始读取最多'n'个字节，并将它们存储在
//结果，提供的“scratch”大小至少应为“n”。
//返回成功时的状态：：OK（）。
  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const override;

  virtual Status Flush() override;

  virtual Status Sync() override;

  virtual Status Fsync() { return Sync(); }

  virtual Status Close() override;
};

class WinDirectory : public Directory {
 public:
  WinDirectory() {}

  virtual Status Fsync() override;
};

class WinFileLock : public FileLock {
 public:
  explicit WinFileLock(HANDLE hFile) : hFile_(hFile) {
    assert(hFile != NULL);
    assert(hFile != INVALID_HANDLE_VALUE);
  }

  ~WinFileLock();

 private:
  HANDLE hFile_;
};
}
}
