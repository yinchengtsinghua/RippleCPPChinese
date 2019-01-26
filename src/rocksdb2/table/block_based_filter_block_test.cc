
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
//版权所有（c）2012 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#include "table/block_based_filter_block.h"

#include "rocksdb/filter_policy.h"
#include "util/coding.h"
#include "util/hash.h"
#include "util/string_util.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace rocksdb {

//测试：发出一个数组，每个键有一个哈希值
class TestHashFilter : public FilterPolicy {
 public:
  virtual const char* Name() const override { return "TestHashFilter"; }

  virtual void CreateFilter(const Slice* keys, int n,
                            std::string* dst) const override {
    for (int i = 0; i < n; i++) {
      uint32_t h = Hash(keys[i].data(), keys[i].size(), 1);
      PutFixed32(dst, h);
    }
  }

  virtual bool KeyMayMatch(const Slice& key,
                           const Slice& filter) const override {
    uint32_t h = Hash(key.data(), key.size(), 1);
    for (unsigned int i = 0; i + 4 <= filter.size(); i += 4) {
      if (h == DecodeFixed32(filter.data() + i)) {
        return true;
      }
    }
    return false;
  }
};

class FilterBlockTest : public testing::Test {
 public:
  TestHashFilter policy_;
  BlockBasedTableOptions table_options_;

  FilterBlockTest() {
    table_options_.filter_policy.reset(new TestHashFilter());
  }
};

TEST_F(FilterBlockTest, EmptyBuilder) {
  BlockBasedFilterBlockBuilder builder(nullptr, table_options_);
  BlockContents block(builder.Finish(), false, kNoCompression);
  ASSERT_EQ("\\x00\\x00\\x00\\x00\\x0b", EscapeString(block.data));
  BlockBasedFilterBlockReader reader(nullptr, table_options_, true,
                                     std::move(block), nullptr);
  ASSERT_TRUE(reader.KeyMayMatch("foo", 0));
  ASSERT_TRUE(reader.KeyMayMatch("foo", 100000));
}

TEST_F(FilterBlockTest, SingleChunk) {
  BlockBasedFilterBlockBuilder builder(nullptr, table_options_);
  builder.StartBlock(100);
  builder.Add("foo");
  builder.Add("bar");
  builder.Add("box");
  builder.StartBlock(200);
  builder.Add("box");
  builder.StartBlock(300);
  builder.Add("hello");
  BlockContents block(builder.Finish(), false, kNoCompression);
  BlockBasedFilterBlockReader reader(nullptr, table_options_, true,
                                     std::move(block), nullptr);
  ASSERT_TRUE(reader.KeyMayMatch("foo", 100));
  ASSERT_TRUE(reader.KeyMayMatch("bar", 100));
  ASSERT_TRUE(reader.KeyMayMatch("box", 100));
  ASSERT_TRUE(reader.KeyMayMatch("hello", 100));
  ASSERT_TRUE(reader.KeyMayMatch("foo", 100));
  ASSERT_TRUE(!reader.KeyMayMatch("missing", 100));
  ASSERT_TRUE(!reader.KeyMayMatch("other", 100));
}

TEST_F(FilterBlockTest, MultiChunk) {
  BlockBasedFilterBlockBuilder builder(nullptr, table_options_);

//第一滤波器
  builder.StartBlock(0);
  builder.Add("foo");
  builder.StartBlock(2000);
  builder.Add("bar");

//第二滤波器
  builder.StartBlock(3100);
  builder.Add("box");

//第三个筛选器为空

//最后过滤器
  builder.StartBlock(9000);
  builder.Add("box");
  builder.Add("hello");

  BlockContents block(builder.Finish(), false, kNoCompression);
  BlockBasedFilterBlockReader reader(nullptr, table_options_, true,
                                     std::move(block), nullptr);

//检查第一个过滤器
  ASSERT_TRUE(reader.KeyMayMatch("foo", 0));
  ASSERT_TRUE(reader.KeyMayMatch("bar", 2000));
  ASSERT_TRUE(!reader.KeyMayMatch("box", 0));
  ASSERT_TRUE(!reader.KeyMayMatch("hello", 0));

//检查第二个过滤器
  ASSERT_TRUE(reader.KeyMayMatch("box", 3100));
  ASSERT_TRUE(!reader.KeyMayMatch("foo", 3100));
  ASSERT_TRUE(!reader.KeyMayMatch("bar", 3100));
  ASSERT_TRUE(!reader.KeyMayMatch("hello", 3100));

//检查第三个过滤器（空）
  ASSERT_TRUE(!reader.KeyMayMatch("foo", 4100));
  ASSERT_TRUE(!reader.KeyMayMatch("bar", 4100));
  ASSERT_TRUE(!reader.KeyMayMatch("box", 4100));
  ASSERT_TRUE(!reader.KeyMayMatch("hello", 4100));

//检查最后一个筛选器
  ASSERT_TRUE(reader.KeyMayMatch("box", 9000));
  ASSERT_TRUE(reader.KeyMayMatch("hello", 9000));
  ASSERT_TRUE(!reader.KeyMayMatch("foo", 9000));
  ASSERT_TRUE(!reader.KeyMayMatch("bar", 9000));
}

