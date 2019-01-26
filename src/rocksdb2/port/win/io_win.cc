
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

#include "port/win/io_win.h"

#include "monitoring/iostats_context_imp.h"
#include "util/aligned_buffer.h"
#include "util/coding.h"
#include "util/sync_point.h"

namespace rocksdb {
namespace port {

/*
*直接助手
**/

namespace {

const size_t kSectorSize = 512;

inline
bool IsPowerOfTwo(const size_t alignment) {
  return ((alignment) & (alignment - 1)) == 0;
}

inline
bool IsSectorAligned(const size_t off) { 
  return (off & (kSectorSize - 1)) == 0;
}

inline
bool IsAligned(size_t alignment, const void* ptr) {
  return ((uintptr_t(ptr)) & (alignment - 1)) == 0;
}
}


std::string GetWindowsErrSz(DWORD err) {
  LPSTR lpMsgBuf;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, err,
0,  //默认语言
    reinterpret_cast<LPSTR>(&lpMsgBuf), 0, NULL);

  std::string Err = lpMsgBuf;
  LocalFree(lpMsgBuf);
  return Err;
}

//我们保留这个接口的原始名称来表示原始想法
//在它后面。
//所有读取都由指定的偏移量进行，并且pwrite接口不会更改
//文件指针的位置。从手册上判断，是的。
//执行
//lseek自动返回文件的位置。
//WriteFile（）没有
//具备这种能力。因此，对于pread和pwrite，指针是
//前进到下一个位置
//这对于写来说是很好的，因为它们是（应该）连续的。
//因为所有的读/写都是按指定的偏移量进行的，所以调用者
//理论不应该
//依赖于当前文件偏移量。
SSIZE_T pwrite(HANDLE hFile, const char* src, size_t numBytes,
  uint64_t offset) {
  assert(numBytes <= std::numeric_limits<DWORD>::max());
  OVERLAPPED overlapped = { 0 };
  ULARGE_INTEGER offsetUnion;
  offsetUnion.QuadPart = offset;

  overlapped.Offset = offsetUnion.LowPart;
  overlapped.OffsetHigh = offsetUnion.HighPart;

  SSIZE_T result = 0;

  unsigned long bytesWritten = 0;

  if (FALSE == WriteFile(hFile, src, static_cast<DWORD>(numBytes), &bytesWritten,
    &overlapped)) {
    result = -1;
  } else {
    result = bytesWritten;
  }

  return result;
}

//请参阅上面的pwrite注释
SSIZE_T pread(HANDLE hFile, char* src, size_t numBytes, uint64_t offset) {
  assert(numBytes <= std::numeric_limits<DWORD>::max());
  OVERLAPPED overlapped = { 0 };
  ULARGE_INTEGER offsetUnion;
  offsetUnion.QuadPart = offset;

  overlapped.Offset = offsetUnion.LowPart;
  overlapped.OffsetHigh = offsetUnion.HighPart;

  SSIZE_T result = 0;

  unsigned long bytesRead = 0;

  if (FALSE == ReadFile(hFile, src, static_cast<DWORD>(numBytes), &bytesRead,
    &overlapped)) {
    return -1;
  } else {
    result = bytesRead;
  }

  return result;
}

//setFileInformationByHandle（）能够快速预分配。
//但是，除非文件
//截断和预先分配的空间不视为用零填充。
Status fallocate(const std::string& filename, HANDLE hFile,
  uint64_t to_size) {
  Status status;

  FILE_ALLOCATION_INFO alloc_info;
  alloc_info.AllocationSize.QuadPart = to_size;

  if (!SetFileInformationByHandle(hFile, FileAllocationInfo, &alloc_info,
    sizeof(FILE_ALLOCATION_INFO))) {
    auto lastError = GetLastError();
    status = IOErrorFromWindowsError(
      "Failed to pre-allocate space: " + filename, lastError);
  }

  return status;
}

Status ftruncate(const std::string& filename, HANDLE hFile,
  uint64_t toSize) {
  Status status;

  FILE_END_OF_FILE_INFO end_of_file;
  end_of_file.EndOfFile.QuadPart = toSize;

  if (!SetFileInformationByHandle(hFile, FileEndOfFileInfo, &end_of_file,
    sizeof(FILE_END_OF_FILE_INFO))) {
    auto lastError = GetLastError();
    status = IOErrorFromWindowsError("Failed to Set end of file: " + filename,
      lastError);
  }

  return status;
}

size_t GetUniqueIdFromFile(HANDLE hFile, char* id, size_t max_size) {

  if (max_size < kMaxVarint64Length * 3) {
    return 0;
  }

//此功能必须在以下情况下重新工作：
//使用Windows Server 2012上引入的refs文件系统
  BY_HANDLE_FILE_INFORMATION FileInfo;

  BOOL result = GetFileInformationByHandle(hFile, &FileInfo);

  TEST_SYNC_POINT_CALLBACK("GetUniqueIdFromFile:FS_IOC_GETVERSION", &result);

  if (!result) {
    return 0;
  }

  char* rid = id;
  rid = EncodeVarint64(rid, uint64_t(FileInfo.dwVolumeSerialNumber));
  rid = EncodeVarint64(rid, uint64_t(FileInfo.nFileIndexHigh));
  rid = EncodeVarint64(rid, uint64_t(FileInfo.nFileIndexLow));

  assert(rid >= id);
  return static_cast<size_t>(rid - id);
}

////////////////////////////////////////////////////
//winmmapreadable文件

WinMmapReadableFile::WinMmapReadableFile(const std::string& fileName,
                                         HANDLE hFile, HANDLE hMap,
                                         const void* mapped_region,
                                         size_t length)
    /*InfileData（文件名，hfile，false/*使用\u direct \u io*/），
      HMAPG（HMAP），
      映射的区域（映射的区域）
      长度

winmmapeadablefile:：~winmmapeadablefile（）
  bool ret=：：unmapviewoffile（mapped_region_u）；
  （无效）RET；
  断言（RET）；

