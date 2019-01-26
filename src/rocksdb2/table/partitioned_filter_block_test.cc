
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

#include <map>

#include "rocksdb/filter_policy.h"

#include "table/full_filter_bits_builder.h"
#include "table/index_builder.h"
#include "table/partitioned_filter_block.h"
#include "util/coding.h"
#include "util/hash.h"
#include "util/logging.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace rocksdb {

std::map<uint64_t, Slice> slices;

class MockedBlockBasedTable : public BlockBasedTable {
 public:
  explicit MockedBlockBasedTable(Rep* rep) : BlockBasedTable(rep) {
//初始化open通常做的测试所需的一切。
    rep->cache_key_prefix_size = 10;
  }

  virtual CachableEntry<FilterBlockReader> GetFilter(
      FilePrefetchBuffer*, const BlockHandle& filter_blk_handle,
      /*st bool/*unused*/，bool/*unused*/）const override
    slice slice=切片[filter_blk_handle.offset（）]；
    auto obj=新的fullfilterblockreader（
        nullptr、true、blockcontent（slice、false、knocompression）
        rep_u->table_options.filter_policy->getfilterbitsreader（slice），nullptr）；
    返回obj，nullptr
  }
}；

类PartitionedFilterBlockTest:公共测试：：测试
 公众：
  BlockBasedTableOptions表“选项”
  InternalKeyComparator ICOMP=InternalKeyComparator（BytewIsComparator（））；

  PartitionedFilterBlockTest（）
    表“选项”筛选器策略重置（newbloomfilterpolicy（10，false））；
    table_options_u.no_block_cache=true；//否则blockbasedtable:：close
                                           //将访问不是
                                           //在模拟版本中初始化
  }

  std:：shared_ptr<cache>cache_uu；
  ~分区筛选器块测试（）

  const std：：字符串键[4]=“afoo”，“bar”，“box”，“hello”
  const std：：字符串缺少_键[2]=“缺少”，“其他”

  uint64_t maxindexize（）
    int num_keys=sizeof（keys）/sizeof（*keys）；
    uint64_t max_key_size=0；
    对于（int i=1；i<num_keys；i++）
      max_key_size=std:：max（max_key_size，static_cast<uint64_t>（key[i].size（））；
    }
    uint64_t max_index_size=num_keys*（max_key_size+8/*handl*/);

    return max_index_size;
  }

  uint64_t MaxFilterSize() {
    uint32_t dont_care1, dont_care2;
    int num_keys = sizeof(keys) / sizeof(*keys);
    auto filter_bits_reader = dynamic_cast<rocksdb::FullFilterBitsBuilder*>(
        table_options_.filter_policy->GetFilterBitsBuilder());
    assert(filter_bits_reader);
    auto partition_size =
        filter_bits_reader->CalculateSpace(num_keys, &dont_care1, &dont_care2);
    delete filter_bits_reader;
    return partition_size + table_options_.block_size_deviation;
  }

  int last_offset = 10;
  BlockHandle Write(const Slice& slice) {
    BlockHandle bh(last_offset + 1, slice.size());
    slices[bh.offset()] = slice;
    last_offset += bh.size();
    return bh;
  }

  PartitionedIndexBuilder* NewIndexBuilder() {
    return PartitionedIndexBuilder::CreateIndexBuilder(&icomp, table_options_);
  }

  PartitionedFilterBlockBuilder* NewBuilder(
      PartitionedIndexBuilder* const p_index_builder) {
    assert(table_options_.block_size_deviation <= 100);
    auto partition_size = static_cast<uint32_t>(
        table_options_.metadata_block_size *
        ( 100 - table_options_.block_size_deviation));
    partition_size = std::max(partition_size, static_cast<uint32_t>(1));
    return new PartitionedFilterBlockBuilder(
        nullptr, table_options_.whole_key_filtering,
        table_options_.filter_policy->GetFilterBitsBuilder(),
        table_options_.index_block_restart_interval, p_index_builder,
        partition_size);
  }

  std::unique_ptr<MockedBlockBasedTable> table;

  PartitionedFilterBlockReader* NewReader(
      PartitionedFilterBlockBuilder* builder) {
    BlockHandle bh;
    Status status;
    Slice slice;
    do {
      slice = builder->Finish(bh, &status);
      bh = Write(slice);
    } while (status.IsIncomplete());
    const Options options;
    const ImmutableCFOptions ioptions(options);
    const EnvOptions env_options;
    table.reset(new MockedBlockBasedTable(new BlockBasedTable::Rep(
        ioptions, env_options, table_options_, icomp, false)));
    auto reader = new PartitionedFilterBlockReader(
        nullptr, true, BlockContents(slice, false, kNoCompression), nullptr,
        nullptr, *icomp.user_comparator(), table.get());
    return reader;
  }

  void VerifyReader(PartitionedFilterBlockBuilder* builder,
                    bool empty = false) {
    std::unique_ptr<PartitionedFilterBlockReader> reader(NewReader(builder));
//查询添加的键
    const bool no_io = true;
    for (auto key : keys) {
      auto ikey = InternalKey(key, 0, ValueType::kTypeValue);
      const Slice ikey_slice = Slice(*ikey.rep());
      ASSERT_TRUE(reader->KeyMayMatch(key, kNotValid, !no_io, &ikey_slice));
    }
    {
//查询键两次
      auto ikey = InternalKey(keys[0], 0, ValueType::kTypeValue);
      const Slice ikey_slice = Slice(*ikey.rep());
      ASSERT_TRUE(reader->KeyMayMatch(keys[0], kNotValid, !no_io, &ikey_slice));
    }
//查询缺少的键
    for (auto key : missing_keys) {
      auto ikey = InternalKey(key, 0, ValueType::kTypeValue);
      const Slice ikey_slice = Slice(*ikey.rep());
      if (empty) {
        ASSERT_TRUE(reader->KeyMayMatch(key, kNotValid, !no_io, &ikey_slice));
      } else {
//假设一个好的哈希函数
        ASSERT_FALSE(reader->KeyMayMatch(key, kNotValid, !no_io, &ikey_slice));
      }
    }
  }

  int TestBlockPerKey() {
    std::unique_ptr<PartitionedIndexBuilder> pib(NewIndexBuilder());
    std::unique_ptr<PartitionedFilterBlockBuilder> builder(
        NewBuilder(pib.get()));
    int i = 0;
    builder->Add(keys[i]);
    CutABlock(pib.get(), keys[i], keys[i + 1]);
    i++;
    builder->Add(keys[i]);
    CutABlock(pib.get(), keys[i], keys[i + 1]);
    i++;
    builder->Add(keys[i]);
    builder->Add(keys[i]);
    CutABlock(pib.get(), keys[i], keys[i + 1]);
    i++;
    builder->Add(keys[i]);
    CutABlock(pib.get(), keys[i]);

    VerifyReader(builder.get());
    return CountNumOfIndexPartitions(pib.get());
  }

  void TestBlockPerTwoKeys() {
    std::unique_ptr<PartitionedIndexBuilder> pib(NewIndexBuilder());
    std::unique_ptr<PartitionedFilterBlockBuilder> builder(
        NewBuilder(pib.get()));
    int i = 0;
    builder->Add(keys[i]);
    i++;
    builder->Add(keys[i]);
    CutABlock(pib.get(), keys[i], keys[i + 1]);
    i++;
    builder->Add(keys[i]);
    builder->Add(keys[i]);
    i++;
    builder->Add(keys[i]);
    CutABlock(pib.get(), keys[i]);

    VerifyReader(builder.get());
  }

  void TestBlockPerAllKeys() {
    std::unique_ptr<PartitionedIndexBuilder> pib(NewIndexBuilder());
    std::unique_ptr<PartitionedFilterBlockBuilder> builder(
        NewBuilder(pib.get()));
    int i = 0;
    builder->Add(keys[i]);
    i++;
    builder->Add(keys[i]);
    i++;
    builder->Add(keys[i]);
    builder->Add(keys[i]);
    i++;
    builder->Add(keys[i]);
    CutABlock(pib.get(), keys[i]);

    VerifyReader(builder.get());
  }

  void CutABlock(PartitionedIndexBuilder* builder,
                 const std::string& user_key) {
//假设一个块被剪切，向索引中添加一个条目
    std::string key =
        std::string(*InternalKey(user_key, 0, ValueType::kTypeValue).rep());
    BlockHandle dont_care_block_handle(1, 1);
    builder->AddIndexEntry(&key, nullptr, dont_care_block_handle);
  }

  void CutABlock(PartitionedIndexBuilder* builder, const std::string& user_key,
                 const std::string& next_user_key) {
//假设一个块被剪切，向索引中添加一个条目
    std::string key =
        std::string(*InternalKey(user_key, 0, ValueType::kTypeValue).rep());
    std::string next_key = std::string(
        *InternalKey(next_user_key, 0, ValueType::kTypeValue).rep());
    BlockHandle dont_care_block_handle(1, 1);
    Slice slice = Slice(next_key.data(), next_key.size());
    builder->AddIndexEntry(&key, &slice, dont_care_block_handle);
  }

  int CountNumOfIndexPartitions(PartitionedIndexBuilder* builder) {
    IndexBuilder::IndexBlocks dont_care_ib;
    BlockHandle dont_care_bh(10, 10);
    Status s;
    int cnt = 0;
    do {
      s = builder->Finish(&dont_care_ib, dont_care_bh);
      cnt++;
    } while (s.IsIncomplete());
return cnt - 1;  //1为二级指标
  }
};

