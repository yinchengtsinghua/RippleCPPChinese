
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
#include <algorithm>
#include <deque>
#include <string>
#include <vector>

#include "rocksdb/compaction_filter.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/merge_operator.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/table.h"
#include "table/block_based_table_factory.h"
#include "table/internal_iterator.h"
#include "table/plain_table_factory.h"
#include "util/mutexlock.h"
#include "util/random.h"

namespace rocksdb {
class SequentialFile;
class SequentialFileReader;

namespace test {

//在*dst中存储一个长度为“len”的随机字符串，并返回一个
//引用生成的数据。
extern Slice RandomString(Random* rnd, int len, std::string* dst);

extern std::string RandomHumanReadableString(Random* rnd, int len);

//返回具有指定长度的随机键，该长度可能包含有趣的
//字符（例如\x00、\xff等）。
enum RandomKeyType : char { RANDOM, LARGEST, SMALLEST, MIDDLE };
extern std::string RandomKey(Random* rnd, int len,
                             RandomKeyType type = RandomKeyType::RANDOM);

//在*dst中存储一个长度为“len”的字符串，该字符串将压缩为
//“n*压缩分数”字节并返回引用的切片
//生成的数据。
extern Slice CompressibleString(Random* rnd, double compressed_fraction,
                                int len, std::string* dst);

//允许错误注入的包装器。
class ErrorEnv : public EnvWrapper {
 public:
  bool writable_file_error_;
  int num_writable_file_errors_;

  ErrorEnv() : EnvWrapper(Env::Default()),
               writable_file_error_(false),
               num_writable_file_errors_(0) { }

  virtual Status NewWritableFile(const std::string& fname,
                                 unique_ptr<WritableFile>* result,
                                 const EnvOptions& soptions) override {
    result->reset();
    if (writable_file_error_) {
      ++num_writable_file_errors_;
      return Status::IOError(fname, "fake error");
    }
    return target()->NewWritableFile(fname, result, soptions);
  }
};

//一种内部比较器，只向前比较
//其中的用户比较器。可用于测试没有依赖关系的实体
//在内部键结构上，但使用内部键比较器，例如
//基于块的表。
class PlainInternalKeyComparator : public InternalKeyComparator {
 public:
  explicit PlainInternalKeyComparator(const Comparator* c)
      : InternalKeyComparator(c) {}

  virtual ~PlainInternalKeyComparator() {}

  virtual int Compare(const Slice& a, const Slice& b) const override {
    return user_comparator()->Compare(a, b);
  }
  virtual void FindShortestSeparator(std::string* start,
                                     const Slice& limit) const override {
    user_comparator()->FindShortestSeparator(start, limit);
  }
  virtual void FindShortSuccessor(std::string* key) const override {
    user_comparator()->FindShortSuccessor(key);
  }
};

//用这种方式比较两个字符串的测试比较器：
//（1）首先按字母顺序比较8个字节的前缀，
//（2）如果两个字符串共享同一前缀，则对字符串的另一部分进行排序。
//按字母顺序倒序排列。
//这有助于模拟[实体][时间戳]的复合键的情况，以及
//首先是最新的时间戳。
class SimpleSuffixReverseComparator : public Comparator {
 public:
  SimpleSuffixReverseComparator() {}

  virtual const char* Name() const override {
    return "SimpleSuffixReverseComparator";
  }

  virtual int Compare(const Slice& a, const Slice& b) const override {
    Slice prefix_a = Slice(a.data(), 8);
    Slice prefix_b = Slice(b.data(), 8);
    int prefix_comp = prefix_a.compare(prefix_b);
    if (prefix_comp != 0) {
      return prefix_comp;
    } else {
      Slice suffix_a = Slice(a.data() + 8, a.size() - 8);
      Slice suffix_b = Slice(b.data() + 8, b.size() - 8);
      return -(suffix_a.compare(suffix_b));
    }
  }
  virtual void FindShortestSeparator(std::string* start,
                                     const Slice& limit) const override {}

  virtual void FindShortSuccessor(std::string* key) const override {}
};

//返回可用于比较两个uint64的用户键比较器
//片。它不是逐字节比较切片，而是比较所有8个字节
//马上。假设在数据库的生存期内使用相同的尾数。
//比较符号与比较符号的差别不大。
//Endian机器。
extern const Comparator* Uint64Comparator();

//迭代键/值的向量
class VectorIterator : public InternalIterator {
 public:
  explicit VectorIterator(const std::vector<std::string>& keys)
      : keys_(keys), current_(keys.size()) {
    std::sort(keys_.begin(), keys_.end());
    values_.resize(keys.size());
  }