  ret=：：闭合手柄（hmap_uu）；
  断言（RET）；
}

状态winmmapreadablefile:：read（uint64_t offset，size_t n，slice*result，
  char*scratch）常量
  状态S；

  如果（偏移>长度_
    *结果=slice（）；
    返回ioerror（文件名，einval）；
  else if（offset+n>length_
    n=长度偏移量；
  }
  ＊结果＝
    slice（reinterpret_cast<const char*>（mapped_region_uux）+offset，n）；
  返回S；
}

状态winmmapreadablefile:：invalidCache（大小偏移量，大小长度）
  返回状态：：OK（）；
}

size_t winmmapreadablefile:：getuniqueid（char*id，size_t max_size）const_
  返回getuniqueidfromfile（hfile_uuid，max_size）；
}

////////////////////////////////////////////////
///WiMMAP文件


//只有在以下情况下才能截断或保留到对齐的扇区大小
//用于使用未缓冲的I/O打开的文件
状态winmmapfile:：truncatefile（uint64_t tosize）
  返回ftruncate（文件名_uuhfile_uutosize）；
}

状态winmmapfile:：unmapcurrenterregion（）
  现状；

  如果（映射开始）= null pTr）{
    如果（！）：：unmapviewoffile（mapped_begin_uu））
      状态=IOErrorFromWindowsError（
        “无法取消映射文件视图：”+FileName_uuGetLastError（））；
    }

    //转到文件的下一部分
    文件\偏移量\+=视图\大小\；

    //unmapview自动将数据发送到磁盘，而不是元数据
    //这很好，在Linux上提供了相当于fdatasync（）的功能
    //因此，元数据不需要单独的标志
    映射的_begin_=nullptr；
    映射的“结束”=nullptr；
    dSTL= null pTr；

    上次同步时间
    挂起的同步错误；
  }

  返回状态；
}

