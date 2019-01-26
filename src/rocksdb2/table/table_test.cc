
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

#include <stdio.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cache/lru_cache.h"
#include "db/dbformat.h"
#include "db/memtable.h"
#include "db/write_batch_internal.h"
#include "memtable/stl_wrappers.h"
#include "monitoring/statistics.h"
#include "port/port.h"
#include "rocksdb/cache.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/perf_context.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/statistics.h"
#include "rocksdb/write_buffer_manager.h"
#include "table/block.h"
#include "table/block_based_table_builder.h"
#include "table/block_based_table_factory.h"
#include "table/block_based_table_reader.h"
#include "table/block_builder.h"
#include "table/format.h"
#include "table/get_context.h"
#include "table/internal_iterator.h"
#include "table/meta_blocks.h"
#include "table/plain_table_factory.h"
#include "table/scoped_arena_iterator.h"
#include "table/sst_file_writer_collectors.h"
#include "util/compression.h"
#include "util/random.h"
#include "util/string_util.h"
#include "util/sync_point.h"
#include "util/testharness.h"
#include "util/testutil.h"
#include "utilities/merge_operators.h"

namespace rocksdb {

extern const uint64_t kLegacyBlockBasedTableMagicNumber;
extern const uint64_t kLegacyPlainTableMagicNumber;
extern const uint64_t kBlockBasedTableMagicNumber;
extern const uint64_t kPlainTableMagicNumber;

namespace {

//用于测试基于块的表属性的DummyPropertiesCollector
class DummyPropertiesCollector : public TablePropertiesCollector {
 public:
  const char* Name() const { return ""; }

  Status Finish(UserCollectedProperties* properties) { return Status::OK(); }

  Status Add(const Slice& user_key, const Slice& value) { return Status::OK(); }

  virtual UserCollectedProperties GetReadableProperties() const {
    return UserCollectedProperties{};
  }
};

class DummyPropertiesCollectorFactory1
    : public TablePropertiesCollectorFactory {
 public:
  virtual TablePropertiesCollector* CreateTablePropertiesCollector(
      TablePropertiesCollectorFactory::Context context) {
    return new DummyPropertiesCollector();
  }
  const char* Name() const { return "DummyPropertiesCollector1"; }
};

class DummyPropertiesCollectorFactory2
    : public TablePropertiesCollectorFactory {
 public:
  virtual TablePropertiesCollector* CreateTablePropertiesCollector(
      TablePropertiesCollectorFactory::Context context) {
    return new DummyPropertiesCollector();
  }
  const char* Name() const { return "DummyPropertiesCollector2"; }
};

//返回“键”的相反方向。
//用于测试非词典比较器。
std::string Reverse(const Slice& key) {
  auto rev = key.ToString();
  std::reverse(rev.begin(), rev.end());
  return rev;
}

class ReverseKeyComparator : public Comparator {
 public:
  virtual const char* Name() const override {
    return "rocksdb.ReverseBytewiseComparator";
  }

  virtual int Compare(const Slice& a, const Slice& b) const override {
    return BytewiseComparator()->Compare(Reverse(a), Reverse(b));
  }

  virtual void FindShortestSeparator(std::string* start,
                                     const Slice& limit) const override {
    std::string s = Reverse(*start);
    std::string l = Reverse(limit);
    BytewiseComparator()->FindShortestSeparator(&s, l);
    *start = Reverse(s);
  }

  virtual void FindShortSuccessor(std::string* key) const override {
    std::string s = Reverse(*key);
    BytewiseComparator()->FindShortSuccessor(&s);
    *key = Reverse(s);
  }
};

ReverseKeyComparator reverse_key_comparator;

void Increment(const Comparator* cmp, std::string* key) {
  if (cmp == BytewiseComparator()) {
    key->push_back('\0');
  } else {
    assert(cmp == &reverse_key_comparator);
    std::string rev = Reverse(*key);
    rev.push_back('\0');
    *key = Reverse(rev);
  }
}

}  //命名空间

//用于测试的帮助程序类，以统一
//blockbuilder/tablebuilder和block/table。
class Constructor {
 public:
  explicit Constructor(const Comparator* cmp)
      : data_(stl_wrappers::LessOfComparator(cmp)) {}
  virtual ~Constructor() { }

  void Add(const std::string& key, const Slice& value) {
    data_[key] = value.ToString();
  }

//使用所有具有
//到目前为止已添加。按“*键”顺序返回键
//并将键/值对存储在“*kvmap”中。
  void Finish(const Options& options, const ImmutableCFOptions& ioptions,
              const BlockBasedTableOptions& table_options,
              const InternalKeyComparator& internal_comparator,
              std::vector<std::string>* keys, stl_wrappers::KVMap* kvmap) {
    last_internal_key_ = &internal_comparator;
    *kvmap = data_;
    keys->clear();
    for (const auto& kv : data_) {
      keys->push_back(kv.first);
    }
    data_.clear();
    Status s = FinishImpl(options, ioptions, table_options,
                          internal_comparator, *kvmap);
    ASSERT_TRUE(s.ok()) << s.ToString();
  }

//从“数据”中的数据构造数据结构
  virtual Status FinishImpl(const Options& options,
                            const ImmutableCFOptions& ioptions,
                            const BlockBasedTableOptions& table_options,
                            const InternalKeyComparator& internal_comparator,
                            const stl_wrappers::KVMap& data) = 0;

  virtual InternalIterator* NewIterator() const = 0;

  virtual const stl_wrappers::KVMap& data() { return data_; }

  virtual bool IsArenaMode() const { return false; }

virtual DB* db() const { return nullptr; }  //在dbconstructor中重写

  virtual bool AnywayDeleteIterator() const { return false; }

 protected:
  const InternalKeyComparator* last_internal_key_;

 private:
  stl_wrappers::KVMap data_;
};

class BlockConstructor: public Constructor {
 public:
  explicit BlockConstructor(const Comparator* cmp)
      : Constructor(cmp),
        comparator_(cmp),
        block_(nullptr) { }
  ~BlockConstructor() {
    delete block_;
  }
  virtual Status FinishImpl(const Options& options,
                            const ImmutableCFOptions& ioptions,
                            const BlockBasedTableOptions& table_options,
                            const InternalKeyComparator& internal_comparator,
                            const stl_wrappers::KVMap& kv_map) override {
    delete block_;
    block_ = nullptr;
    BlockBuilder builder(table_options.block_restart_interval);

    for (const auto kv : kv_map) {
      builder.Add(kv.first, kv.second);
    }
//打开街区
    data_ = builder.Finish().ToString();
    BlockContents contents;
    contents.data = data_;
    contents.cachable = false;
    block_ = new Block(std::move(contents), kDisableGlobalSequenceNumber);
    return Status::OK();
  }
  virtual InternalIterator* NewIterator() const override {
    return block_->NewIterator(comparator_);
  }

 private:
  const Comparator* comparator_;
  std::string data_;
  Block* block_;

  BlockConstructor();
};

//将内部格式键转换为用户键的帮助程序类
class KeyConvertingIterator : public InternalIterator {
 public:
  explicit KeyConvertingIterator(InternalIterator* iter,
                                 bool arena_mode = false)
      : iter_(iter), arena_mode_(arena_mode) {}
  virtual ~KeyConvertingIterator() {
    if (arena_mode_) {
      iter_->~InternalIterator();
    } else {
      delete iter_;
    }
  }
  virtual bool Valid() const override { return iter_->Valid(); }
  virtual void Seek(const Slice& target) override {
    ParsedInternalKey ikey(target, kMaxSequenceNumber, kTypeValue);
    std::string encoded;
    AppendInternalKey(&encoded, ikey);
    iter_->Seek(encoded);
  }
  virtual void SeekForPrev(const Slice& target) override {
    ParsedInternalKey ikey(target, kMaxSequenceNumber, kTypeValue);
    std::string encoded;
    AppendInternalKey(&encoded, ikey);
    iter_->SeekForPrev(encoded);
  }
  virtual void SeekToFirst() override { iter_->SeekToFirst(); }
  virtual void SeekToLast() override { iter_->SeekToLast(); }
  virtual void Next() override { iter_->Next(); }
  virtual void Prev() override { iter_->Prev(); }

  virtual Slice key() const override {
    assert(Valid());
    ParsedInternalKey parsed_key;
    if (!ParseInternalKey(iter_->key(), &parsed_key)) {
      status_ = Status::Corruption("malformed internal key");
      return Slice("corrupted key");
    }
    return parsed_key.user_key;
  }

  virtual Slice value() const override { return iter_->value(); }
  virtual Status status() const override {
    return status_.ok() ? iter_->status() : status_;
  }

 private:
  mutable Status status_;
  InternalIterator* iter_;
  bool arena_mode_;

//不允许复制
  KeyConvertingIterator(const KeyConvertingIterator&);
  void operator=(const KeyConvertingIterator&);
};

class TableConstructor: public Constructor {
 public:
  explicit TableConstructor(const Comparator* cmp,
                            bool convert_to_internal_key = false)
      : Constructor(cmp),
        convert_to_internal_key_(convert_to_internal_key) {}
  ~TableConstructor() { Reset(); }

  virtual Status FinishImpl(const Options& options,
                            const ImmutableCFOptions& ioptions,
                            const BlockBasedTableOptions& table_options,
                            const InternalKeyComparator& internal_comparator,
                            const stl_wrappers::KVMap& kv_map) override {
    Reset();
    soptions.use_mmap_reads = ioptions.allow_mmap_reads;
    file_writer_.reset(test::GetWritableFileWriter(new test::StringSink()));
    unique_ptr<TableBuilder> builder;
    std::vector<std::unique_ptr<IntTblPropCollectorFactory>>
        int_tbl_prop_collector_factories;
    std::string column_family_name;
    int unknown_level = -1;
    builder.reset(ioptions.table_factory->NewTableBuilder(
        TableBuilderOptions(ioptions, internal_comparator,
                            &int_tbl_prop_collector_factories,
                            options.compression, CompressionOptions(),
                            /*lptr/*压缩\u dict*/，
                            假/*跳过过滤器*/, column_family_name,

                            unknown_level),
        TablePropertiesCollectorFactory::Context::kUnknownColumnFamily,
        file_writer_.get()));

    for (const auto kv : kv_map) {
      if (convert_to_internal_key_) {
        ParsedInternalKey ikey(kv.first, kMaxSequenceNumber, kTypeValue);
        std::string encoded;
        AppendInternalKey(&encoded, ikey);
        builder->Add(encoded, kv.second);
      } else {
        builder->Add(kv.first, kv.second);
      }
      EXPECT_TRUE(builder->status().ok());
    }
    Status s = builder->Finish();
    file_writer_->Flush();
    EXPECT_TRUE(s.ok()) << s.ToString();

    EXPECT_EQ(GetSink()->contents().size(), builder->FileSize());

//打开桌子
    uniq_id_ = cur_uniq_id_++;
    file_reader_.reset(test::GetRandomAccessFileReader(new test::StringSource(
        GetSink()->contents(), uniq_id_, ioptions.allow_mmap_reads)));
    return ioptions.table_factory->NewTableReader(
        TableReaderOptions(ioptions, soptions, internal_comparator),
        std::move(file_reader_), GetSink()->contents().size(), &table_reader_);
  }

  virtual InternalIterator* NewIterator() const override {
    ReadOptions ro;
    InternalIterator* iter = table_reader_->NewIterator(ro);
    if (convert_to_internal_key_) {
      return new KeyConvertingIterator(iter);
    } else {
      return iter;
    }
  }

  uint64_t ApproximateOffsetOf(const Slice& key) const {
    if (convert_to_internal_key_) {
      InternalKey ikey(key, kMaxSequenceNumber, kTypeValue);
      const Slice skey = ikey.Encode();
      return table_reader_->ApproximateOffsetOf(skey);
    }
    return table_reader_->ApproximateOffsetOf(key);
  }

  virtual Status Reopen(const ImmutableCFOptions& ioptions) {
    file_reader_.reset(test::GetRandomAccessFileReader(new test::StringSource(
        GetSink()->contents(), uniq_id_, ioptions.allow_mmap_reads)));
    return ioptions.table_factory->NewTableReader(
        TableReaderOptions(ioptions, soptions, *last_internal_key_),
        std::move(file_reader_), GetSink()->contents().size(), &table_reader_);
  }

  virtual TableReader* GetTableReader() {
    return table_reader_.get();
  }

  virtual bool AnywayDeleteIterator() const override {
    return convert_to_internal_key_;
  }

  void ResetTableReader() { table_reader_.reset(); }

  bool ConvertToInternalKey() { return convert_to_internal_key_; }

 private:
  void Reset() {
    uniq_id_ = 0;
    table_reader_.reset();
    file_writer_.reset();
    file_reader_.reset();
  }

  test::StringSink* GetSink() {
    return static_cast<test::StringSink*>(file_writer_->writable_file());
  }

  uint64_t uniq_id_;
  unique_ptr<WritableFileWriter> file_writer_;
  unique_ptr<RandomAccessFileReader> file_reader_;
  unique_ptr<TableReader> table_reader_;
  bool convert_to_internal_key_;

  TableConstructor();

  static uint64_t cur_uniq_id_;
  EnvOptions soptions;
};
uint64_t TableConstructor::cur_uniq_id_ = 1;

class MemTableConstructor: public Constructor {
 public:
  explicit MemTableConstructor(const Comparator* cmp, WriteBufferManager* wb)
      : Constructor(cmp),
        internal_comparator_(cmp),
        write_buffer_manager_(wb),
        table_factory_(new SkipListFactory) {
    options_.memtable_factory = table_factory_;
    ImmutableCFOptions ioptions(options_);
    memtable_ =
        new MemTable(internal_comparator_, ioptions, MutableCFOptions(options_),
                     /*kmaxSequenceNumber，0/*列_-family_-id*/）；
    memtable_->ref（）；
  }
  ~memTableConstructor（）
    删除memtable_->unref（）；
  }
  虚拟状态完成impl（const options&，const immutablecfoptions&ioptions，
                            Const BlockBasedTableOptions和Table_选项，
                            常量InternalKeyComparator和Internal_Comparator，
                            const stl_wrappers:：kv map&kv_map）override_
    删除memtable_->unref（）；
    不变的参数（IOPTIONS）；
    memtable_ux=新memtable（内部_比较器_u、mem_IOptions，
                             可变的选项（选项），写缓冲区管理器，
                             kmaxsequencenumber，0/*列\u族\u id*/);

    memtable_->Ref();
    int seq = 1;
    for (const auto kv : kv_map) {
      memtable_->Add(seq, kTypeValue, kv.first, kv.second);
      seq++;
    }
    return Status::OK();
  }
  virtual InternalIterator* NewIterator() const override {
    return new KeyConvertingIterator(
        memtable_->NewIterator(ReadOptions(), &arena_), true);
  }

  virtual bool AnywayDeleteIterator() const override { return true; }

  virtual bool IsArenaMode() const override { return true; }

 private:
  mutable Arena arena_;
  InternalKeyComparator internal_comparator_;
  Options options_;
  WriteBufferManager* write_buffer_manager_;
  MemTable* memtable_;
  std::shared_ptr<SkipListFactory> table_factory_;
};