  VectorIterator(const std::vector<std::string>& keys,
      const std::vector<std::string>& values)
    : keys_(keys), values_(values), current_(keys.size()) {
    assert(keys_.size() == values_.size());
  }

  virtual bool Valid() const override { return current_ < keys_.size(); }

  virtual void SeekToFirst() override { current_ = 0; }
  virtual void SeekToLast() override { current_ = keys_.size() - 1; }

  virtual void Seek(const Slice& target) override {
    current_ = std::lower_bound(keys_.begin(), keys_.end(), target.ToString()) -
               keys_.begin();
  }

  virtual void SeekForPrev(const Slice& target) override {
    current_ = std::upper_bound(keys_.begin(), keys_.end(), target.ToString()) -
               keys_.begin();
    if (!Valid()) {
      SeekToLast();
    } else {
      Prev();
    }
  }

  virtual void Next() override { current_++; }
  virtual void Prev() override { current_--; }

  virtual Slice key() const override { return Slice(keys_[current_]); }
  virtual Slice value() const override { return Slice(values_[current_]); }

  virtual Status status() const override { return Status::OK(); }

 private:
  std::vector<std::string> keys_;
  std::vector<std::string> values_;
  size_t current_;
};
extern WritableFileWriter* GetWritableFileWriter(WritableFile* wf);

extern RandomAccessFileReader* GetRandomAccessFileReader(RandomAccessFile* raf);

extern SequentialFileReader* GetSequentialFileReader(SequentialFile* se);

class StringSink: public WritableFile {
 public:
  std::string contents_;

  explicit StringSink(Slice* reader_contents = nullptr) :
      WritableFile(),
      contents_(""),
      reader_contents_(reader_contents),
      last_flush_(0) {
    if (reader_contents_ != nullptr) {
      *reader_contents_ = Slice(contents_.data(), 0);
    }
  }

  const std::string& contents() const { return contents_; }

  virtual Status Truncate(uint64_t size) override {
    contents_.resize(static_cast<size_t>(size));
    return Status::OK();
  }
  virtual Status Close() override { return Status::OK(); }
  virtual Status Flush() override {
    if (reader_contents_ != nullptr) {
      assert(reader_contents_->size() <= last_flush_);
      size_t offset = last_flush_ - reader_contents_->size();
      *reader_contents_ = Slice(
          contents_.data() + offset,
          contents_.size() - offset);
      last_flush_ = contents_.size();
    }

    return Status::OK();
  }
  virtual Status Sync() override { return Status::OK(); }
  virtual Status Append(const Slice& slice) override {
    contents_.append(slice.data(), slice.size());
    return Status::OK();
  }
  void Drop(size_t bytes) {
    if (reader_contents_ != nullptr) {
      contents_.resize(contents_.size() - bytes);
      *reader_contents_ = Slice(
          reader_contents_->data(), reader_contents_->size() - bytes);
      last_flush_ = contents_.size();
    }
  }

 private:
  Slice* reader_contents_;
  size_t last_flush_;
};

//一个围绕StringSink的包装器，为它提供一个RandomRwFile接口
class RandomRWStringSink : public RandomRWFile {
 public:
  explicit RandomRWStringSink(StringSink* ss) : ss_(ss) {}

  Status Write(uint64_t offset, const Slice& data) {
    if (offset + data.size() > ss_->contents_.size()) {
      ss_->contents_.resize(offset + data.size(), '\0');
    }

    char* pos = const_cast<char*>(ss_->contents_.data() + offset);
    memcpy(pos, data.data(), data.size());
    return Status::OK();
  }

  Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const {
    *result = Slice(nullptr, 0);
    if (offset < ss_->contents_.size()) {
      size_t str_res_sz =
          std::min(static_cast<size_t>(ss_->contents_.size() - offset), n);
      *result = Slice(ss_->contents_.data() + offset, str_res_sz);
    }
    return Status::OK();
  }

  Status Flush() { return Status::OK(); }

  Status Sync() { return Status::OK(); }

  Status Close() { return Status::OK(); }

  const std::string& contents() const { return ss_->contents(); }

 private:
  StringSink* ss_;
};

//和StringSink一样，它将写入一个字符串。取消链接StringSink，它
//有一些初始内容并覆盖它，就像回收
//日志文件。
class OverwritingStringSink : public WritableFile {
 public:
  explicit OverwritingStringSink(Slice* reader_contents)
      : WritableFile(),
        contents_(""),
        reader_contents_(reader_contents),
        last_flush_(0) {}