//块基滤块试验
//使用filterpolicy中的新接口创建筛选器生成器/读取器
class BlockBasedFilterBlockTest : public testing::Test {
 public:
  BlockBasedTableOptions table_options_;

  BlockBasedFilterBlockTest() {
    table_options_.filter_policy.reset(NewBloomFilterPolicy(10));
  }

  ~BlockBasedFilterBlockTest() {}
};

TEST_F(BlockBasedFilterBlockTest, BlockBasedEmptyBuilder) {
  FilterBlockBuilder* builder = new BlockBasedFilterBlockBuilder(
      nullptr, table_options_);
  BlockContents block(builder->Finish(), false, kNoCompression);
  ASSERT_EQ("\\x00\\x00\\x00\\x00\\x0b", EscapeString(block.data));
  FilterBlockReader* reader = new BlockBasedFilterBlockReader(
      nullptr, table_options_, true, std::move(block), nullptr);
  ASSERT_TRUE(reader->KeyMayMatch("foo", 0));
  ASSERT_TRUE(reader->KeyMayMatch("foo", 100000));

  delete builder;
  delete reader;
}

TEST_F(BlockBasedFilterBlockTest, BlockBasedSingleChunk) {
  FilterBlockBuilder* builder = new BlockBasedFilterBlockBuilder(
      nullptr, table_options_);
  builder->StartBlock(100);
  builder->Add("foo");
  builder->Add("bar");
  builder->Add("box");
  builder->StartBlock(200);
  builder->Add("box");
  builder->StartBlock(300);
  builder->Add("hello");
  BlockContents block(builder->Finish(), false, kNoCompression);
  FilterBlockReader* reader = new BlockBasedFilterBlockReader(
      nullptr, table_options_, true, std::move(block), nullptr);
  ASSERT_TRUE(reader->KeyMayMatch("foo", 100));
  ASSERT_TRUE(reader->KeyMayMatch("bar", 100));
  ASSERT_TRUE(reader->KeyMayMatch("box", 100));
  ASSERT_TRUE(reader->KeyMayMatch("hello", 100));
  ASSERT_TRUE(reader->KeyMayMatch("foo", 100));
  ASSERT_TRUE(!reader->KeyMayMatch("missing", 100));
  ASSERT_TRUE(!reader->KeyMayMatch("other", 100));

  delete builder;
  delete reader;
}

TEST_F(BlockBasedFilterBlockTest, BlockBasedMultiChunk) {
  FilterBlockBuilder* builder = new BlockBasedFilterBlockBuilder(
      nullptr, table_options_);

//第一滤波器
  builder->StartBlock(0);
  builder->Add("foo");
  builder->StartBlock(2000);
  builder->Add("bar");

//第二滤波器
  builder->StartBlock(3100);
  builder->Add("box");

//第三个筛选器为空

//最后过滤器
  builder->StartBlock(9000);
  builder->Add("box");
  builder->Add("hello");

  BlockContents block(builder->Finish(), false, kNoCompression);
  FilterBlockReader* reader = new BlockBasedFilterBlockReader(
      nullptr, table_options_, true, std::move(block), nullptr);

//检查第一个过滤器
  ASSERT_TRUE(reader->KeyMayMatch("foo", 0));
  ASSERT_TRUE(reader->KeyMayMatch("bar", 2000));
  ASSERT_TRUE(!reader->KeyMayMatch("box", 0));
  ASSERT_TRUE(!reader->KeyMayMatch("hello", 0));

//检查第二个过滤器
  ASSERT_TRUE(reader->KeyMayMatch("box", 3100));
  ASSERT_TRUE(!reader->KeyMayMatch("foo", 3100));
  ASSERT_TRUE(!reader->KeyMayMatch("bar", 3100));
  ASSERT_TRUE(!reader->KeyMayMatch("hello", 3100));

//检查第三个过滤器（空）
  ASSERT_TRUE(!reader->KeyMayMatch("foo", 4100));
  ASSERT_TRUE(!reader->KeyMayMatch("bar", 4100));
  ASSERT_TRUE(!reader->KeyMayMatch("box", 4100));
  ASSERT_TRUE(!reader->KeyMayMatch("hello", 4100));

//检查最后一个筛选器
  ASSERT_TRUE(reader->KeyMayMatch("box", 9000));
  ASSERT_TRUE(reader->KeyMayMatch("hello", 9000));
  ASSERT_TRUE(!reader->KeyMayMatch("foo", 9000));
  ASSERT_TRUE(!reader->KeyMayMatch("bar", 9000));

  delete builder;
  delete reader;
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
