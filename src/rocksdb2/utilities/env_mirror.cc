
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2015，Red Hat，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#ifndef ROCKSDB_LITE

#include "rocksdb/utilities/env_mirror.h"

namespace rocksdb {

//在两个后端上镜像所有工作的env的实现
//这对于调试很有用。
class SequentialFileMirror : public SequentialFile {
 public:
  unique_ptr<SequentialFile> a_, b_;
  std::string fname;
  explicit SequentialFileMirror(std::string f) : fname(f) {}

  Status Read(size_t n, Slice* result, char* scratch) {
    Slice aslice;
    Status as = a_->Read(n, &aslice, scratch);
    if (as == Status::OK()) {
      char* bscratch = new char[n];
      Slice bslice;
      size_t off = 0;
      size_t left = aslice.size();
      while (left) {
        Status bs = b_->Read(left, &bslice, bscratch);
        assert(as == bs);
        assert(memcmp(bscratch, scratch + off, bslice.size()) == 0);
        off += bslice.size();
        left -= bslice.size();
      }
      delete[] bscratch;
      *result = aslice;
    } else {
      Status bs = b_->Read(n, result, scratch);
      assert(as == bs);
    }
    return as;
  }

  Status Skip(uint64_t n) {
    Status as = a_->Skip(n);
    Status bs = b_->Skip(n);
    assert(as == bs);
    return as;
  }
  Status InvalidateCache(size_t offset, size_t length) {
    Status as = a_->InvalidateCache(offset, length);
    Status bs = b_->InvalidateCache(offset, length);
    assert(as == bs);
    return as;
  };
};

class RandomAccessFileMirror : public RandomAccessFile {
 public:
  unique_ptr<RandomAccessFile> a_, b_;
  std::string fname;
  explicit RandomAccessFileMirror(std::string f) : fname(f) {}

  Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const {
    Status as = a_->Read(offset, n, result, scratch);
    if (as == Status::OK()) {
      char* bscratch = new char[n];
      Slice bslice;
      size_t off = 0;
      size_t left = result->size();
      while (left) {
        Status bs = b_->Read(offset + off, left, &bslice, bscratch);
        assert(as == bs);
        assert(memcmp(bscratch, scratch + off, bslice.size()) == 0);
        off += bslice.size();
        left -= bslice.size();
      }
      delete[] bscratch;
    } else {
      Status bs = b_->Read(offset, n, result, scratch);
      assert(as == bs);
    }
    return as;
  }

  size_t GetUniqueId(char* id, size_t max_size) const {
//注：未验证
    return a_->GetUniqueId(id, max_size);
  }
};

class WritableFileMirror : public WritableFile {
 public:
  unique_ptr<WritableFile> a_, b_;
  std::string fname;
  explicit WritableFileMirror(std::string f) : fname(f) {}

  Status Append(const Slice& data) override {
    Status as = a_->Append(data);
    Status bs = b_->Append(data);
    assert(as == bs);
    return as;
  }
  Status PositionedAppend(const Slice& data, uint64_t offset) override {
    Status as = a_->PositionedAppend(data, offset);
    Status bs = b_->PositionedAppend(data, offset);
    assert(as == bs);
    return as;
  }
  Status Truncate(uint64_t size) override {
    Status as = a_->Truncate(size);
    Status bs = b_->Truncate(size);
    assert(as == bs);
    return as;
  }
  Status Close() override {
    Status as = a_->Close();
    Status bs = b_->Close();
    assert(as == bs);
    return as;
  }
  Status Flush() override {
    Status as = a_->Flush();
    Status bs = b_->Flush();
    assert(as == bs);
    return as;
  }
  Status Sync() override {
    Status as = a_->Sync();
    Status bs = b_->Sync();
    assert(as == bs);
    return as;
  }
  Status Fsync() override {
    Status as = a_->Fsync();
    Status bs = b_->Fsync();
    assert(as == bs);
    return as;
  }
  bool IsSyncThreadSafe() const override {
    bool as = a_->IsSyncThreadSafe();
    assert(as == b_->IsSyncThreadSafe());
    return as;
  }
  void SetIOPriority(Env::IOPriority pri) override {
    a_->SetIOPriority(pri);
    b_->SetIOPriority(pri);
  }
  Env::IOPriority GetIOPriority() override {
//注意：我们不验证这个
    return a_->GetIOPriority();
  }
  uint64_t GetFileSize() override {
    uint64_t as = a_->GetFileSize();
    assert(as == b_->GetFileSize());
    return as;
  }
  void GetPreallocationStatus(size_t* block_size,
                              size_t* last_allocated_block) override {
//注意：我们不验证这个
    return a_->GetPreallocationStatus(block_size, last_allocated_block);
  }
  size_t GetUniqueId(char* id, size_t max_size) const override {
//注意：我们不验证这个
    return a_->GetUniqueId(id, max_size);
  }
  Status InvalidateCache(size_t offset, size_t length) override {
    Status as = a_->InvalidateCache(offset, length);
    Status bs = b_->InvalidateCache(offset, length);
    assert(as == bs);
    return as;
  }

