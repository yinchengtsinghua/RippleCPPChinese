
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

#include <algorithm>
#include <cctype>
#include <iostream>

#include "rocksdb/env_encryption.h"
#include "util/aligned_buffer.h"
#include "util/coding.h"
#include "util/random.h"

#endif

namespace rocksdb {

#ifndef ROCKSDB_LITE

class EncryptedSequentialFile : public SequentialFile {
  private:
    std::unique_ptr<SequentialFile> file_;
    std::unique_ptr<BlockAccessCipherStream> stream_;
    uint64_t offset_;
    size_t prefixLength_;

     public:
//缺省CCTR。假定基础序列文件应该位于
//偏移量=前缀长度。
  EncryptedSequentialFile(SequentialFile* f, BlockAccessCipherStream* s, size_t prefixLength)
      : file_(f), stream_(s), offset_(prefixLength), prefixLength_(prefixLength) {
  }

//从文件中读取最多“n”个字节。划痕[0..n-1]“可能是
//由这个程序编写。将“*result”设置为
//读取（包括成功读取少于“n”个字节）。
//可以将“*result”设置为指向“scratch[0..n-1]”中的数据，因此
//使用“*result”时，“scratch[0..n-1]”必须有效。
//如果遇到错误，则返回非OK状态。
//
//要求：外部同步
  virtual Status Read(size_t n, Slice* result, char* scratch) override {
    assert(scratch);
    Status status = file_->Read(n, result, scratch);
    if (!status.ok()) {
      return status;
    }
    status = stream_->Decrypt(offset_, (char*)result->data(), result->size());
offset_ += result->size(); //我们已经准备好了来自磁盘的数据，所以即使解密失败，也要更新偏移量。
    return status;
  }

//跳过文件中的“n”字节。这肯定不是
//读取相同数据的速度较慢，但可能更快。
//
//如果到达文件结尾，跳过将在
//文件，跳过将返回OK。
//
//要求：外部同步
  virtual Status Skip(uint64_t n) override {
    auto status = file_->Skip(n);
    if (!status.ok()) {
      return status;
    }
    offset_ += n;
    return status;
  }

//指示当前序列文件实现的上层
//使用直接IO。
  virtual bool use_direct_io() const override { 
    return file_->use_direct_io(); 
  }

//使用返回的对齐值分配
//用于直接I/O的对齐缓冲区
  virtual size_t GetRequiredBufferAlignment() const override { 
    return file_->GetRequiredBufferAlignment(); 
  }

//删除从偏移量到偏移量+长度的任何类型的数据缓存
//这个文件。如果长度为0，则表示文件结尾。
//如果系统没有缓存文件内容，那么这是一个noop。
  virtual Status InvalidateCache(size_t offset, size_t length) override {
    return file_->InvalidateCache(offset + prefixLength_, length);
  }

//用于直接I/O的定位读取
//如果启用直接I/O，则偏移量、n和划痕应正确对齐。
  virtual Status PositionedRead(uint64_t offset, size_t n, Slice* result, char* scratch) override {
    assert(scratch);
offset += prefixLength_; //跳过前缀
    auto status = file_->PositionedRead(offset, n, result, scratch);
    if (!status.ok()) {
      return status;
    }
    offset_ = offset + result->size();
    status = stream_->Decrypt(offset, (char*)result->data(), result->size());
    return status;
  }

};

//随机读取文件内容的文件抽象。
class EncryptedRandomAccessFile : public RandomAccessFile {
  private:
    std::unique_ptr<RandomAccessFile> file_;
    std::unique_ptr<BlockAccessCipherStream> stream_;
    size_t prefixLength_;

