
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

#include "util/file_reader_writer.h"

#include <algorithm>
#include <mutex>

#include "monitoring/histogram.h"
#include "monitoring/iostats_context_imp.h"
#include "port/port.h"
#include "util/random.h"
#include "util/rate_limiter.h"
#include "util/sync_point.h"

namespace rocksdb {

#ifndef NDEBUG
namespace {
bool IsFileSectorAligned(const size_t off, size_t sector_size) {
  return off % sector_size == 0;
}
}
#endif

Status SequentialFileReader::Read(size_t n, Slice* result, char* scratch) {
  Status s;
  if (use_direct_io()) {
#ifndef ROCKSDB_LITE
    size_t offset = offset_.fetch_add(n);
    size_t alignment = file_->GetRequiredBufferAlignment();
    size_t aligned_offset = TruncateToPageBoundary(alignment, offset);
    size_t offset_advance = offset - aligned_offset;
    size_t size = Roundup(offset + n, alignment) - aligned_offset;
    size_t r = 0;
    AlignedBuffer buf;
    buf.Alignment(alignment);
    buf.AllocateNewBuffer(size);
    Slice tmp;
    s = file_->PositionedRead(aligned_offset, size, &tmp, buf.BufferStart());
    if (s.ok() && offset_advance < tmp.size()) {
      buf.Size(tmp.size());
      r = buf.Read(scratch, offset_advance,
                   std::min(tmp.size() - offset_advance, n));
    }
    *result = Slice(scratch, r);
#endif  //！摇滚乐
  } else {
    s = file_->Read(n, result, scratch);
  }
  IOSTATS_ADD(bytes_read, result->size());
  return s;
}


Status SequentialFileReader::Skip(uint64_t n) {
#ifndef ROCKSDB_LITE
  if (use_direct_io()) {
    offset_ += n;
    return Status::OK();
  }
#endif  //！摇滚乐
  return file_->Skip(n);
}

Status RandomAccessFileReader::Read(uint64_t offset, size_t n, Slice* result,
                                    char* scratch) const {
  Status s;
  uint64_t elapsed = 0;
  {
    StopWatch sw(env_, stats_, hist_type_,
                 (stats_ != nullptr) ? &elapsed : nullptr);
    IOSTATS_TIMER_GUARD(read_nanos);
    if (use_direct_io()) {
#ifndef ROCKSDB_LITE
      size_t alignment = file_->GetRequiredBufferAlignment();
      size_t aligned_offset = TruncateToPageBoundary(alignment, offset);
      size_t offset_advance = offset - aligned_offset;
      size_t read_size = Roundup(offset + n, alignment) - aligned_offset;
      AlignedBuffer buf;
      buf.Alignment(alignment);
      buf.AllocateNewBuffer(read_size);
      while (buf.CurrentSize() < read_size) {
        size_t allowed;
        if (rate_limiter_ != nullptr) {
          allowed = rate_limiter_->RequestToken(
              buf.Capacity() - buf.CurrentSize(), buf.Alignment(),
              Env::IOPriority::IO_LOW, stats_, RateLimiter::OpType::kRead);
        } else {
          assert(buf.CurrentSize() == 0);
          allowed = read_size;
        }
        Slice tmp;
        s = file_->Read(aligned_offset + buf.CurrentSize(), allowed, &tmp,
                        buf.Destination());
        buf.Size(buf.CurrentSize() + tmp.size());
        if (!s.ok() || tmp.size() < allowed) {
          break;
        }
      }
      size_t res_len = 0;
      if (s.ok() && offset_advance < buf.CurrentSize()) {
        res_len = buf.Read(scratch, offset_advance,
                           std::min(buf.CurrentSize() - offset_advance, n));
      }
      *result = Slice(scratch, res_len);
#endif  //！摇滚乐
    } else {
      size_t pos = 0;
      const char* res_scratch = nullptr;
      while (pos < n) {
        size_t allowed;
        if (for_compaction_ && rate_limiter_ != nullptr) {
          /*欠=费率限制器->requesttoken（n-位置，0/*对齐方式*/，
                                                env:：iopriority:：io_low，stats_
                                                ratelimiter:：optype:：kread）；
        }否则{
          允许＝n；
        }
        切片tmp_结果；
        s=文件_u->read（offset+pos，allowed，&tmp_result，scratch+pos）；
        if（res_scratch==nullptr）
          //我们不能简单地使用“scratch”，因为mmap文件的读取返回
          //其他缓冲区中的数据。
          res_scratch=tmp_result.data（）；
        }否则{
          //确保块连续插入到“res-scratch”中。
          断言（tmp_result.data（）==res_scratch+pos）；
        }
        pos+=tmp_result.size（）；
        如果（！）s.ok（）tmp_result.size（）<allowed）
          断裂；
        }
      }
      *结果=切片（Res-Scratch，S.OK（）？POS：0）；
    }
    iostats_add_if_positive（bytes_read，result->size（））；
  }
  如果（STATSY）！=nullptr&&文件读取历史记录！= null pTr）{
    文件读取历史记录添加（经过）；
  }
  返回S；
}

