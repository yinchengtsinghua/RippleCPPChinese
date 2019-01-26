
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

#include <atomic>
#include <iostream>
#include <string>
#include <utility>

#include "rocksdb/env.h"
#include "util/autovector.h"
#include "util/string_util.h"
#include "util/testharness.h"
#include "util/testutil.h"

using std::cout;
using std::endl;

namespace rocksdb {

class AutoVectorTest : public testing::Test {};
const unsigned long kSize = 8;

namespace {
template <class T>
void AssertAutoVectorOnlyInStack(autovector<T, kSize>* vec, bool result) {
#ifndef ROCKSDB_LITE
  ASSERT_EQ(vec->only_in_stack(), result);
#endif  //！摇滚乐
}
}  //命名空间

TEST_F(AutoVectorTest, PushBackAndPopBack) {
  autovector<size_t, kSize> vec;
  ASSERT_TRUE(vec.empty());
  ASSERT_EQ(0ul, vec.size());

  for (size_t i = 0; i < 1000 * kSize; ++i) {
    vec.push_back(i);
    ASSERT_TRUE(!vec.empty());
    if (i < kSize) {
      AssertAutoVectorOnlyInStack(&vec, true);
    } else {
      AssertAutoVectorOnlyInStack(&vec, false);
    }
    ASSERT_EQ(i + 1, vec.size());
    ASSERT_EQ(i, vec[i]);
    ASSERT_EQ(i, vec.at(i));
  }

  size_t size = vec.size();
  while (size != 0) {
    vec.pop_back();
//总是在堆里
    AssertAutoVectorOnlyInStack(&vec, false);
    ASSERT_EQ(--size, vec.size());
  }

  ASSERT_TRUE(vec.empty());
}

TEST_F(AutoVectorTest, EmplaceBack) {
  typedef std::pair<size_t, std::string> ValType;
  autovector<ValType, kSize> vec;

  for (size_t i = 0; i < 1000 * kSize; ++i) {
    vec.emplace_back(i, ToString(i + 123));
    ASSERT_TRUE(!vec.empty());
    if (i < kSize) {
      AssertAutoVectorOnlyInStack(&vec, true);
    } else {
      AssertAutoVectorOnlyInStack(&vec, false);
    }

    ASSERT_EQ(i + 1, vec.size());
    ASSERT_EQ(i, vec[i].first);
    ASSERT_EQ(ToString(i + 123), vec[i].second);
  }

  vec.clear();
  ASSERT_TRUE(vec.empty());
  AssertAutoVectorOnlyInStack(&vec, false);
}

TEST_F(AutoVectorTest, Resize) {
  autovector<size_t, kSize> vec;

  vec.resize(kSize);
  AssertAutoVectorOnlyInStack(&vec, true);
  for (size_t i = 0; i < kSize; ++i) {
    vec[i] = i;
  }

  vec.resize(kSize * 2);
  AssertAutoVectorOnlyInStack(&vec, false);
  for (size_t i = 0; i < kSize; ++i) {
    ASSERT_EQ(vec[i], i);
  }
  for (size_t i = 0; i < kSize; ++i) {
    vec[i + kSize] = i;
  }

  vec.resize(1);
  ASSERT_EQ(1U, vec.size());
}

namespace {
void AssertEqual(
    const autovector<size_t, kSize>& a, const autovector<size_t, kSize>& b) {
  ASSERT_EQ(a.size(), b.size());
  ASSERT_EQ(a.empty(), b.empty());
#ifndef ROCKSDB_LITE
  ASSERT_EQ(a.only_in_stack(), b.only_in_stack());
#endif  //！摇滚乐
  for (size_t i = 0; i < a.size(); ++i) {
    ASSERT_EQ(a[i], b[i]);
  }
}
}  //命名空间

TEST_F(AutoVectorTest, CopyAndAssignment) {
//测试已分配的堆和已分配的堆栈的情况。
  for (auto size : { kSize / 2, kSize * 1000 }) {
    autovector<size_t, kSize> vec;
    for (size_t i = 0; i < size; ++i) {
      vec.push_back(i);
    }

    {
      autovector<size_t, kSize> other;
      other = vec;
      AssertEqual(other, vec);
    }

    {
      autovector<size_t, kSize> other(vec);
      AssertEqual(other, vec);
    }
  }
}

TEST_F(AutoVectorTest, Iterators) {
  autovector<std::string, kSize> vec;
  for (size_t i = 0; i < kSize * 1000; ++i) {
    vec.push_back(ToString(i));
  }

//基本操作员测试
  ASSERT_EQ(vec.front(), *vec.begin());
  ASSERT_EQ(vec.back(), *(vec.end() - 1));
  ASSERT_TRUE(vec.begin() < vec.end());

//非常量迭代器
  size_t index = 0;
  for (const auto& item : vec) {
    ASSERT_EQ(vec[index++], item);
  }

  index = vec.size() - 1;
  for (auto pos = vec.rbegin(); pos != vec.rend(); ++pos) {
    ASSERT_EQ(vec[index--], *pos);
  }

//常量迭代型
  const auto& cvec = vec;
  index = 0;
  for (const auto& item : cvec) {
    ASSERT_EQ(cvec[index++], item);
  }

  index = vec.size() - 1;
  for (auto pos = cvec.rbegin(); pos != cvec.rend(); ++pos) {
    ASSERT_EQ(cvec[index--], *pos);
  }

//向前和向后
  auto pos = vec.begin();
  while (pos != vec.end()) {
    auto old_val = *pos;
    auto old = pos++;
//黑客：确保->有效
    ASSERT_TRUE(!old->empty());
    ASSERT_EQ(old_val, *old);
    ASSERT_TRUE(pos == vec.end() || old_val != *pos);
  }

  pos = vec.begin();
  for (size_t i = 0; i < vec.size(); i += 2) {
//无法使用assert_eq，因为该宏依赖于iostream序列化
    ASSERT_TRUE(pos + 2 - 2 == pos);
    pos += 2;
    ASSERT_TRUE(pos >= vec.begin());
    ASSERT_TRUE(pos <= vec.end());

    size_t diff = static_cast<size_t>(pos - vec.begin());
    ASSERT_EQ(i + 2, diff);
  }
}

namespace {
std::vector<std::string> GetTestKeys(size_t size) {
  std::vector<std::string> keys;
  keys.resize(size);

  int index = 0;
  for (auto& key : keys) {
    key = "item-" + rocksdb::ToString(index++);
  }
  return keys;
}
}  //命名空间

template <class TVector>
void BenchmarkVectorCreationAndInsertion(
    std::string name, size_t ops, size_t item_size,
    const std::vector<typename TVector::value_type>& items) {
  auto env = Env::Default();

  int index = 0;
  auto start_time = env->NowNanos();
  auto ops_remaining = ops;
  while(ops_remaining--) {
    TVector v;
    for (size_t i = 0; i < item_size; ++i) {
      v.push_back(items[index++]);
    }
  }
  auto elapsed = env->NowNanos() - start_time;
  cout << "created " << ops << " " << name << " instances:\n\t"
       << "each was inserted with " << item_size << " elements\n\t"
       << "total time elapsed: " << elapsed << " (ns)" << endl;
}

template <class TVector>
size_t BenchmarkSequenceAccess(std::string name, size_t ops, size_t elem_size) {
  TVector v;
  for (const auto& item : GetTestKeys(elem_size)) {
    v.push_back(item);
  }
  auto env = Env::Default();

  auto ops_remaining = ops;
  auto start_time = env->NowNanos();
  size_t total = 0;
  while (ops_remaining--) {
    auto end = v.end();
    for (auto pos = v.begin(); pos != end; ++pos) {
      total += pos->size();
    }
  }
  auto elapsed = env->NowNanos() - start_time;
  cout << "performed " << ops << " sequence access against " << name << "\n\t"
       << "size: " << elem_size << "\n\t"
       << "total time elapsed: " << elapsed << " (ns)" << endl;
//避免编译器优化忽略总计
  return total;
}

//此测试用例只报告std:：vector<std:：string>
//以及autovector<std:：string>。我们选择字符串进行比较，因为在大多数情况下
//在我们的用例中，我们使用了std:：vector<std:：string>。
TEST_F(AutoVectorTest, PerfBench) {
//为了得到一个更公平的结果，我们对Kops进行相同的操作。
  size_t kOps = 100000;

//创建和插入测试
//有以下情况时测试案例：
//*未插入元素：std:：vector的内部数组可能无法真正获得
//初始化。
//*插入一个元素：std:：vector的内部数组必须具有
//初始化。
//*插入ksize元素。这显示了如果我们
//把所有东西都叠起来。
//*2*ksize元素已插入。的内部向量
//AutoVector必须已初始化。
  cout << "=====================================================" << endl;
  cout << "Creation and Insertion Test (value type: std::string)" << endl;
  cout << "=====================================================" << endl;

//预生成的唯一键
  auto string_keys = GetTestKeys(kOps * 2 * kSize);
  for (auto insertions : { 0ul, 1ul, kSize / 2, kSize, 2 * kSize }) {
    BenchmarkVectorCreationAndInsertion<std::vector<std::string>>(
        "std::vector<std::string>", kOps, insertions, string_keys);
    BenchmarkVectorCreationAndInsertion<autovector<std::string, kSize>>(
        "autovector<std::string>", kOps, insertions, string_keys);
    cout << "-----------------------------------" << endl;
  }

  cout << "=====================================================" << endl;
  cout << "Creation and Insertion Test (value type: uint64_t)" << endl;
  cout << "=====================================================" << endl;

//预生成的唯一键
  std::vector<uint64_t> int_keys(kOps * 2 * kSize);
  for (size_t i = 0; i < kOps * 2 * kSize; ++i) {
    int_keys[i] = i;
  }
  for (auto insertions : { 0ul, 1ul, kSize / 2, kSize, 2 * kSize }) {
    BenchmarkVectorCreationAndInsertion<std::vector<uint64_t>>(
        "std::vector<uint64_t>", kOps, insertions, int_keys);
    BenchmarkVectorCreationAndInsertion<autovector<uint64_t, kSize>>(
      "autovector<uint64_t>", kOps, insertions, int_keys
    );
    cout << "-----------------------------------" << endl;
  }

//顺序访问测试
  cout << "=====================================================" << endl;
  cout << "Sequence Access Test" << endl;
  cout << "=====================================================" << endl;
  for (auto elem_size : { kSize / 2, kSize, 2 * kSize }) {
    BenchmarkSequenceAccess<std::vector<std::string>>("std::vector", kOps,
                                                      elem_size);
    BenchmarkSequenceAccess<autovector<std::string, kSize>>("autovector", kOps,
                                                            elem_size);
    cout << "-----------------------------------" << endl;
  }
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