class InternalIteratorFromIterator : public InternalIterator {
 public:
  explicit InternalIteratorFromIterator(Iterator* it) : it_(it) {}
  virtual bool Valid() const override { return it_->Valid(); }
  virtual void Seek(const Slice& target) override { it_->Seek(target); }
  virtual void SeekForPrev(const Slice& target) override {
    it_->SeekForPrev(target);
  }
  virtual void SeekToFirst() override { it_->SeekToFirst(); }
  virtual void SeekToLast() override { it_->SeekToLast(); }
  virtual void Next() override { it_->Next(); }
  virtual void Prev() override { it_->Prev(); }
  Slice key() const override { return it_->key(); }
  Slice value() const override { return it_->value(); }
  virtual Status status() const override { return it_->status(); }

 private:
  unique_ptr<Iterator> it_;
};

class DBConstructor: public Constructor {
 public:
  explicit DBConstructor(const Comparator* cmp)
      : Constructor(cmp),
        comparator_(cmp) {
    db_ = nullptr;
    NewDB();
  }
  ~DBConstructor() {
    delete db_;
  }
  virtual Status FinishImpl(const Options& options,
                            const ImmutableCFOptions& ioptions,
                            const BlockBasedTableOptions& table_options,
                            const InternalKeyComparator& internal_comparator,
                            const stl_wrappers::KVMap& kv_map) override {
    delete db_;
    db_ = nullptr;
    NewDB();
    for (const auto kv : kv_map) {
      WriteBatch batch;
      batch.Put(kv.first, kv.second);
      EXPECT_TRUE(db_->Write(WriteOptions(), &batch).ok());
    }
    return Status::OK();
  }

  virtual InternalIterator* NewIterator() const override {
    return new InternalIteratorFromIterator(db_->NewIterator(ReadOptions()));
  }

  virtual DB* db() const override { return db_; }

 private:
  void NewDB() {
    std::string name = test::TmpDir() + "/table_testdb";

    Options options;
    options.comparator = comparator_;
    Status status = DestroyDB(name, options);
    ASSERT_TRUE(status.ok()) << status.ToString();

    options.create_if_missing = true;
    options.error_if_exists = true;
options.write_buffer_size = 10000;  //小到可以强制合并的东西
    status = DB::Open(options, name, &db_);
    ASSERT_TRUE(status.ok()) << status.ToString();
  }

  const Comparator* comparator_;
  DB* db_;
};

enum TestType {
  BLOCK_BASED_TABLE_TEST,
#ifndef ROCKSDB_LITE
  PLAIN_TABLE_SEMI_FIXED_PREFIX,
  PLAIN_TABLE_FULL_STR_PREFIX,
  PLAIN_TABLE_TOTAL_ORDER,
#endif  //！摇滚乐
  BLOCK_TEST,
  MEMTABLE_TEST,
  DB_TEST
};

struct TestArgs {
  TestType type;
  bool reverse_compare;
  int restart_interval;
  CompressionType compression;
  uint32_t format_version;
  bool use_mmap;
};

static std::vector<TestArgs> GenerateArgList() {
  std::vector<TestArgs> test_args;
  std::vector<TestType> test_types = {
      BLOCK_BASED_TABLE_TEST,
#ifndef ROCKSDB_LITE
      PLAIN_TABLE_SEMI_FIXED_PREFIX,
      PLAIN_TABLE_FULL_STR_PREFIX,
      PLAIN_TABLE_TOTAL_ORDER,
#endif  //！摇滚乐
      BLOCK_TEST,
      MEMTABLE_TEST, DB_TEST};
  std::vector<bool> reverse_compare_types = {false, true};
  std::vector<int> restart_intervals = {16, 1, 1024};

//仅在支持时添加压缩
  std::vector<std::pair<CompressionType, bool>> compression_types;
  compression_types.emplace_back(kNoCompression, false);
  if (Snappy_Supported()) {
    compression_types.emplace_back(kSnappyCompression, false);
  }
  if (Zlib_Supported()) {
    compression_types.emplace_back(kZlibCompression, false);
    compression_types.emplace_back(kZlibCompression, true);
  }
  if (BZip2_Supported()) {
    compression_types.emplace_back(kBZip2Compression, false);
    compression_types.emplace_back(kBZip2Compression, true);
  }
  if (LZ4_Supported()) {
    compression_types.emplace_back(kLZ4Compression, false);
    compression_types.emplace_back(kLZ4Compression, true);
    compression_types.emplace_back(kLZ4HCCompression, false);
    compression_types.emplace_back(kLZ4HCCompression, true);
  }
  if (XPRESS_Supported()) {
    compression_types.emplace_back(kXpressCompression, false);
    compression_types.emplace_back(kXpressCompression, true);
  }
  if (ZSTD_Supported()) {
    compression_types.emplace_back(kZSTD, false);
    compression_types.emplace_back(kZSTD, true);
  }

  for (auto test_type : test_types) {
    for (auto reverse_compare : reverse_compare_types) {
#ifndef ROCKSDB_LITE
      if (test_type == PLAIN_TABLE_SEMI_FIXED_PREFIX ||
          test_type == PLAIN_TABLE_FULL_STR_PREFIX ||
          test_type == PLAIN_TABLE_TOTAL_ORDER) {
//普通表不使用重新启动索引或压缩。
        TestArgs one_arg;
        one_arg.type = test_type;
        one_arg.reverse_compare = reverse_compare;
        one_arg.restart_interval = restart_intervals[0];
        one_arg.compression = compression_types[0].first;
        one_arg.use_mmap = true;
        test_args.push_back(one_arg);
        one_arg.use_mmap = false;
        test_args.push_back(one_arg);
        continue;
      }
#endif  //！摇滚乐

      for (auto restart_interval : restart_intervals) {
        for (auto compression_type : compression_types) {
          TestArgs one_arg;
          one_arg.type = test_type;
          one_arg.reverse_compare = reverse_compare;
          one_arg.restart_interval = restart_interval;
          one_arg.compression = compression_type.first;
          one_arg.format_version = compression_type.second ? 2 : 1;
          one_arg.use_mmap = false;
          test_args.push_back(one_arg);
        }
      }
    }
  }
  return test_args;
}

//为了使所有测试以纯表格式运行，包括
//那些在空键上操作的，创建一个新的前缀转换器
//如果切片不短于前缀长度，则返回固定前缀，
//如果比较短的话，整个切片。
class FixedOrLessPrefixTransform : public SliceTransform {
 private:
  const size_t prefix_len_;

 public:
  explicit FixedOrLessPrefixTransform(size_t prefix_len) :
      prefix_len_(prefix_len) {
  }

  virtual const char* Name() const override { return "rocksdb.FixedPrefix"; }

  virtual Slice Transform(const Slice& src) const override {
    assert(InDomain(src));
    if (src.size() < prefix_len_) {
      return src;
    }
    return Slice(src.data(), prefix_len_);
  }

  virtual bool InDomain(const Slice& src) const override { return true; }

  virtual bool InRange(const Slice& dst) const override {
    return (dst.size() <= prefix_len_);
  }
};

class HarnessTest : public testing::Test {
 public:
  HarnessTest()
      : ioptions_(options_),
        constructor_(nullptr),
        write_buffer_(options_.db_write_buffer_size) {}

  void Init(const TestArgs& args) {
    delete constructor_;
    constructor_ = nullptr;
    options_ = Options();
    options_.compression = args.compression;
//对测试使用较短的块大小来练习块边界
//更多条件。
    if (args.reverse_compare) {
      options_.comparator = &reverse_key_comparator;
    }

    internal_comparator_.reset(
        new test::PlainInternalKeyComparator(options_.comparator));

    support_prev_ = true;
    only_support_prefix_seek_ = false;
    options_.allow_mmap_reads = args.use_mmap;
    switch (args.type) {
      case BLOCK_BASED_TABLE_TEST:
        table_options_.flush_block_policy_factory.reset(
            new FlushBlockBySizePolicyFactory());
        table_options_.block_size = 256;
        table_options_.block_restart_interval = args.restart_interval;
        table_options_.index_block_restart_interval = args.restart_interval;
        table_options_.format_version = args.format_version;
        options_.table_factory.reset(
            new BlockBasedTableFactory(table_options_));
        constructor_ = new TableConstructor(
            /*离子比较器，真/*转换为内部键
        内部比较器重置（
            新的InternalKeyComparator（Options_.Comparator））；
        断裂；
//rocksdb-lite中不支持普通表
ifndef岩石
      case plain_table_semi_fixed_前缀：
        支持“prev”=false；
        只有_支持_前缀_seek_u=true；
        选项前缀提取程序重置（新FixedOrlessPrefixTransform（2））；
        选项“table_factory.reset”（newPlainTableFactory（））；
        constructor_u=new tableconstructor（
            选项“比较器”，真/*将“转换为”内部“键”*/);

        internal_comparator_.reset(
            new InternalKeyComparator(options_.comparator));
        break;
      case PLAIN_TABLE_FULL_STR_PREFIX:
        support_prev_ = false;
        only_support_prefix_seek_ = true;
        options_.prefix_extractor.reset(NewNoopTransform());
        options_.table_factory.reset(NewPlainTableFactory());
        constructor_ = new TableConstructor(
            /*离子比较器，真/*转换为内部键
        内部比较器重置（
            新的InternalKeyComparator（Options_.Comparator））；
        断裂；
      case plain_table_total_订单：
        支持“prev”=false；
        只支持前缀查找错误；
        选项前缀提取程序=nullptr；

        {
          普通表选项普通表选项；
          plain_table_options.user_key_len=kPlainTableVariableLength；
          plain_table_options.bloom_bits_per_key=0；
          plain_table_options.hash_table_ratio=0；

          选项表工厂重置（
              NewPlainTableFactory（Plain_Table_选项））；
        }
        constructor_u=new tableconstructor（
            选项“比较器”，真/*将“转换为”内部“键”*/);

        internal_comparator_.reset(
            new InternalKeyComparator(options_.comparator));
        break;
#endif  //！摇滚乐
      case BLOCK_TEST:
        table_options_.block_size = 256;
        options_.table_factory.reset(
            new BlockBasedTableFactory(table_options_));
        constructor_ = new BlockConstructor(options_.comparator);
        break;
      case MEMTABLE_TEST:
        table_options_.block_size = 256;
        options_.table_factory.reset(
            new BlockBasedTableFactory(table_options_));
        constructor_ = new MemTableConstructor(options_.comparator,
                                               &write_buffer_);
        break;
      case DB_TEST:
        table_options_.block_size = 256;
        options_.table_factory.reset(
            new BlockBasedTableFactory(table_options_));
        constructor_ = new DBConstructor(options_.comparator);
        break;
    }
    ioptions_ = ImmutableCFOptions(options_);
  }

  ~HarnessTest() { delete constructor_; }

  void Add(const std::string& key, const std::string& value) {
    constructor_->Add(key, value);
  }

  void Test(Random* rnd) {
    std::vector<std::string> keys;
    stl_wrappers::KVMap data;
    constructor_->Finish(options_, ioptions_, table_options_,
                         *internal_comparator_, &keys, &data);

    TestForwardScan(keys, data);
    if (support_prev_) {
      TestBackwardScan(keys, data);
    }
    TestRandomAccess(rnd, keys, data);
  }

  void TestForwardScan(const std::vector<std::string>& keys,
                       const stl_wrappers::KVMap& data) {
    InternalIterator* iter = constructor_->NewIterator();
    ASSERT_TRUE(!iter->Valid());
    iter->SeekToFirst();
    for (stl_wrappers::KVMap::const_iterator model_iter = data.begin();
         model_iter != data.end(); ++model_iter) {
      ASSERT_EQ(ToString(data, model_iter), ToString(iter));
      iter->Next();
    }
    ASSERT_TRUE(!iter->Valid());
    if (constructor_->IsArenaMode() && !constructor_->AnywayDeleteIterator()) {
      iter->~InternalIterator();
    } else {
      delete iter;
    }
  }

  void TestBackwardScan(const std::vector<std::string>& keys,
                        const stl_wrappers::KVMap& data) {
    InternalIterator* iter = constructor_->NewIterator();
    ASSERT_TRUE(!iter->Valid());
    iter->SeekToLast();
    for (stl_wrappers::KVMap::const_reverse_iterator model_iter = data.rbegin();
         model_iter != data.rend(); ++model_iter) {
      ASSERT_EQ(ToString(data, model_iter), ToString(iter));
      iter->Prev();
    }
    ASSERT_TRUE(!iter->Valid());
    if (constructor_->IsArenaMode() && !constructor_->AnywayDeleteIterator()) {
      iter->~InternalIterator();
    } else {
      delete iter;
    }
  }

  void TestRandomAccess(Random* rnd, const std::vector<std::string>& keys,
                        const stl_wrappers::KVMap& data) {
    static const bool kVerbose = false;
    InternalIterator* iter = constructor_->NewIterator();
    ASSERT_TRUE(!iter->Valid());
    stl_wrappers::KVMap::const_iterator model_iter = data.begin();
    if (kVerbose) fprintf(stderr, "---\n");
    for (int i = 0; i < 200; i++) {
      const int toss = rnd->Uniform(support_prev_ ? 5 : 3);
      switch (toss) {
        case 0: {
          if (iter->Valid()) {
            if (kVerbose) fprintf(stderr, "Next\n");
            iter->Next();
            ++model_iter;
            ASSERT_EQ(ToString(data, model_iter), ToString(iter));
          }
          break;
        }

        case 1: {
          if (kVerbose) fprintf(stderr, "SeekToFirst\n");
          iter->SeekToFirst();
          model_iter = data.begin();
          ASSERT_EQ(ToString(data, model_iter), ToString(iter));
          break;
        }

        case 2: {
          std::string key = PickRandomKey(rnd, keys);
          model_iter = data.lower_bound(key);
          if (kVerbose) fprintf(stderr, "Seek '%s'\n",
                                EscapeString(key).c_str());
          iter->Seek(Slice(key));
          ASSERT_EQ(ToString(data, model_iter), ToString(iter));
          break;
        }

        case 3: {
          if (iter->Valid()) {
            if (kVerbose) fprintf(stderr, "Prev\n");
            iter->Prev();
            if (model_iter == data.begin()) {
model_iter = data.end();   //换行到无效值
            } else {
              --model_iter;
            }
            ASSERT_EQ(ToString(data, model_iter), ToString(iter));
          }
          break;
        }

        case 4: {
          if (kVerbose) fprintf(stderr, "SeekToLast\n");
          iter->SeekToLast();
          if (keys.empty()) {
            model_iter = data.end();
          } else {
            std::string last = data.rbegin()->first;
            model_iter = data.lower_bound(last);
          }
          ASSERT_EQ(ToString(data, model_iter), ToString(iter));
          break;
        }
      }
    }
    if (constructor_->IsArenaMode() && !constructor_->AnywayDeleteIterator()) {
      iter->~InternalIterator();
    } else {
      delete iter;
    }
  }