TEST_F(PartitionedFilterBlockTest, EmptyBuilder) {
  std::unique_ptr<PartitionedIndexBuilder> pib(NewIndexBuilder());
  std::unique_ptr<PartitionedFilterBlockBuilder> builder(NewBuilder(pib.get()));
  const bool empty = true;
  VerifyReader(builder.get(), empty);
}

TEST_F(PartitionedFilterBlockTest, OneBlock) {
  uint64_t max_index_size = MaxIndexSize();
  for (uint64_t i = 1; i < max_index_size + 1; i++) {
    table_options_.metadata_block_size = i;
    TestBlockPerAllKeys();
  }
}

TEST_F(PartitionedFilterBlockTest, TwoBlocksPerKey) {
  uint64_t max_index_size = MaxIndexSize();
  for (uint64_t i = 1; i < max_index_size + 1; i++) {
    table_options_.metadata_block_size = i;
    TestBlockPerTwoKeys();
  }
}

TEST_F(PartitionedFilterBlockTest, OneBlockPerKey) {
  uint64_t max_index_size = MaxIndexSize();
  for (uint64_t i = 1; i < max_index_size + 1; i++) {
    table_options_.metadata_block_size = i;
    TestBlockPerKey();
  }
}

TEST_F(PartitionedFilterBlockTest, PartitionCount) {
  int num_keys = sizeof(keys) / sizeof(*keys);
  table_options_.metadata_block_size =
      std::max(MaxIndexSize(), MaxFilterSize());
  int partitions = TestBlockPerKey();
  ASSERT_EQ(partitions, 1);
//低数字确保在每个键后切割一个块
  table_options_.metadata_block_size = 1;
  partitions = TestBlockPerKey();
  /*ert_eq（分区，num_keys-1/*最后两个键使一个刷新*/）；
}

//命名空间rocksdb

int main（int argc，char**argv）
  ：：测试：：initgoogletest（&argc，argv）；
  返回run_all_tests（）；
}
