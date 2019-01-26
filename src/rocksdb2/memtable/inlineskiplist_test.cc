
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

#include "memtable/inlineskiplist.h"
#include <set>
#include <unordered_set>
#include "rocksdb/env.h"
#include "util/concurrent_arena.h"
#include "util/hash.h"
#include "util/random.h"
#include "util/testharness.h"

namespace rocksdb {

//我们的测试跳过列表存储8字节无符号整数
typedef uint64_t Key;

static const char* Encode(const uint64_t* key) {
  return reinterpret_cast<const char*>(key);
}

static Key Decode(const char* key) {
  Key rv;
  memcpy(&rv, key, sizeof(Key));
  return rv;
}

struct TestComparator {
  int operator()(const char* a, const char* b) const {
    if (Decode(a) < Decode(b)) {
      return -1;
    } else if (Decode(a) > Decode(b)) {
      return +1;
    } else {
      return 0;
    }
  }
};

typedef InlineSkipList<TestComparator> TestInlineSkipList;

class InlineSkipTest : public testing::Test {
 public:
  void Insert(TestInlineSkipList* list, Key key) {
    char* buf = list->AllocateKey(sizeof(Key));
    memcpy(buf, &key, sizeof(Key));
    list->Insert(buf);
    keys_.insert(key);
  }

  void InsertWithHint(TestInlineSkipList* list, Key key, void** hint) {
    char* buf = list->AllocateKey(sizeof(Key));
    memcpy(buf, &key, sizeof(Key));
    list->InsertWithHint(buf, hint);
    keys_.insert(key);
  }

  void Validate(TestInlineSkipList* list) {
//检查钥匙是否存在。
    for (Key key : keys_) {
      ASSERT_TRUE(list->Contains(Encode(&key)));
    }
//遍历列表，确保键按顺序出现，并且没有多余的键
//存在密钥。
    TestInlineSkipList::Iterator iter(list);
    ASSERT_FALSE(iter.Valid());
    Key zero = 0;
    iter.Seek(Encode(&zero));
    for (Key key : keys_) {
      ASSERT_TRUE(iter.Valid());
      ASSERT_EQ(key, Decode(iter.key()));
      iter.Next();
    }
    ASSERT_FALSE(iter.Valid());
//验证列表的格式是否正确。
    list->TEST_Validate();
  }