状态可写文件编写器：：append（const slice&data）
  const char*src=data.data（）；
  size_t left=data.size（）；
  状态S；
  挂起的“同步”为真；

  test_kill_random（“writablefilewriter:：append:0”，
                   Rocksdb_杀掉_几率*减少_几率2）；

  {
    iostats计时器保护（准备写入纳米）；
    测试同步点（“writablefilewriter:：append:beforePrepareWrite”）；
    可写的_file_u->PrepareWrite（static_cast<size_t>（getfileSize（）），左侧）；
  }

  //看是否需要扩大缓冲区以避免刷新
  if（buf_u.capacity（）-buf_u.currentsize（）<left）
    对于（size_t cap=buf_u.capacity（）；
         cap<max_buffer_size_u；//还有空间增加
         CAP＝2）{
      //查看下一个可用大小是否足够大。
      //缓冲区永远不会增加到最大缓冲区大小。
      尺寸_t所需_容量=标准：：最小（cap*2，最大_缓冲区_大小）；
      如果（所需的容量-buf_.currentsize（）>=左
          （使用_direct_io（）&&desired_capacity==max_buffer_size_）
        buf_u.allocatenewbuffer（所需容量，真）；
        断裂；
      }
    }
  }

  //仅在缓冲I/O时刷新
  如果（！）使用_direct_io（）&&（buf_u.capacity（）-buf_u.currentsize（））<left）
    如果（buf_u.currentsize（）>0）
      S＝FrHuSH（）；
      如果（！）S.O.（））{
        返回S；
      }
    }
    断言（buf_u.currentsize（）==0）；
  }

  //我们从不在直接I/O打开的情况下直接写入磁盘。
  //或者我们只是为了它的最初目的而使用它来积累许多小的
  //块
  if（使用_direct_o（）（buf_u.capacity（）>=left））
    同时（左>0）
      size_t appended=buf_u.append（src，左）；
      左-=附加的；
      SRC+=附加的；

      如果（左＞0）{
        S＝FrHuSH（）；
        如果（！）S.O.（））{
          断裂；
        }
      }
    }
  }否则{
    //绕过缓冲区直接写入文件
    断言（buf_u.currentsize（）==0）；
    S=写缓冲（SRC，左）；
  }

  测试_kill_random（“writablefilewriter:：append:1”，rocksdb_kill_odds）；
  如果（S.O.（））{
    fileSize_+=data.size（）；
  }
  返回S；
}

状态可写文件编写器：：close（）

  //失败时不立即退出文件必须关闭
  状态S；

  //现在可以关闭它两次，因为我们必须关闭它
  //在\u dtor中，仅仅冲洗是不够的
  //预分配时的窗口不填充零
  //另外，对于非缓冲访问，我们还设置了数据的结尾。
  如果（！）可写文件
    返回S；
  }

  s=flush（）；//将缓存刷新到OS

  状态暂行；
  //在直接I/O模式下，我们编写整个页面，因此
  //我们需要让文件知道数据的结束位置。
  if（使用_direct_io（））
    interim=可写_文件_u->截断（文件大小_u）；
    如果（！）临时.ok（）&&s.ok（））
      S＝过渡期；
    }
  }

  测试_kill_random（“writablefilewriter:：close:0”，rocksdb_kill_odds）；
  interim=可写_file_u->close（）；
  如果（！）临时.ok（）&&s.ok（））
    S＝过渡期；
  }

  可写的_file_u.reset（）；
  测试_kill_random（“writablefilewriter:：close:1”，rocksdb_kill_odds）；

  返回S；
}

//如果直接I/O，则将缓存数据写入OS缓存或存储
/ /启用
状态可写文件编写器：：flush（）
  状态S；
  test_kill_random（“writablefilewriter:：flush:0”，
                   Rocksdb_杀掉_几率*减少_几率2）；

  如果（buf_u.currentsize（）>0）
    if（使用_direct_io（））
ifndef岩石
      s=writeDirect（）；