 public:
  EncryptedRandomAccessFile(RandomAccessFile* f, BlockAccessCipherStream* s, size_t prefixLength)
    : file_(f), stream_(s), prefixLength_(prefixLength) { }

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
  virtual Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const override {
    assert(scratch);
    offset += prefixLength_;
    auto status = file_->Read(offset, n, result, scratch);
    if (!status.ok()) {
      return status;
    }
    status = stream_->Decrypt(offset, (char*)result->data(), result->size());
    return status;
  }

//将文件从偏移量n字节开始预读以进行缓存。
  virtual Status Prefetch(uint64_t offset, size_t n) override {
//返回状态：：OK（）；
    return file_->Prefetch(offset + prefixLength_, n);
  }

//尝试为此文件获取每次相同的唯一ID
//打开文件（打开文件时保持不变）。
//此外，它还试图使这个ID最大为“最大\大小”字节。如果这样的话
//可以创建ID。此函数返回ID的长度并将其放置
//在“id”中；否则，此函数返回0，在这种情况下，“id”
//可能尚未修改。
//
//对于给定环境中的ID，此函数保证两个唯一的ID
//无法通过将任意字节添加到
//他们。也就是说，没有唯一的ID是另一个的前缀。
//
//此函数保证返回的ID不能解释为
//单个变量。
//
//注意：这些ID仅在进程期间有效。
  virtual size_t GetUniqueId(char* id, size_t max_size) const override {
    return file_->GetUniqueId(id, max_size);
  };

  virtual void Hint(AccessPattern pattern) override {
    file_->Hint(pattern);
  }

//指示当前RandomAccessFile实现的上层
//使用直接IO。
  virtual bool use_direct_io() const override {
     return file_->use_direct_io(); 
  }

//使用返回的对齐值分配
//用于直接I/O的对齐缓冲区
  virtual size_t GetRequiredBufferAlignment() const override { 
    return file_->GetRequiredBufferAlignment(); 
  }

//删除从偏移量到偏移量+长度的任何类型的数据缓存
//这个文件。如果长度为0，则表示文件结尾。
//如果系统没有缓存文件内容，那么这是一个noop。
  virtual Status InvalidateCache(size_t offset, size_t length) override {
    return file_->InvalidateCache(offset + prefixLength_, length);
  }
};

//用于顺序写入的文件抽象。实施
//必须提供缓冲，因为调用方可能附加小片段
//一次到文件。
class EncryptedWritableFile : public WritableFileWrapper {
  private:
    std::unique_ptr<WritableFile> file_;
    std::unique_ptr<BlockAccessCipherStream> stream_;
    size_t prefixLength_;

 public:
//缺省CCTR。假定前缀已被写入。
  EncryptedWritableFile(WritableFile* f, BlockAccessCipherStream* s, size_t prefixLength)
    : WritableFileWrapper(f), file_(f), stream_(s), prefixLength_(prefixLength) { }

  Status Append(const Slice& data) override { 
    AlignedBuffer buf;
    Status status;
    Slice dataToAppend(data); 
    if (data.size() > 0) {
auto offset = file_->GetFileSize(); //大小包括前缀
//在克隆的缓冲区中加密
      buf.Alignment(GetRequiredBufferAlignment());
      buf.AllocateNewBuffer(data.size());
      memmove(buf.BufferStart(), data.data(), data.size());
      status = stream_->Encrypt(offset, buf.BufferStart(), data.size());
      if (!status.ok()) {
        return status;
      }
      dataToAppend = Slice(buf.BufferStart(), data.size());
    }
    status = file_->Append(dataToAppend); 
    if (!status.ok()) {
      return status;
    }
    return status;
  }

  Status PositionedAppend(const Slice& data, uint64_t offset) override {
    AlignedBuffer buf;
    Status status;
    Slice dataToAppend(data); 
    offset += prefixLength_;
    if (data.size() > 0) {
//在克隆的缓冲区中加密
      buf.Alignment(GetRequiredBufferAlignment());
      buf.AllocateNewBuffer(data.size());
      memmove(buf.BufferStart(), data.data(), data.size());
      status = stream_->Encrypt(offset, buf.BufferStart(), data.size());
      if (!status.ok()) {
        return status;
      }
      dataToAppend = Slice(buf.BufferStart(), data.size());
    }
    status = file_->PositionedAppend(dataToAppend, offset);
    if (!status.ok()) {
      return status;
    }
    return status;
  }

//指示当前可写文件实现的上层
//使用直接IO。
  virtual bool use_direct_io() const override { return file_->use_direct_io(); }

//使用返回的对齐值分配
//用于直接I/O的对齐缓冲区
  virtual size_t GetRequiredBufferAlignment() const override { return file_->GetRequiredBufferAlignment(); } 

