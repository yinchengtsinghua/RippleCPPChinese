
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
#include <stdio.h>
#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "db/dbformat.h"
#include "db/write_batch_internal.h"
#include "db/memtable.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/table.h"
#include "rocksdb/slice_transform.h"
#include "table/block.h"
#include "table/block_builder.h"
#include "table/format.h"
#include "util/random.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace rocksdb {

static std::string RandomString(Random* rnd, int len) {
  std::string r;
  test::RandomString(rnd, len, &r);
  return r;
}
std::string GenerateKey(int primary_key, int secondary_key, int padding_size,
                        Random *rnd) {
  char buf[50];
  char *p = &buf[0];
  snprintf(buf, sizeof(buf), "%6d%4d", primary_key, secondary_key);
  std::string k(p);
  if (padding_size) {
    k += RandomString(rnd, padding_size);
  }

  return k;
}

//生成随机键值对。
//将对生成的密钥进行排序。您可以将参数调到生成的
//针对不同场景的不同类型的测试键/值对。
void GenerateRandomKVs(std::vector<std::string> *keys,
                       std::vector<std::string> *values, const int from,
                       const int len, const int step = 1,
                       const int padding_size = 0,
                       const int keys_share_prefix = 1) {
  Random rnd(302);

//生成不同的前缀
  for (int i = from; i < from + len; i += step) {
//生成共享前缀的密钥
    for (int j = 0; j < keys_share_prefix; ++j) {
      keys->emplace_back(GenerateKey(i, j, padding_size, &rnd));

//100字节值
      values->emplace_back(RandomString(&rnd, 100));
    }
  }
}

class BlockTest : public testing::Test {};

//分块试验
TEST_F(BlockTest, SimpleTest) {
  Random rnd(301);
  Options options = Options();
  std::unique_ptr<InternalKeyComparator> ic;
  ic.reset(new test::PlainInternalKeyComparator(options.comparator));

  std::vector<std::string> keys;
  std::vector<std::string> values;
  BlockBuilder builder(16);
  int num_records = 100000;

  GenerateRandomKVs(&keys, &values, 0, num_records);
//将一组记录添加到一个块中
  for (int i = 0; i < num_records; i++) {
    builder.Add(keys[i], values[i]);
  }

//读取块的序列化内容
  Slice rawblock = builder.Finish();

//创建块读取器
  BlockContents contents;
  contents.data = rawblock;
  contents.cachable = false;
  Block reader(std::move(contents), kDisableGlobalSequenceNumber);

//按顺序读取块的内容
  int count = 0;
  InternalIterator *iter = reader.NewIterator(options.comparator);
  for (iter->SeekToFirst();iter->Valid(); count++, iter->Next()) {

//从块读取kv
    Slice k = iter->key();
    Slice v = iter->value();

//与lookaside数组比较
    ASSERT_EQ(k.ToString().compare(keys[count]), 0);
    ASSERT_EQ(v.ToString().compare(values[count]), 0);
  }
  delete iter;

//随机读取块内容
  iter = reader.NewIterator(options.comparator);
  for (int i = 0; i < num_records; i++) {

//在lookaside数组中查找随机键
    int index = rnd.Uniform(num_records);
    Slice k(keys[index]);

//在块中搜索此密钥
    iter->Seek(k);
    ASSERT_TRUE(iter->Valid());
    Slice v = iter->value();
    ASSERT_EQ(v.ToString().compare(values[index]), 0);
  }
  delete iter;
}

//返回块内容
BlockContents GetBlockContents(std::unique_ptr<BlockBuilder> *builder,
                               const std::vector<std::string> &keys,
                               const std::vector<std::string> &values,
                               const int prefix_group_size = 1) {
  /*lder->reset（新建blockbuilder（1/*重启间隔*/）；

  //只添加一半的键
  对于（size_t i=0；i<keys.size（）；++i）
    （*生成器）->添加（键[i]，值[i]）；
  }
  slice rawblock=（*builder）->finish（）；

  块内容物内容；
  contents.data=rawblock；
  contents.cachable=false；

  退货内容；
}