我很喜欢你！摇滚乐
    }否则{
      S=写缓冲（buf_u.bufferstart（），buf_.currentsize（））；
    }
    如果（！）S.O.（））{
      返回S；
    }
  }

  s=可写_file_u->flush（）；

  如果（！）S.O.（））{
    返回S；
  }

  //为每个字节同步OS缓存到磁盘
  //TODO:为日志文件和sst文件提供不同的选项（日志
  //文件可能缓存在整个操作系统中
  //生命时间，因此我们可能根本不想冲水）。

  //我们尽量避免与最后的1MB数据同步。有两个原因：
  //（1）避免重写以后修改的同一页。
  //（2）对于旧版本的操作系统，写入时可能会阻塞
  //页面。
  //XFS在指定范围之外执行邻居页刷新。我们
  //需要确保同步范围远离写入偏移量。
  如果（！）使用_direct_io（）&&bytes_per_sync_
    const uint64_t kbytesnotsyncrange=1024*1024；//最近的1MB未同步。
    const uint64_kytesAlignwhensync=4*1024；//对齐4KB。
    if（文件大小>kbytesnotsyncrange）
      uint64_t offset_sync_to=filesize_uukbytesnotsyncrange；
      offset_sync_to-=offset_sync_to%kbytesAlignwhensync；
      断言（offset_sync_to>=last_sync_size_u）；
      如果（偏移同步到>0&&
          offset_sync_to-last_sync_size_>>bytes_per_sync_
        S=RangeSync（上次同步\大小\，偏移\同步\到-上次同步\大小\）；
        最后一次同步大小偏移同步到；
      }
    }
  }

  返回S；
}

状态可写文件编写器：：同步（bool使用同步）
  状态s=flush（）；
  如果（！）S.O.（））{
    返回S；
  }
  测试_kill_random（“writablefilewriter:：sync:0”，rocksdb_kill_odds）；
  如果（！）使用_direct_io（）&&pending_sync_
    s=同步内部（使用同步）；
    如果（！）S.O.（））{
      返回S；
    }
  }
  测试_kill_random（“writablefilewriter:：sync:1”，rocksdb_kill_odds）；
  挂起的同步错误；
  返回状态：：OK（）；
}

状态可写入文件编写器：：SyncWithOutFlush（bool use_fsync）
  如果（！）可写的_文件_->isSyncThreadSafe（））
    返回状态：：不支持（
      “无法写入FileWriter:：SyncWithOutFlush（），因为”
      “writablefile:：issynchThreadSafe（）为false”）；
  }
  测试同步点（“writablefilewriter:：syncwithoutflush:1”）；
  状态s=同步内部（使用同步）；
  测试同步点（“writablefilewriter:：syncwithoutflush:2”）；
  返回S；
}

状态可写入文件编写器：：SyncInternal（bool use_fsync）
  状态S；
  iostats定时器保护（fsync纳米）；
  测试同步点（“writablefilewriter:：syncInternal:0”）；
  如果（使用同步）
    s=可写_file_u->fsync（）；
  }否则{
    s=可写_file_u->sync（）；
  }
  返回S；
}

状态可写入文件编写器：：RangeSync（uint64_t offset，uint64_t nbytes）
  iostats计时器保护（范围同步纳米）；
  测试同步点（“writablefilewriter:：rangesync:0”）；
  返回可写的_file_u->rangesync（偏移量，nbytes）；
}

//此方法将指定的数据写入磁盘并使用速率
//限制器（如果可用）
状态可写文件编写器：：写缓冲（const char*data，size_t size）
  状态S；
  断言（！）使用_direct_io（））；
  const char*src=数据；
  size_t left=尺寸；

  同时（左>0）
    允许的尺寸；
    如果（速率限制器）= null pTr）{
      allowed=rate_limiter_u->requesttoken（
          左，0/*对齐*/, writable_file_->GetIOPriority(), stats_,

          RateLimiter::OpType::kWrite);
    } else {
      allowed = left;
    }

    {
      IOSTATS_TIMER_GUARD(write_nanos);
      TEST_SYNC_POINT("WritableFileWriter::Flush:BeforeAppend");
      s = writable_file_->Append(Slice(src, allowed));
      if (!s.ok()) {
        return s;
      }
    }

    IOSTATS_ADD(bytes_written, allowed);
    TEST_KILL_RANDOM("WritableFileWriter::WriteBuffered:0", rocksdb_kill_odds);

    left -= allowed;
    src += allowed;
  }
  buf_.Size(0);
  return s;
}