    /*
   *获取文件中有效数据的大小。
   **/

  virtual uint64_t GetFileSize() override {
    return file_->GetFileSize() - prefixLength_;
  }

//需要截断才能将文件修剪为正确的大小
//关闭前。不可能总是跟踪文件
//由于整个页面写入而导致的大小。如果调用，则行为未定义
//接下来是其他的写作。
  virtual Status Truncate(uint64_t size) override {
    return file_->Truncate(size + prefixLength_);
  }

//删除从偏移量到偏移量+长度的任何类型的数据缓存
//这个文件。如果长度为0，则表示文件结尾。
//如果系统没有缓存文件内容，那么这是一个noop。
//此调用对缓存中的脏页没有影响。
  virtual Status InvalidateCache(size_t offset, size_t length) override {
    return file_->InvalidateCache(offset + prefixLength_, length);
  }

//将文件范围与磁盘同步。
//偏移量是要同步的文件范围的起始字节。
//nbytes指定要同步的范围的长度。
//这要求操作系统启动将缓存数据刷新到磁盘，
//不等待完成。
//默认实现什么也不做。
  virtual Status RangeSync(uint64_t offset, uint64_t nbytes) override { 
    return file_->RangeSync(offset + prefixLength_, nbytes);
  }

//PrepareWrite执行写入的任何必要准备
//在写入实际发生之前。这允许预先分配
//设备上的空间可以减少文件
//由于过分狂热的文件系统而导致的碎片和/或更少的浪费
//预分配。
  virtual void PrepareWrite(size_t offset, size_t len) override {
    file_->PrepareWrite(offset + prefixLength_, len);
  }

//预先为文件分配空间。
  virtual Status Allocate(uint64_t offset, uint64_t len) override {
    return file_->Allocate(offset + prefixLength_, len);
  }
};

//用于随机读写的文件抽象。
class EncryptedRandomRWFile : public RandomRWFile {
  private:
    std::unique_ptr<RandomRWFile> file_;
    std::unique_ptr<BlockAccessCipherStream> stream_;
    size_t prefixLength_;

 public:
  EncryptedRandomRWFile(RandomRWFile* f, BlockAccessCipherStream* s, size_t prefixLength)
    : file_(f), stream_(s), prefixLength_(prefixLength) {}

//指示类是否使用直接I/O
//如果为false，则必须将对齐的缓冲区传递给write（）。
  virtual bool use_direct_io() const override { return file_->use_direct_io(); }

//使用返回的对齐值分配
//用于直接I/O的对齐缓冲区
  virtual size_t GetRequiredBufferAlignment() const override { 
    return file_->GetRequiredBufferAlignment(); 
  }

//在“offset”处的“data”中写入字节，成功时返回status:：ok（）。
//当使用_Direct_IO（）返回true时，传递对齐的缓冲区。
  virtual Status Write(uint64_t offset, const Slice& data) override {
    AlignedBuffer buf;
    Status status;
    Slice dataToWrite(data); 
    offset += prefixLength_;
    if (data.size() > 0) {
//在克隆的缓冲区中加密
      buf.Alignment(GetRequiredBufferAlignment());
      buf.AllocateNewBuffer(data.size());
      memmove(buf.BufferStart(), data.data(), data.size());
      status = stream_->Encrypt(offset, buf.BufferStart(), data.size());
      if (!status.ok()) {
        return status;
      }
      dataToWrite = Slice(buf.BufferStart(), data.size());
    }
    status = file_->Write(offset, dataToWrite);
    return status;
  }

//从offset'offset'开始读取最多'n'个字节，并将它们存储在
//结果，提供的“scratch”大小至少应为“n”。
//返回成功时的状态：：OK（）。
  virtual Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const override { 
    assert(scratch);
    offset += prefixLength_;
    auto status = file_->Read(offset, n, result, scratch);
    if (!status.ok()) {
      return status;
    }
    status = stream_->Decrypt(offset, (char*)result->data(), result->size());
    return status;
  }

