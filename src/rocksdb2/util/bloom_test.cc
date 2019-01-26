
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

#ifndef GFLAGS
#include <cstdio>
int main() {
  fprintf(stderr, "Please install gflags to run this test... Skipping...\n");
  return 0;
}
#else

#include <gflags/gflags.h>
#include <vector>

#include "rocksdb/filter_policy.h"
#include "table/full_filter_bits_builder.h"
#include "util/arena.h"
#include "util/logging.h"
#include "util/testharness.h"
#include "util/testutil.h"

using GFLAGS::ParseCommandLineFlags;

DEFINE_int32(bits_per_key, 10, "");

namespace rocksdb {

static const int kVerbose = 1;

static Slice Key(int i, char* buffer) {
  std::string s;
  PutFixed32(&s, static_cast<uint32_t>(i));
  memcpy(buffer, s.c_str(), sizeof(i));
  return Slice(buffer, sizeof(i));
}

static int NextLength(int length) {
  if (length < 10) {
    length += 1;
  } else if (length < 100) {
    length += 10;
  } else if (length < 1000) {
    length += 100;
  } else {
    length += 1000;
  }
  return length;
}

class BloomTest : public testing::Test {
 private:
  const FilterPolicy* policy_;
  std::string filter_;
  std::vector<std::string> keys_;

 public:
  BloomTest() : policy_(
      NewBloomFilterPolicy(FLAGS_bits_per_key)) {}

  ~BloomTest() {
    delete policy_;
  }

  void Reset() {
    keys_.clear();
    filter_.clear();
  }

  void Add(const Slice& s) {
    keys_.push_back(s.ToString());
  }

  void Build() {
    std::vector<Slice> key_slices;
    for (size_t i = 0; i < keys_.size(); i++) {
      key_slices.push_back(Slice(keys_[i]));
    }
    filter_.clear();
    policy_->CreateFilter(&key_slices[0], static_cast<int>(key_slices.size()),
                          &filter_);
    keys_.clear();
    if (kVerbose >= 2) DumpFilter();
  }

  size_t FilterSize() const {
    return filter_.size();
  }

  void DumpFilter() {
    fprintf(stderr, "F(");
    for (size_t i = 0; i+1 < filter_.size(); i++) {
      const unsigned int c = static_cast<unsigned int>(filter_[i]);
      for (int j = 0; j < 8; j++) {
        fprintf(stderr, "%c", (c & (1 <<j)) ? '1' : '.');
      }
    }
    fprintf(stderr, ")\n");
  }

  bool Matches(const Slice& s) {
    if (!keys_.empty()) {
      Build();
    }
    return policy_->KeyMayMatch(s, filter_);
  }

  double FalsePositiveRate() {
    char buffer[sizeof(int)];
    int result = 0;
    for (int i = 0; i < 10000; i++) {
      if (Matches(Key(i + 1000000000, buffer))) {
        result++;
      }
    }
    return result / 10000.0;
  }
};

TEST_F(BloomTest, EmptyFilter) {
  ASSERT_TRUE(! Matches("hello"));
  ASSERT_TRUE(! Matches("world"));
}

TEST_F(BloomTest, Small) {
  Add("hello");
  Add("world");
  ASSERT_TRUE(Matches("hello"));
  ASSERT_TRUE(Matches("world"));
  ASSERT_TRUE(! Matches("x"));
  ASSERT_TRUE(! Matches("foo"));
}

TEST_F(BloomTest, VaryingLengths) {
  char buffer[sizeof(int)];

//统计明显超过假阳性率的筛选数
  int mediocre_filters = 0;
  int good_filters = 0;

  for (int length = 1; length <= 10000; length = NextLength(length)) {
    Reset();
    for (int i = 0; i < length; i++) {
      Add(Key(i, buffer));
    }
    Build();

    ASSERT_LE(FilterSize(), (size_t)((length * 10 / 8) + 40)) << length;

//所有添加的密钥必须匹配
    for (int i = 0; i < length; i++) {
      ASSERT_TRUE(Matches(Key(i, buffer)))
          << "Length " << length << "; key " << i;
    }

//检查假阳性率
    double rate = FalsePositiveRate();
    if (kVerbose >= 1) {
      fprintf(stderr, "False positives: %5.2f%% @ length = %6d ; bytes = %6d\n",
              rate*100.0, length, static_cast<int>(FilterSize()));
    }
ASSERT_LE(rate, 0.02);   //不得超过2%
if (rate > 0.0125) mediocre_filters++;  //允许，但不太经常
    else good_filters++;
  }
  if (kVerbose >= 1) {
    fprintf(stderr, "Filters: %d good, %d mediocre\n",
            good_filters, mediocre_filters);
  }
  ASSERT_LE(mediocre_filters, good_filters/5);
}

//每字节不同的位

class FullBloomTest : public testing::Test {
 private:
  const FilterPolicy* policy_;
  std::unique_ptr<FilterBitsBuilder> bits_builder_;
  std::unique_ptr<FilterBitsReader> bits_reader_;
  std::unique_ptr<const char[]> buf_;
  size_t filter_size_;