 private:
  std::set<Key> keys_;
};

TEST_F(InlineSkipTest, Empty) {
  Arena arena;
  TestComparator cmp;
  InlineSkipList<TestComparator> list(cmp, &arena);
  Key key = 10;
  ASSERT_TRUE(!list.Contains(Encode(&key)));

  InlineSkipList<TestComparator>::Iterator iter(&list);
  ASSERT_TRUE(!iter.Valid());
  iter.SeekToFirst();
  ASSERT_TRUE(!iter.Valid());
  key = 100;
  iter.Seek(Encode(&key));
  ASSERT_TRUE(!iter.Valid());
  iter.SeekForPrev(Encode(&key));
  ASSERT_TRUE(!iter.Valid());
  iter.SeekToLast();
  ASSERT_TRUE(!iter.Valid());
}

TEST_F(InlineSkipTest, InsertAndLookup) {
  const int N = 2000;
  const int R = 5000;
  Random rnd(1000);
  std::set<Key> keys;
  ConcurrentArena arena;
  TestComparator cmp;
  InlineSkipList<TestComparator> list(cmp, &arena);
  for (int i = 0; i < N; i++) {
    Key key = rnd.Next() % R;
    if (keys.insert(key).second) {
      char* buf = list.AllocateKey(sizeof(Key));
      memcpy(buf, &key, sizeof(Key));
      list.Insert(buf);
    }
  }

  for (Key i = 0; i < R; i++) {
    if (list.Contains(Encode(&i))) {
      ASSERT_EQ(keys.count(i), 1U);
    } else {
      ASSERT_EQ(keys.count(i), 0U);
    }
  }

//简单迭代器测试
  {
    InlineSkipList<TestComparator>::Iterator iter(&list);
    ASSERT_TRUE(!iter.Valid());

    uint64_t zero = 0;
    iter.Seek(Encode(&zero));
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(*(keys.begin()), Decode(iter.key()));

    uint64_t max_key = R - 1;
    iter.SeekForPrev(Encode(&max_key));
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(*(keys.rbegin()), Decode(iter.key()));

    iter.SeekToFirst();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(*(keys.begin()), Decode(iter.key()));

    iter.SeekToLast();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(*(keys.rbegin()), Decode(iter.key()));
  }

//正迭代测试
  for (Key i = 0; i < R; i++) {
    InlineSkipList<TestComparator>::Iterator iter(&list);
    iter.Seek(Encode(&i));

//与模型迭代器比较
    std::set<Key>::iterator model_iter = keys.lower_bound(i);
    for (int j = 0; j < 3; j++) {
      if (model_iter == keys.end()) {
        ASSERT_TRUE(!iter.Valid());
        break;
      } else {
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*model_iter, Decode(iter.key()));
        ++model_iter;
        iter.Next();
      }
    }
  }

//反向迭代测试
  for (Key i = 0; i < R; i++) {
    InlineSkipList<TestComparator>::Iterator iter(&list);
    iter.SeekForPrev(Encode(&i));

//与模型迭代器比较
    std::set<Key>::iterator model_iter = keys.upper_bound(i);
    for (int j = 0; j < 3; j++) {
      if (model_iter == keys.begin()) {
        ASSERT_TRUE(!iter.Valid());
        break;
      } else {
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*--model_iter, Decode(iter.key()));
        iter.Prev();
      }
    }
  }
}

TEST_F(InlineSkipTest, InsertWithHint_Sequential) {
  const int N = 100000;
  Arena arena;
  TestComparator cmp;
  TestInlineSkipList list(cmp, &arena);
  void* hint = nullptr;
  for (int i = 0; i < N; i++) {
    Key key = i;
    InsertWithHint(&list, key, &hint);
  }
  Validate(&list);
}

TEST_F(InlineSkipTest, InsertWithHint_MultipleHints) {
  const int N = 100000;
  const int S = 100;
  Random rnd(534);
  Arena arena;
  TestComparator cmp;
  TestInlineSkipList list(cmp, &arena);
  void* hints[S];
  Key last_key[S];
  for (int i = 0; i < S; i++) {
    hints[i] = nullptr;
    last_key[i] = 0;
  }
  for (int i = 0; i < N; i++) {
    Key s = rnd.Uniform(S);
    Key key = (s << 32) + (++last_key[s]);
    InsertWithHint(&list, key, &hints[s]);
  }
  Validate(&list);
}

TEST_F(InlineSkipTest, InsertWithHint_MultipleHintsRandom) {
  const int N = 100000;
  const int S = 100;
  Random rnd(534);
  Arena arena;
  TestComparator cmp;
  TestInlineSkipList list(cmp, &arena);
  void* hints[S];
  for (int i = 0; i < S; i++) {
    hints[i] = nullptr;
  }
  for (int i = 0; i < N; i++) {
    Key s = rnd.Uniform(S);
    Key key = (s << 32) + rnd.Next();
    InsertWithHint(&list, key, &hints[s]);
  }
  Validate(&list);
}