状态winmmapfile:：mapNewRegion（）

  现状；

  断言（mapped_begin_==nullptr）；

  size_t mindisksize=文件\u偏移量\查看\u大小；

  如果（mindisksize>保留的尺寸）
    状态=分配（文件\偏移量\，视图\大小\）；
    如果（！）状态，OK（））{
      返回状态；
    }
  }

  //需要重新映射
  if（hmap_==null reserved_size_u>mapping_size_

    如果（HAMAPXI）！{NULL）{
      //取消前一个的映射
      bool ret=：关闭手柄（hmap_uu）；
      断言（RET）；
      HAMAPG=空；
    }

    整数映射大小；
    mappingSize.quadpart=保留的\u大小\；

    hmap_uu=createfilemappinga（
      希菲尔
      空，//安全属性
      page_readwrite，//没有映射的只写模式
      mappingsize.highpart，//启用映射整个文件，但实际
      //映射的数量由mapviewoffile确定
      映射大小.lowpart，
      空）；//映射名称

    如果（空==hmap_
      返回IOErrorFromWindowsError（
        “windowsmmapfile未能为：”+文件名“，
        getLastError（））；
    }

    映射\大小\=保留\大小\；
  }

  Ularge_整数偏移量；
  offset.quadpart=文件\偏移量\；

  //视图必须从粒度对齐的偏移量开始
  mapped_begin_=reinterpret_cast<char*>（）
    mapviewoffileex（hmap_u，file_map_write，offset.highpart，offset.lowpart，
    查看“大小”（空））；

  如果（！）地图开始
    状态=IOErrorFromWindowsError（
      “windowsmmapfile未能映射文件视图：”+文件名“，
      getLastError（））；
  }否则{
    映射的\结束的\映射的\开始的\视图的\大小的\；
    DST_uu=映射的_u begin_uu；
    最后一次同步\映射\开始\；
    挂起的同步错误；
  }
  返回状态；
}

状态winmmapfile:：preallocateinternal（uint64_t spacetoreserver）
  返回fallocate（文件名_uuhfile_uuspacetoreserver）；
}

winmmapfile:：winmmapfile（const std:：string&fname，handle hfile，size_t page_size，
  大小分配粒度、常量环境选项和选项）
  ：winfiledata（fname、hfile、false）
  HAMAPZ（NULL），
  页面尺寸（页面尺寸）
  分配粒度（分配粒度）
  预留尺寸（0）
  映射尺寸（0）
  VIEVION SIZEZ（0）
  映射的_begin_u（nullptr），
  映射的“结束”（nullptr），
  dSTLZ（null pTr），
  最后一次同步（nullptr）
  文件偏移量（0）
  挂起“同步”（假）
  //分配粒度必须从getSystemInfo（）获取，并且必须
  //二次幂。
  断言（分配粒度>0）；
  断言（（分配粒度和（分配粒度-1））=0）；

  断言（页面大小>0）；
  断言（（页大小和（页大小-1））=0）；

  //仅用于内存映射写入
  断言（选项。使用命令写入）；

  //视图大小必须是分配粒度和
  //页面大小和粒度通常是页面大小的倍数。
  const size_t viewsize=32*1024；//32kb，类似于缓冲模式下的Windows文件缓存
  View_Size_uuu=Roundup（视图大小，分配粒度）
}

winmmapfile:：~winmmapfile（）
  如果（HFLIEEZ）{
    这个->闭包（）；
  }
}

状态winmmapfile:：append（const slice&data）
  const char*src=data.data（）；
  size_t left=data.size（）；

  同时（左>0）
    断言（映射的_Begin_<=dst_u）；
    size_t avail=映射的_end_uudst_uu；

    如果（可用性=0）
      状态s=unmapcurrentRegion（）；
      如果（S.O.（））{
        s=mapNewRegion（）；
      }

      如果（！）S.O.（））{
        返回S；
      }
    }否则{
      尺寸_t n=std:：min（左，可用）；
      memcpy（dst_uu，src，n）；
      dSTy++n；
      SRC+= n；
      左-n；
      挂起的“同步”为真；
    }
  }

  //现在，如果需要，请确保最后一个部分页用零填充。
  size_t bytestopad=roundup（size_t（dst_u），page_size_u）-size_t（dst_u）；
  如果（bytestopad>0）
    memset（dst_u0，bytestopad）；
  }

  返回状态：：OK（）；
}

//表示close（）将正确处理truncate
//不需要任何附加信息
状态winmmapfile:：truncate（uint64_t size）
  返回状态：：OK（）；
}

状态winmmapfile:：close（）
  状态S；

  声明（null）！= HFLIEEZ）；

  //我们截断到精确的大小，所以不
  //结尾处的数据未初始化。设置文件结束标志
  //我们使用的不写零，它是好的。
  uint64_t targetsize=getfilesize（）；

  如果（映射开始）= null pTr）{
    //取消映射前同步以确保所有内容
    //在磁盘上，没有延迟写入
    //所以我们对测试具有确定性
    同步（）；
    s=unmapcurrentRegion（）；
  }

  如果（NULL！{HMAP}）
    bool ret=：关闭手柄（hmap_uu）；
    如果（！）ret&&s.ok（））
      Auto LastError=GetLastError（）；
      s=ioErrorFromWindowsError（
        “无法关闭文件的映射：”+文件名_uuLastError）；
    }

    HAMAPG=空；
  }

  如果（HFieLi）！{NULL）{

    截断文件（targetsize）；

    bool ret=：关闭手柄（hfile_uuu）；
    HFLIEZ=空；

    如果（！）ret&&s.ok（））
      Auto LastError=GetLastError（）；
      s=ioErrorFromWindowsError（
        “关闭文件映射句柄失败：”+filename_uuLastError）；
    }
  }

  返回S；
}