  std::string ToString(const stl_wrappers::KVMap& data,
                       const stl_wrappers::KVMap::const_iterator& it) {
    if (it == data.end()) {
      return "END";
    } else {
      return "'" + it->first + "->" + it->second + "'";
    }
  }

  std::string ToString(const stl_wrappers::KVMap& data,
                       const stl_wrappers::KVMap::const_reverse_iterator& it) {
    if (it == data.rend()) {
      return "END";
    } else {
      return "'" + it->first + "->" + it->second + "'";
    }
  }

  std::string ToString(const InternalIterator* it) {
    if (!it->Valid()) {
      return "END";
    } else {
      return "'" + it->key().ToString() + "->" + it->value().ToString() + "'";
    }
  }

  std::string PickRandomKey(Random* rnd, const std::vector<std::string>& keys) {
    if (keys.empty()) {
      return "foo";
    } else {
      const int index = rnd->Uniform(static_cast<int>(keys.size()));
      std::string result = keys[index];
      switch (rnd->Uniform(support_prev_ ? 3 : 1)) {
        case 0:
//返回现有密钥
          break;
        case 1: {
//尝试返回小于现有密钥的内容
          if (result.size() > 0 && result[result.size() - 1] > '\0'
              && (!only_support_prefix_seek_
                  || options_.prefix_extractor->Transform(result).size()
                  < result.size())) {
            result[result.size() - 1]--;
          }
          break;
      }
        case 2: {
//返回大于现有密钥的内容
          Increment(options_.comparator, &result);
          break;
        }
      }
      return result;
    }
  }

//如果不针对数据库运行，则返回nullptr
  DB* db() const { return constructor_->db(); }

 private:
  Options options_ = Options();
  ImmutableCFOptions ioptions_;
  BlockBasedTableOptions table_options_ = BlockBasedTableOptions();
  Constructor* constructor_;
  WriteBufferManager write_buffer_;
  bool support_prev_;
  bool only_support_prefix_seek_;
  shared_ptr<InternalKeyComparator> internal_comparator_;
};

static bool Between(uint64_t val, uint64_t low, uint64_t high) {
  bool result = (val >= low) && (val <= high);
  if (!result) {
    fprintf(stderr, "Value %llu is not in range [%llu, %llu]\n",
            (unsigned long long)(val),
            (unsigned long long)(low),
            (unsigned long long)(high));
  }
  return result;
}

//对各种表格进行测试
class TableTest : public testing::Test {
 public:
  const InternalKeyComparator& GetPlainInternalComparator(
      const Comparator* comp) {
    if (!plain_internal_comparator) {
      plain_internal_comparator.reset(
          new test::PlainInternalKeyComparator(comp));
    }
    return *plain_internal_comparator;
  }
  void IndexTest(BlockBasedTableOptions table_options);

 private:
  std::unique_ptr<InternalKeyComparator> plain_internal_comparator;
};

class GeneralTableTest : public TableTest {};
class BlockBasedTableTest : public TableTest {};
class PlainTableTest : public TableTest {};
class TablePropertyTest : public testing::Test {};

//此测试是用户收集的前缀扫描的活动教程。
//性质。
TEST_F(TablePropertyTest, PrefixScanTest) {
  UserCollectedProperties props{{"num.111.1", "1"},
                                {"num.111.2", "2"},
                                {"num.111.3", "3"},
                                {"num.333.1", "1"},
                                {"num.333.2", "2"},
                                {"num.333.3", "3"},
                                {"num.555.1", "1"},
                                {"num.555.2", "2"},
                                {"num.555.3", "3"}, };

//存在的前缀
  for (const std::string& prefix : {"num.111", "num.333", "num.555"}) {
    int num = 0;
    for (auto pos = props.lower_bound(prefix);
         pos != props.end() &&
             pos->first.compare(0, prefix.size(), prefix) == 0;
         ++pos) {
      ++num;
      auto key = prefix + "." + ToString(num);
      ASSERT_EQ(key, pos->first);
      ASSERT_EQ(ToString(num), pos->second);
    }
    ASSERT_EQ(3, num);
  }

//不存在的前缀
  for (const std::string& prefix :
       {"num.000", "num.222", "num.444", "num.666"}) {
    auto pos = props.lower_bound(prefix);
    ASSERT_TRUE(pos == props.end() ||
                pos->first.compare(0, prefix.size(), prefix) != 0);
  }
}

//此测试包括除索引大小和块以外的所有基本检查
//尺寸，将在单独的单元测试中进行。
TEST_F(BlockBasedTableTest, BasicBlockBasedTableProperties) {
  /*leconstructor c（bytewiseComparator（），true/*将_转换为_internal_key_*/）；

  C.添加（“A1”，“VAL1”）；
  C.添加（“B2”，“VAL2”）；
  C.添加（“c3”，“val3”）；
  C.添加（“D4”，“VAL4”）；
  C.添加（“E5”，“VAL5”）；
  C.添加（“F6”，“VAL6”）；
  C.添加（“G7”，“VAL7”）；
  C.添加（“H8”，“VAL8”）；
  C.添加（“J9”，“VAL9”）；
  uint64_t diff_internal_user_bytes=9*8；//8为序列大小，共9 k-v

  std:：vector<std:：string>keys；
  stl_wrappers：：kvmap kvmap；
  期权期权；
  options.compression=knocompression；
  BlockBasedTableOptions表\u选项；
  表\u options.block \u restart \u interval=1；
  options.table_factory.reset（newblockbasedTableFactory（table_options））；

  常量不变的选项（选项）；
  C.完成（选项，IOPTIONS，表格选项，
           getplainInternalComparator（options.comparator）、&keys和kvmap）；

  auto&props=*c.getTableReader（）->getTableProperties（）；
  断言eu eq（kvmap.size（），props.num_entries）；

  auto raw_key_size=kvmap.size（）*2ul；
  auto raw_value_size=kvmap.size（）*4ul；

  断言eq（原始\密钥\大小+差异\内部\用户\字节，props.raw \密钥\大小）；
  断言eq（原始值大小，属性，原始值大小）；
  断言eq（1ul，props.num_data_块）；
  assert_eq（“”，props.filter_policy_name）；//不使用筛选策略

  //验证数据大小。
  块生成器块生成器（1）；
  用于（const auto&item:kvmap）
    块生成器添加（item.first，item.second）；
  }
  slice content=block_builder.finish（）；
  断言eq（content.size（）+kblocktrailersize+diff_internal_user_bytes，
            属性数据大小）；
  c.重置读卡器（）；
}

测试_F（BlockBasedTableTest，BlockBasedTableProperties2）
  TableConstructor C（&reverse_key_comparator）；
  std:：vector<std:：string>keys；
  stl_wrappers：：kvmap kvmap；

  {
    期权期权；
    options.compression=压缩类型：：knocompression；
    BlockBasedTableOptions表\u选项；
    options.table_factory.reset（newblockbasedTableFactory（table_options））；

    常量不变的选项（选项）；
    C.完成（选项，IOPTIONS，表格选项，
             getplainInternalComparator（options.comparator）、&keys和kvmap）；

    auto&props=*c.getTableReader（）->getTableProperties（）；

    //默认比较器
    断言“eq”（“leveldb.bytewiseComparator”，props.comparator_name）；
    //没有合并运算符
    断言eu eq（“nullptr”，props.merge_operator_name）；
    //没有前缀提取程序
    断言“eq”（“nullptr”，props.prefix“提取器名称”）；
    //没有属性收集器
    断言“eq”（“[]”，props.property“collectors”名称）；
    //未使用筛选策略
    断言“eq”（“，props.filter”策略名称）；
    //压缩类型==该集：
    断言eq（“nocompression”，props.compression，名称）；
    c.重置读卡器（）；
  }

  {
    期权期权；
    BlockBasedTableOptions表\u选项；
    options.table_factory.reset（newblockbasedTableFactory（table_options））；
    options.comparator=&reverse_key_comparator；
    options.merge_operator=mergeOperators:：createUInt64AddOperator（）；
    options.prefix_extractor.reset（newNoopTransform（））；
    选项。table_properties_collector_factures.emplace_back（
        新建DummyPropertiesCollectorFactory1（））；
    选项。table_properties_collector_factures.emplace_back（
        新建DummyPropertiesCollectorFactory2（））；

    常量不变的选项（选项）；
    C.完成（选项，IOPTIONS，表格选项，
             getplainInternalComparator（options.comparator）、&keys和kvmap）；

    auto&props=*c.getTableReader（）->getTableProperties（）；

    断言“eq”（“rocksdb.reverseByteIsComparator”，props.comparator_name）；
    断言“eq”（“uint64addoperator”，props.merge“operator”名称）；
    断言“eq”（“rocksdb.noop”，props.prefix“提取器名称”）；
    断言“eq（”[DummyPropertiesCollector1，DummyPropertiesCollector2]“，
              财产收藏家姓名）；
    assert_eq（“”，props.filter_policy_name）；//不使用筛选策略
    c.重置读卡器（）；
  }
}

测试F（BlockBasedTableTest，RangeDelBlock）
  TableConstructor C（bytewiseComparator（））；
  std:：vector<std:：string>keys=“1pika”，“2chu”
  std:：vector<std:：string>vals=“p”，“c”

  对于（int i=0；i<2；i++）
    RangeTombstone T（键[I]，VAL[I]，I）；
    std:：pair<internalkey，slice>p=t.serialize（）；
    c.add（p.first.encode（）.toString（），p.second）；
  }

  std:：vector<std:：string>排序的_键；
  stl_wrappers：：kvmap kvmap；
  期权期权；
  options.compression=knocompression；
  BlockBasedTableOptions表\u选项；
  表\u options.block \u restart \u interval=1；
  options.table_factory.reset（newblockbasedTableFactory（table_options））；

  常量不变的选项（选项）；
  std：：unique_ptr<internalkeyComparator>internal_cmp（
      新的InternalKeyComparator（Options.Comparator））；
  C.finish（选项，ioptions，表选项，*内部cmp，&排序键，
           和KVMAP）；

  对于（int j=0；j<2；++j）
    std:：unique_ptr<internaliterator>iter（
        c.getTableReader（）->newRangeTombstoneIterator（readOptions（））；
    如果（j＞0）{
      //对于第二次迭代，请删除表读取器对象并验证
      //迭代器仍然可以访问其元块的范围逻辑删除。
      c.重置读卡器（）；
    }
    断言“假”（iter->valid（））；
    iter->seektofirst（）；
    断言“真”（iter->valid（））；
    对于（int i=0；i<2；i++）
      断言“真”（iter->valid（））；
      parsedinteralkey parsed_键；
      断言_true（parseInternalKey（iter->key（），&parsed_key））；
      rangetombstone t（parsed_key，iter->value（））；
      断言_Eq（t.开始_key_uukeys[i]）；
      断言eq（t.end_key_uu，vals[i]）；
      断言方程（t.seq_uu，i）；
      ITER > NEXT（）；
    }
    断言是真的（！）ITER >验证（））；
  }
}

test_f（blockbasedTableTest，filterPolicyName属性）
  TableConstructor C（bytewiseComparator（），true/*将_转换为_内部_键_*/);

  c.Add("a1", "val1");
  std::vector<std::string> keys;
  stl_wrappers::KVMap kvmap;
  BlockBasedTableOptions table_options;
  table_options.filter_policy.reset(NewBloomFilterPolicy(10));
  Options options;
  options.table_factory.reset(NewBlockBasedTableFactory(table_options));

  const ImmutableCFOptions ioptions(options);
  c.Finish(options, ioptions, table_options,
           GetPlainInternalComparator(options.comparator), &keys, &kvmap);
  auto& props = *c.GetTableReader()->GetTableProperties();
  ASSERT_EQ("rocksdb.BuiltinBloomFilter", props.filter_policy_name);
  c.ResetTableReader();
}

//
//BlockBasedTableTest：：预取测试
//
void AssertKeysInCache(BlockBasedTable* table_reader,
                       const std::vector<std::string>& keys_in_cache,
                       const std::vector<std::string>& keys_not_in_cache,
                       bool convert = false) {
  if (convert) {
    for (auto key : keys_in_cache) {
      InternalKey ikey(key, kMaxSequenceNumber, kTypeValue);
      ASSERT_TRUE(table_reader->TEST_KeyInCache(ReadOptions(), ikey.Encode()));
    }
    for (auto key : keys_not_in_cache) {
      InternalKey ikey(key, kMaxSequenceNumber, kTypeValue);
      ASSERT_TRUE(!table_reader->TEST_KeyInCache(ReadOptions(), ikey.Encode()));
    }
  } else {
    for (auto key : keys_in_cache) {
      ASSERT_TRUE(table_reader->TEST_KeyInCache(ReadOptions(), key));
    }
    for (auto key : keys_not_in_cache) {
      ASSERT_TRUE(!table_reader->TEST_KeyInCache(ReadOptions(), key));
    }
  }
}

void PrefetchRange(TableConstructor* c, Options* opt,
                   BlockBasedTableOptions* table_options, const char* key_begin,
                   const char* key_end,
                   const std::vector<std::string>& keys_in_cache,
                   const std::vector<std::string>& keys_not_in_cache,
                   const Status expected_status = Status::OK()) {
//重置缓存并重新打开表
  table_options->block_cache = NewLRUCache(16 * 1024 * 1024, 4);
  opt->table_factory.reset(NewBlockBasedTableFactory(*table_options));
  const ImmutableCFOptions ioptions2(*opt);
  ASSERT_OK(c->Reopen(ioptions2));

//预取
  auto* table_reader = dynamic_cast<BlockBasedTable*>(c->GetTableReader());
  Status s;
  unique_ptr<Slice> begin, end;
  unique_ptr<InternalKey> i_begin, i_end;
  if (key_begin != nullptr) {
    if (c->ConvertToInternalKey()) {
      i_begin.reset(new InternalKey(key_begin, kMaxSequenceNumber, kTypeValue));
      begin.reset(new Slice(i_begin->Encode()));
    } else {
      begin.reset(new Slice(key_begin));
    }
  }
  if (key_end != nullptr) {
    if (c->ConvertToInternalKey()) {
      i_end.reset(new InternalKey(key_end, kMaxSequenceNumber, kTypeValue));
      end.reset(new Slice(i_end->Encode()));
    } else {
      end.reset(new Slice(key_end));
    }
  }
  s = table_reader->Prefetch(begin.get(), end.get());

  ASSERT_TRUE(s.code() == expected_status.code());

//在缓存预热中维护我们的期望
  AssertKeysInCache(table_reader, keys_in_cache, keys_not_in_cache,
                    c->ConvertToInternalKey());
  c->ResetTableReader();
}