TEST_F(InlineSkipTest, InsertWithHint_CompatibleWithInsertWithoutHint) {
  const int N = 100000;
  const int S1 = 100;
  const int S2 = 100;
  Random rnd(534);
  Arena arena;
  TestComparator cmp;
  TestInlineSkipList list(cmp, &arena);
  std::unordered_set<Key> used;
  Key with_hint[S1];
  Key without_hint[S2];
  void* hints[S1];
  for (int i = 0; i < S1; i++) {
    hints[i] = nullptr;
    while (true) {
      Key s = rnd.Next();
      if (used.insert(s).second) {
        with_hint[i] = s;
        break;
      }
    }
  }
  for (int i = 0; i < S2; i++) {
    while (true) {
      Key s = rnd.Next();
      if (used.insert(s).second) {
        without_hint[i] = s;
        break;
      }
    }
  }
  for (int i = 0; i < N; i++) {
    Key s = rnd.Uniform(S1 + S2);
    if (s < S1) {
      Key key = (with_hint[s] << 32) + rnd.Next();
      InsertWithHint(&list, key, &hints[s]);
    } else {
      Key key = (without_hint[s - S1] << 32) + rnd.Next();
      Insert(&list, key);
    }
  }
  Validate(&list);
}

//我们要确保只有一个作者和多个
//并发读卡器（除了
//读者的迭代器被创建），读者总是观察
//当迭代器为
//构造函数。因为插入是同时发生的，所以我们可以
//同时观察自迭代器
//构造，但我们不应错过任何存在的值
//在迭代器构造时。
//
//我们生成多部分密钥：
//<密钥，Gen，hash >
//哪里：
//键在范围[0..K-1]内
//gen是密钥的生成号
//hash是hash（key，gen）
//
//插入代码选择一个随机键，将gen设置为1+最后一个
//为该键插入的生成号，并将哈希设置为哈希（key，gen）。
//
//在读取开始时，我们快照最后插入的
//每个键的生成编号。然后我们迭代，包括随机的
//调用Next（）和Seek（）。对于我们遇到的每一把钥匙，我们
//检查它是在给定初始快照的情况下预期的，还是
//自迭代器启动后并发添加。
class ConcurrentTest {
 public:
  static const uint32_t K = 8;

 private:
  static uint64_t key(Key key) { return (key >> 40); }
  static uint64_t gen(Key key) { return (key >> 8) & 0xffffffffu; }
  static uint64_t hash(Key key) { return key & 0xff; }

  static uint64_t HashNumbers(uint64_t k, uint64_t g) {
    uint64_t data[2] = {k, g};
    return Hash(reinterpret_cast<char*>(data), sizeof(data), 0);
  }

  static Key MakeKey(uint64_t k, uint64_t g) {
    assert(sizeof(Key) == sizeof(uint64_t));
assert(k <= K);  //我们有时通过K来寻找滑雪运动员的终点
    assert(g <= 0xffffffffu);
    return ((k << 40) | (g << 8) | (HashNumbers(k, g) & 0xff));
  }

  static bool IsValidKey(Key k) {
    return hash(k) == (HashNumbers(key(k), gen(k)) & 0xff);
  }

  static Key RandomTarget(Random* rnd) {
    switch (rnd->Next() % 10) {
      case 0:
//寻找开始
        return MakeKey(0, 0);
      case 1:
//寻求结束
        return MakeKey(K, 0);
      default:
//寻求中间
        return MakeKey(rnd->Next() % K, 0);
    }
  }

//按密钥生成
  struct State {
    std::atomic<int> generation[K];
    void Set(int k, int v) {
      generation[k].store(v, std::memory_order_release);
    }
    int Get(int k) { return generation[k].load(std::memory_order_acquire); }

    State() {
      for (unsigned int k = 0; k < K; k++) {
        Set(k, 0);
      }
    }
  };

//测试的当前状态
  State current_;

  ConcurrentArena arena_;

//inlineskiplist不受mu_uu保护。我们只用一个作家
//线程来修改它。
  InlineSkipList<TestComparator> list_;