 protected:
  Status Allocate(uint64_t offset, uint64_t length) override {
    Status as = a_->Allocate(offset, length);
    Status bs = b_->Allocate(offset, length);
    assert(as == bs);
    return as;
  }
  Status RangeSync(uint64_t offset, uint64_t nbytes) override {
    Status as = a_->RangeSync(offset, nbytes);
    Status bs = b_->RangeSync(offset, nbytes);
    assert(as == bs);
    return as;
  }
};

Status EnvMirror::NewSequentialFile(const std::string& f,
                                    unique_ptr<SequentialFile>* r,
                                    const EnvOptions& options) {
  if (f.find("/proc/") == 0) {
    return a_->NewSequentialFile(f, r, options);
  }
  SequentialFileMirror* mf = new SequentialFileMirror(f);
  Status as = a_->NewSequentialFile(f, &mf->a_, options);
  Status bs = b_->NewSequentialFile(f, &mf->b_, options);
  assert(as == bs);
  if (as.ok())
    r->reset(mf);
  else
    delete mf;
  return as;
}

Status EnvMirror::NewRandomAccessFile(const std::string& f,
                                      unique_ptr<RandomAccessFile>* r,
                                      const EnvOptions& options) {
  if (f.find("/proc/") == 0) {
    return a_->NewRandomAccessFile(f, r, options);
  }
  RandomAccessFileMirror* mf = new RandomAccessFileMirror(f);
  Status as = a_->NewRandomAccessFile(f, &mf->a_, options);
  Status bs = b_->NewRandomAccessFile(f, &mf->b_, options);
  assert(as == bs);
  if (as.ok())
    r->reset(mf);
  else
    delete mf;
  return as;
}

Status EnvMirror::NewWritableFile(const std::string& f,
                                  unique_ptr<WritableFile>* r,
                                  const EnvOptions& options) {
  if (f.find("/proc/") == 0) return a_->NewWritableFile(f, r, options);
  WritableFileMirror* mf = new WritableFileMirror(f);
  Status as = a_->NewWritableFile(f, &mf->a_, options);
  Status bs = b_->NewWritableFile(f, &mf->b_, options);
  assert(as == bs);
  if (as.ok())
    r->reset(mf);
  else
    delete mf;
  return as;
}

Status EnvMirror::ReuseWritableFile(const std::string& fname,
                                    const std::string& old_fname,
                                    unique_ptr<WritableFile>* r,
                                    const EnvOptions& options) {
  if (fname.find("/proc/") == 0)
    return a_->ReuseWritableFile(fname, old_fname, r, options);
  WritableFileMirror* mf = new WritableFileMirror(fname);
  Status as = a_->ReuseWritableFile(fname, old_fname, &mf->a_, options);
  Status bs = b_->ReuseWritableFile(fname, old_fname, &mf->b_, options);
  assert(as == bs);
  if (as.ok())
    r->reset(mf);
  else
    delete mf;
  return as;
}

}  //命名空间rocksdb
#endif
