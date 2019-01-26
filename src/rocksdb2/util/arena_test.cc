
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

#include "util/arena.h"
#include "util/random.h"
#include "util/testharness.h"

namespace rocksdb {

namespace {
const size_t kHugePageSize = 2 * 1024 * 1024;
}  //命名空间
class ArenaTest : public testing::Test {};

TEST_F(ArenaTest, Empty) { Arena arena0; }

namespace {
bool CheckMemoryAllocated(size_t allocated, size_t expected) {
//Arena:：MemoryAllocatedBytes（）返回的值可能大于
//请求的内存。我们选择一个有点武断的上界
//最大预期值=预期值*1.1以检测严重过度分配。
  size_t max_expected = expected + expected / 10;
  return allocated >= expected && allocated <= max_expected;
}

void MemoryAllocatedBytesTest(size_t huge_page_size) {
  const int N = 17;
size_t req_sz;  //请求尺寸
size_t bsz = 32 * 1024;  //块大小
  size_t expected_memory_allocated;

  Arena arena(bsz, nullptr, huge_page_size);

//请求的大小>块的四分之一：
//单独分配请求的大小
  req_sz = 12 * 1024;
  for (int i = 0; i < N; i++) {
    arena.Allocate(req_sz);
  }
  expected_memory_allocated = req_sz * N + Arena::kInlineSize;
  ASSERT_PRED2(CheckMemoryAllocated, arena.MemoryAllocatedBytes(),
               expected_memory_allocated);

  arena.Allocate(Arena::kInlineSize - 1);

//请求的大小小于块的四分之一：
//使用默认大小分配块，然后尝试使用未使用的部分
//街区的因此将为第一个分配一个新块
//分配（99）呼叫。所有剩余的呼叫不会导致新的分配。
  req_sz = 99;
  for (int i = 0; i < N; i++) {
    arena.Allocate(req_sz);
  }
  if (huge_page_size) {
    ASSERT_TRUE(
        CheckMemoryAllocated(arena.MemoryAllocatedBytes(),
                             expected_memory_allocated + bsz) ||
        CheckMemoryAllocated(arena.MemoryAllocatedBytes(),
                             expected_memory_allocated + huge_page_size));
  } else {
    expected_memory_allocated += bsz;
    ASSERT_PRED2(CheckMemoryAllocated, arena.MemoryAllocatedBytes(),
                 expected_memory_allocated);
  }

//请求的大小>块的大小：
//单独分配请求的大小
  expected_memory_allocated = arena.MemoryAllocatedBytes();
  req_sz = 8 * 1024 * 1024;
  for (int i = 0; i < N; i++) {
    arena.Allocate(req_sz);
  }
  expected_memory_allocated += req_sz * N;
  ASSERT_PRED2(CheckMemoryAllocated, arena.MemoryAllocatedBytes(),
               expected_memory_allocated);
}

//确保我们没有计算分配的内存空间，但没有使用
//竞技场：：近似晨曦（）
static void ApproximateMemoryUsageTest(size_t huge_page_size) {
  const size_t kBlockSize = 4096;
  const size_t kEntrySize = kBlockSize / 8;
  const size_t kZero = 0;
  Arena arena(kBlockSize, nullptr, huge_page_size);
  ASSERT_EQ(kZero, arena.ApproximateMemoryUsage());

//分配内联字节
  EXPECT_TRUE(arena.IsInInlineBlock());
  arena.AllocateAligned(8);
  EXPECT_TRUE(arena.IsInInlineBlock());
  arena.AllocateAligned(Arena::kInlineSize / 2 - 16);
  EXPECT_TRUE(arena.IsInInlineBlock());
  arena.AllocateAligned(Arena::kInlineSize / 2);
  EXPECT_TRUE(arena.IsInInlineBlock());
  ASSERT_EQ(arena.ApproximateMemoryUsage(), Arena::kInlineSize - 8);
  ASSERT_PRED2(CheckMemoryAllocated, arena.MemoryAllocatedBytes(),
               Arena::kInlineSize);

  auto num_blocks = kBlockSize / kEntrySize;

//第一次分配
  arena.AllocateAligned(kEntrySize);
  EXPECT_FALSE(arena.IsInInlineBlock());
  auto mem_usage = arena.MemoryAllocatedBytes();
  if (huge_page_size) {
    ASSERT_TRUE(
        CheckMemoryAllocated(mem_usage, kBlockSize + Arena::kInlineSize) ||
        CheckMemoryAllocated(mem_usage, huge_page_size + Arena::kInlineSize));
  } else {
    ASSERT_PRED2(CheckMemoryAllocated, mem_usage,
                 kBlockSize + Arena::kInlineSize);
  }
  auto usage = arena.ApproximateMemoryUsage();
  ASSERT_LT(usage, mem_usage);
  for (size_t i = 1; i < num_blocks; ++i) {
    arena.AllocateAligned(kEntrySize);
    ASSERT_EQ(mem_usage, arena.MemoryAllocatedBytes());
    ASSERT_EQ(arena.ApproximateMemoryUsage(), usage + kEntrySize);
    EXPECT_FALSE(arena.IsInInlineBlock());
    usage = arena.ApproximateMemoryUsage();
  }
  if (huge_page_size) {
    ASSERT_TRUE(usage > mem_usage ||
                usage + huge_page_size - kBlockSize == mem_usage);
  } else {
    ASSERT_GT(usage, mem_usage);
  }
}

static void SimpleTest(size_t huge_page_size) {
  std::vector<std::pair<size_t, char*>> allocated;
  Arena arena(Arena::kMinBlockSize, nullptr, huge_page_size);
  const int N = 100000;
  size_t bytes = 0;
  Random rnd(301);
  for (int i = 0; i < N; i++) {
    size_t s;
    if (i % (N / 10) == 0) {
      s = i;
    } else {
      s = rnd.OneIn(4000)
              ? rnd.Uniform(6000)
              : (rnd.OneIn(10) ? rnd.Uniform(100) : rnd.Uniform(20));
    }
    if (s == 0) {
//我们的竞技场不允许0号的分配。
      s = 1;
    }
    char* r;
    if (rnd.OneIn(10)) {
      r = arena.AllocateAligned(s);
    } else {
      r = arena.Allocate(s);
    }

    for (unsigned int b = 0; b < s; b++) {
//用已知的位模式填充第i个分配
      r[b] = i % 256;
    }
    bytes += s;
    allocated.push_back(std::make_pair(s, r));
    ASSERT_GE(arena.ApproximateMemoryUsage(), bytes);
    if (i > N / 10) {
      ASSERT_LE(arena.ApproximateMemoryUsage(), bytes * 1.10);
    }
  }
  for (unsigned int i = 0; i < allocated.size(); i++) {
    size_t num_bytes = allocated[i].first;
    const char* p = allocated[i].second;
    for (unsigned int b = 0; b < num_bytes; b++) {
//检查已知位模式的第i个分配
      ASSERT_EQ(int(p[b]) & 0xff, (int)(i % 256));
    }
  }
}
}  //命名空间

TEST_F(ArenaTest, MemoryAllocatedBytes) {
  MemoryAllocatedBytesTest(0);
  MemoryAllocatedBytesTest(kHugePageSize);
}

TEST_F(ArenaTest, ApproximateMemoryUsage) {
  ApproximateMemoryUsageTest(0);
  ApproximateMemoryUsageTest(kHugePageSize);
}

TEST_F(ArenaTest, Simple) {
  SimpleTest(0);
  SimpleTest(kHugePageSize);
}
}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