TEST_F(BlockBasedTableTest, PrefetchTest) {
//此测试的目的是测试内置的预取操作
//基于块的表。
  Options opt;
  unique_ptr<InternalKeyComparator> ikc;
  ikc.reset(new test::PlainInternalKeyComparator(opt.comparator));
  opt.compression = kNoCompression;
  BlockBasedTableOptions table_options;
  table_options.block_size = 1024;
//足够大，这样我们就不会丢失缓存值。
  table_options.block_cache = NewLRUCache(16 * 1024 * 1024, 4);
  opt.table_factory.reset(NewBlockBasedTableFactory(table_options));

  /*leconstructor c（bytewiseComparator（），true/*将_转换为_internal_key_*/）；
  C.ADD（“K01”，“你好”）；
  C.添加（“K02”，“hello2”）；
  c.add（“k03”，std:：string（10000，'x'））；
  c.add（“k04”，std:：string（200000，'x'））；
  c.add（“k05”，std:：string（300000，'x'））；
  C.添加（“K06”，“hello3”）；
  c.add（“K07”，std:：string（100000，'x'））；
  std:：vector<std:：string>keys；
  stl_wrappers：：kvmap kvmap；
  不变常数变量IOPTIONS（opt）；
  C.完成（opt，ioptions，table_options，*ikc，&keys，&kvmap）；
  c.重置读卡器（）；

  //我们得到以下数据排列：
  / /
  //数据块索引
  //======================
  //[K01 K02 K03]K03
  //[K04]K04
  //[K05]K05
  //[K06 K07]K07


  /简单
  预取变量（&c，&opt，&table_选项，
                /*键范围*/"k01", "k05",

                /*eys_in_cache=*/“k01”，“k02”，“k03”，“k04”，“k05”，
                /*密钥不在缓存中*/{"k06", "k07"});

  PrefetchRange(&c, &opt, &table_options, "k01", "k01", {"k01", "k02", "k03"},
                {"k04", "k05", "k06", "k07"});
//古怪的
  PrefetchRange(&c, &opt, &table_options, "a", "z",
                {"k01", "k02", "k03", "k04", "k05", "k06", "k07"}, {});
  PrefetchRange(&c, &opt, &table_options, "k00", "k00", {"k01", "k02", "k03"},
                {"k04", "k05", "k06", "k07"});
//边缘案例
  PrefetchRange(&c, &opt, &table_options, "k00", "k06",
                {"k01", "k02", "k03", "k04", "k05", "k06", "k07"}, {});
  PrefetchRange(&c, &opt, &table_options, "k00", "zzz",
                {"k01", "k02", "k03", "k04", "k05", "k06", "k07"}, {});
//空键
  PrefetchRange(&c, &opt, &table_options, nullptr, nullptr,
                {"k01", "k02", "k03", "k04", "k05", "k06", "k07"}, {});
  PrefetchRange(&c, &opt, &table_options, "k04", nullptr,
                {"k04", "k05", "k06", "k07"}, {"k01", "k02", "k03"});
  PrefetchRange(&c, &opt, &table_options, nullptr, "k05",
                {"k01", "k02", "k03", "k04", "k05"}, {"k06", "k07"});
//无效
  PrefetchRange(&c, &opt, &table_options, "k06", "k00", {}, {},
                Status::InvalidArgument(Slice("k06 "), Slice("k07")));
  c.ResetTableReader();
}

TEST_F(BlockBasedTableTest, TotalOrderSeekOnHashIndex) {
  BlockBasedTableOptions table_options;
  for (int i = 0; i < 4; ++i) {
    Options options;
//使每个键/值成为一个单独的块
    table_options.block_size = 64;
    switch (i) {
    case 0:
//二进制搜索索引
      table_options.index_type = BlockBasedTableOptions::kBinarySearch;
      options.table_factory.reset(new BlockBasedTableFactory(table_options));
      break;
    case 1:
//哈希搜索索引
      table_options.index_type = BlockBasedTableOptions::kHashSearch;
      options.table_factory.reset(new BlockBasedTableFactory(table_options));
      options.prefix_extractor.reset(NewFixedPrefixTransform(4));
      break;
    case 2:
//哈希搜索索引和哈希索引允许冲突
      table_options.index_type = BlockBasedTableOptions::kHashSearch;
      table_options.hash_index_allow_collision = true;
      options.table_factory.reset(new BlockBasedTableFactory(table_options));
      options.prefix_extractor.reset(NewFixedPrefixTransform(4));
      break;
    case 3:
//具有筛选策略的哈希搜索索引
      table_options.index_type = BlockBasedTableOptions::kHashSearch;
      table_options.filter_policy.reset(NewBloomFilterPolicy(10));
      options.table_factory.reset(new BlockBasedTableFactory(table_options));
      options.prefix_extractor.reset(NewFixedPrefixTransform(4));
      break;
    case 4:
    default:
//二进制搜索索引
      table_options.index_type = BlockBasedTableOptions::kTwoLevelIndexSearch;
      options.table_factory.reset(new BlockBasedTableFactory(table_options));
      break;
    }

    TableConstructor c(BytewiseComparator(),
                       /*e/*将_转换为_内部_键_*/）；
    c.add（“aaaa1”，std:：string（“a”，56））；
    c.add（“bbaa1”，std:：string（“a”，56））；
    c.add（“cccc1”，std:：string（“a”，56））；
    c.add（“bbbb1”，std:：string（“a”，56））；
    c.add（“baaa1”，std:：string（“a”，56））；
    c.add（“abb1”，std:：string（“a”，56））；
    c.add（“cccc2”，std:：string（“a”，56））；
    std:：vector<std:：string>keys；
    stl_wrappers：：kvmap kvmap；
    常量不变的选项（选项）；
    C.完成（选项，IOPTIONS，表格选项，
             getplainInternalComparator（options.comparator）、&keys和kvmap）；
    auto props=c.getTableReader（）->getTableProperties（）；
    断言_Eq（7u，props->num_data_块）；
    auto*reader=c.gettablereader（）；
    阅读选项RO；
    ro.total_order_seek=真；
    std:：unique_ptr<internaliterator>iter（reader->newiterator（ro））；

    iter->seek（internalkey（“b”，0，ktypeValue）.encode（））；
    断言_OK（iter->status（））；
    断言“真”（iter->valid（））；
    断言eq（“baaa1”，extractUserKey（iter->key（））.toString（））；
    ITER > NEXT（）；
    断言_OK（iter->status（））；
    断言“真”（iter->valid（））；
    assert_eq（“bbaa1”，extractUserKey（iter->key（））.toString（））；

    iter->seek（internalkey（“bb”，0，ktypeValue）.encode（））；
    断言_OK（iter->status（））；
    断言“真”（iter->valid（））；
    assert_eq（“bbaa1”，extractUserKey（iter->key（））.toString（））；
    ITER > NEXT（）；
    断言_OK（iter->status（））；
    断言“真”（iter->valid（））；
    assert_eq（“bbbb1”，extractUserKey（iter->key（））.toString（））；

    iter->seek（internalkey（“bbb”，0，ktypeValue）.encode（））；
    断言_OK（iter->status（））；
    断言“真”（iter->valid（））；
    assert_eq（“bbbb1”，extractUserKey（iter->key（））.toString（））；
    ITER > NEXT（）；
    断言_OK（iter->status（））；
    断言“真”（iter->valid（））；
    assert_eq（“cccc1”，extractUserKey（iter->key（））.toString（））；
  }
}

测试_F（BlockBasedTableTest，NoopTransformSeek）
  BlockBasedTableOptions表\u选项；
  表_options.filter_policy.reset（newbloomfilterpolicy（10））；

  期权期权；
  options.comparator=bytewiseComparator（）；
  options.table_factory.reset（new blockbasedTableFactory（table_options））；
  options.prefix_extractor.reset（newNoopTransform（））；

  TableConstructor C（Options.Comparator）；
  //要勾选PrefixMayMatch错误，重要的是
  //用户键为单字节，因此索引键完全匹配
  //用户密钥。
  InternalKey键（“A”，1，ktypeValue）；
  c.add（key.encode（）.toString（），“b”）；
  std:：vector<std:：string>keys；
  stl_wrappers：：kvmap kvmap；
  常量不变的选项（选项）；
  const InternalKeyComparator Internal_Comparator（选项.Comparator）；
  C.完成（选项、IOPTIONS、表选项、内部比较工具和键，
           和KVMAP）；

  auto*reader=c.gettablereader（）；
  对于（int i=0；i<2；++i）
    阅读选项RO；
    ro.total_order_seek=（i==0）；
    std:：unique_ptr<internaliterator>iter（reader->newiterator（ro））；

    iter->seek（key.encode（））；
    断言_OK（iter->status（））；
    断言“真”（iter->valid（））；
    断言eq（“a”，extractUserKey（iter->key（））.toString（））；
  }
}

测试_F（BlockBasedTableTest，SkipPrefixBloomFilter）
  //如果使用不同名称的前缀提取程序打开db，
  //读取文件时跳过前缀bloom
  BlockBasedTableOptions表\u选项；
  表_options.filter_policy.reset（newbloomfilterpolicy（2））；
  table_options.whole_key_filtering=false；

  期权期权；
  options.comparator=bytewiseComparator（）；
  options.table_factory.reset（new blockbasedTableFactory（table_options））；
  options.prefix提取程序.reset（newfixedprifixtransform（1））；

  TableConstructor C（Options.Comparator）；
  internalkey key（“abcdefghijk”，1，ktypeValue）；
  c.add（key.encode（）.toString（），“test”）；
  std:：vector<std:：string>keys；
  stl_wrappers：：kvmap kvmap；
  常量不变的选项（选项）；
  const InternalKeyComparator Internal_Comparator（选项.Comparator）；
  C.完成（选项、IOPTIONS、表选项、内部比较工具和键，
           和KVMAP）；
  options.prefix提取程序.reset（newfixedprifixtransform（9））；
  常量不变的选项新选项（选项）；
  c.重新打开（新操作）；
  自动读卡器=c.getTableReader（）；
  std:：unique_ptr<internalIterator>db_iter（reader->newIterator（readoptions（））；

  //测试点查找
  /仅1 kV
  对于（auto&kv:kvmap）
    db-iter->seek（kv.first）；
    断言_true（db_iter->valid（））；
    断言_OK（db_Iter->status（））；
    断言_Eq（db_Iter->key（），kv.first）；
    断言_Eq（db_Iter->value（），kv.second）；
  }
}

static std:：string random string（随机*rnd，int len）
  STD：：
  测试：：randomstring（rnd、len和r）；
  返回R；
}

void addinternalkey（tableconstructor*c，const std:：string&prefix，
                    int后缀_len=800）
  静态随机RND（1023）；
  internalkey k（前缀+randomstring（&rnd，800），0，ktypeValue）；
  c->add（k.encode（）.toString（），“v”）；
}

void tabletest:：indextest（blockbasedtableoptions table_options）
  TableConstructor C（bytewiseComparator（））；

  //前缀长度为3的键，确保键/值足够大，可以填充
  /一块
  AddInternalkey（&C，“0015”）；
  AddInternalkey（&C，“0035”）；

  AddInternalkey（&C，“0054”）；
  AddInternalkey（&C，“0055”）；

  AddInternalkey（&C，“0056”）；
  AddInternalkey（&C，“0057”）；

  AddInternalkey（&C，“0058”）；
  AddInternalkey（&C，“0075”）；

  AddInternalkey（&C，“0076”）；
  AddInternalkey（&C，“0095”）；

  std:：vector<std:：string>keys；
  stl_wrappers：：kvmap kvmap；
  期权期权；
  选项。前缀提取程序。重置（newFixedPrefixTransform（3））；
  表\options.block \u size=1700；
  table_options.block_cache=newlrucache（1024，4）；
  options.table_factory.reset（newblockbasedTableFactory（table_options））；

  std:：unique_ptr<internalKeyComparator>Comparator（
      新的InternalKeyComparator（bytewiseComparator（））；
  常量不变的选项（选项）；
  c.完成（选项、IOPTIONS、表格选项、*比较器、&keys和kvmap）；
  自动读卡器=c.getTableReader（）；

  auto props=reader->getTableProperties（）；
  断言eu eq（5u，props->num_data_块）；

  std:：unique_ptr<internalIterator>index_iter（
      reader->newIterator（readoptions（））；

  //—查找键不存在，但具有公共前缀。
  std:：vector<std:：string>前缀=“001”，“003”，“005”，“007”，“009”
  std:：vector<std:：string>下限键[0]，键[1]，键[2]，
                                          键[7]，键[9]，

  //查找前缀的下界
  对于（size_t i=0；i<prefixes.size（）；++i）
    index_iter->seek（internalkey（prefixes[i]，0，ktypeValue）.encode（））；
    断言_OK（index_Iter->status（））；
    断言_true（index_iter->valid（））；

    //查找块中的第一个元素
    assert_eq（lower_bound[i]，index_iter->key（）.toString（））；
    断言_eq（“v”，index_iter->value（）.toString（））；
  }

  //查找前缀的上界
  std:：vector<std:：string>上限键[1]，键[2]，键[7]，键[9]，

  //查找现有键
  用于（const auto&item:kvmap）
    auto-ukey=extractUserkey（item.first）.toString（）；
    索引器->搜索（Ukey）；

    //assert_ok（regular_iter->status（））；
    断言_OK（index_Iter->status（））；

    //assert_true（regular_iter->valid（））；
    断言_true（index_iter->valid（））；

    断言_eq（item.first，index_iter->key（）.toString（））；
    断言_eq（item.second，index_iter->value（）.toString（））；
  }

  对于（size_t i=0；i<prefixes.size（）；++i）
    //该键大于任何现有键。
    自动键=前缀[I]+“9”；
    index_iter->seek（internalkey（key，0，ktypeValue）.encode（））；

    断言_OK（index_Iter->status（））；
    if（i==前缀.size（）-1）
      /最后一把钥匙
      断言是真的（！）索引->valid（））；
    }否则{
      断言_true（index_iter->valid（））；
      //查找块中的第一个元素
      断言_eq（upper_bound[i]，index_iter->key（）.toString（））；
      断言_eq（“v”，index_iter->value（）.toString（））；
    }
  }

  //查找前缀与任何现有前缀都不匹配的键。
  std:：vector<std:：string>不存在前缀“002”，“004”，“006”，“008”
  for（const auto&prefix：不存在前缀）
    index_iter->seek（internalkey（prefix，0，ktypeValue.encode（））；
    //regulariter->seek（前缀）；

    断言_OK（index_Iter->status（））；
    //查找不存在的前缀应产生无效或
    //前缀大于目标的键。
    if（index-iter->valid（））
      slice ukey=extractUserkey（index_iter->key（））；
      slice ukey_prefix=options.prefix_extractor->transform（ukey）；
      assert_true（bytewiseComparator（）->Compare（prefix，ukey_prefix）<0）；
    }
  }
  c.重置读卡器（）；
}

测试F（TableTest，BinaryIndexTest）
  BlockBasedTableOptions表\u选项；
  tableou options.index_type=blockBasedTableOptions:：kbinarySearch；
  索引测试（表选项）；
}

测试_f（tabletest，hashindextest）
  BlockBasedTableOptions表\u选项；
  tableou options.index_type=blockBasedTableOptions:：khashsearch；
  索引测试（表选项）；
}