状态winmmapfile:：flush（）返回状态：：ok（）；

//仅刷新数据
状态winmmapfile:：sync（）
  状态S；

  //自上次同步后发生了一些写入操作
  如果（dst_u>last_sync_123;
    断言（映射的开始）；
    断言（DSTIZ）；
    断言（dst_u>mapped_u begin_uuu）；
    断言（dst_u<mapped_u end_uu）；

    大小页面开始=
      截断页面边界（页面大小，最后一个同步映射开始）；
    大小页面结束=
      截断页面边界（页面大小、DST映射的开始位置）；

    //仅刷新多个页面的数量
    如果（！）：：flushviewoffile（映射的_begin_uu+page_begin，
      （第_页结束-第_页开始）+第_页大小
      s=ioErrorFromWindowsError（“未能刷新ViewOfFile：”+文件名“，
        getLastError（））；
    }否则{
      最后一次同步
    }
  }

  返回S；
}

/**
*将数据和元数据刷新到稳定存储。
**/

Status WinMmapFile::Fsync() {
  Status s = Sync();

//刷新元数据
  if (s.ok() && pending_sync_) {
    if (!::FlushFileBuffers(hFile_)) {
      s = IOErrorFromWindowsError("Failed to FlushFileBuffers: " + filename_,
        GetLastError());
    }
    pending_sync_ = false;
  }

  return s;
}

/*
*获取文件中有效数据的大小。这与
*文件系统返回的大小，因为我们使用mmap
*每次按地图大小扩展文件。
**/

uint64_t WinMmapFile::GetFileSize() {
  size_t used = dst_ - mapped_begin_;
  return file_offset_ + used;
}

Status WinMmapFile::InvalidateCache(size_t offset, size_t length) {
  return Status::OK();
}

Status WinMmapFile::Allocate(uint64_t offset, uint64_t len) {
  Status status;
  TEST_KILL_RANDOM("WinMmapFile::Allocate", rocksdb_kill_odds);

//确保保留对齐的空间量
//因为预订块的大小是由外部驱动的，所以我们需要
//检查一下我们这里的预订是否可以
  size_t spaceToReserve = Roundup(offset + len, view_size_);
//无事可做
  if (spaceToReserve <= reserved_size_) {
    return status;
  }

  IOSTATS_TIMER_GUARD(allocate_nanos);
  status = PreallocateInternal(spaceToReserve);
  if (status.ok()) {
    reserved_size_ = spaceToReserve;
  }
  return status;
}

size_t WinMmapFile::GetUniqueId(char* id, size_t max_size) const {
  return GetUniqueIdFromFile(hFile_, id, max_size);
}

////////////////////////////////////////////////////
//WinSequentialFile（序列文件）

WinSequentialFile::WinSequentialFile(const std::string& fname, HANDLE f,
                                     const EnvOptions& options)
    : WinFileData(fname, f, options.use_direct_reads) {}

WinSequentialFile::~WinSequentialFile() {
  assert(hFile_ != INVALID_HANDLE_VALUE);
}

Status WinSequentialFile::Read(size_t n, Slice* result, char* scratch) {
  assert(result != nullptr && !WinFileData::use_direct_io());
  Status s;
  size_t r = 0;

//Windows ReadFile API接受一个双字。
//如果n大于uint_max，则可以在循环中读取
//这是一个极不可能的情况。
  if (n > UINT_MAX) {
    return IOErrorFromWindowsError(filename_, ERROR_INVALID_PARAMETER);
  }

DWORD bytesToRead = static_cast<DWORD>(n); //由于上述检查，铸件是安全的。
  DWORD bytesRead = 0;
  BOOL ret = ReadFile(hFile_, scratch, bytesToRead, &bytesRead, NULL);
  if (ret == TRUE) {
    r = bytesRead;
  } else {
    return IOErrorFromWindowsError(filename_, GetLastError());
  }

  *result = Slice(scratch, r);

  return s;
}