void checkblockcontents（blockcontents contents，const int max_key，
                        const std:：vector<std:：string>和keys，
                        const std:：vector<std:：string>和values）
  常量大小前缀大小=6；
  //创建块读取器
  blockContents内容参考（contents.data，contents.cachable，
                             目录.压缩类型）；
  块读卡器1（std:：move（contents），kdisableglobalsequencenumber）；
  块读卡器2（std：：move（contents_-ref），kdisableglobalsequencenumber）；

  std:：unique_ptr<const slicetransform>前缀提取程序（
      NewFixedPrefixTransform（前缀_-size））；

  std：：unique_ptr<internalIterator>regular_iter（
      reader2.newIterator（bytewiseComparator（））；

  //查找现有键
  对于（size_t i=0；i<keys.size（）；i++）
    常规搜索（键[i]）；
    断言_OK（正则_Iter->status（））；
    断言_true（正则_iter->valid（））；

    slice v=正则寄存器->value（）；
    断言_Eq（v.ToString（）。比较（值[i]），0）；
  }

  //查找不存在的键。
  //对于哈希索引，如果找不到具有给定前缀的键，迭代器将
  //只需设置为无效；而基于二进制搜索的迭代器将
  //返回最近的一个。
  对于（int i=1；i<max_key-1；i+=2）
    自动键=生成键（i，0，0，nullptr）；
    常规搜索（键）；
    断言_true（正则_iter->valid（））；
  }
}

//在这个测试用例中，没有两个键共享相同的前缀。
test_f（blocktest，simpleindexhash）
  const int kmaxkey=100000；
  std:：vector<std:：string>keys；
  std:：vector<std:：string>值；
  GenerateRandomkvs（&keys，&values，0/*第一个键ID）*/,

                    /*xkey/*最后一个键id*/，2/*步骤*/，
                    8/*填充大小（8字节随机生成后缀）*/);


  std::unique_ptr<BlockBuilder> builder;
  auto contents = GetBlockContents(&builder, keys, values);

  CheckBlockContents(std::move(contents), kMaxKey, keys, values);
}

TEST_F(BlockTest, IndexHashWithSharedPrefix) {
  const int kMaxKey = 100000;
//对于每个前缀，将有5个键以其开头。
  const int kPrefixGroup = 5;
  std::vector<std::string> keys;
  std::vector<std::string> values;
//生成具有相同前缀的键。
GenerateRandomKVs(&keys, &values, 0,  //第一密钥ID
kMaxKey,            //最后密钥ID
2,                  //步
10,                 //填充尺寸，
                    kPrefixGroup);

  std::unique_ptr<BlockBuilder> builder;
  auto contents = GetBlockContents(&builder, keys, values, kPrefixGroup);

  CheckBlockContents(std::move(contents), kMaxKey, keys, values);
}

//blockreadampbitmap的一个缓慢而准确的版本，它只存储
//集合中所有标记的范围。
class BlockReadAmpBitmapSlowAndAccurate {
 public:
  void Mark(size_t start_offset, size_t end_offset) {
    assert(end_offset >= start_offset);
    marked_ranges_.emplace(end_offset, start_offset);
  }

//如果标记了此范围内的任何字节，则返回true
  bool IsPinMarked(size_t offset) {
    auto it = marked_ranges_.lower_bound(
        std::make_pair(offset, static_cast<size_t>(0)));
    if (it == marked_ranges_.end()) {
      return false;
    }
    return offset <= it->first && offset >= it->second;
  }

 private:
  std::set<std::pair<size_t, size_t>> marked_ranges_;
};

