
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
#pragma once

#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "util/kv_map.h"
#include "port/port.h"
#include "rocksdb/comparator.h"
#include "rocksdb/table.h"
#include "table/internal_iterator.h"
#include "table/table_builder.h"
#include "table/table_reader.h"
#include "util/mutexlock.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace rocksdb {
namespace mock {

stl_wrappers::KVMap MakeMockFile(
    std::initializer_list<std::pair<const std::string, std::string>> l = {});

struct MockTableFileSystem {
  port::Mutex mutex;
  std::map<uint32_t, stl_wrappers::KVMap> files;
};

class MockTableReader : public TableReader {
 public:
  explicit MockTableReader(const stl_wrappers::KVMap& table) : table_(table) {}

  InternalIterator* NewIterator(const ReadOptions&,
                                Arena* arena,
                                bool skip_filters = false) override;

  Status Get(const ReadOptions&, const Slice& key, GetContext* get_context,
             bool skip_filters = false) override;

  uint64_t ApproximateOffsetOf(const Slice& key) override { return 0; }

  virtual size_t ApproximateMemoryUsage() const override { return 0; }

  void SetupForCompaction() override {}

  std::shared_ptr<const TableProperties> GetTableProperties() const override;

  ~MockTableReader() {}

 private:
  const stl_wrappers::KVMap& table_;
};

class MockTableIterator : public InternalIterator {
 public:
  explicit MockTableIterator(const stl_wrappers::KVMap& table) : table_(table) {
    itr_ = table_.end();
  }

  bool Valid() const override { return itr_ != table_.end(); }

  void SeekToFirst() override { itr_ = table_.begin(); }

  void SeekToLast() override {
    itr_ = table_.end();
    --itr_;
  }

  void Seek(const Slice& target) override {
    std::string str_target(target.data(), target.size());
    itr_ = table_.lower_bound(str_target);
  }

  void SeekForPrev(const Slice& target) override {
    std::string str_target(target.data(), target.size());
    itr_ = table_.upper_bound(str_target);
    Prev();
  }

  void Next() override { ++itr_; }

  void Prev() override {
    if (itr_ == table_.begin()) {
      itr_ = table_.end();
    } else {
      --itr_;
    }
  }

  Slice key() const override { return Slice(itr_->first); }

  Slice value() const override { return Slice(itr_->second); }

  Status status() const override { return Status::OK(); }

 private:
  const stl_wrappers::KVMap& table_;
  stl_wrappers::KVMap::const_iterator itr_;
};

class MockTableBuilder : public TableBuilder {
 public:
  MockTableBuilder(uint32_t id, MockTableFileSystem* file_system)
      : id_(id), file_system_(file_system) {
    table_ = MakeMockFile({});
  }

//要求：已调用finish（）或放弃（）。
  ~MockTableBuilder() {}

//向正在构造的表添加键和值。
//要求：根据比较器，键在任何先前添加的键之后。
//要求：finish（），放弃（）尚未调用
  void Add(const Slice& key, const Slice& value) override {
    table_.insert({key.ToString(), value.ToString()});
  }

//如果检测到一些错误，返回非OK。
  Status status() const override { return Status::OK(); }

  Status Finish() override {
    MutexLock lock_guard(&file_system_->mutex);
    file_system_->files.insert({id_, table_});
    return Status::OK();
  }

  void Abandon() override {}

  uint64_t NumEntries() const override { return table_.size(); }

  uint64_t FileSize() const override { return table_.size(); }

  TableProperties GetTableProperties() const override {
    return TableProperties();
  }

 private:
  uint32_t id_;
  MockTableFileSystem* file_system_;
  stl_wrappers::KVMap table_;
};

class MockTableFactory : public TableFactory {
 public:
  MockTableFactory();
  const char* Name() const override { return "MockTable"; }
  Status NewTableReader(
      const TableReaderOptions& table_reader_options,
      unique_ptr<RandomAccessFileReader>&& file, uint64_t file_size,
      unique_ptr<TableReader>* table_reader,
      bool prefetch_index_and_filter_in_cache = true) const override;
  TableBuilder* NewTableBuilder(
      const TableBuilderOptions& table_builder_options,
      uint32_t column_familly_id, WritableFileWriter* file) const override;

//此函数将直接创建模拟表，而不是通过
//MockTableBuilder。文件目录的格式必须为<internal_key，
//值>然后将这些键值对插入模拟表中。
  Status CreateMockTable(Env* env, const std::string& fname,
                         stl_wrappers::KVMap file_contents);

  virtual Status SanitizeOptions(
      const DBOptions& db_opts,
      const ColumnFamilyOptions& cf_opts) const override {
    return Status::OK();
  }

  virtual std::string GetPrintableTableOptions() const override {
    return std::string();
  }

//此函数将断言只有一个文件存在，并且
//内容等于文件目录
  void AssertSingleFile(const stl_wrappers::KVMap& file_contents);
  void AssertLatestFile(const stl_wrappers::KVMap& file_contents);

 private:
  uint32_t GetAndWriteNextID(WritableFileWriter* file) const;
  uint32_t GetIDFromFile(RandomAccessFileReader* file) const;

  mutable MockTableFileSystem file_system_;
  mutable std::atomic<uint32_t> next_id_;
};

}  //命名空间模拟
}  //命名空间rocksdb