SSIZE_T WinSequentialFile::PositionedReadInternal(char* src, size_t numBytes,
  uint64_t offset) const {
  return pread(GetFileHandle(), src, numBytes, offset);
}

Status WinSequentialFile::PositionedRead(uint64_t offset, size_t n, Slice* result,
  char* scratch) {

  Status s;

  assert(WinFileData::use_direct_io());

//Windows ReadFile API接受一个双字。
//如果n大于uint_max，则可以在循环中读取
//这是一个极不可能的情况。
  if (n > UINT_MAX) {
    return IOErrorFromWindowsError(GetName(), ERROR_INVALID_PARAMETER);
  }

  auto r = PositionedReadInternal(scratch, n, offset);

  if (r < 0) {
    auto lastError = GetLastError();
//posix impl希望处理来自外部的读取
//文件正常。
    if (lastError != ERROR_HANDLE_EOF) {
      s = IOErrorFromWindowsError(GetName(), lastError);
    }
  }

  *result = Slice(scratch, (r < 0) ? 0 : size_t(r));
  return s;
}


Status WinSequentialFile::Skip(uint64_t n) {
//由于setfilepointerx接受带符号的64位，无法处理超过带符号的max
//整数。因此，没有这么大的数目是非常不光彩的。
  if (n > _I64_MAX) {
    return IOErrorFromWindowsError(filename_, ERROR_INVALID_PARAMETER);
  }

  LARGE_INTEGER li;
li.QuadPart = static_cast<int64_t>(n); //由于上述检查，铸件是安全的。
  BOOL ret = SetFilePointerEx(hFile_, li, NULL, FILE_CURRENT);
  if (ret == FALSE) {
    return IOErrorFromWindowsError(filename_, GetLastError());
  }
  return Status::OK();
}

Status WinSequentialFile::InvalidateCache(size_t offset, size_t length) {
  return Status::OK();
}

//////////////////////////////////////////////////
///winrandomaccessbase

inline
SSIZE_T WinRandomAccessImpl::PositionedReadInternal(char* src,
  size_t numBytes,
  uint64_t offset) const {
  return pread(file_base_->GetFileHandle(), src, numBytes, offset);
}

inline
WinRandomAccessImpl::WinRandomAccessImpl(WinFileData* file_base,
  size_t alignment,
  const EnvOptions& options) :
    file_base_(file_base),
    alignment_(alignment) {

  assert(!options.use_mmap_reads);
}

inline
Status WinRandomAccessImpl::ReadImpl(uint64_t offset, size_t n, Slice* result,
  char* scratch) const {

  Status s;

//检查缓冲器对齐
  if (file_base_->use_direct_io()) {
    if (!IsAligned(alignment_, scratch)) {
      return Status::InvalidArgument("WinRandomAccessImpl::ReadImpl: scratch is not properly aligned");
    }
  }

  if (n == 0) {
    *result = Slice(scratch, 0);
    return s;
  }

  size_t left = n;
  char* dest = scratch;

  SSIZE_T r = PositionedReadInternal(scratch, left, offset);
  if (r > 0) {
    left -= r;
  } else if (r < 0) {
    auto lastError = GetLastError();
//posix impl希望处理来自外部的读取
//文件正常。
    if(lastError != ERROR_HANDLE_EOF) {
      s = IOErrorFromWindowsError(file_base_->GetName(), lastError);
    }
  }

  *result = Slice(scratch, (r < 0) ? 0 : n - left);

  return s;
}

////////////////////////////////////////////////////
///winrandomaccessfile

WinRandomAccessFile::WinRandomAccessFile(const std::string& fname, HANDLE hFile,
                                         size_t alignment,
                                         const EnvOptions& options)
    : WinFileData(fname, hFile, options.use_direct_reads),
      WinRandomAccessImpl(this, alignment, options) {}

WinRandomAccessFile::~WinRandomAccessFile() {
}

Status WinRandomAccessFile::Read(uint64_t offset, size_t n, Slice* result,
  char* scratch) const {
  return ReadImpl(offset, n, result, scratch);
}

Status WinRandomAccessFile::InvalidateCache(size_t offset, size_t length) {
  return Status::OK();
}

size_t WinRandomAccessFile::GetUniqueId(char* id, size_t max_size) const {
  return GetUniqueIdFromFile(GetFileHandle(), id, max_size);
}