测试_f（TableTest，PartitionIndexTest）
  const int max_index_keys=5；
  const int est_max_index_key_value_size=32；
  const int est_max_index_size=max_index_keys*est_max_index_key_value_size；
  对于（int i=1；i<=est_max_index_size+1；i++）
    BlockBasedTableOptions表\u选项；
    tableou options.index_type=blockBasedTableOptions:：ktwLovelIndexSearch；
    表\u options.metadata \u block \u size=i；
    索引测试（表选项）；
  }
}

//很难准确计算出一个块的索引块大小。
//为了确保得到索引大小，我们只需要确定作为键号
//增大，过滤块大小也增大。
测试（blockbasedTableTest，indexSizeStat）
  uint64_t last_index_size=0；

  //我们需要使用随机键，因为纯人类可读的文本
  //可能被很好地压缩，导致索引的显著变化
  /块大小。
  随机RND（test:：randomseed（））；
  std:：vector<std:：string>keys；

  对于（int i=0；i<100；+i）
    键。向后推（randomstring（&rnd，10000））；
  }

  //每次我们向表中再加载一个键。表索引块
  //大小应大于上一次的大小。
  对于（size_t i=1；i<keys.size（）；++i）
    TableConstructor C（bytewiseComparator（），
                       真的/*将\转换为\内部\键\u*/);

    for (size_t j = 0; j < i; ++j) {
      c.Add(keys[j], "val");
    }

    std::vector<std::string> ks;
    stl_wrappers::KVMap kvmap;
    Options options;
    options.compression = kNoCompression;
    BlockBasedTableOptions table_options;
    table_options.block_restart_interval = 1;
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));

    const ImmutableCFOptions ioptions(options);
    c.Finish(options, ioptions, table_options,
             GetPlainInternalComparator(options.comparator), &ks, &kvmap);
    auto index_size = c.GetTableReader()->GetTableProperties()->index_size;
    ASSERT_GT(index_size, last_index_size);
    last_index_size = index_size;
    c.ResetTableReader();
  }
}

TEST_F(BlockBasedTableTest, NumBlockStat) {
  Random rnd(test::RandomSeed());
  /*leconstructor c（bytewiseComparator（），true/*将_转换为_internal_key_*/）；
  期权期权；
  options.compression=knocompression；
  BlockBasedTableOptions表\u选项；
  表\u options.block \u restart \u interval=1；
  table_options.block_size=1000；
  options.table_factory.reset（newblockbasedTableFactory（table_options））；

  对于（int i=0；i<10；+i）
    //键/val略小于块大小，因此每个块
    //大致包含一个键/值对。
    c.add（随机字符串（&rnd，900），“val”）；
  }

  std:：vector<std:：string>ks；
  stl_wrappers：：kvmap kvmap；
  常量不变的选项（选项）；
  C.完成（选项，IOPTIONS，表格选项，
           getplainInternalComparator（options.comparator），&ks，&kvmap）；
  断言eq（kvmap.size（），
            c.getTableReader（）->getTableProperties（）->num_data_blocks）；
  c.重置读卡器（）；
}

//获取块缓存统计信息快照的简单工具。
类BlockCachePropertiesSnapshot_
 公众：
  显式BlockCachePropertiesSnapshot（统计*统计）
    block_cache_miss=statistics->gettickercount（block_cache_miss）；
    block_cache_hit=statistics->gettickercount（block_cache_hit）；
    index_block_cache_miss=statistics->gettickercount（block_cache_index_miss）；
    index_block_cache_hit=statistics->gettickercount（block_cache_index_hit）；
    data_block_cache_miss=statistics->gettickercount（block_cache_data_miss）；
    data_block_cache_hit=statistics->gettickercount（block_cache_data_hit）；
    筛选器块缓存未命中=
        statistics->gettickercount（block_cache_filter_miss）；
    filter_block_cache_hit=statistics->gettickercount（block_cache_filter_hit）；
    block_cache_bytes_read=statistics->gettickercount（block_cache_bytes_read）；
    块缓存字节写入=
        statistics->gettickercount（block_cache_bytes_write）；
  }

  void assertendexblockstat（int64_t expected_index_block_cache_miss，
                            Int64_t预期_index_block_cache_hit）
    断言eq（预期的索引块缓存未命中，索引块缓存未命中）；
    断言eq（预期的索引块缓存命中，索引块缓存命中）；
  }

  void assertfilterblockstat（Int64_t预期的_filter_block_cache_未命中，
                             Int64_t预期_filter_block_cache_hit）
    断言eq（预期的过滤块缓存未命中，过滤块缓存未命中）；
    断言eq（预期的过滤器块缓存命中，过滤器块缓存命中）；
  }

  //检查获取的属性是否与预期的属性匹配。
  //TODO（kailiu）仅在禁用筛选策略时使用此选项！
  void assertequal（Int64_t预期的_index_block_cache_miss，
                   Int64_t预期的_index_block_cache_命中，
                   Int64_t预期的_data_block_cache_miss，
                   Int64_t expected_data_block_cache_hit）const_
    断言eq（预期的索引块缓存未命中，索引块缓存未命中）；
    断言eq（预期的索引块缓存命中，索引块缓存命中）；
    断言eq（预期的数据块缓存未命中、数据块缓存未命中）；
    断言eq（预期的数据块缓存命中，数据块缓存命中）；
    断言eq（预期的_index_block_cache_miss+预期的_data_block_cache_miss，
              块缓存未命中）；
    断言eq（预期的索引块缓存命中+预期的数据块缓存命中，
              块缓存命中）；
  }

  int64_t getcachebytesread（）返回块_cache_bytes_read；

  int64_t getcachebyteswrite（）返回块_cache_bytes_write；

 私人：
  Int64_t block_cache_miss=0；
  int64_t block_cache_hit=0；
  int64_t index_block_cache_miss=0；
  int64_t index_block_cache_hit=0；
  Int64_t data_block_cache_miss=0；
  Int64_t data_block_cache_hit=0；
  int64_t filter_block_cache_miss=0；
  int64_t filter_block_cache_hit=0；
  int64_t block_cache_bytes_read=0；
  int64_t block_cache_bytes_write=0；
}；

//确保在默认情况下预加载了索引/筛选块（这意味着我们不会
//使用块缓存来存储它们）。
测试_F（BlockBasedTableTest，BlockCacheDisabledTest）
  期权期权；
  options.create_if_missing=true；
  options.statistics=createdbstatistics（）；
  BlockBasedTableOptions表\u选项；
  table_options.block_cache=newlrucache（1024，4）；
  表_options.filter_policy.reset（newbloomfilterpolicy（10））；
  options.table_factory.reset（new blockbasedTableFactory（table_options））；
  std:：vector<std:：string>keys；
  stl_wrappers：：kvmap kvmap；

  TableConstructor C（bytewiseComparator（），true/*将_转换为_内部_键_*/);

  c.Add("key", "value");
  const ImmutableCFOptions ioptions(options);
  c.Finish(options, ioptions, table_options,
           GetPlainInternalComparator(options.comparator), &keys, &kvmap);

//已启用预加载筛选器/索引块。
  auto reader = dynamic_cast<BlockBasedTable*>(c.GetTableReader());
  ASSERT_TRUE(reader->TEST_filter_block_preloaded());
  ASSERT_TRUE(reader->TEST_index_reader_preloaded());

  {
//一开始什么都没发生
    BlockCachePropertiesSnapshot props(options.statistics.get());
    props.AssertIndexBlockStat(0, 0);
    props.AssertFilterBlockStat(0, 0);
  }

  {
    GetContext get_context(options.comparator, nullptr, nullptr, nullptr,
                           GetContext::kNotFound, Slice(), nullptr, nullptr,
                           nullptr, nullptr, nullptr);
//只触发blockbasedtable:：getfilter的黑客。
    reader->Get(ReadOptions(), "non-exist-key", &get_context);
    BlockCachePropertiesSnapshot props(options.statistics.get());
    props.AssertIndexBlockStat(0, 0);
    props.AssertFilterBlockStat(0, 0);
  }
}

//由于统计数据之间相互作用的困难，本测试
//仅测试“将索引块放入块缓存”时的情况
TEST_F(BlockBasedTableTest, FilterBlockInBlockCache) {
//——台面结构
  Options options;
  options.create_if_missing = true;
  options.statistics = CreateDBStatistics();

//为索引/筛选块启用缓存
  BlockBasedTableOptions table_options;
  table_options.block_cache = NewLRUCache(1024, 4);
  table_options.cache_index_and_filter_blocks = true;
  options.table_factory.reset(new BlockBasedTableFactory(table_options));
  std::vector<std::string> keys;
  stl_wrappers::KVMap kvmap;

  /*leconstructor c（bytewiseComparator（），true/*将_转换为_internal_key_*/）；
  C.添加（“键”，“值”）；
  常量不变的选项（选项）；
  C.完成（选项，IOPTIONS，表格选项，
           getplainInternalComparator（options.comparator）、&keys和kvmap）；
  //禁止预加载筛选器/索引块。
  auto*reader=dynamic_cast<blockBasedTable*>（c.getTableReader（））；
  断言是真的（！）reader->test_filter_block_preloaded（））；
  断言是真的（！）reader->test_index_reader_preloaded（））；

  //—第1部分：使用常规块缓存打开。
  //由于块缓存被禁用，因此不会涉及任何缓存活动。
  唯一的指针<InternalIterator>Iter；

  int64_t last_cache_bytes_read=0；
  //首先，不会访问任何块。
  {
    blockCachePropertiesSnapshot属性（options.statistics.get（））；
    //索引将添加到块缓存中。
    props.assertequal（1，//索引块丢失
                      0, 0, 0）；
    断言eq（props.getcachebytesread（），0）；
    断言eq（props.getcachebyteswrite（），
              table_options.block_cache->getusage（））；
    最后一个缓存字节数=props.getcachebytesread（）；
  }

  //只访问索引块
  {
    iter.reset（c.newIterator（））；
    blockCachePropertiesSnapshot属性（options.statistics.get（））；
    //注意：为了帮助更好地突出每个断续器的“detla”，我使用
    //<last_value>>+<added_value>表示更改后的增量
    //值；其他数字保持不变。
    props.assertequal（1，0+1，//索引块命中
                      0, 0）；
    //缓存命中，从缓存读取的字节数应增加
    断言_gt（props.getcachebytesread（），最后一个_cache_bytes_read）；
    断言eq（props.getcachebyteswrite（），
              table_options.block_cache->getusage（））；
    最后一个缓存字节数=props.getcachebytesread（）；
  }

  //只访问数据块
  {
    iter->seektofirst（）；
    blockCachePropertiesSnapshot属性（options.statistics.get（））；
    props.assertequal（1，1，0+1，//数据块丢失
                      0）；
    //缓存未命中，从缓存读取的字节不应更改
    断言_eq（props.getcachebytesread（），最后一个_cache_bytes_read）；
    断言eq（props.getcachebyteswrite（），
              table_options.block_cache->getusage（））；
    最后一个缓存字节数=props.getcachebytesread（）；
  }

  //数据块将在缓存中
  {
    iter.reset（c.newIterator（））；
    iter->seektofirst（）；
    blockCachePropertiesSnapshot属性（options.statistics.get（））；
    props.assertequal（1，1+1，/*索引块命中*/

                      /*0+1/*数据块命中率*/）；
    //缓存命中，从缓存读取的字节数应增加
    断言_gt（props.getcachebytesread（），最后一个_cache_bytes_read）；
    断言eq（props.getcachebyteswrite（），
              table_options.block_cache->getusage（））；
  }
  //释放迭代器，以便块缓存可以正确重置。
  ITER

  c.重置读卡器（）；

  //—第2部分：用非常小的块缓存打开
  //在这个测试中，由于块缓存是
  //太小，无法容纳一个条目。
  table_options.block_cache=newlrucache（1，4）；
  options.statistics=createdbstatistics（）；
  options.table_factory.reset（new blockbasedTableFactory（table_options））；
  常量不变的选项IOPTIONS2（选项）；
  c.重新打开（操作2）；
  {
    blockCachePropertiesSnapshot属性（options.statistics.get（））；
    props.assertequal（1，//索引块丢失
                      0, 0, 0）；
    //缓存未命中，从缓存读取的字节不应更改
    断言eq（props.getcachebytesread（），0）；
  }

  {
    //同时访问索引和数据块。
    //首先缓存索引块，然后缓存数据块。但是由于缓存的大小
    //仅为1，插入数据块后将清除索引块。
    iter.reset（c.newIterator（））；
    blockCachePropertiesSnapshot属性（options.statistics.get（））；
    props.assertequal（1+1，//索引块丢失
                      0，0，//数据块丢失
                      0）；
    //缓存命中，从缓存读取的字节数应增加
    断言eq（props.getcachebytesread（），0）；
  }

  {
    //seektofirst（）访问数据块。由于同样的原因，我们期望数据
    //块的缓存未命中。
    iter->seektofirst（）；
    blockCachePropertiesSnapshot属性（options.statistics.get（））；
    props.assertequal（2，0，0+1，//数据块丢失
                      0）；
    //缓存未命中，从缓存读取的字节不应更改
    断言eq（props.getcachebytesread（），0）；
  }
  ITER
  c.重置读卡器（）；

  //—第3部分：打开表格，启用Bloom过滤器，但不在SST文件中
  table_options.block_cache=newlrucache（4096，4）；
  tableeu options.cache_index_and_filter_blocks=false；
  options.table_factory.reset（newblockbasedTableFactory（table_options））；

  TableConstructor c3（bytewiseComparator（））；
  std:：string用户_key=“k01”；
  内部密钥内部密钥（用户密钥，0，ktypeValue）；
  c3.add（internal_key.encode（）.toString（），“hello”）；
  不可变的选项IOPTIONS3（选项）；
  //生成不带筛选策略的表
  c3.完成（选项，ioptions3，表选项，
            getplainInternalComparator（options.comparator）、&keys和kvmap）；
  c3.重置读卡器（）；

  //用筛选策略打开表
  表_options.filter_policy.reset（newbloomfilterpolicy（1））；
  options.table_factory.reset（new blockbasedTableFactory（table_options））；
  options.statistics=createdbstatistics（）；
  不可变的选项IOPTONS4（选项）；
  断言“OK”（c3.reopen（ioptions4））；
  reader=dynamic_cast<blockBasedTable*>（c3.getTableReader（））；
  断言是真的（！）reader->test_filter_block_preloaded（））；
  pinnableslice值；
  getContext获取上下文（options.comparator、nullptr、nullptr、nullptr，
                         getContext:：knotfound，用户键，&value，nullptr，
                         nullptr、nullptr、nullptr）；
  断言ou ok（reader->get（readoptions（），user_key，&get_context））；
  assert_streq（value.data（），“hello”）；
  blockCachePropertiesSnapshot属性（options.statistics.get（））；
  props.assertfilterblockstat（0，0）；
  c3.重置读卡器（）；
}

void validateblocksizedeviation（需要int值，int值）
  BlockBasedTableOptions表\u选项；
  表\u options.block \u size \u deviation=value；
  blockBasedTableFactory*工厂=新的blockBasedTableFactory（表选项）；

  const blockbasedTableOptions*规范化的表选项=
      （const blockbasedTableOptions*）工厂->getOptions（）；
  断言“eq”（标准化的“table”选项->块“size”偏差，预期值）；

  删除工厂；
}