 public:
  ConcurrentTest() : list_(TestComparator(), &arena_) {}

//要求：没有对WriteStep或ConcurrentWriteStep的并发调用
  void WriteStep(Random* rnd) {
    const uint32_t k = rnd->Next() % K;
    const int g = current_.Get(k) + 1;
    const Key new_key = MakeKey(k, g);
    char* buf = list_.AllocateKey(sizeof(Key));
    memcpy(buf, &new_key, sizeof(Key));
    list_.Insert(buf);
    current_.Set(k, g);
  }

//要求：同一k无并发调用
  void ConcurrentWriteStep(uint32_t k) {
    const int g = current_.Get(k) + 1;
    const Key new_key = MakeKey(k, g);
    char* buf = list_.AllocateKey(sizeof(Key));
    memcpy(buf, &new_key, sizeof(Key));
    list_.InsertConcurrently(buf);
    ASSERT_EQ(g, current_.Get(k) + 1);
    current_.Set(k, g);
  }

  void ReadStep(Random* rnd) {
//记住skiplist的初始提交状态。
    State initial_state;
    for (unsigned int k = 0; k < K; k++) {
      initial_state.Set(k, current_.Get(k));
    }

    Key pos = RandomTarget(rnd);
    InlineSkipList<TestComparator>::Iterator iter(&list_);
    iter.Seek(Encode(&pos));
    while (true) {
      Key current;
      if (!iter.Valid()) {
        current = MakeKey(K, 0);
      } else {
        current = Decode(iter.key());
        ASSERT_TRUE(IsValidKey(current)) << current;
      }
      ASSERT_LE(pos, current) << "should not go backwards";

//验证[pos，current]中的所有内容都不存在于
//初始状态。
      while (pos < current) {
        ASSERT_LT(key(pos), K) << pos;

//请注意，从未插入第0代，因此如果
//<*，0，*>丢失。
        ASSERT_TRUE((gen(pos) == 0U) ||
                    (gen(pos) > static_cast<uint64_t>(initial_state.Get(
                                    static_cast<int>(key(pos))))))
            << "key: " << key(pos) << "; gen: " << gen(pos)
            << "; initgen: " << initial_state.Get(static_cast<int>(key(pos)));

//前进到有效密钥空间中的下一个密钥
        if (key(pos) < key(current)) {
          pos = MakeKey(key(pos) + 1, 0);
        } else {
          pos = MakeKey(key(pos), gen(pos) + 1);
        }
      }

      if (!iter.Valid()) {
        break;
      }

      if (rnd->Next() % 2) {
        iter.Next();
        pos = MakeKey(key(pos), gen(pos) + 1);
      } else {
        Key new_target = RandomTarget(rnd);
        if (new_target > pos) {
          pos = new_target;
          iter.Seek(Encode(&new_target));
        }
      }
    }
  }
};
const uint32_t ConcurrentTest::K;

//对ConcurrentTest执行单线程测试的简单测试
//脚手架。
TEST_F(InlineSkipTest, ConcurrentReadWithoutThreads) {
  ConcurrentTest test;
  Random rnd(test::RandomSeed());
  for (int i = 0; i < 10000; i++) {
    test.ReadStep(&rnd);
    test.WriteStep(&rnd);
  }
}

TEST_F(InlineSkipTest, ConcurrentInsertWithoutThreads) {
  ConcurrentTest test;
  Random rnd(test::RandomSeed());
  for (int i = 0; i < 10000; i++) {
    test.ReadStep(&rnd);
    uint32_t base = rnd.Next();
    for (int j = 0; j < 4; ++j) {
      test.ConcurrentWriteStep((base + j) % ConcurrentTest::K);
    }
  }
}

class TestState {
 public:
  ConcurrentTest t_;
  int seed_;
  std::atomic<bool> quit_flag_;
  std::atomic<uint32_t> next_writer_;

  enum ReaderState { STARTING, RUNNING, DONE };

  explicit TestState(int s)
      : seed_(s),
        quit_flag_(false),
        state_(STARTING),
        pending_writers_(0),
        state_cv_(&mu_) {}

  void Wait(ReaderState s) {
    mu_.Lock();
    while (state_ != s) {
      state_cv_.Wait();
    }
    mu_.Unlock();
  }

  void Change(ReaderState s) {
    mu_.Lock();
    state_ = s;
    state_cv_.Signal();
    mu_.Unlock();
  }