  virtual Status Flush() override {
    return file_->Flush();
  }

  virtual Status Sync() override {
    return file_->Sync();
  }

  virtual Status Fsync() override { 
    return file_->Fsync();
  }

  virtual Status Close() override {
    return file_->Close();
  }
};

//EncryptedEnv实现了一个env包装器，它向存储在磁盘上的文件添加加密。
class EncryptedEnv : public EnvWrapper {
 public:
  EncryptedEnv(Env* base_env, EncryptionProvider *provider)
      : EnvWrapper(base_env) {
    provider_ = provider;
  }

//NewSequentialFile打开一个文件进行顺序读取。
  virtual Status NewSequentialFile(const std::string& fname,
                                   std::unique_ptr<SequentialFile>* result,
                                   const EnvOptions& options) override {
    result->reset();
    if (options.use_mmap_reads) {
      return Status::InvalidArgument();
    }
//使用底层env实现打开文件
    std::unique_ptr<SequentialFile> underlying;
    auto status = EnvWrapper::NewSequentialFile(fname, &underlying, options);
    if (!status.ok()) {
      return status;
    }
//读取前缀（如果需要）
    AlignedBuffer prefixBuf;
    Slice prefixSlice;
    size_t prefixLength = provider_->GetPrefixLength();
    if (prefixLength > 0) {
//读前缀
      prefixBuf.Alignment(underlying->GetRequiredBufferAlignment());
      prefixBuf.AllocateNewBuffer(prefixLength);
      status = underlying->Read(prefixLength, &prefixSlice, prefixBuf.BufferStart());
      if (!status.ok()) {
        return status;
      }
    }
//创建密码流
    std::unique_ptr<BlockAccessCipherStream> stream;
    status = provider_->CreateCipherStream(fname, options, prefixSlice, &stream);
    if (!status.ok()) {
      return status;
    }
    (*result) = std::unique_ptr<SequentialFile>(new EncryptedSequentialFile(underlying.release(), stream.release(), prefixLength));
    return Status::OK();
  }

//newrandomaccessfile打开一个文件进行随机读取访问。
  virtual Status NewRandomAccessFile(const std::string& fname,
                                     unique_ptr<RandomAccessFile>* result,
                                     const EnvOptions& options) override {
    result->reset();
    if (options.use_mmap_reads) {
      return Status::InvalidArgument();
    }
//使用底层env实现打开文件
    std::unique_ptr<RandomAccessFile> underlying;
    auto status = EnvWrapper::NewRandomAccessFile(fname, &underlying, options);
    if (!status.ok()) {
      return status;
    }
//读取前缀（如果需要）
    AlignedBuffer prefixBuf;
    Slice prefixSlice;
    size_t prefixLength = provider_->GetPrefixLength();
    if (prefixLength > 0) {
//读前缀
      prefixBuf.Alignment(underlying->GetRequiredBufferAlignment());
      prefixBuf.AllocateNewBuffer(prefixLength);
      status = underlying->Read(0, prefixLength, &prefixSlice, prefixBuf.BufferStart());
      if (!status.ok()) {
        return status;
      }
    }
//创建密码流
    std::unique_ptr<BlockAccessCipherStream> stream;
    status = provider_->CreateCipherStream(fname, options, prefixSlice, &stream);
    if (!status.ok()) {
      return status;
    }
    (*result) = std::unique_ptr<RandomAccessFile>(new EncryptedRandomAccessFile(underlying.release(), stream.release(), prefixLength));
    return Status::OK();
  }
  
//newwritablefile打开一个文件进行顺序写入。
  virtual Status NewWritableFile(const std::string& fname,
                                 unique_ptr<WritableFile>* result,
                                 const EnvOptions& options) override {
    result->reset();
    if (options.use_mmap_writes) {
      return Status::InvalidArgument();
    }
//使用底层env实现打开文件
    std::unique_ptr<WritableFile> underlying;
    Status status = EnvWrapper::NewWritableFile(fname, &underlying, options);
    if (!status.ok()) {
      return status;
    }
//初始化和写入前缀（如果需要）
    AlignedBuffer prefixBuf;
    Slice prefixSlice;
    size_t prefixLength = provider_->GetPrefixLength();
    if (prefixLength > 0) {
//初始化前缀
      prefixBuf.Alignment(underlying->GetRequiredBufferAlignment());
      prefixBuf.AllocateNewBuffer(prefixLength);
      provider_->CreateNewPrefix(fname, prefixBuf.BufferStart(), prefixLength);
      prefixSlice = Slice(prefixBuf.BufferStart(), prefixLength);
//写前缀
      status = underlying->Append(prefixSlice);
      if (!status.ok()) {
        return status;
      }
    }
//创建密码流
    std::unique_ptr<BlockAccessCipherStream> stream;
    status = provider_->CreateCipherStream(fname, options, prefixSlice, &stream);
    if (!status.ok()) {
      return status;
    }
    (*result) = std::unique_ptr<WritableFile>(new EncryptedWritableFile(underlying.release(), stream.release(), prefixLength));
    return Status::OK();
  }

//创建一个对象，该对象使用指定的
//姓名。删除具有相同名称的任何现有文件并创建
//新文件。成功后，将指向新文件的指针存储在
//*结果并返回OK。失败时将nullptr存储在*result和
//返回非OK。
//
//返回的文件一次只能由一个线程访问。
  virtual Status ReopenWritableFile(const std::string& fname,
                                   unique_ptr<WritableFile>* result,
                                   const EnvOptions& options) override {
    result->reset();
    if (options.use_mmap_writes) {
      return Status::InvalidArgument();
    }
//使用底层env实现打开文件
    std::unique_ptr<WritableFile> underlying;
    Status status = EnvWrapper::ReopenWritableFile(fname, &underlying, options);
    if (!status.ok()) {
      return status;
    }
//初始化和写入前缀（如果需要）
    AlignedBuffer prefixBuf;
    Slice prefixSlice;
    size_t prefixLength = provider_->GetPrefixLength();
    if (prefixLength > 0) {
//初始化前缀
      prefixBuf.Alignment(underlying->GetRequiredBufferAlignment());
      prefixBuf.AllocateNewBuffer(prefixLength);
      provider_->CreateNewPrefix(fname, prefixBuf.BufferStart(), prefixLength);
      prefixSlice = Slice(prefixBuf.BufferStart(), prefixLength);
//写前缀
      status = underlying->Append(prefixSlice);
      if (!status.ok()) {
        return status;
      }
    }
//创建密码流
    std::unique_ptr<BlockAccessCipherStream> stream;
    status = provider_->CreateCipherStream(fname, options, prefixSlice, &stream);
    if (!status.ok()) {
      return status;
    }
    (*result) = std::unique_ptr<WritableFile>(new EncryptedWritableFile(underlying.release(), stream.release(), prefixLength));
    return Status::OK();
  }

//通过重命名现有文件并将其打开为可写文件来重用该文件。
  virtual Status ReuseWritableFile(const std::string& fname,
                                   const std::string& old_fname,
                                   unique_ptr<WritableFile>* result,
                                   const EnvOptions& options) override {
    result->reset();
    if (options.use_mmap_writes) {
      return Status::InvalidArgument();
    }
//使用底层env实现打开文件
    std::unique_ptr<WritableFile> underlying;
    Status status = EnvWrapper::ReuseWritableFile(fname, old_fname, &underlying, options);
    if (!status.ok()) {
      return status;
    }
//初始化和写入前缀（如果需要）
    AlignedBuffer prefixBuf;
    Slice prefixSlice;
    size_t prefixLength = provider_->GetPrefixLength();
    if (prefixLength > 0) {
//初始化前缀
      prefixBuf.Alignment(underlying->GetRequiredBufferAlignment());
      prefixBuf.AllocateNewBuffer(prefixLength);
      provider_->CreateNewPrefix(fname, prefixBuf.BufferStart(), prefixLength);
      prefixSlice = Slice(prefixBuf.BufferStart(), prefixLength);
//写前缀
      status = underlying->Append(prefixSlice);
      if (!status.ok()) {
        return status;
      }
    }
//创建密码流
    std::unique_ptr<BlockAccessCipherStream> stream;
    status = provider_->CreateCipherStream(fname, options, prefixSlice, &stream);
    if (!status.ok()) {
      return status;
    }
    (*result) = std::unique_ptr<WritableFile>(new EncryptedWritableFile(underlying.release(), stream.release(), prefixLength));
    return Status::OK();
  }

//如果文件不存在，则打开“fname”进行随机读写
//将被创建。成功后，将指向新文件的指针存储在
//*结果并返回OK。失败时返回不正常。
//
//返回的文件一次只能由一个线程访问。
  virtual Status NewRandomRWFile(const std::string& fname,
                                 unique_ptr<RandomRWFile>* result,
                                 const EnvOptions& options) override {
    result->reset();
    if (options.use_mmap_reads || options.use_mmap_writes) {
      return Status::InvalidArgument();
    }
//检查文件是否存在
    bool isNewFile = !FileExists(fname).ok();

//使用底层env实现打开文件
    std::unique_ptr<RandomRWFile> underlying;
    Status status = EnvWrapper::NewRandomRWFile(fname, &underlying, options);
    if (!status.ok()) {
      return status;
    }
//读取或初始化&写入前缀（如果需要）
    AlignedBuffer prefixBuf;
    Slice prefixSlice;
    size_t prefixLength = provider_->GetPrefixLength();
    if (prefixLength > 0) {
      prefixBuf.Alignment(underlying->GetRequiredBufferAlignment());
      prefixBuf.AllocateNewBuffer(prefixLength);
      if (!isNewFile) {
//文件已存在，读取前缀
        status = underlying->Read(0, prefixLength, &prefixSlice, prefixBuf.BufferStart());
        if (!status.ok()) {
          return status;
        }
      } else {
//文件是新的，初始化和写入前缀
        provider_->CreateNewPrefix(fname, prefixBuf.BufferStart(), prefixLength);
        prefixSlice = Slice(prefixBuf.BufferStart(), prefixLength);
//写前缀
        status = underlying->Write(0, prefixSlice);
        if (!status.ok()) {
          return status;
        }
      }
    }
//创建密码流
    std::unique_ptr<BlockAccessCipherStream> stream;
    status = provider_->CreateCipherStream(fname, options, prefixSlice, &stream);
    if (!status.ok()) {
      return status;
    }
    (*result) = std::unique_ptr<RandomRWFile>(new EncryptedRandomRWFile(underlying.release(), stream.release(), prefixLength));
    return Status::OK();
  }

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
  virtual Status GetChildrenFileAttributes(const std::string& dir, std::vector<FileAttributes>* result) override {
    auto status = EnvWrapper::GetChildrenFileAttributes(dir, result);
    if (!status.ok()) {
      return status;
    }
    size_t prefixLength = provider_->GetPrefixLength();
    for (auto it = std::begin(*result); it!=std::end(*result); ++it) {
      assert(it->size_bytes >= prefixLength);
      it->size_bytes -= prefixLength;
    }
    return Status::OK();
 }

//将fname的大小存储在*文件\大小中。
  virtual Status GetFileSize(const std::string& fname, uint64_t* file_size) override {
    auto status = EnvWrapper::GetFileSize(fname, file_size);
    if (!status.ok()) {
      return status;
    }
    size_t prefixLength = provider_->GetPrefixLength();
    assert(*file_size >= prefixLength);
    *file_size -= prefixLength;
    return Status::OK();    
  }