//这将刷新缓冲区中的累积数据。我们用零填充数据，如果
//对整个页面都是必需的。
//但是，在自动冲洗过程中，不需要填充。
//如果可以的话，我们总是使用Ratelimiter。我们移动（重新编译）任何缓冲区字节
//那是剩下的
//下一次刷新时要重新写入的总页数，因为我们可以
//仅在对齐时写入
//偏移量。
#ifndef ROCKSDB_LITE
Status WritableFileWriter::WriteDirect() {
  assert(use_direct_io());
  Status s;
  const size_t alignment = buf_.Alignment();
  assert((next_write_offset_ % alignment) == 0);

//如果所有写入成功，则计算整个页面的最终文件前进速度
  size_t file_advance =
    TruncateToPageBoundary(alignment, buf_.CurrentSize());

//计算剩余的尾巴，我们在这里用零填充，但是我们
//将写
//在以后的close（）或当当前整个页面
//填充
  size_t leftover_tail = buf_.CurrentSize() - file_advance;

//围拢和衬垫
  buf_.PadToAlignmentWith(0);

  const char* src = buf_.BufferStart();
  uint64_t write_offset = next_write_offset_;
  size_t left = buf_.CurrentSize();

  while (left > 0) {
//检查允许多少
    size_t size;
    if (rate_limiter_ != nullptr) {
      size = rate_limiter_->RequestToken(left, buf_.Alignment(),
                                         writable_file_->GetIOPriority(),
                                         stats_, RateLimiter::OpType::kWrite);
    } else {
      size = left;
    }

    {
      IOSTATS_TIMER_GUARD(write_nanos);
      TEST_SYNC_POINT("WritableFileWriter::Flush:BeforeAppend");
//直接写入必须是位置写入
      s = writable_file_->PositionedAppend(Slice(src, size), write_offset);
      if (!s.ok()) {
        buf_.Size(file_advance + leftover_tail);
        return s;
      }
    }

    IOSTATS_ADD(bytes_written, size);
    left -= size;
    src += size;
    write_offset += size;
    assert((next_write_offset_ % alignment) == 0);
  }

  if (s.ok()) {
//将尾部移动到缓冲区的开始处
//这不会在正常附加期间发生，而是在
//显式调用flush（）/sync（）或close（）。
    buf_.RefitTail(file_advance, leftover_tail);
//这就是我们下次开始写作的地方
//磁盘上的实际文件大小。如果缓冲区大小
//是整页的倍数，否则文件大小是剩余的
//后面
    next_write_offset_ += file_advance;
  }
  return s;
}
#endif  //！摇滚乐

namespace {
class ReadaheadRandomAccessFile : public RandomAccessFile {
 public:
  ReadaheadRandomAccessFile(std::unique_ptr<RandomAccessFile>&& file,
                            size_t readahead_size)
      : file_(std::move(file)),
        alignment_(file_->GetRequiredBufferAlignment()),
        readahead_size_(Roundup(readahead_size, alignment_)),
        buffer_(),
        buffer_offset_(0),
        buffer_len_(0) {

    buffer_.Alignment(alignment_);
    buffer_.AllocateNewBuffer(readahead_size_);
  }

 ReadaheadRandomAccessFile(const ReadaheadRandomAccessFile&) = delete;

 ReadaheadRandomAccessFile& operator=(const ReadaheadRandomAccessFile&) = delete;

  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const override {

    if (n + alignment_ >= readahead_size_) {
      return file_->Read(offset, n, result, scratch);
    }

    std::unique_lock<std::mutex> lk(lock_);

    size_t cached_len = 0;
//检查是否有缓存命中，意味着[offset，offset+n）是
//完全或部分在缓冲器中
//如果完全缓存，包括偏移量+n时的文件尾大小写
//大于EOF，返回
    if (TryReadFromCache(offset, n, &cached_len, scratch) &&
        (cached_len == n ||
//文件结束
         buffer_len_ < readahead_size_)) {
      *result = Slice(scratch, cached_len);
      return Status::OK();
    }
    size_t advanced_offset = offset + cached_len;
//如果缓存命中高级缓存偏移量已对齐，则意味着
//chunk_offset等于高级_offset
    size_t chunk_offset = TruncateToPageBoundary(alignment_, advanced_offset);
    Slice readahead_result;

    Status s = ReadIntoBuffer(chunk_offset, readahead_size_);
    if (s.ok()) {
//在缓存未命中的情况下，即当缓存长度等于0时，偏移量可以
//超出文件结束位置，因此需要进行以下检查
      if (advanced_offset < chunk_offset + buffer_len_) {
//在缓存未命中的情况下，缓冲区中的第一个块填充字节
//是
//仅为对齐而存储，必须跳过
        size_t chunk_padding = advanced_offset - chunk_offset;
        auto remaining_len =
            std::min(buffer_len_ - chunk_padding, n - cached_len);
        memcpy(scratch + cached_len, buffer_.BufferStart() + chunk_padding,
               remaining_len);
        *result = Slice(scratch, cached_len + remaining_len);
      } else {
        *result = Slice(scratch, cached_len);
      }
    }
    return s;
  }