  const std::string& contents() const { return contents_; }

  virtual Status Truncate(uint64_t size) override {
    contents_.resize(static_cast<size_t>(size));
    return Status::OK();
  }
  virtual Status Close() override { return Status::OK(); }
  virtual Status Flush() override {
    if (last_flush_ < contents_.size()) {
      assert(reader_contents_->size() >= contents_.size());
      memcpy((char*)reader_contents_->data() + last_flush_,
             contents_.data() + last_flush_, contents_.size() - last_flush_);
      last_flush_ = contents_.size();
    }
    return Status::OK();
  }
  virtual Status Sync() override { return Status::OK(); }
  virtual Status Append(const Slice& slice) override {
    contents_.append(slice.data(), slice.size());
    return Status::OK();
  }
  void Drop(size_t bytes) {
    contents_.resize(contents_.size() - bytes);
    if (last_flush_ > contents_.size()) last_flush_ = contents_.size();
  }

 private:
  std::string contents_;
  Slice* reader_contents_;
  size_t last_flush_;
};

class StringSource: public RandomAccessFile {
 public:
  explicit StringSource(const Slice& contents, uint64_t uniq_id = 0,
                        bool mmap = false)
      : contents_(contents.data(), contents.size()),
        uniq_id_(uniq_id),
        mmap_(mmap),
        total_reads_(0) {}

  virtual ~StringSource() { }

  uint64_t Size() const { return contents_.size(); }

  virtual Status Read(uint64_t offset, size_t n, Slice* result,
      char* scratch) const override {
    total_reads_++;
    if (offset > contents_.size()) {
      return Status::InvalidArgument("invalid Read offset");
    }
    if (offset + n > contents_.size()) {
      n = contents_.size() - static_cast<size_t>(offset);
    }
    if (!mmap_) {
      memcpy(scratch, &contents_[static_cast<size_t>(offset)], n);
      *result = Slice(scratch, n);
    } else {
      *result = Slice(&contents_[static_cast<size_t>(offset)], n);
    }
    return Status::OK();
  }

  virtual size_t GetUniqueId(char* id, size_t max_size) const override {
    if (max_size < 20) {
      return 0;
    }

    char* rid = id;
    rid = EncodeVarint64(rid, uniq_id_);
    rid = EncodeVarint64(rid, 0);
    return static_cast<size_t>(rid-id);
  }

  int total_reads() const { return total_reads_; }

  void set_total_reads(int tr) { total_reads_ = tr; }

 private:
  std::string contents_;
  uint64_t uniq_id_;
  bool mmap_;
  mutable int total_reads_;
};

class NullLogger : public Logger {
 public:
  using Logger::Logv;
  virtual void Logv(const char* format, va_list ap) override {}
  virtual size_t GetLogFileSize() const override { return 0; }
};

//通过更改类型损坏密钥
extern void CorruptKeyType(InternalKey* ikey);

extern std::string KeyStr(const std::string& user_key,
                          const SequenceNumber& seq, const ValueType& t,
                          bool corrupt = false);

class SleepingBackgroundTask {
 public:
  SleepingBackgroundTask()
      : bg_cv_(&mutex_),
        should_sleep_(true),
        done_with_sleep_(false),
        sleeping_(false) {}

  bool IsSleeping() {
    MutexLock l(&mutex_);
    return sleeping_;
  }
  void DoSleep() {
    MutexLock l(&mutex_);
    sleeping_ = true;
    bg_cv_.SignalAll();
    while (should_sleep_) {
      bg_cv_.Wait();
    }
    sleeping_ = false;
    done_with_sleep_ = true;
    bg_cv_.SignalAll();
  }
  void WaitUntilSleeping() {
    MutexLock l(&mutex_);
    while (!sleeping_ || !should_sleep_) {
      bg_cv_.Wait();
    }
  }
  void WakeUp() {
    MutexLock l(&mutex_);
    should_sleep_ = false;
    bg_cv_.SignalAll();
  }
  void WaitUntilDone() {
    MutexLock l(&mutex_);
    while (!done_with_sleep_) {
      bg_cv_.Wait();
    }
  }
  bool WokenUp() {
    MutexLock l(&mutex_);
    return should_sleep_ == false;
  }

  void Reset() {
    MutexLock l(&mutex_);
    should_sleep_ = true;
    done_with_sleep_ = false;
  }

  static void DoSleepTask(void* arg) {
    reinterpret_cast<SleepingBackgroundTask*>(arg)->DoSleep();
  }