void validateBlockRestartInterval（需要int值，int）
  BlockBasedTableOptions表\u选项；
  表\u options.block \u restart \u interval=value；
  blockBasedTableFactory*工厂=新的blockBasedTableFactory（表选项）；

  const blockbasedTableOptions*规范化的表选项=
      （const blockbasedTableOptions*）工厂->getOptions（）；
  assert_eq（规范化的_table_options->block_restart_interval，预期）；

  删除工厂；
}

测试_F（BlockBasedTableTest，InvalidOptions）
  //块大小偏差（<0或>100）的无效值被静默设置为0
  validateblocksizedeviation（-10，0）；
  validateblocksizedeviation（-1，0）；
  validateblocksizedeviation（0，0）；
  validateblocksizedeviation（1，1）；
  validateblocksizedeviation（99，99）验证块大小偏差（99，99）；
  validateblocksizedeviation（100，100）；
  validateblocksizedeviation（101，0）；
  validateblocksizedeviation（1000，0）；

  //块重新启动间隔（<1）的无效值被静默设置为1
  validateBlockRestartInterval（-10，1）；
  validateBlockRestartInterval（-1，1）；
  validateBlockRestartInterval（0，1）；
  validateBlockRestartInterval（1，1）；
  validateBlockRestartInterval（2，2）时间间隔；
  validateBlockRestartInterval（1000，1000）次；
}

测试（blockbasedTableTest，blockreadCountTest）
  //bloom_filter_type=0——基于块的过滤器
  //Bloom_filter_type=0——全过滤
  对于（int bloom_filter_type=0；bloom_filter_type<2；++bloom_filter_type）
    for（int index_and_filter_in_cache=0；index_and_filter_in_cache<2；
         +index_and_filter_in_cache_
      期权期权；
      options.create_if_missing=true；

      BlockBasedTableOptions表\u选项；
      table_options.block_cache=newlrucache（1，0）；
      table_options.cache_index_and_filter_blocks=index_and_filter_in_cache；
      表\u options.filter \u policy.reset（
          Newbloomfilterpolicy（10，bloom_filter_type==0））；
      options.table_factory.reset（new blockbasedTableFactory（table_options））；
      std:：vector<std:：string>keys；
      stl_wrappers：：kvmap kvmap；

      TableConstructor C（bytewiseComparator（））；
      std:：string user_key=“k04”；
      内部密钥内部密钥（用户密钥，0，ktypeValue）；
      std:：string encoded_key=internal_key.encode（）.toString（）；
      c.add（编码的_键，“hello”）；
      不变的选项（选项）；
      //使用筛选策略生成表
      C.完成（选项，IOPTIONS，表格选项，
               getplainInternalComparator（options.comparator）、&keys和kvmap）；
      自动读卡器=c.getTableReader（）；
      pinnableslice值；
      getContext获取上下文（options.comparator、nullptr、nullptr、nullptr，
                             getContext:：knotfound，用户键，&value，nullptr，
                             nullptr、nullptr、nullptr）；
      get_perf_context（）->reset（）；
      断言ou ok（reader->get（readoptions（），encoded_key，&get_context））；
      if（index_and_filter_in_cache）
        //数据、索引和筛选器块
        断言_eq（get_perf_context（）->block_read_count，3）；
      }否则{
        //只是数据块
        assert_eq（get_perf_context（）->block_read_count，1）；
      }
      断言_eq（get_context.state（），getContext:：kFound）；
      assert_streq（value.data（），“hello”）；

      //获取不存在的密钥
      user_key=“不存在”；
      内部关键字=内部关键字（用户关键字，0，ktypeValue）；
      encoded_key=internal_key.encode（）.toString（）；

      REST（）；
      获取context=getContext（options.comparator、nullptr、nullptr、nullptr，
                               getContext:：knotfound，用户键，&value，nullptr，
                               nullptr、nullptr、nullptr）；
      get_perf_context（）->reset（）；
      断言ou ok（reader->get（readoptions（），encoded_key，&get_context））；
      断言_Eq（get_Context.state（），getContext:：knotFound）；

      if（index_and_filter_in_cache）
        如果（Bloom_filter_type==0）
          //对于基于块的，我们先读取索引，然后读取筛选器
          断言eq（get_perf_context（）->block_read_count，2）；
        }否则{
          //使用全过滤器时，我们先读取过滤器，然后停止
          assert_eq（get_perf_context（）->block_read_count，1）；
        }
      }否则{
        //filter已经在内存中，它发现键没有
        /存在
        assert_eq（get_perf_context（）->block_read_count，0）；
      }
    }
  }
}

//围绕lricache的包装器，它还跟踪数据块（相反
//在缓存中。这个类非常简单，只能使用
//对于普通测试。
类mockcache:public lrucache_
 公众：
  mockcache（大小\t capacity，int num \u shard \u位，bool strict \u capacity \u限制，
            双倍高的池池比率）
      ：lrucache（容量，num_shard_位，严格的_容量_限制，
                 __
  虚拟状态插入（常量切片和键、void*值、大小和费用，
                        void（*deleter）（const slice&key，void*值）
                        句柄**句柄=nullptr，
                        优先级=优先级：：低）覆盖
    //用自己的删除程序替换删除程序，以便跟踪数据块
    //从缓存中删除
    deleters_[key.toString（）]=删除程序；
    返回sharedcache:：insert（key，value，charge，&mockdeleter，handle，
                                优先权；
  }
  //应用程序在插入数据块后立即调用此函数
  虚拟无效测试将标记为数据块（常量切片和键，
                                       尺寸收费）覆盖
    在“缓存”中标记为“数据”[key.toString（）]=充电；
    标记的尺寸=电荷；
  }
  使用deleterfunc=void（*）（const slice&key，void*值）；
  static std:：map<std:：string，deleterfunc>deleters_u；
  static std:：map<std:：string，size_t>在缓存中标记了_data_；
  静态尺寸标记尺寸；
  静态void mockdeleter（const slice&key，void*value）
    //如果该项被标记为数据块，请从
    //缓存的数据块使用总量
    如果（在缓存中标记了_data_.find（key.toString（））！=
        _cache_u.end（）中标记的_data__
      marked_size_-=在_cache_u[key.toString（）]中标记的_data_；
    }
    //然后调用origianl deleter
    断言（deleters_.find（key.toString（））！=deleters_.end（））；
    auto-deleter=deleters_[key.toString（）]；
    删除器（键，值）；
  }
}；

大小\u t mockcache：：已标记\u大小\=0；
std:：map<std:：string，mockcache:：deleterfnc>mockcache:：deleters_u；
std:：map<std:：string，size_t>mockcache:：marked_data_in_cache_uu；

//块缓存可以包含原始数据块和常规对象。如果一个
//对象依赖于要活动的表，然后必须在
//表已关闭。此测试确保只有项保留在
//表关闭后缓存为原始数据块。
TEST_F（BlockBasedTableTest，NoObjecticacheaftTableClose）
  对于（自动索引类型：
       blockbasedTableOptions:：indexType:：kbinarySearch，
        blockBasedTableOptions:：indexType:：ktwLovelIndexSearch）
    for（bool block_-based_filter:真，假）
      for（bool分区_filter:真，假）
        如果（分区过滤器和
            （block_-based_filter_
             索引类型！=
                 blockBasedTableOptions:：indexType:：ktwLovelIndexSearch））
          继续；
        }
        for（bool index_and_filter_in_cache:真，假）
          对于（bool pin_l0：真，假）
            如果（PixL0 & &！索引_和_过滤器_
              继续；
            }
            //创建表
            期权选择；
            唯一的_ptr<InternalKeyComparator>ikc；
            重置（new test:：plainInternalKeyComparator（opt.Comparator））；
            opt.compression=knocompression；
            BlockBasedTableOptions表\u选项；
            表\options.block \u size=1024；
            表\选项.索引\类型=
                blockBasedTableOptions:：indexType:：ktwLovelIndexSearch；
            table_options.pin_l0_filter_and_index_blocks_in_cache=pin_l0；
            tableeu options.partition_filters=分区_filter；
            table_options.cache_index_and_filter_块=
                索引缓存中的_和_过滤器；
            //足够大，这样我们就不会丢失缓存值。
            table_options.block_cache=std:：shared_ptr<rocksdb:：cache>（）
                新建mockcache（16*1024*1024，4，false，0.0））；
            表\u options.filter \u policy.reset（
                rocksdb:：newbloomfilterpolicy（10，block_-based_-filter））；
            opt.table_factory.reset（newblockbasedTableFactory（table_options））；

            TableConstructor C（bytewiseComparator（））；
            std:：string用户_key=“k01”；
            标准：：字符串键=
                internalkey（user_key，0，ktypeValue）.encode（）.toString（）；
            C.添加（键，“你好”）；
            std:：vector<std:：string>keys；
            stl_wrappers：：kvmap kvmap；
            不变常数变量IOPTIONS（opt）；
            C.完成（opt，ioptions，table_options，*ikc，&keys，&kvmap）；

            //进行读取以使索引/筛选器加载到缓存中
            自动表格阅读器=
                dynamic_cast<blockBasedTable*>（c.getTableReader（））；
            pinnableslice值；
            getContext获取上下文（opt.comparator、nullptr、nullptr、nullptr，
                                   getContext:：knotfound，用户密钥和值，
                                   nullptr、nullptr、nullptr、nullptr）；
            internalkey ikey（用户密钥，0，ktypeValue）；
            auto s=table_reader->get（readoptions（），key，&get_context）；
            断言_eq（get_context.state（），getContext:：kFound）；
            assert_streq（value.data（），“hello”）；

            //关闭表
            c.重置读卡器（）；

            auto usage=table_options.block_cache->getusage（）；
            auto-pinned_usage=table_options.block_cache->getpinnedusage（）；
            //唯一的用法必须用于标记的数据块
            断言“eq”（用法，mockcache：：已标记“大小”）；
            //必须有一些固定数据，因为pinnableSlice没有
            //已经释放了它们
            断言gt（固定使用，0）；
            //释放可固定切片
            REST（）；
            pinned_usage=table_options.block_cache->getpinnedusage（）；
            断言eq（钉住的用法，0）；
          }
        }
      }
    }
  }
}

测试_F（BlockBasedTableTest，BlockCacheLeak）
  //检查当我们重新打开表时，是否已经失去对块的访问权限
  //在缓存中。此测试检查表是否实际使用
  //文件中的唯一ID。

  期权选择；
  唯一的_ptr<InternalKeyComparator>ikc；
  重置（new test:：plainInternalKeyComparator（opt.Comparator））；
  opt.compression=knocompression；
  BlockBasedTableOptions表\u选项；
  表\options.block \u size=1024；
  //足够大，这样我们就不会丢失缓存值。
  table_options.block_cache=newlrucache（16*1024*1024，4）；
  opt.table_factory.reset（newblockbasedTableFactory（table_options））；

  TableConstructor C（bytewiseComparator（），true/*将_转换为_内部_键_*/);

  c.Add("k01", "hello");
  c.Add("k02", "hello2");
  c.Add("k03", std::string(10000, 'x'));
  c.Add("k04", std::string(200000, 'x'));
  c.Add("k05", std::string(300000, 'x'));
  c.Add("k06", "hello3");
  c.Add("k07", std::string(100000, 'x'));
  std::vector<std::string> keys;
  stl_wrappers::KVMap kvmap;
  const ImmutableCFOptions ioptions(opt);
  c.Finish(opt, ioptions, table_options, *ikc, &keys, &kvmap);

  unique_ptr<InternalIterator> iter(c.NewIterator());
  iter->SeekToFirst();
  while (iter->Valid()) {
    iter->key();
    iter->value();
    iter->Next();
  }
  ASSERT_OK(iter->status());

  const ImmutableCFOptions ioptions1(opt);
  ASSERT_OK(c.Reopen(ioptions1));
  auto table_reader = dynamic_cast<BlockBasedTable*>(c.GetTableReader());
  for (const std::string& key : keys) {
    ASSERT_TRUE(table_reader->TEST_KeyInCache(ReadOptions(), key));
  }
  c.ResetTableReader();

//使用不同的块缓存重新运行
  table_options.block_cache = NewLRUCache(16 * 1024 * 1024, 4);
  opt.table_factory.reset(NewBlockBasedTableFactory(table_options));
  const ImmutableCFOptions ioptions2(opt);
  ASSERT_OK(c.Reopen(ioptions2));
  table_reader = dynamic_cast<BlockBasedTable*>(c.GetTableReader());
  for (const std::string& key : keys) {
    ASSERT_TRUE(!table_reader->TEST_KeyInCache(ReadOptions(), key));
  }
  c.ResetTableReader();
}

TEST_F(BlockBasedTableTest, NewIndexIteratorLeak) {
//避免数据争用的回归测试
//https://github.com/facebook/rocksdb/issues/1267
  /*leconstructor c（bytewiseComparator（），true/*将_转换为_internal_key_*/）；
  std:：vector<std:：string>keys；
  stl_wrappers：：kvmap kvmap；
  C.添加（“A1”，“VAL1”）；
  期权期权；
  options.prefix提取程序.reset（newfixedprifixtransform（1））；
  BlockBasedTableOptions表\u选项；
  tableou options.index_type=blockBasedTableOptions:：khashsearch；
  table_options.cache_index_and_filter_blocks=true；
  table_options.block_cache=newlrucache（0）；
  options.table_factory.reset（newblockbasedTableFactory（table_options））；
  常量不变的选项（选项）；
  C.完成（选项，IOPTIONS，表格选项，
           getplainInternalComparator（options.comparator）、&keys和kvmap）；

  rocksdb:：syncpoint:：getInstance（）->loadDependencyAndMarkers（）。
      {
          “BlockBasedTable:：NewIndexIterator:：Thread1:1”，
           “BlockBasedTable:：NewIndexIterator:：Thread2:2”，
          “BlockBasedTable:：NewIndexIterator:：Thread2:3”，
           “BlockBasedTable:：NewIndexIterator:：Thread1:4”，
      }
      {
          “BlockBasedTableTest:：NewIndexIteratorLeak:线程1标记”，
           “BlockBasedTable:：NewIndexIterator:：Thread1:1”，
          “BlockBasedTableTest:：NewIndexIteratorLeak:线程1标记”，
           “BlockBasedTable:：NewIndexIterator:：Thread1:4”，
          “BlockBasedTableTest:：NewIndexIteratorLeak:线程标记2”，
           “BlockBasedTable:：NewIndexIterator:：Thread2:2”，
          “BlockBasedTableTest:：NewIndexIteratorLeak:线程标记2”，
           “BlockBasedTable:：NewIndexIterator:：Thread2:3”，
      （}）；

  rocksdb:：syncpoint:：getInstance（）->启用处理（）；
  阅读选项RO；
  auto*reader=c.gettablereader（）；

  std:：function<void（）>func1=[&]（）
    测试同步点（“BlockBasedTableTest:：NewIndexIteratorLeak:Thread1Marker”）；
    std:：unique_ptr<internaliterator>iter（reader->newiterator（ro））；
    iter->seek（internalkey（“a1”，0，ktypeValue）.encode（））；
  }；

  std:：function<void（）>func2=[&]（）
    测试同步点（“BlockBasedTableTest:：NewIndexIteratorLeak:Thread2Marker”）；
    std:：unique_ptr<internaliterator>iter（reader->newiterator（ro））；
  }；

  auto thread1=端口：：线程（func1）；
  auto thread2=端口：：线程（func2）；
  TyRe1..Cube（）；
  TyRe2..Cube（）；
  rocksdb:：syncpoint:：getInstance（）->disableProcessing（）；
  c.重置读卡器（）；
}