size_t WinRandomAccessFile::GetRequiredBufferAlignment() const {
  return GetAlignment();
}

//////////////////////////////////////////////////
//WiWrrababl IMPL
//

inline
Status WinWritableImpl::PreallocateInternal(uint64_t spaceToReserve) {
  return fallocate(file_data_->GetName(), file_data_->GetFileHandle(), spaceToReserve);
}

inline
WinWritableImpl::WinWritableImpl(WinFileData* file_data, size_t alignment)
  : file_data_(file_data),
  alignment_(alignment),
  next_write_offset_(0),
  reservedsize_(0) {

//调用reopenwritablefile时查询当前位置
//此位置仅对缓冲写入很重要
//对于未缓冲的写入，我们明确指定位置。
  LARGE_INTEGER zero_move;
zero_move.QuadPart = 0; //不动
  LARGE_INTEGER pos;
  pos.QuadPart = 0;
  BOOL ret = SetFilePointerEx(file_data_->GetFileHandle(), zero_move, &pos,
      FILE_CURRENT);
//查询不支持失败
  if (ret) {
    next_write_offset_ = pos.QuadPart;
  } else {
    assert(false);
  }
}

inline
Status WinWritableImpl::AppendImpl(const Slice& data) {

  Status s;

  assert(data.size() < std::numeric_limits<DWORD>::max());

  uint64_t written = 0;
  (void)written;

  if (file_data_->use_direct_io()) {

//没有指定偏移量，我们将追加
//到文件结尾

    assert(IsSectorAligned(next_write_offset_));
    assert(IsSectorAligned(data.size()));
    assert(IsAligned(GetAlignement(), data.data()));

    SSIZE_T ret = pwrite(file_data_->GetFileHandle(), data.data(),
     data.size(), next_write_offset_);

    if (ret < 0) {
      auto lastError = GetLastError();
      s = IOErrorFromWindowsError(
        "Failed to pwrite for: " + file_data_->GetName(), lastError);
    }
    else {
      written = ret;
    }

  } else {

    DWORD bytesWritten = 0;
    if (!WriteFile(file_data_->GetFileHandle(), data.data(),
      static_cast<DWORD>(data.size()), &bytesWritten, NULL)) {
      auto lastError = GetLastError();
      s = IOErrorFromWindowsError(
        "Failed to WriteFile: " + file_data_->GetName(),
        lastError);
    }
    else {
      written = bytesWritten;
    }
  }

  if(s.ok()) {
    assert(written == data.size());
    next_write_offset_ += data.size();
  }

  return s;
}

inline
Status WinWritableImpl::PositionedAppendImpl(const Slice& data, uint64_t offset) {

  if(file_data_->use_direct_io()) {
    assert(IsSectorAligned(offset));
    assert(IsSectorAligned(data.size()));
    assert(IsAligned(GetAlignement(), data.data()));
  }

  Status s;

  SSIZE_T ret = pwrite(file_data_->GetFileHandle(), data.data(), data.size(), offset);

//错误中断
  if (ret < 0) {
    auto lastError = GetLastError();
    s = IOErrorFromWindowsError(
      "Failed to pwrite for: " + file_data_->GetName(), lastError);
  }
  else {
    assert(size_t(ret) == data.size());
//对于顺序写入，这很简单
//按数据调整扩展名的大小。大小（）
    uint64_t write_end = offset + data.size();
    if (write_end >= next_write_offset_) {
      next_write_offset_ = write_end;
    }
  }
  return s;
}

//需要实现此功能，以便正确截断文件
//缓冲和非缓冲模式时
inline
Status WinWritableImpl::TruncateImpl(uint64_t size) {
  Status s = ftruncate(file_data_->GetName(), file_data_->GetFileHandle(),
    size);
  if (s.ok()) {
    next_write_offset_ = size;
  }
  return s;
}

inline
Status WinWritableImpl::CloseImpl() {

  Status s;

  auto hFile = file_data_->GetFileHandle();
  assert(INVALID_HANDLE_VALUE != hFile);

  if (fsync(hFile) < 0) {
    auto lastError = GetLastError();
    s = IOErrorFromWindowsError("fsync failed at Close() for: " +
      file_data_->GetName(),
      lastError);
  }

  if(!file_data_->CloseFile()) {
    auto lastError = GetLastError();
    s = IOErrorFromWindowsError("CloseHandle failed for: " + file_data_->GetName(),
      lastError);
  }
  return s;
}