 private:
  port::Mutex mutex_;
port::CondVar bg_cv_;  //后台工作完成时发出信号
  bool should_sleep_;
  bool done_with_sleep_;
  bool sleeping_;
};

//筛选器合并等于“num”的操作数和值。
class FilterNumber : public CompactionFilter {
 public:
  explicit FilterNumber(uint64_t num) : num_(num) {}

  std::string last_merge_operand_key() { return last_merge_operand_key_; }

  bool Filter(int level, const rocksdb::Slice& key, const rocksdb::Slice& value,
              std::string* new_value, bool* value_changed) const override {
    if (value.size() == sizeof(uint64_t)) {
      return num_ == DecodeFixed64(value.data());
    }
    return true;
  }

  bool FilterMergeOperand(int level, const rocksdb::Slice& key,
                          const rocksdb::Slice& value) const override {
    last_merge_operand_key_ = key.ToString();
    if (value.size() == sizeof(uint64_t)) {
      return num_ == DecodeFixed64(value.data());
    }
    return true;
  }

  const char* Name() const override { return "FilterBadMergeOperand"; }

 private:
  mutable std::string last_merge_operand_key_;
  uint64_t num_;
};

inline std::string EncodeInt(uint64_t x) {
  std::string result;
  PutFixed64(&result, x);
  return result;
}

class StringEnv : public EnvWrapper {
 public:
  class SeqStringSource : public SequentialFile {
   public:
    explicit SeqStringSource(const std::string& data)
        : data_(data), offset_(0) {}
    ~SeqStringSource() {}
    Status Read(size_t n, Slice* result, char* scratch) override {
      std::string output;
      if (offset_ < data_.size()) {
        n = std::min(data_.size() - offset_, n);
        memcpy(scratch, data_.data() + offset_, n);
        offset_ += n;
        *result = Slice(scratch, n);
      } else {
        return Status::InvalidArgument(
            "Attemp to read when it already reached eof.");
      }
      return Status::OK();
    }
    Status Skip(uint64_t n) override {
      if (offset_ >= data_.size()) {
        return Status::InvalidArgument(
            "Attemp to read when it already reached eof.");
      }
//TODO（YCHIANG）：目前不处理溢出情况。
      offset_ += n;
      return Status::OK();
    }

   private:
    std::string data_;
    size_t offset_;
  };

  class StringSink : public WritableFile {
   public:
    explicit StringSink(std::string* contents)
        : WritableFile(), contents_(contents) {}
    virtual Status Truncate(uint64_t size) override {
      contents_->resize(size);
      return Status::OK();
    }
    virtual Status Close() override { return Status::OK(); }
    virtual Status Flush() override { return Status::OK(); }
    virtual Status Sync() override { return Status::OK(); }
    virtual Status Append(const Slice& slice) override {
      contents_->append(slice.data(), slice.size());
      return Status::OK();
    }

   private:
    std::string* contents_;
  };

  explicit StringEnv(Env* t) : EnvWrapper(t) {}
  virtual ~StringEnv() {}

  const std::string& GetContent(const std::string& f) { return files_[f]; }

  const Status WriteToNewFile(const std::string& file_name,
                              const std::string& content) {
    unique_ptr<WritableFile> r;
    auto s = NewWritableFile(file_name, &r, EnvOptions());
    if (!s.ok()) {
      return s;
    }
    r->Append(content);
    r->Flush();
    r->Close();
    assert(files_[file_name] == content);
    return Status::OK();
  }

//下面的文本是将所有方法转发到target（）的样板文件。
  Status NewSequentialFile(const std::string& f, unique_ptr<SequentialFile>* r,
                           const EnvOptions& options) override {
    auto iter = files_.find(f);
    if (iter == files_.end()) {
      return Status::NotFound("The specified file does not exist", f);
    }
    r->reset(new SeqStringSource(iter->second));
    return Status::OK();
  }
  Status NewRandomAccessFile(const std::string& f,
                             unique_ptr<RandomAccessFile>* r,
                             const EnvOptions& options) override {
    return Status::NotSupported();
  }
  Status NewWritableFile(const std::string& f, unique_ptr<WritableFile>* r,
                         const EnvOptions& options) override {
    auto iter = files_.find(f);
    if (iter != files_.end()) {
      return Status::IOError("The specified file already exists", f);
    }
    r->reset(new StringSink(&files_[f]));
    return Status::OK();
  }
  virtual Status NewDirectory(const std::string& name,
                              unique_ptr<Directory>* result) override {
    return Status::NotSupported();
  }
  Status FileExists(const std::string& f) override {
    if (files_.find(f) == files_.end()) {
      return Status::NotFound();
    }
    return Status::OK();
  }
  Status GetChildren(const std::string& dir,
                     std::vector<std::string>* r) override {
    return Status::NotSupported();
  }
  Status DeleteFile(const std::string& f) override {
    files_.erase(f);
    return Status::OK();
  }
  Status CreateDir(const std::string& d) override {
    return Status::NotSupported();
  }
  Status CreateDirIfMissing(const std::string& d) override {
    return Status::NotSupported();
  }
  Status DeleteDir(const std::string& d) override {
    return Status::NotSupported();
  }
  Status GetFileSize(const std::string& f, uint64_t* s) override {
    auto iter = files_.find(f);
    if (iter == files_.end()) {
      return Status::NotFound("The specified file does not exist:", f);
    }
    *s = iter->second.size();
    return Status::OK();
  }

