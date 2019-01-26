
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

#include <gtest/gtest.h>

#include <climits>

#include <queue>
#include <random>
#include <utility>

#include "util/heap.h"

#ifndef GFLAGS
const int64_t FLAGS_iters = 100000;
#else
#include <gflags/gflags.h>
DEFINE_int64(iters, 100000, "number of pseudo-random operations in each test");
#endif  //GFLAGS

/*
 *将util/heap.h中的自定义堆实现与
 *std：：伪随机操作序列上的优先级队列。
 **/


namespace rocksdb {

using HeapTestValue = uint64_t;
using Params = std::tuple<size_t, HeapTestValue, int64_t>;

class HeapTest : public ::testing::TestWithParam<Params> {
};

TEST_P(HeapTest, Test) {
//此测试对
//binaryheap和std:：priority_队列，比较输出。三
//可能的操作是插入、替换top和pop。
//
//选择插入的频率比其他插入的频率稍高，因此
//这堆东西慢慢地长起来。一旦大小达到最大堆大小限制，我们
//不允许在堆变为空之前插入，测试“排出”
//脚本。

  const auto MAX_HEAP_SIZE = std::get<0>(GetParam());
  const auto MAX_VALUE = std::get<1>(GetParam());
  const auto RNG_SEED = std::get<2>(GetParam());

  BinaryHeap<HeapTestValue> heap;
  std::priority_queue<HeapTestValue> ref;

  std::mt19937 rng(static_cast<unsigned int>(RNG_SEED));
  std::uniform_int_distribution<HeapTestValue> value_dist(0, MAX_VALUE);
  int ndrains = 0;
bool draining = false;     //达到最大大小，排出直到清空堆
  size_t size = 0;
  for (int64_t i = 0; i < FLAGS_iters; ++i) {
    if (size == 0) {
      draining = false;
    }

    if (!draining &&
        (size == 0 || std::bernoulli_distribution(0.4)(rng))) {
//插入
      HeapTestValue val = value_dist(rng);
      heap.push(val);
      ref.push(val);
      ++size;
      if (size == MAX_HEAP_SIZE) {
        draining = true;
        ++ndrains;
      }
    } else if (std::bernoulli_distribution(0.5)(rng)) {
//换顶
      HeapTestValue val = value_dist(rng);
      heap.replace_top(val);
      ref.pop();
      ref.push(val);
    } else {
//流行音乐
      assert(size > 0);
      heap.pop();
      ref.pop();
      --size;
    }

//每次操作后，检查公共方法是否给出相同的
//结果
    assert((size == 0) == ref.empty());
    ASSERT_EQ(size == 0, heap.empty());
    if (size > 0) {
      ASSERT_EQ(ref.top(), heap.top());
    }
  }

//概率应该设置为偶尔达到最大堆大小和
//排水
  assert(ndrains > 0);

  heap.clear();
  ASSERT_TRUE(heap.empty());
}

//基本测试，最大值=3*最大堆大小（偶尔重复）
INSTANTIATE_TEST_CASE_P(
  Basic, HeapTest,
  ::testing::Values(Params(1000, 3000, 0x1b575cf05b708945))
);
//具有小值的中等大小堆（多个重复项）
INSTANTIATE_TEST_CASE_P(
  SmallValues, HeapTest,
  ::testing::Values(Params(100, 10, 0x5ae213f7bd5dccd0))
);
//小堆，大值范围（无重复项）
INSTANTIATE_TEST_CASE_P(
  SmallHeap, HeapTest,
  ::testing::Values(Params(10, ULLONG_MAX, 0x3e1fa8f4d01707cf))
);
//两元素堆
INSTANTIATE_TEST_CASE_P(
  TwoElementHeap, HeapTest,
  ::testing::Values(Params(2, 5, 0x4b5e13ea988c6abc))
);
//单元素堆
INSTANTIATE_TEST_CASE_P(
  OneElementHeap, HeapTest,
  ::testing::Values(Params(1, 3, 0x176a1019ab0b612e))
);

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
#ifdef GFLAGS
  GFLAGS::ParseCommandLineFlags(&argc, &argv, true);
#endif  //GFLAGS
  return RUN_ALL_TESTS();
}