  virtual Status Prefetch(uint64_t offset, size_t n) override {
    size_t prefetch_offset = TruncateToPageBoundary(alignment_, offset);
    if (prefetch_offset == buffer_offset_) {
      return Status::OK();
    }
    return ReadIntoBuffer(prefetch_offset,
                          Roundup(offset + n, alignment_) - prefetch_offset);
  }

  virtual size_t GetUniqueId(char* id, size_t max_size) const override {
    return file_->GetUniqueId(id, max_size);
  }

  virtual void Hint(AccessPattern pattern) override { file_->Hint(pattern); }

  virtual Status InvalidateCache(size_t offset, size_t length) override {
    return file_->InvalidateCache(offset, length);
  }

  virtual bool use_direct_io() const override {
    return file_->use_direct_io();
  }

 private:
  bool TryReadFromCache(uint64_t offset, size_t n, size_t* cached_len,
                         char* scratch) const {
    if (offset < buffer_offset_ || offset >= buffer_offset_ + buffer_len_) {
      *cached_len = 0;
      return false;
    }
    uint64_t offset_in_buffer = offset - buffer_offset_;
    *cached_len =
        std::min(buffer_len_ - static_cast<size_t>(offset_in_buffer), n);
    memcpy(scratch, buffer_.BufferStart() + offset_in_buffer, *cached_len);
    return true;
  }

  Status ReadIntoBuffer(uint64_t offset, size_t n) const {
    if (n > buffer_.Capacity()) {
      n = buffer_.Capacity();
    }
    assert(IsFileSectorAligned(offset, alignment_));
    assert(IsFileSectorAligned(n, alignment_));
    Slice result;
    Status s = file_->Read(offset, n, &result, buffer_.BufferStart());
    if (s.ok()) {
      buffer_offset_ = offset;
      buffer_len_ = result.size();
    }
    return s;
  }

  std::unique_ptr<RandomAccessFile> file_;
  const size_t alignment_;
  size_t               readahead_size_;

  mutable std::mutex lock_;
  mutable AlignedBuffer buffer_;
  mutable uint64_t buffer_offset_;
  mutable size_t buffer_len_;
};
}  //命名空间

Status FilePrefetchBuffer::Prefetch(RandomAccessFileReader* reader,
                                    uint64_t offset, size_t n) {
  size_t alignment = reader->file()->GetRequiredBufferAlignment();
  uint64_t roundup_offset = Roundup(offset, alignment);
  uint64_t roundup_len = Roundup(n, alignment);
  buffer_.Alignment(alignment);
  buffer_.AllocateNewBuffer(roundup_len);

  Slice result;
  Status s =
      reader->Read(roundup_offset, roundup_len, &result, buffer_.BufferStart());
  if (s.ok()) {
    buffer_offset_ = roundup_offset;
    buffer_len_ = result.size();
  }
  return s;
}

bool FilePrefetchBuffer::TryReadFromCache(uint64_t offset, size_t n,
                                          Slice* result) const {
  if (offset < buffer_offset_ || offset + n > buffer_offset_ + buffer_len_) {
    return false;
  }
  uint64_t offset_in_buffer = offset - buffer_offset_;
  *result = Slice(buffer_.BufferStart() + offset_in_buffer, n);
  return true;
}

std::unique_ptr<RandomAccessFile> NewReadaheadRandomAccessFile(
    std::unique_ptr<RandomAccessFile>&& file, size_t readahead_size) {
  std::unique_ptr<RandomAccessFile> result(
    new ReadaheadRandomAccessFile(std::move(file), readahead_size));
  return result;
}

Status NewWritableFile(Env* env, const std::string& fname,
                       unique_ptr<WritableFile>* result,
                       const EnvOptions& options) {
  Status s = env->NewWritableFile(fname, result, options);
  TEST_KILL_RANDOM("NewWritableFile:0", rocksdb_kill_odds * REDUCE_ODDS2);
  return s;
}

}  //命名空间rocksdb