  Status GetFileModificationTime(const std::string& fname,
                                 uint64_t* file_mtime) override {
    return Status::NotSupported();
  }

  Status RenameFile(const std::string& s, const std::string& t) override {
    return Status::NotSupported();
  }

  Status LinkFile(const std::string& s, const std::string& t) override {
    return Status::NotSupported();
  }

  Status LockFile(const std::string& f, FileLock** l) override {
    return Status::NotSupported();
  }

  Status UnlockFile(FileLock* l) override { return Status::NotSupported(); }

 protected:
  std::unordered_map<std::string, std::string> files_;
};

//随机初始化给定的dboptions
void RandomInitDBOptions(DBOptions* db_opt, Random* rnd);

//随机初始化给定的列FamilyOptions
//请注意，调用方负责释放非空值
//cf_opt->compaction_filter（压缩过滤器）。
void RandomInitCFOptions(ColumnFamilyOptions* cf_opt, Random* rnd);

//可以更改其名称的伪合并运算符
class ChanglingMergeOperator : public MergeOperator {
 public:
  explicit ChanglingMergeOperator(const std::string& name)
      : name_(name + "MergeOperator") {}
  ~ChanglingMergeOperator() {}

  void SetName(const std::string& name) { name_ = name; }

  virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                           MergeOperationOutput* merge_out) const override {
    return false;
  }
  virtual bool PartialMergeMulti(const Slice& key,
                                 const std::deque<Slice>& operand_list,
                                 std::string* new_value,
                                 Logger* logger) const override {
    return false;
  }
  virtual const char* Name() const override { return name_.c_str(); }

 protected:
  std::string name_;
};

//返回具有随机名称的伪合并运算符。
MergeOperator* RandomMergeOperator(Random* rnd);

//可以更改其名称的虚拟压缩过滤器
class ChanglingCompactionFilter : public CompactionFilter {
 public:
  explicit ChanglingCompactionFilter(const std::string& name)
      : name_(name + "CompactionFilter") {}
  ~ChanglingCompactionFilter() {}

  void SetName(const std::string& name) { name_ = name; }

  bool Filter(int level, const Slice& key, const Slice& existing_value,
              std::string* new_value, bool* value_changed) const override {
    return false;
  }

  const char* Name() const override { return name_.c_str(); }

 private:
  std::string name_;
};

//返回具有随机名称的虚拟压缩筛选器。
CompactionFilter* RandomCompactionFilter(Random* rnd);

//可以更改其名称的虚拟压缩过滤器工厂
class ChanglingCompactionFilterFactory : public CompactionFilterFactory {
 public:
  explicit ChanglingCompactionFilterFactory(const std::string& name)
      : name_(name + "CompactionFilterFactory") {}
  ~ChanglingCompactionFilterFactory() {}

  void SetName(const std::string& name) { name_ = name; }

  std::unique_ptr<CompactionFilter> CreateCompactionFilter(
      const CompactionFilter::Context& context) override {
    return std::unique_ptr<CompactionFilter>();
  }

//返回标识此压缩筛选器工厂的名称。
  const char* Name() const override { return name_.c_str(); }

 protected:
  std::string name_;
};

CompressionType RandomCompressionType(Random* rnd);

void RandomCompressionTypeVector(const size_t count,
                                 std::vector<CompressionType>* types,
                                 Random* rnd);

CompactionFilterFactory* RandomCompactionFilterFactory(Random* rnd);

const SliceTransform* RandomSliceTransform(Random* rnd, int pre_defined = -1);

TableFactory* RandomTableFactory(Random* rnd, int pre_defined = -1);

std::string RandomName(Random* rnd, const size_t len);

Status DestroyDir(Env* env, const std::string& dir);

}  //命名空间测试
}  //命名空间rocksdb