//rocksdb-lite中不支持普通表
ifndef岩石
测试_f（PlaintableTest，BasicPlaintableProperties）
  普通表选项普通表选项；
  plain_table_options.user_key_len=8；
  plain_table_options.bloom_bits_per_key=8；
  plain_table_options.hash_table_ratio=0；

  平板工厂（平板选项）；
  测试：：StringSink接收器；
  唯一的文件写入程序（
      test:：getWritableFileWriter（new test:：StringSink（））；
  期权期权；
  常量不变的选项（选项）；
  内部键比较器ikc（options.comparator）；
  std:：vector<std:：unique_ptr<inttblpropcollectorFactory>>
      内特布尔道具收藏家工厂；
  std：：字符串列_family_name；
  int未知_级=-1；
  std:：unique_ptr<tablebuilder>builder（factory.newtablebuilder（
      TableBuilderOptions（IOOptions、IKC和Int_-Tbl_Prop_Collector_工厂，
                          knocompression，compressionOptions（），
                          nullptr/*压缩\u dict*/,

                          /*SE/*跳过\过滤器*/，列\族\名称，
                          未知级别）
      TablePropertiesCollectorFactory:：Context:：KunknownColumnFamily，
      文件_writer.get（））；

  for（char c='a'；c<='z'；++c）
    std：：字符串键（8，c）；
    key.append（“\1”）；//PlainTable需要内部键结构
    std：：字符串值（28，c+42）；
    生成器->添加（键，值）；
  }
  断言_OK（builder->finish（））；
  文件_writer->flush（）；

  测试：：StringSink*SS=
    static_cast<test:：stringsink*>（file_writer->writable_file（））；
  唯一的读卡器
      测试：：GetRandomAccessFileReader（
          新测试：：stringsource（ss->contents（），72242，true））；

  tableproperties*props=nullptr；
  auto s=readTableProperties（file_reader.get（），s s->contents（）.size（），
                               kPlainTableMagicNumber，IOptions，数字，
                               道具）；
  std:：unique_ptr<tableproperties>props_guard（props）；
  SalpTyk OK（s）；

  断言“eq”（0ul，props->index”大小）；
  断言eq（0ul，props->filter_size）；
  断言eq（16ul*26，props->raw_key_size）；
  断言eq（28ul*26，props->raw_value_size）；
  断言_Eq（26ul，props->num_entries）；
  断言_Eq（1ul，props->num_data_blocks）；
}
我很喜欢你！摇滚乐

试验F（一般表试验，平原近似偏移）
  TableConstructor C（bytewiseComparator（），true/*将_转换为_内部_键_*/);

  c.Add("k01", "hello");
  c.Add("k02", "hello2");
  c.Add("k03", std::string(10000, 'x'));
  c.Add("k04", std::string(200000, 'x'));
  c.Add("k05", std::string(300000, 'x'));
  c.Add("k06", "hello3");
  c.Add("k07", std::string(100000, 'x'));
  std::vector<std::string> keys;
  stl_wrappers::KVMap kvmap;
  Options options;
  test::PlainInternalKeyComparator internal_comparator(options.comparator);
  options.compression = kNoCompression;
  BlockBasedTableOptions table_options;
  table_options.block_size = 1024;
  const ImmutableCFOptions ioptions(options);
  c.Finish(options, ioptions, table_options, internal_comparator,
           &keys, &kvmap);

  ASSERT_TRUE(Between(c.ApproximateOffsetOf("abc"),       0,      0));
  ASSERT_TRUE(Between(c.ApproximateOffsetOf("k01"),       0,      0));
  ASSERT_TRUE(Between(c.ApproximateOffsetOf("k01a"),      0,      0));
  ASSERT_TRUE(Between(c.ApproximateOffsetOf("k02"),       0,      0));
  ASSERT_TRUE(Between(c.ApproximateOffsetOf("k03"),       0,      0));
  ASSERT_TRUE(Between(c.ApproximateOffsetOf("k04"),   10000,  11000));
//K04和K05将在两个连续的块中，索引为
//K04和K05之间的任意切片，在K04A之前或之后
  ASSERT_TRUE(Between(c.ApproximateOffsetOf("k04a"), 10000, 211000));
  ASSERT_TRUE(Between(c.ApproximateOffsetOf("k05"),  210000, 211000));
  ASSERT_TRUE(Between(c.ApproximateOffsetOf("k06"),  510000, 511000));
  ASSERT_TRUE(Between(c.ApproximateOffsetOf("k07"),  510000, 511000));
  ASSERT_TRUE(Between(c.ApproximateOffsetOf("xyz"),  610000, 612000));
  c.ResetTableReader();
}

static void DoCompressionTest(CompressionType comp) {
  Random rnd(301);
  /*leconstructor c（bytewiseComparator（），true/*将_转换为_internal_key_*/）；
  std：：字符串tmp；
  C.ADD（“K01”，“你好”）；
  C.ADD（“K02”，测试：：压缩管柱（&RND，0.25，10000，&TMP））；
  C.添加（“K03”，“hello3”）；
  C.ADD（“K04”，测试：：压缩管柱（&RND，0.25，10000，&TMP））；
  std:：vector<std:：string>keys；
  stl_wrappers：：kvmap kvmap；
  期权期权；
  测试：：plainInternalKeyComparator ikc（options.comparator）；
  options.compression=压缩；
  BlockBasedTableOptions表\u选项；
  表\options.block \u size=1024；
  常量不变的选项（选项）；
  c.完成（选项、ioptions、table_选项、ikc、&keys和kvmap）；

  断言“真”（在（c.approceoffsetof（“abc”），0，0）之间）；
  断言“真”（介于（c.approceoffsetof（“k01”），0，0）之间）；
  断言“真”（介于（c.approceoffsetof（“k02”），0，0）之间）；
  断言“真”（介于（c.approceoffsetof（“k03”），2000，3000））；
  断言“真”（介于（c.approceoffsetof（“k04”），2000，3000））；
  断言“真”（介于（c.approceoffsetof（“xyz”），4000，6100）之间）；
  c.重置读卡器（）；
}

测试F（一般表测试，压缩后的近似值）
  std：：vector<compressionType>compression_state；
  如果（！）snappy_supported（））
    fprintf（stderr，“跳过快速压缩测试”）；
  }否则{
    压缩状态。推回（ksnappycompression）；
  }

  如果（！）zlib_supported（））
    fprintf（stderr，“跳过zlib压缩测试”）；
  }否则{
    压缩状态。向后推（kzlibcompression）；
  }

  //todo（kailiu）doccompressontest（）不适用于bzip2。
  /*
  如果（！）bzip2_supported（））
    fprintf（stderr，“跳过bzip2压缩测试”）；
  }否则{
    压缩状态。向后推（kbzip2压缩）；
  }
  **/


  if (!LZ4_Supported()) {
    fprintf(stderr, "skipping lz4 and lz4hc compression tests\n");
  } else {
    compression_state.push_back(kLZ4Compression);
    compression_state.push_back(kLZ4HCCompression);
  }

  if (!XPRESS_Supported()) {
    fprintf(stderr, "skipping xpress and xpress compression tests\n");
  }
  else {
    compression_state.push_back(kXpressCompression);
  }

  for (auto state : compression_state) {
    DoCompressionTest(state);
  }
}

TEST_F(HarnessTest, Randomized) {
  std::vector<TestArgs> args = GenerateArgList();
  for (unsigned int i = 0; i < args.size(); i++) {
    Init(args[i]);
    Random rnd(test::RandomSeed() + 5);
    for (int num_entries = 0; num_entries < 2000;
         num_entries += (num_entries < 50 ? 1 : 200)) {
      if ((num_entries % 10) == 0) {
        fprintf(stderr, "case %d of %d: num_entries = %d\n", (i + 1),
                static_cast<int>(args.size()), num_entries);
      }
      for (int e = 0; e < num_entries; e++) {
        std::string v;
        Add(test::RandomKey(&rnd, rnd.Skewed(4)),
            test::RandomString(&rnd, rnd.Skewed(5), &v).ToString());
      }
      Test(&rnd);
    }
  }
}

#ifndef ROCKSDB_LITE
TEST_F(HarnessTest, RandomizedLongDB) {
  Random rnd(test::RandomSeed());
  TestArgs args = {DB_TEST, false, 16, kNoCompression, 0, false};
  Init(args);
  int num_entries = 100000;
  for (int e = 0; e < num_entries; e++) {
    std::string v;
    Add(test::RandomKey(&rnd, rnd.Skewed(4)),
        test::RandomString(&rnd, rnd.Skewed(5), &v).ToString());
  }
  Test(&rnd);

//我们必须创建足够的数据来强制合并
  int files = 0;
  for (int level = 0; level < db()->NumberLevels(); level++) {
    std::string value;
    char name[100];
    snprintf(name, sizeof(name), "rocksdb.num-files-at-level%d", level);
    ASSERT_TRUE(db()->GetProperty(name, &value));
    files += atoi(value.c_str());
  }
  ASSERT_GT(files, 0);
}
#endif  //摇滚乐

class MemTableTest : public testing::Test {};

TEST_F(MemTableTest, Simple) {
  InternalKeyComparator cmp(BytewiseComparator());
  auto table_factory = std::make_shared<SkipListFactory>();
  Options options;
  options.memtable_factory = table_factory;
  ImmutableCFOptions ioptions(options);
  WriteBufferManager wb(options.db_write_buffer_size);
  MemTable* memtable =
      new MemTable(cmp, ioptions, MutableCFOptions(options), &wb,
                   /*xSequenceNumber，0/*列_-family_-id*/）；
  memtable->ref（）；
  写入批处理；
  WriteBatchInternal:：SetSequence（&batch，100）；
  batch.put（std:：string（“k1”），std:：string（“v1”））；
  batch.put（std:：string（“k2”），std:：string（“v2”））；
  batch.put（std:：string（“k3”），std:：string（“v3”））；
  batch.put（std:：string（“largekey”），std:：string（“vlarge”））；
  batch.deleterage（std:：string（“chi”），std:：string（“xigua”））；
  batch.deleterage（std:：string（“begin”），std:：string（“end”））；
  ColumnFamilyMemTablesDefault cf_Mems_Default（MemTable）；
  断言（真）
      writeBatchInternal:：Insertinto（&batch，&cf_mems_default，nullptr）.ok（））；

  对于（int i=0；i<2；++i）
    竞技场竞技场；
    竞技场守卫；
    std:：unique_ptr<internaliterator>iter_guard；
    内部迭代器*iter；
    如果（i＝0）{
      iter=memtable->newiterator（readoptions（），&arena）；
      竞技场警卫室（ITER）；
    }否则{
      iter=memtable->newRangeTombstoneIterator（readOptions（））；
      ITER保护重置（ITER）；
    }
    if（iter==nullptr）
      继续；
    }
    iter->seektofirst（）；
    while（iter->valid（））
      fprintf（stderr，“键：”%s“->”%s'\n“，iter->key（）.toString（）.c_str（），
              iter->value（）.toString（）.c_str（））；
      ITER > NEXT（）；
    }
  }

  删除memtable->unref（）；
}

//测试空键
测试_F（线束测试，单工空车）
  auto args=generatearglist（）；
  用于（const auto&arg:args）
    初始化（ARG）；
    随机RND（测试：：randomseed（）+1）；
    添加（“”，“V”）；
    测试（＆RND）；
  }
}

测试F（线束测试，单工）
  auto args=generatearglist（）；
  用于（const auto&arg:args）
    初始化（ARG）；
    随机RND（测试：：randomseed（）+2）；
    添加（“abc”，“v”）；
    测试（＆RND）；
  }
}

测试_F（线束测试，单工多功能）
  auto args=generatearglist（）；
  用于（const auto&arg:args）
    初始化（ARG）；
    随机RND（测试：：randomseed（）+3）；
    添加（“abc”，“v”）；
    添加（“abcd”，“v”）；
    添加（“ac”，“v2”）；
    测试（＆RND）；
  }
}

测试_F（线束测试，简单专用钥匙）
  auto args=generatearglist（）；
  用于（const auto&arg:args）
    初始化（ARG）；
    随机RND（测试：：randomseed（）+4）；
    添加（“\xff\xff”，“v3”）；
    测试（＆RND）；
  }
}

测试_F（线束测试、脚部测试）
  {
    //基于上转换的旧块
    std：：字符串编码；
    页脚（KlegacyBlockBasedTableMagicNumber，0）；
    块句柄元索引（10，5），索引（20，15）；
    footer.set_meta index_handle（meta_index）；
    footer.设置索引句柄（index）；
    footer.encodeto（&encoded）；
    页脚解码后的页脚；
    切片编码的切片（编码的）；
    解码后的页脚。解码自（&encoded\u slice）；
    assert_eq（解码的_footer.table_magic_number（），kblockbasedTablemagicNumber）；
    断言_eq（解码_footer.checksum（），kcrc32c）；
    断言_eq（解码后的_footer.meta index_handle（）.offset（），meta_index.offset（））；
    断言_eq（解码后的_footer.meta index_handle（）.size（），meta_index.size（））；
    断言_eq（解码后的_footer.index_handle（）.offset（），index.offset（））；
    断言_eq（解码后的_footer.index_handle（）.size（），index.size（））；
    断言_eq（解码后的_footer.version（），0u）；
  }
  {
    //基于XXhash块
    std：：字符串编码；
    页脚（KblockBasedTableMagicNumber，1）；
    块句柄元索引（10，5），索引（20，15）；
    footer.set_meta index_handle（meta_index）；
    footer.设置索引句柄（index）；
    footer.set_checksum（kxxash）；
    footer.encodeto（&encoded）；
    页脚解码后的页脚；
    切片编码的切片（编码的）；
    解码后的页脚。解码自（&encoded\u slice）；
    assert_eq（解码的_footer.table_magic_number（），kblockbasedTablemagicNumber）；
    断言_Eq（解码后的_footer.checksum（），kxxhash）；
    断言_eq（解码后的_footer.meta index_handle（）.offset（），meta_index.offset（））；
    断言_eq（解码后的_footer.meta index_handle（）.size（），meta_index.size（））；
    断言_eq（解码后的_footer.index_handle（）.offset（），index.offset（））；
    断言_eq（解码后的_footer.index_handle（）.size（），index.size（））；
    断言_eq（解码后的_footer.version（），1u）；
  }
