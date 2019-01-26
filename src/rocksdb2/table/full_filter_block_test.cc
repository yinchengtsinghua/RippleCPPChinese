
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

#include "table/full_filter_block.h"

#include "rocksdb/filter_policy.h"
#include "util/coding.h"
#include "util/hash.h"
#include "util/string_util.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace rocksdb {

class TestFilterBitsBuilder : public FilterBitsBuilder {
 public:
  explicit TestFilterBitsBuilder() {}

//添加筛选键
  virtual void AddKey(const Slice& key) override {
    hash_entries_.push_back(Hash(key.data(), key.size(), 1));
  }

//使用添加的键生成筛选器
  virtual Slice Finish(std::unique_ptr<const char[]>* buf) override {
    uint32_t len = static_cast<uint32_t>(hash_entries_.size()) * 4;
    char* data = new char[len];
    for (size_t i = 0; i < hash_entries_.size(); i++) {
      EncodeFixed32(data + i * 4, hash_entries_[i]);
    }
    const char* const_data = data;
    buf->reset(const_data);
    return Slice(data, len);
  }

 private:
  std::vector<uint32_t> hash_entries_;
};

class TestFilterBitsReader : public FilterBitsReader {
 public:
  explicit TestFilterBitsReader(const Slice& contents)
      : data_(contents.data()), len_(static_cast<uint32_t>(contents.size())) {}

  virtual bool MayMatch(const Slice& entry) override {
    uint32_t h = Hash(entry.data(), entry.size(), 1);
    for (size_t i = 0; i + 4 <= len_; i += 4) {
      if (h == DecodeFixed32(data_ + i)) {
        return true;
      }
    }
    return false;
  }

 private:
  const char* data_;
  uint32_t len_;
};


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

  virtual FilterBitsBuilder* GetFilterBitsBuilder() const override {
    return new TestFilterBitsBuilder();
  }

  virtual FilterBitsReader* GetFilterBitsReader(const Slice& contents)
      const override {
    return new TestFilterBitsReader(contents);
  }
};

class PluginFullFilterBlockTest : public testing::Test {
 public:
  BlockBasedTableOptions table_options_;

  PluginFullFilterBlockTest() {
    table_options_.filter_policy.reset(new TestHashFilter());
  }
};

TEST_F(PluginFullFilterBlockTest, PluginEmptyBuilder) {
  FullFilterBlockBuilder builder(
      nullptr, true, table_options_.filter_policy->GetFilterBitsBuilder());
  Slice block = builder.Finish();
  ASSERT_EQ("", EscapeString(block));

  FullFilterBlockReader reader(
      nullptr, true, block,
      table_options_.filter_policy->GetFilterBitsReader(block), nullptr);
//与基于块的过滤器保持相同的符号
  ASSERT_TRUE(reader.KeyMayMatch("foo"));
}

TEST_F(PluginFullFilterBlockTest, PluginSingleChunk) {
  FullFilterBlockBuilder builder(
      nullptr, true, table_options_.filter_policy->GetFilterBitsBuilder());
  builder.Add("foo");
  builder.Add("bar");
  builder.Add("box");
  builder.Add("box");
  builder.Add("hello");
  Slice block = builder.Finish();
  FullFilterBlockReader reader(
      nullptr, true, block,
      table_options_.filter_policy->GetFilterBitsReader(block), nullptr);
  ASSERT_TRUE(reader.KeyMayMatch("foo"));
  ASSERT_TRUE(reader.KeyMayMatch("bar"));
  ASSERT_TRUE(reader.KeyMayMatch("box"));
  ASSERT_TRUE(reader.KeyMayMatch("hello"));
  ASSERT_TRUE(reader.KeyMayMatch("foo"));
  ASSERT_TRUE(!reader.KeyMayMatch("missing"));
  ASSERT_TRUE(!reader.KeyMayMatch("other"));
}

class FullFilterBlockTest : public testing::Test {
 public:
  BlockBasedTableOptions table_options_;

  FullFilterBlockTest() {
    table_options_.filter_policy.reset(NewBloomFilterPolicy(10, false));
  }

  ~FullFilterBlockTest() {}
};

TEST_F(FullFilterBlockTest, EmptyBuilder) {
  FullFilterBlockBuilder builder(
      nullptr, true, table_options_.filter_policy->GetFilterBitsBuilder());
  Slice block = builder.Finish();
  ASSERT_EQ("", EscapeString(block));

  FullFilterBlockReader reader(
      nullptr, true, block,
      table_options_.filter_policy->GetFilterBitsReader(block), nullptr);
//与基于块的过滤器保持相同的符号
  ASSERT_TRUE(reader.KeyMayMatch("foo"));
}

TEST_F(FullFilterBlockTest, SingleChunk) {
  FullFilterBlockBuilder builder(
      nullptr, true, table_options_.filter_policy->GetFilterBitsBuilder());
  builder.Add("foo");
  builder.Add("bar");
  builder.Add("box");
  builder.Add("box");
  builder.Add("hello");
  Slice block = builder.Finish();
  FullFilterBlockReader reader(
      nullptr, true, block,
      table_options_.filter_policy->GetFilterBitsReader(block), nullptr);
  ASSERT_TRUE(reader.KeyMayMatch("foo"));
  ASSERT_TRUE(reader.KeyMayMatch("bar"));
  ASSERT_TRUE(reader.KeyMayMatch("box"));
  ASSERT_TRUE(reader.KeyMayMatch("hello"));
  ASSERT_TRUE(reader.KeyMayMatch("foo"));
  ASSERT_TRUE(!reader.KeyMayMatch("missing"));
  ASSERT_TRUE(!reader.KeyMayMatch("other"));
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