TEST_F(BlockTest, BlockReadAmpBitmap) {
  uint32_t pin_offset = 0;
  SyncPoint::GetInstance()->SetCallBack(
    "BlockReadAmpBitmap:rnd", [&pin_offset](void* arg) {
      pin_offset = *(static_cast<uint32_t*>(arg));
    });
  SyncPoint::GetInstance()->EnableProcessing();
  std::vector<size_t> block_sizes = {
1,                 //1字节
32,                //32字节
61,                //61字节
64,                //64字节
512,               //0.5千字节
1024,              //1千字节
1024 * 4,          //4千字节
1024 * 10,         //10千字节
1024 * 50,         //50千字节
1024 * 1024,       //1兆字节
1024 * 1024 * 4,   //4兆字节
1024 * 1024 * 50,  //10兆字节
      777,
      124653,
  };
  const size_t kBytesPerBit = 64;

  Random rnd(301);
  for (size_t block_size : block_sizes) {
    std::shared_ptr<Statistics> stats = rocksdb::CreateDBStatistics();
    BlockReadAmpBitmap read_amp_bitmap(block_size, kBytesPerBit, stats.get());
    BlockReadAmpBitmapSlowAndAccurate read_amp_slow_and_accurate;

    size_t needed_bits = (block_size / kBytesPerBit);
    if (block_size % kBytesPerBit != 0) {
      needed_bits++;
    }
    size_t bitmap_size = needed_bits / 32;
    if (needed_bits % 32 != 0) {
      bitmap_size++;
    }

    ASSERT_EQ(stats->getTickerCount(READ_AMP_TOTAL_READ_BYTES), block_size);

//生成一些随机条目
    std::vector<size_t> random_entry_offsets;
    for (int i = 0; i < 1000; i++) {
      random_entry_offsets.push_back(rnd.Next() % block_size);
    }
    std::sort(random_entry_offsets.begin(), random_entry_offsets.end());
    auto it =
        std::unique(random_entry_offsets.begin(), random_entry_offsets.end());
    random_entry_offsets.resize(
        std::distance(random_entry_offsets.begin(), it));

    std::vector<std::pair<size_t, size_t>> random_entries;
    for (size_t i = 0; i < random_entry_offsets.size(); i++) {
      size_t entry_start = random_entry_offsets[i];
      size_t entry_end;
      if (i + 1 < random_entry_offsets.size()) {
        entry_end = random_entry_offsets[i + 1] - 1;
      } else {
        entry_end = block_size - 1;
      }
      random_entries.emplace_back(entry_start, entry_end);
    }

    for (size_t i = 0; i < random_entries.size(); i++) {
      auto &current_entry = random_entries[rnd.Next() % random_entries.size()];

      read_amp_bitmap.Mark(static_cast<uint32_t>(current_entry.first),
                           static_cast<uint32_t>(current_entry.second));
      read_amp_slow_and_accurate.Mark(current_entry.first,
                                      current_entry.second);

      size_t total_bits = 0;
      for (size_t bit_idx = 0; bit_idx < needed_bits; bit_idx++) {
        total_bits += read_amp_slow_and_accurate.IsPinMarked(
          bit_idx * kBytesPerBit + pin_offset);
      }
      size_t expected_estimate_useful = total_bits * kBytesPerBit;
      size_t got_estimate_useful =
        stats->getTickerCount(READ_AMP_ESTIMATE_USEFUL_BYTES);
      ASSERT_EQ(expected_estimate_useful, got_estimate_useful);
    }
  }
  SyncPoint::GetInstance()->DisableProcessing();
  SyncPoint::GetInstance()->ClearAllCallBacks();
}

TEST_F(BlockTest, BlockWithReadAmpBitmap) {
  Random rnd(301);
  Options options = Options();
  std::unique_ptr<InternalKeyComparator> ic;
  ic.reset(new test::PlainInternalKeyComparator(options.comparator));

  std::vector<std::string> keys;
  std::vector<std::string> values;
  BlockBuilder builder(16);
  int num_records = 10000;

  GenerateRandomKVs(&keys, &values, 0, num_records, 1);
//将一组记录添加到一个块中
  for (int i = 0; i < num_records; i++) {
    builder.Add(keys[i], values[i]);
  }

  Slice rawblock = builder.Finish();
  const size_t kBytesPerBit = 8;

//使用next（）按顺序读取块
  {
    std::shared_ptr<Statistics> stats = rocksdb::CreateDBStatistics();

//创建块读取器
    BlockContents contents;
    contents.data = rawblock;
    contents.cachable = true;
    Block reader(std::move(contents), kDisableGlobalSequenceNumber,
                 kBytesPerBit, stats.get());

//按顺序读取块的内容
    size_t read_bytes = 0;
    BlockIter *iter = static_cast<BlockIter *>(
        reader.NewIterator(options.comparator, nullptr, true, stats.get()));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
      iter->value();
      read_bytes += iter->TEST_CurrentEntrySize();

      double semi_acc_read_amp =
          static_cast<double>(read_bytes) / rawblock.size();
      double read_amp = static_cast<double>(stats->getTickerCount(
                            READ_AMP_ESTIMATE_USEFUL_BYTES)) /
                        stats->getTickerCount(READ_AMP_TOTAL_READ_BYTES);

//如果我们正在读取，则读取放大错误将小于1%。
//顺序地
      double error_pct = fabs(semi_acc_read_amp - read_amp) * 100;
      EXPECT_LT(error_pct, 1);
    }

    delete iter;
  }