 public:
  FullBloomTest() :
      policy_(NewBloomFilterPolicy(FLAGS_bits_per_key, false)),
      filter_size_(0) {
    Reset();
  }

  ~FullBloomTest() {
    delete policy_;
  }

  FullFilterBitsBuilder* GetFullFilterBitsBuilder() {
    return dynamic_cast<FullFilterBitsBuilder*>(bits_builder_.get());
  }

  void Reset() {
    bits_builder_.reset(policy_->GetFilterBitsBuilder());
    bits_reader_.reset(nullptr);
    buf_.reset(nullptr);
    filter_size_ = 0;
  }

  void Add(const Slice& s) {
    bits_builder_->AddKey(s);
  }

  void Build() {
    Slice filter = bits_builder_->Finish(&buf_);
    bits_reader_.reset(policy_->GetFilterBitsReader(filter));
    filter_size_ = filter.size();
  }

  size_t FilterSize() const {
    return filter_size_;
  }

  bool Matches(const Slice& s) {
    if (bits_reader_ == nullptr) {
      Build();
    }
    return bits_reader_->MayMatch(s);
  }

  double FalsePositiveRate() {
    char buffer[sizeof(int)];
    int result = 0;
    for (int i = 0; i < 10000; i++) {
      if (Matches(Key(i + 1000000000, buffer))) {
        result++;
      }
    }
    return result / 10000.0;
  }
};

TEST_F(FullBloomTest, FilterSize) {
  uint32_t dont_care1, dont_care2;
  auto full_bits_builder = GetFullFilterBitsBuilder();
  for (int n = 1; n < 100; n++) {
    auto space = full_bits_builder->CalculateSpace(n, &dont_care1, &dont_care2);
    auto n2 = full_bits_builder->CalculateNumEntry(space);
    ASSERT_GE(n2, n);
    auto space2 =
        full_bits_builder->CalculateSpace(n2, &dont_care1, &dont_care2);
    ASSERT_EQ(space, space2);
  }
}

TEST_F(FullBloomTest, FullEmptyFilter) {
//在此级别上，空筛选器不匹配
  ASSERT_TRUE(!Matches("hello"));
  ASSERT_TRUE(!Matches("world"));
}

TEST_F(FullBloomTest, FullSmall) {
  Add("hello");
  Add("world");
  ASSERT_TRUE(Matches("hello"));
  ASSERT_TRUE(Matches("world"));
  ASSERT_TRUE(!Matches("x"));
  ASSERT_TRUE(!Matches("foo"));
}

TEST_F(FullBloomTest, FullVaryingLengths) {
  char buffer[sizeof(int)];

//统计明显超过假阳性率的筛选数
  int mediocre_filters = 0;
  int good_filters = 0;

  for (int length = 1; length <= 10000; length = NextLength(length)) {
    Reset();
    for (int i = 0; i < length; i++) {
      Add(Key(i, buffer));
    }
    Build();

    ASSERT_LE(FilterSize(), (size_t)((length * 10 / 8) + 128 + 5)) << length;

//所有添加的密钥必须匹配
    for (int i = 0; i < length; i++) {
      ASSERT_TRUE(Matches(Key(i, buffer)))
          << "Length " << length << "; key " << i;
    }

//检查假阳性率
    double rate = FalsePositiveRate();
    if (kVerbose >= 1) {
      fprintf(stderr, "False positives: %5.2f%% @ length = %6d ; bytes = %6d\n",
              rate*100.0, length, static_cast<int>(FilterSize()));
    }
ASSERT_LE(rate, 0.02);   //不得超过2%
    if (rate > 0.0125)
mediocre_filters++;  //允许，但不太经常
    else
      good_filters++;
  }
  if (kVerbose >= 1) {
    fprintf(stderr, "Filters: %d good, %d mediocre\n",
            good_filters, mediocre_filters);
  }
  ASSERT_LE(mediocre_filters, good_filters/5);
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ParseCommandLineFlags(&argc, &argv, true);

  return RUN_ALL_TESTS();
}

#endif  //GFLAGS