 private:
  EncryptionProvider *provider_;
};


//返回一个env，该env在存储在磁盘上时加密数据，在
//从磁盘读取。
Env* NewEncryptedEnv(Env* base_env, EncryptionProvider* provider) {
  return new EncryptedEnv(base_env, provider);
}

//在文件偏移量处加密一个或多个（部分）数据块。
//数据长度以数据大小表示。
Status BlockAccessCipherStream::Encrypt(uint64_t fileOffset, char *data, size_t dataSize) {
//计算块索引
  auto blockSize = BlockSize();
  uint64_t blockIndex = fileOffset / blockSize;
  size_t blockOffset = fileOffset % blockSize;
  unique_ptr<char[]> blockBuffer;

  std::string scratch;
  AllocateScratch(scratch);

//加密单个块。
  while (1) {
    char *block = data;
    size_t n = std::min(dataSize, blockSize - blockOffset);
    if (n != blockSize) {
//我们没有加密一个完整的块。
//将数据复制到blockbuffer
      if (!blockBuffer.get()) {
//分配缓冲区
        blockBuffer = unique_ptr<char[]>(new char[blockSize]);
      }
      block = blockBuffer.get();
//将普通数据复制到块缓冲区
      memmove(block + blockOffset, data, n);
    }
    auto status = EncryptBlock(blockIndex, block, (char*)scratch.data());
    if (!status.ok()) {
      return status;
    }
    if (block != data) {
//将加密数据复制回“data”。
      memmove(data, block + blockOffset, n);
    }
    dataSize -= n;
    if (dataSize == 0) {
      return Status::OK();
    }
    data += n;
    blockOffset = 0;
    blockIndex++;
  }
}

//在文件偏移量处解密一个或多个（部分）数据块。
//数据长度以数据大小表示。
Status BlockAccessCipherStream::Decrypt(uint64_t fileOffset, char *data, size_t dataSize) {
//计算块索引
  auto blockSize = BlockSize();
  uint64_t blockIndex = fileOffset / blockSize;
  size_t blockOffset = fileOffset % blockSize;
  unique_ptr<char[]> blockBuffer;

  std::string scratch;
  AllocateScratch(scratch);

//解密各个块。
  while (1) {
    char *block = data;
    size_t n = std::min(dataSize, blockSize - blockOffset);
    if (n != blockSize) {
//我们不能解密一个完整的块。
//将数据复制到blockbuffer
      if (!blockBuffer.get()) {
//分配缓冲区
        blockBuffer = unique_ptr<char[]>(new char[blockSize]);
      }
      block = blockBuffer.get();
//将加密数据复制到块缓冲区
      memmove(block + blockOffset, data, n);
    }
    auto status = DecryptBlock(blockIndex, block, (char*)scratch.data());
    if (!status.ok()) {
      return status;
    }
    if (block != data) {
//将解密的数据复制回“data”。
      memmove(data, block + blockOffset, n);
    }
    dataSize -= n;
    if (dataSize == 0) {
      return Status::OK();
    }
    data += n;
    blockOffset = 0;
    blockIndex++;
  }
}

//加密数据块。
//数据长度等于blockSize（）。
Status ROT13BlockCipher::Encrypt(char *data) {
  for (size_t i = 0; i < blockSize_; ++i) {
      data[i] += 13;
  }
  return Status::OK();
}

//解密一块数据。
//数据长度等于blockSize（）。
Status ROT13BlockCipher::Decrypt(char *data) {
  return Encrypt(data);
}

//分配传递给encryptblock/decryptblock的临时空间。
void CTRCipherStream::AllocateScratch(std::string& scratch) {
  auto blockSize = cipher_.BlockSize();
  scratch.reserve(blockSize);
}

//在给定的块索引处加密数据块。
//数据长度等于blockSize（）；
Status CTRCipherStream::EncryptBlock(uint64_t blockIndex, char *data, char* scratch) {

//创建nonce+计数器
  auto blockSize = cipher_.BlockSize();
  memmove(scratch, iv_.data(), blockSize);
  EncodeFixed64(scratch, blockIndex + initialCounter_);

//加密nonce+计数器
  auto status = cipher_.Encrypt(scratch);
  if (!status.ok()) {
    return status;
  }

//带密文的XOR数据。
  for (size_t i = 0; i < blockSize; i++) {
    data[i] = data[i] ^ scratch[i];
  }
  return Status::OK();
}

//在给定的块索引处解密数据块。
//数据长度等于blockSize（）；
Status CTRCipherStream::DecryptBlock(uint64_t blockIndex, char *data, char* scratch) {
//对于ctr，解密和加密是相同的
  return EncryptBlock(blockIndex, data, scratch);
}

//GetPrefixLength返回添加到每个文件的前缀的长度
//用于存储加密选项。
//为了获得最佳性能，前缀长度应该是
//A页大小。
size_t CTREncryptionProvider::GetPrefixLength() {
  return defaultPrefixLength;
}

//decodectrParameters从给定的
//（纯文本）前缀。
static void decodeCTRParameters(const char *prefix, size_t blockSize, uint64_t &initialCounter, Slice &iv) {
//第一个块包含64位初始计数器
  initialCounter = DecodeFixed64(prefix);
//第二块包含IV
  iv = Slice(prefix + blockSize, blockSize);
}

//CreateNewPrefix初始化了分配的前缀内存块
//一个新文件。
Status CTREncryptionProvider::CreateNewPrefix(const std::string& fname, char *prefix, size_t prefixLength) {
//创建并设定RND。
  Random rnd((uint32_t)Env::Default()->NowMicros());
//用随机值填充整个前缀块。
  for (size_t i = 0; i < prefixLength; i++) {
    prefix[i] = rnd.Uniform(256) & 0xFF;
  }
//随机抽取初始计数器
  auto blockSize = cipher_.BlockSize();
  uint64_t initialCounter;
  Slice prefixIV;
  decodeCTRParameters(prefix, blockSize, initialCounter, prefixIV);

//现在填充前缀的其余部分，从第三个块开始。
  PopulateSecretPrefixPart(prefix + (2 * blockSize), prefixLength - (2 * blockSize), blockSize);

//加密前缀，从块2开始（保留块0、1和初始计数器&iv未加密）
  CTRCipherStream cipherStream(cipher_, prefixIV.data(), initialCounter);
  auto status = cipherStream.Encrypt(0, prefix + (2 * blockSize), prefixLength - (2 * blockSize));
  if (!status.ok()) {
    return status;
  }
  return Status::OK();
}

//PopulateSecretPrefixPart将数据初始化为新的前缀块
//用纯文本。
//返回空间量（从前缀开头开始）
//已初始化。
size_t CTREncryptionProvider::PopulateSecretPrefixPart(char *prefix, size_t prefixLength, size_t blockSize) {
//这里不做任何事情，需要时将自定义数据置于覆盖中。
  return 0;
}

Status CTREncryptionProvider::CreateCipherStream(const std::string& fname, const EnvOptions& options, Slice &prefix, unique_ptr<BlockAccessCipherStream>* result) {
//阅读前缀的纯文本部分。
  auto blockSize = cipher_.BlockSize();
  uint64_t initialCounter;
  Slice iv;
  decodeCTRParameters(prefix.data(), blockSize, initialCounter, iv);

//解密前缀的加密部分，从块2开始（块0、带初始计数器的块1和IV未加密）
  CTRCipherStream cipherStream(cipher_, iv.data(), initialCounter);
  auto status = cipherStream.Decrypt(0, (char*)prefix.data() + (2 * blockSize), prefix.size() - (2 * blockSize));
  if (!status.ok()) {
    return status;
  }

//创建密码流
  return CreateCipherStreamFromPrefix(fname, options, initialCounter, iv, prefix, result);
}

//createCipherstreamFromPrefix为给定的文件创建块访问密码流
//给定名称和选项。给定的前缀已被解密。
Status CTREncryptionProvider::CreateCipherStreamFromPrefix(const std::string& fname, const EnvOptions& options,
    uint64_t initialCounter, const Slice& iv, const Slice& prefix, unique_ptr<BlockAccessCipherStream>* result) {
  (*result) = unique_ptr<BlockAccessCipherStream>(new CTRCipherStream(cipher_, iv.data(), initialCounter));
  return Status::OK();
}

#endif //摇滚乐

}  //命名空间rocksdb