//使用seek（）按顺序读取块
  {
    std::shared_ptr<Statistics> stats = rocksdb::CreateDBStatistics();

//创建块读取器
    BlockContents contents;
    contents.data = rawblock;
    contents.cachable = true;
    Block reader(std::move(contents), kDisableGlobalSequenceNumber,
                 kBytesPerBit, stats.get());

    size_t read_bytes = 0;
    BlockIter *iter = static_cast<BlockIter *>(
        reader.NewIterator(options.comparator, nullptr, true, stats.get()));
    for (int i = 0; i < num_records; i++) {
      Slice k(keys[i]);

//在块中搜索此密钥
      iter->Seek(k);
      iter->value();
      read_bytes += iter->TEST_CurrentEntrySize();

      double semi_acc_read_amp =
          static_cast<double>(read_bytes) / rawblock.size();
      double read_amp = static_cast<double>(stats->getTickerCount(
                            READ_AMP_ESTIMATE_USEFUL_BYTES)) /
                        stats->getTickerCount(READ_AMP_TOTAL_READ_BYTES);

//如果我们正在读取，则读取放大错误将小于1%。
//顺序地
      double error_pct = fabs(semi_acc_read_amp - read_amp) * 100;
      EXPECT_LT(error_pct, 1);
    }
    delete iter;
  }

//随机读取块
  {
    std::shared_ptr<Statistics> stats = rocksdb::CreateDBStatistics();

//创建块读取器
    BlockContents contents;
    contents.data = rawblock;
    contents.cachable = true;
    Block reader(std::move(contents), kDisableGlobalSequenceNumber,
                 kBytesPerBit, stats.get());

    size_t read_bytes = 0;
    BlockIter *iter = static_cast<BlockIter *>(
        reader.NewIterator(options.comparator, nullptr, true, stats.get()));
    std::unordered_set<int> read_keys;
    for (int i = 0; i < num_records; i++) {
      int index = rnd.Uniform(num_records);
      Slice k(keys[index]);

      iter->Seek(k);
      iter->value();
      if (read_keys.find(index) == read_keys.end()) {
        read_keys.insert(index);
        read_bytes += iter->TEST_CurrentEntrySize();
      }

      double semi_acc_read_amp =
          static_cast<double>(read_bytes) / rawblock.size();
      double read_amp = static_cast<double>(stats->getTickerCount(
                            READ_AMP_ESTIMATE_USEFUL_BYTES)) /
                        stats->getTickerCount(READ_AMP_TOTAL_READ_BYTES);

      double error_pct = fabs(semi_acc_read_amp - read_amp) * 100;
//如果我们正在读取，则读取放大误差将小于2%。
//随机
      EXPECT_LT(error_pct, 2);
    }
    delete iter;
  }
}

TEST_F(BlockTest, ReadAmpBitmapPow2) {
  std::shared_ptr<Statistics> stats = rocksdb::CreateDBStatistics();
  ASSERT_EQ(BlockReadAmpBitmap(100, 1, stats.get()).GetBytesPerBit(), 1);
  ASSERT_EQ(BlockReadAmpBitmap(100, 2, stats.get()).GetBytesPerBit(), 2);
  ASSERT_EQ(BlockReadAmpBitmap(100, 4, stats.get()).GetBytesPerBit(), 4);
  ASSERT_EQ(BlockReadAmpBitmap(100, 8, stats.get()).GetBytesPerBit(), 8);
  ASSERT_EQ(BlockReadAmpBitmap(100, 16, stats.get()).GetBytesPerBit(), 16);
  ASSERT_EQ(BlockReadAmpBitmap(100, 32, stats.get()).GetBytesPerBit(), 32);

  ASSERT_EQ(BlockReadAmpBitmap(100, 3, stats.get()).GetBytesPerBit(), 2);
  ASSERT_EQ(BlockReadAmpBitmap(100, 7, stats.get()).GetBytesPerBit(), 4);
  ASSERT_EQ(BlockReadAmpBitmap(100, 11, stats.get()).GetBytesPerBit(), 8);
  ASSERT_EQ(BlockReadAmpBitmap(100, 17, stats.get()).GetBytesPerBit(), 16);
  ASSERT_EQ(BlockReadAmpBitmap(100, 33, stats.get()).GetBytesPerBit(), 32);
  ASSERT_EQ(BlockReadAmpBitmap(100, 35, stats.get()).GetBytesPerBit(), 32);
}

}  //命名空间rocksdb

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