  void AdjustPendingWriters(int delta) {
    mu_.Lock();
    pending_writers_ += delta;
    if (pending_writers_ == 0) {
      state_cv_.Signal();
    }
    mu_.Unlock();
  }

  void WaitForPendingWriters() {
    mu_.Lock();
    while (pending_writers_ != 0) {
      state_cv_.Wait();
    }
    mu_.Unlock();
  }

 private:
  port::Mutex mu_;
  ReaderState state_;
  int pending_writers_;
  port::CondVar state_cv_;
};

static void ConcurrentReader(void* arg) {
  TestState* state = reinterpret_cast<TestState*>(arg);
  Random rnd(state->seed_);
  int64_t reads = 0;
  state->Change(TestState::RUNNING);
  while (!state->quit_flag_.load(std::memory_order_acquire)) {
    state->t_.ReadStep(&rnd);
    ++reads;
  }
  state->Change(TestState::DONE);
}

static void ConcurrentWriter(void* arg) {
  TestState* state = reinterpret_cast<TestState*>(arg);
  uint32_t k = state->next_writer_++ % ConcurrentTest::K;
  state->t_.ConcurrentWriteStep(k);
  state->AdjustPendingWriters(-1);
}

static void RunConcurrentRead(int run) {
  const int seed = test::RandomSeed() + (run * 100);
  Random rnd(seed);
  const int N = 1000;
  const int kSize = 1000;
  for (int i = 0; i < N; i++) {
    if ((i % 100) == 0) {
      fprintf(stderr, "Run %d of %d\n", i, N);
    }
    TestState state(seed + 1);
    Env::Default()->SetBackgroundThreads(1);
    Env::Default()->Schedule(ConcurrentReader, &state);
    state.Wait(TestState::RUNNING);
    for (int k = 0; k < kSize; ++k) {
      state.t_.WriteStep(&rnd);
    }
    state.quit_flag_.store(true, std::memory_order_release);
    state.Wait(TestState::DONE);
  }
}

static void RunConcurrentInsert(int run, int write_parallelism = 4) {
  Env::Default()->SetBackgroundThreads(1 + write_parallelism,
                                       Env::Priority::LOW);
  const int seed = test::RandomSeed() + (run * 100);
  Random rnd(seed);
  const int N = 1000;
  const int kSize = 1000;
  for (int i = 0; i < N; i++) {
    if ((i % 100) == 0) {
      fprintf(stderr, "Run %d of %d\n", i, N);
    }
    TestState state(seed + 1);
    Env::Default()->Schedule(ConcurrentReader, &state);
    state.Wait(TestState::RUNNING);
    for (int k = 0; k < kSize; k += write_parallelism) {
      state.next_writer_ = rnd.Next();
      state.AdjustPendingWriters(write_parallelism);
      for (int p = 0; p < write_parallelism; ++p) {
        Env::Default()->Schedule(ConcurrentWriter, &state);
      }
      state.WaitForPendingWriters();
    }
    state.quit_flag_.store(true, std::memory_order_release);
    state.Wait(TestState::DONE);
  }
}

TEST_F(InlineSkipTest, ConcurrentRead1) { RunConcurrentRead(1); }
TEST_F(InlineSkipTest, ConcurrentRead2) { RunConcurrentRead(2); }
TEST_F(InlineSkipTest, ConcurrentRead3) { RunConcurrentRead(3); }
TEST_F(InlineSkipTest, ConcurrentRead4) { RunConcurrentRead(4); }
TEST_F(InlineSkipTest, ConcurrentRead5) { RunConcurrentRead(5); }
TEST_F(InlineSkipTest, ConcurrentInsert1) { RunConcurrentInsert(1); }
TEST_F(InlineSkipTest, ConcurrentInsert2) { RunConcurrentInsert(2); }
TEST_F(InlineSkipTest, ConcurrentInsert3) { RunConcurrentInsert(3); }

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