//rocksdb-lite中不支持普通表
ifndef岩石
  {
    //向上转换旧的普通表
    std：：字符串编码；
    页脚（KlegacyPlainTableMagicNumber，0）；
    块句柄元索引（10，5），索引（20，15）；
    footer.set_meta index_handle（meta_index）；
    footer.设置索引句柄（index）；
    footer.encodeto（&encoded）；
    页脚解码后的页脚；
    切片编码的切片（编码的）；
    解码后的页脚。解码自（&encoded\u slice）；
    断言_eq（解码后的_footer.table_magic_number（），kPlainTablemagicNumber）；
    断言_eq（解码_footer.checksum（），kcrc32c）；
    断言_eq（解码后的_footer.meta index_handle（）.offset（），meta_index.offset（））；
    断言_eq（解码后的_footer.meta index_handle（）.size（），meta_index.size（））；
    断言_eq（解码后的_footer.index_handle（）.offset（），index.offset（））；
    断言_eq（解码后的_footer.index_handle（）.size（），index.size（））；
    断言_eq（解码后的_footer.version（），0u）；
  }
  {
    //基于XXhash块
    std：：字符串编码；
    页脚（kPlainTableMagicNumber，1）；
    块句柄元索引（10，5），索引（20，15）；
    footer.set_meta index_handle（meta_index）；
    footer.设置索引句柄（index）；
    footer.set_checksum（kxxash）；
    footer.encodeto（&encoded）；
    页脚解码后的页脚；
    切片编码的切片（编码的）；
    解码后的页脚。解码自（&encoded\u slice）；
    断言_eq（解码后的_footer.table_magic_number（），kPlainTablemagicNumber）；
    断言_Eq（解码后的_footer.checksum（），kxxhash）；
    断言_eq（解码后的_footer.meta index_handle（）.offset（），meta_index.offset（））；
    断言_eq（解码后的_footer.meta index_handle（）.size（），meta_index.size（））；
    断言_eq（解码后的_footer.index_handle（）.offset（），index.offset（））；
    断言_eq（解码后的_footer.index_handle（）.size（），index.size（））；
    断言_eq（解码后的_footer.version（），1u）；
  }
我很喜欢你！摇滚乐
  {
    //版本＝2
    std：：字符串编码；
    页脚（KblockBasedTableMagicNumber，2）；
    块句柄元索引（10，5），索引（20，15）；
    footer.set_meta index_handle（meta_index）；
    footer.设置索引句柄（index）；
    footer.encodeto（&encoded）；
    页脚解码后的页脚；
    切片编码的切片（编码的）；
    解码后的页脚。解码自（&encoded\u slice）；
    assert_eq（解码的_footer.table_magic_number（），kblockbasedTablemagicNumber）；
    断言_eq（解码_footer.checksum（），kcrc32c）；
    断言_eq（解码后的_footer.meta index_handle（）.offset（），meta_index.offset（））；
    断言_eq（解码后的_footer.meta index_handle（）.size（），meta_index.size（））；
    断言_eq（解码后的_footer.index_handle（）.offset（），index.offset（））；
    断言_eq（解码后的_footer.index_handle（）.size（），index.size（））；
    断言_eq（解码后的_footer.version（），2u）；
  }
}

类indexBlockRestartIntervalTest
    ：公共BlockBasedTable测试，
      public:：testing:：withParamInterface<int>
 公众：
  static std:：vector<int>getRestartValues（）返回-1，0，1，8，16，32
}；

实例化测试用例
    indexBlockRestartIntervalTest，indexBlockRestartIntervalTest，索引块重新启动IntervalTest，
    ：：测试：：valuesin（indexBlockRestartIntervalTest:：GetRestartValues（））；

测试_p（indexblockrestartinterval test，indexblockrestartinterval）
  const int kkeystintable=10000；
  const int kkeysize=100；
  const int kvalsize=500；

  int index_block_restart_interval=getparam（）；

  期权期权；
  BlockBasedTableOptions表\u选项；
  table_options.block_size=64；//获得大索引块的小块大小
  表\u options.index_block_restart_interval=index_block_restart_interval；
  options.table_factory.reset（new blockbasedTableFactory（table_options））；

  TableConstructor C（bytewiseComparator（））；
  静态随机RND（301）；
  对于（int i=0；i<kkeystintable；i++）
    internalkey k（randomstring（&rnd，kkeysize），0，ktypeValue）；
    c.add（k.encode（）.toString（），randomstring（&rnd，kvalsize））；
  }

  std:：vector<std:：string>keys；
  stl_wrappers：：kvmap kvmap；
  std:：unique_ptr<internalKeyComparator>Comparator（
      新的InternalKeyComparator（bytewiseComparator（））；
  常量不变的选项（选项）；
  c.完成（选项、IOPTIONS、表格选项、*比较器、&keys和kvmap）；
  自动读卡器=c.getTableReader（）；

  std:：unique_ptr<internalIterator>db_iter（reader->newIterator（readoptions（））；

  //测试点查找
  对于（auto&kv:kvmap）
    db-iter->seek（kv.first）；

    断言_true（db_iter->valid（））；
    断言_OK（db_Iter->status（））；
    断言_Eq（db_Iter->key（），kv.first）；
    断言_Eq（db_Iter->value（），kv.second）；
  }

  //测试迭代
  auto-kv？iter=kvmap.begin（）；
  对于（db_Iter->seektoffirst（）；db_Iter->valid（）；db_Iter->next（））；
    断言_Eq（db_Iter->key（），kv_Iter->first）；
    断言_Eq（db_Iter->value（），kv_Iter->second）；
    Kviist+；
  }
  断言eu eq（kveu iter，kvmap.end（））；
  c.重置读卡器（）；
}

类前缀测试：公共测试：：测试
 公众：
  prefixtest（）：测试：：test（）
  ~prefixtest（）
}；

命名空间{
//一个简单的前缀量角器，只适用于测试prefix和wholekeytest
类TestPrefixtractor:public rocksdb:：sliceTransform_
 公众：
  ~testPrefixtractor（）重写；
  const char*name（）const override返回“testforxextractor”；

  rocksdb:：slice transform（const rocksdb:：slice&src）const override_
    断言（isvalid（src））；
    返回rocksdb:：slice（src.data（），3）；
  }

  bool indomain（const rocksdb:：slice&src）const override_
    断言（isvalid（src））；
    回归真实；
  }

  bool inrange（const rocksdb:：slice&dst）const override返回true；

  bool is valid（const rocksdb:：slice&src）const_
    如果（src.size（）！= 4）{
      返回错误；
    }
    如果（SRC〔0〕！= [（]）{
      返回错误；
    }
    if（src[1]<'0'src[1]>'9'）
      返回错误；
    }
    如果（SRC〔2〕！= '']）{
      返回错误；
    }
    if（src[3]<'0'src[3]>'9'）
      返回错误；
    }
    回归真实；
  }
}；
} / /命名空间

测试_F（prefixtest、prefix和wholekeytest）
  rocksdb：：选项；
  options.compaction_style=rocksdb:：kCompactionStyleUniversal；
  options.num_级别=20；
  options.create_if_missing=true；
  options.optimize_filters_for_hits=false；
  options.target_file_size_base=268435456；
  options.prefix_extractor=std:：make_shared<testPrefixtractor>（）；
  rocksdb:：blockbasedTableOptions bbto；
  bbto.filter_policy.reset（rocksdb:：newbloomfilterpolicy（10））；
  bbto.block_大小=262144；
  bbto.whole_key_filtering=true；

  const std:：string kdbpath=test:：tmpdir（）+“/table_prefix_test”；
  选项.table_factory.reset（newblockbasedTableFactory（bbto））；
  DestroyDB（kdbpath，选项）；
  RockSDB：：分贝*分贝；
  断言ou OK（rocksdb:：db:：open（options，kdbpath，&db））；

  //创建一组带有10个过滤器的键。
  对于（int i=0；i<10；i++）
    std:：string prefix=“[”+std:：to_string（i）+“]”；
    对于（int j=0；j<10；j++）
      std:：string key=prefix+std:：to_string（j）；
      db->put（rocksdb:：writeoptions（），key，“1”）；
    }
  }

  //触发压缩。
  db->compactRange（compactRangeOptions（），nullptr，nullptr）；
  删除数据库；
  //在第二轮中，关闭整个\键\过滤并预期
  //rocksdb仍然有效。
}

测试_f（blockbasedtabletest，tablewithglobalseqno）
  基于块的表选项bbto；
  测试：：StringSink*Sink=新测试：：StringSink（）；
  唯一的_ptr<writablefilewriter>文件_writer（test:：getwritablefilewriter（sink））；
  期权期权；
  选项.table_factory.reset（newblockbasedTableFactory（bbto））；
  常量不变的选项（选项）；
  内部键比较器ikc（options.comparator）；
  std:：vector<std:：unique_ptr<inttblpropcollectorFactory>>
      内特布尔道具收藏家工厂；
  内特堡道具收藏家工厂。安置后（
      新的sstfileWriterPropertiesCollectorFactory（2/*版本*/,

                                                  /**全球电话号码*/）；
  std：：字符串列_family_name；
  std:：unique_ptr<table builder>builder（options.table_factory->newtablebuilder（
      TableBuilderOptions（IOOptions、IKC和Int_-Tbl_Prop_Collector_工厂，
                          knocompression，compressionOptions（），
                          nullptr/*压缩\u dict*/,

                          /*SE/*跳过\过滤器*/，列\族\名称，-1），
      TablePropertiesCollectorFactory:：Context:：KunknownColumnFamily，
      文件_writer.get（））；

  for（char c='a'；c<='z'；++c）
    std：：字符串键（8，c）；
    std：：字符串值=键；
    internalkey ik（key，0，ktypeValue）；

    builder->add（ik.encode（），value）；
  }
  断言_OK（builder->finish（））；
  文件_writer->flush（）；

  测试：：randomrwstringsink ss_rw（sink）；
  uint32版本；
  uint64全球编号；
  uint64_t全局_seqno_偏移；

  //获取版本的helper函数，全局\seqno，全局\seqno \u offset
  std:：function<void（）>getVersionAndGlobalseqno=[&]（）
    唯一的读卡器
        测试：：GetRandomAccessFileReader（
            新测试：：StringSource（ss_rw.contents（），73342，true））；

    tableproperties*props=nullptr；
    断言_OK（readTableProperties（file_reader.get（），ss_rw.contents（）.size（），
                                  KblockBasedTablemagicNumber、IOptions和
                                  道具）；

    userCollectedProperties user_props=props->user_Collected_properties；
    版本=解码固定32（
        user_props[externalsstfilepropertynames:：kversion].c_str（））；
    global_seqno=decodefixed64（
        user_props[externalstfilepropertyname:：kglobalseqno].c_str（））；
    全局\seqno \u偏移=
        props->properties_offsets[externalstfilepropertyname:：kglobalseqno]；

    删除道具；
  }；

  //更新文件中全局seqno值的helper函数
  std:：function<void（uint64_t）>setglobalseqno=[&]（uint64_t val）
    std：：字符串new_global_seqno；
    putfiexed64（&new_global_seqno，val）；

    断言“OK”（ss_rw.write（global_seqno_offset，new_global_seqno））；
  }；

  //获取表InternalIterator内容的helper函数
  唯一的读卡器；
  std:：function<internaliterator*（）>getTableInternaliter=[&]（）
    唯一的读卡器
        测试：：GetRandomAccessFileReader（
            新测试：：StringSource（ss_rw.contents（），73342，true））；

    选项。Table_Factory->NewTableReader（
        tablereaderOptions（ioOptions，envOptions（），ikc），std:：move（file_reader）
        ss_w.contents（）.size（），&table_reader）；

    返回表_reader->newIterator（readoptions（））；
  }；

  getVersionAndGlobalseqno（）；
  断言eq（2，版本）；
  断言“eq”（0，全局“seqno”）；

  internalIterator*iter=getTableInternalIter（）；
  char电流
  for（iter->seektofirst（）；iter->valid（）；iter->next（））
    帕西丁内尔基派克；
    断言_true（parseInternalKey（iter->key（），&pik））；

    断言eq（pik.type，valuetype:：ktypevalue）；
    断言eq（pik.sequence，0）；
    断言eq（pik.user_key，iter->value（））；
    断言_eq（pik.user_key.toString（），std:：string（8，current_c））；
    Currnt+C++；
  }
  断言_eq（当前_c，'z'+1）；
  删除ITER；

  //将全局序列号更新为10
  setglobalseqno（10个）；
  getVersionAndGlobalseqno（）；
  断言eq（2，版本）；
  断言eq（10，Global_Seqno）；

  iter=getTableInternaliter（）；
  电流
  for（iter->seektofirst（）；iter->valid（）；iter->next（））
    帕西丁内尔基派克；
    断言_true（parseInternalKey（iter->key（），&pik））；

    断言eq（pik.type，valuetype:：ktypevalue）；
    断言eq（pik.sequence，10）；
    断言eq（pik.user_key，iter->value（））；
    断言_eq（pik.user_key.toString（），std:：string（8，current_c））；
    Currnt+C++；
  }
  断言_eq（当前_c，'z'+1）；

  /查证求索
  对于（chc＝‘a’；c<＝z’；c++）{
    std:：string k=std:：string（8，c）；
    内键ik（k，10，kvaruetypeforseek）；
    iter->seek（ik.encode（））；
    断言“真”（iter->valid（））；

    帕西丁内尔基派克；
    断言_true（parseInternalKey（iter->key（），&pik））；

    断言eq（pik.type，valuetype:：ktypevalue）；
    断言eq（pik.sequence，10）；
    断言_Eq（pik.user_key.toString（），k）；
    断言eq（iter->value（）.toString（），k）；
  }
  删除ITER；

  //将全局序列号更新为3
  设置全局序号（3）；
  getVersionAndGlobalseqno（）；
  断言eq（2，版本）；
  断言eq（3，Global_Seqno）；

  iter=getTableInternaliter（）；
  电流
  for（iter->seektofirst（）；iter->valid（）；iter->next（））
    帕西丁内尔基派克；
    断言_true（parseInternalKey（iter->key（），&pik））；

    断言eq（pik.type，valuetype:：ktypevalue）；
    断言eq（pik.sequence，3）；
    断言eq（pik.user_key，iter->value（））；
    断言_eq（pik.user_key.toString（），std:：string（8，current_c））；
    Currnt+C++；
  }
  断言_eq（当前_c，'z'+1）；

  /查证求索
  对于（chc＝‘a’；c<＝z’；c++）{
    std:：string k=std:：string（8，c）；
    //seqno=4小于3，因此我们仍应获取密钥
    内键ik（k，4，kvaluetypeforseek）；
    iter->seek（ik.encode（））；
    断言“真”（iter->valid（））；

    帕西丁内尔基派克；
    断言_true（parseInternalKey（iter->key（），&pik））；

    断言eq（pik.type，valuetype:：ktypevalue）；
    断言eq（pik.sequence，3）；
    断言_Eq（pik.user_key.toString（），k）；
    断言eq（iter->value（）.toString（），k）；
  }

  删除ITER；
}

//命名空间rocksdb

int main（int argc，char**argv）
  ：：测试：：initgoogletest（&argc，argv）；
  返回run_all_tests（）；
}