inline
Status WinWritableImpl::SyncImpl() {
  Status s;
//调用刷新缓冲区
  if (fsync(file_data_->GetFileHandle()) < 0) {
    auto lastError = GetLastError();
    s = IOErrorFromWindowsError(
        "fsync failed at Sync() for: " + file_data_->GetName(), lastError);
  }
  return s;
}


inline
Status WinWritableImpl::AllocateImpl(uint64_t offset, uint64_t len) {
  Status status;
  TEST_KILL_RANDOM("WinWritableFile::Allocate", rocksdb_kill_odds);

//确保保留对齐的空间量
//因为预订块的大小是由外部驱动的，所以我们需要
//检查一下我们这里的预订是否可以
  size_t spaceToReserve = Roundup(offset + len, alignment_);
//无事可做
  if (spaceToReserve <= reservedsize_) {
    return status;
  }

  IOSTATS_TIMER_GUARD(allocate_nanos);
  status = PreallocateInternal(spaceToReserve);
  if (status.ok()) {
    reservedsize_ = spaceToReserve;
  }
  return status;
}


//////////////////////////////////////////////////
///winwritable文件

WinWritableFile::WinWritableFile(const std::string& fname, HANDLE hFile,
                                 /*对齐，大小，容量
                                 常量环境选项）
    ：winfiledata（fname，hfile，选项。使用直接写入）
      WinWritableImpl（此，对齐）
  断言（！）选项。使用“mmap”写入）；
}

winwritablefile:：~winwritablefile（）
}

//指示类是否使用直接I/O
bool winwritablefile:：use_direct_io（）const return winfiledata:：use_direct_io（）；

大小\u t WinWritableFile:：GetRequiredBufferAlignment（）const_
  返回getAlignment（）；
}

状态winwritablefile:：append（const slice&data）
  返回appendimpl（数据）；
}

状态winwritablefile:：positionedappend（const slice&data，uint64_t offset）
  返回positionedAppendImpl（数据，偏移量）；
}

//需要实现此函数，以便正确截断文件
//缓冲和非缓冲模式时
状态winwritablefile:：truncate（uint64_t size）
  返回truncateimpl（大小）；
}

状态winwritablefile:：close（）
  返回closeImpl（）；
}

  //将缓存数据写入OS缓存
  //现在由可写文件编写器负责
状态winwritablefile:：flush（）
  返回状态：：OK（）；
}

状态winwritablefile:：sync（）
  返回syncimpl（）；
}

状态winwritablefile:：fsync（）返回syncimpl（）；

uint64_t winwritablefile:：getfileSize（）
  返回getFileNextWriteOffset（）；
}

状态winwritablefile:：allocate（uint64_t offset，uint64_t len）
  返回allocateimpl（offset，len）；
}

size_t winwritablefile:：getuniqueid（char*id，size_t max_size）const_
  返回getuniqueidfromfile（getfilehandle（），id，max_-size）；
}

//////////////////////////////////////////////
///winrandomrwfile

winrandomrwfile:：winrandomrwfile（const std:：string&fname，handle hfile，
                                 大小对齐、常量环境选项和选项）
    ：winfiledata（fname，hfile，
                  选项。使用“直接读取”和“选项”。使用“直接写入”，
      WinRandomAccessImpl（此，对齐，选项）
      winwritableimpl（this，Alignment）

bool winrandomrwfile:：use_direct_io（）const return winfiledata:：use_direct_io（）；

size_t winrandomrwfile:：getRequiredBufferAlignment（）常量
  返回getAlignment（）；
}

状态winrandomrwfile:：write（uint64_t offset，const slice&data）
  返回positionedAppendImpl（数据，偏移量）；
}

状态winrandomrwfile:：read（uint64_t offset，size_t n，slice*result，
                             char*scratch）常量
  返回readimpl（offset，n，result，scratch）；
}

状态winrandomrwfile:：flush（）
  返回状态：：OK（）；
}

状态winrandomrwfile:：sync（）
  返回syncimpl（）；
}

状态winrandomrwfile:：close（）
  返回closeImpl（）；
}

////////////////////////////////////////////////
///风目录

状态windirectory:：fsync（）返回状态：：ok（）；

////////////////////////////////////////////////
///WiFieleLoCox

winfilelock:：~winfilelock（）
  bool ret=：关闭手柄（hfile_uuu）；
  断言（RET）；
}

}
}
