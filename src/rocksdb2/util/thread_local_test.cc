
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

#include <thread>
#include <atomic>
#include <string>

#include "rocksdb/env.h"
#include "port/port.h"
#include "util/autovector.h"
#include "util/sync_point.h"
#include "util/testharness.h"
#include "util/testutil.h"
#include "util/thread_local.h"

namespace rocksdb {

class ThreadLocalTest : public testing::Test {
 public:
  ThreadLocalTest() : env_(Env::Default()) {}

  Env* env_;
};

namespace {

struct Params {
  Params(port::Mutex* m, port::CondVar* c, int* u, int n,
         UnrefHandler handler = nullptr)
      : mu(m),
        cv(c),
        unref(u),
        total(n),
        started(0),
        completed(0),
        doWrite(false),
        tls1(handler),
        tls2(nullptr) {}

  port::Mutex* mu;
  port::CondVar* cv;
  int* unref;
  int total;
  int started;
  int completed;
  bool doWrite;
  ThreadLocalPtr tls1;
  ThreadLocalPtr* tls2;
};

class IDChecker : public ThreadLocalPtr {
public:
  static uint32_t PeekId() {
    return TEST_PeekId();
  }
};

}  //匿名命名空间

//抑制假阳性clang分析器警告。
#ifndef __clang_analyzer__
TEST_F(ThreadLocalTest, UniqueIdTest) {
  port::Mutex mu;
  port::CondVar cv(&mu);

  ASSERT_EQ(IDChecker::PeekId(), 0u);
//新的threadlocal实例bumps id by 1
  {
//ID使用0
    Params p1(&mu, &cv, nullptr, 1u);
    ASSERT_EQ(IDChecker::PeekId(), 1u);
//ID使用1
    Params p2(&mu, &cv, nullptr, 1u);
    ASSERT_EQ(IDChecker::PeekId(), 2u);
//ID使用2
    Params p3(&mu, &cv, nullptr, 1u);
    ASSERT_EQ(IDChecker::PeekId(), 3u);
//ID使用3
    Params p4(&mu, &cv, nullptr, 1u);
    ASSERT_EQ(IDChecker::PeekId(), 4u);
  }
//ID 3、2、1、0依次位于空闲队列中
  ASSERT_EQ(IDChecker::PeekId(), 0u);

//拿起0
  Params p1(&mu, &cv, nullptr, 1u);
  ASSERT_EQ(IDChecker::PeekId(), 1u);
//拿起1
  Params* p2 = new Params(&mu, &cv, nullptr, 1u);
  ASSERT_EQ(IDChecker::PeekId(), 2u);
//拿起2
  Params p3(&mu, &cv, nullptr, 1u);
  ASSERT_EQ(IDChecker::PeekId(), 3u);
//返回1
  delete p2;
  ASSERT_EQ(IDChecker::PeekId(), 1u);
//现在我们排了3，1队
//拿起1
  Params p4(&mu, &cv, nullptr, 1u);
  ASSERT_EQ(IDChecker::PeekId(), 3u);
//拿起3
  Params p5(&mu, &cv, nullptr, 1u);
//下一个新标识
  ASSERT_EQ(IDChecker::PeekId(), 4u);
//退出后，队列中的ID序列：
//3, 1, 2，0
}
#endif  //_uuu-clang_分析仪

TEST_F(ThreadLocalTest, SequentialReadWriteTest) {
//全局ID列表包含超过3、1、2、0
  ASSERT_EQ(IDChecker::PeekId(), 0u);

  port::Mutex mu;
  port::CondVar cv(&mu);
  Params p(&mu, &cv, nullptr, 1);
  ThreadLocalPtr tls2;
  p.tls2 = &tls2;

  auto func = [](void* ptr) {
    auto& params = *static_cast<Params*>(ptr);

    ASSERT_TRUE(params.tls1.Get() == nullptr);
    params.tls1.Reset(reinterpret_cast<int*>(1));
    ASSERT_TRUE(params.tls1.Get() == reinterpret_cast<int*>(1));
    params.tls1.Reset(reinterpret_cast<int*>(2));
    ASSERT_TRUE(params.tls1.Get() == reinterpret_cast<int*>(2));

    ASSERT_TRUE(params.tls2->Get() == nullptr);
    params.tls2->Reset(reinterpret_cast<int*>(1));
    ASSERT_TRUE(params.tls2->Get() == reinterpret_cast<int*>(1));
    params.tls2->Reset(reinterpret_cast<int*>(2));
    ASSERT_TRUE(params.tls2->Get() == reinterpret_cast<int*>(2));

    params.mu->Lock();
    ++(params.completed);
    params.cv->SignalAll();
    params.mu->Unlock();
  };

  for (int iter = 0; iter < 1024; ++iter) {
    ASSERT_EQ(IDChecker::PeekId(), 1u);
//另一个新线程，读/写不应该看到上一个线程的值
    env_->StartThread(func, static_cast<void*>(&p));
    mu.Lock();
    while (p.completed != iter + 1) {
      cv.Wait();
    }
    mu.Unlock();
    ASSERT_EQ(IDChecker::PeekId(), 1u);
  }
}

TEST_F(ThreadLocalTest, ConcurrentReadWriteTest) {
//全局ID列表包含超过3、1、2、0
  ASSERT_EQ(IDChecker::PeekId(), 0u);

  ThreadLocalPtr tls2;
  port::Mutex mu1;
  port::CondVar cv1(&mu1);
  Params p1(&mu1, &cv1, nullptr, 16);
  p1.tls2 = &tls2;

  port::Mutex mu2;
  port::CondVar cv2(&mu2);
  Params p2(&mu2, &cv2, nullptr, 16);
  p2.doWrite = true;
  p2.tls2 = &tls2;

  auto func = [](void* ptr) {
    auto& p = *static_cast<Params*>(ptr);

    p.mu->Lock();
//大小切换大小和ptr大小
//我们想演。
    size_t own = ++(p.started);
    p.cv->SignalAll();
    while (p.started != p.total) {
      p.cv->Wait();
    }
    p.mu->Unlock();

//让写入线程写入与读取线程不同的值
    if (p.doWrite) {
      own += 8192;
    }

    ASSERT_TRUE(p.tls1.Get() == nullptr);
    ASSERT_TRUE(p.tls2->Get() == nullptr);

    auto* env = Env::Default();
    auto start = env->NowMicros();

    p.tls1.Reset(reinterpret_cast<size_t*>(own));
    p.tls2->Reset(reinterpret_cast<size_t*>(own + 1));
//循环1秒
    while (env->NowMicros() - start < 1000 * 1000) {
      for (int iter = 0; iter < 100000; ++iter) {
        ASSERT_TRUE(p.tls1.Get() == reinterpret_cast<size_t*>(own));
        ASSERT_TRUE(p.tls2->Get() == reinterpret_cast<size_t*>(own + 1));
        if (p.doWrite) {
          p.tls1.Reset(reinterpret_cast<size_t*>(own));
          p.tls2->Reset(reinterpret_cast<size_t*>(own + 1));
        }
      }
    }

    p.mu->Lock();
    ++(p.completed);
    p.cv->SignalAll();
    p.mu->Unlock();
  };

//开始2个练习：一个保持写作，一个保持阅读。
//读取实例不应看到来自写入实例的数据。
//值的每个线程本地副本也不同于
//其他。
  for (int th = 0; th < p1.total; ++th) {
    env_->StartThread(func, static_cast<void*>(&p1));
  }
  for (int th = 0; th < p2.total; ++th) {
    env_->StartThread(func, static_cast<void*>(&p2));
  }

  mu1.Lock();
  while (p1.completed != p1.total) {
    cv1.Wait();
  }
  mu1.Unlock();

  mu2.Lock();
  while (p2.completed != p2.total) {
    cv2.Wait();
  }
  mu2.Unlock();

  ASSERT_EQ(IDChecker::PeekId(), 3u);
}

TEST_F(ThreadLocalTest, Unref) {
  ASSERT_EQ(IDChecker::PeekId(), 0u);

  auto unref = [](void* ptr) {
    auto& p = *static_cast<Params*>(ptr);
    p.mu->Lock();
    ++(*p.unref);
    p.mu->Unlock();
  };

//案例0：如果从未访问threadloclptr，则不会触发unref
  auto func0 = [](void* ptr) {
    auto& p = *static_cast<Params*>(ptr);

    p.mu->Lock();
    ++(p.started);
    p.cv->SignalAll();
    while (p.started != p.total) {
      p.cv->Wait();
    }
    p.mu->Unlock();
  };

  for (int th = 1; th <= 128; th += th) {
    port::Mutex mu;
    port::CondVar cv(&mu);
    int unref_count = 0;
    Params p(&mu, &cv, &unref_count, th, unref);

    for (int i = 0; i < p.total; ++i) {
      env_->StartThread(func0, static_cast<void*>(&p));
    }
    env_->WaitForJoin();
    ASSERT_EQ(unref_count, 0);
  }

//案例1：线程退出触发的UNREF
  auto func1 = [](void* ptr) {
    auto& p = *static_cast<Params*>(ptr);

    p.mu->Lock();
    ++(p.started);
    p.cv->SignalAll();
    while (p.started != p.total) {
      p.cv->Wait();
    }
    p.mu->Unlock();

    ASSERT_TRUE(p.tls1.Get() == nullptr);
    ASSERT_TRUE(p.tls2->Get() == nullptr);

    p.tls1.Reset(ptr);
    p.tls2->Reset(ptr);

    p.tls1.Reset(ptr);
    p.tls2->Reset(ptr);
  };

  for (int th = 1; th <= 128; th += th) {
    port::Mutex mu;
    port::CondVar cv(&mu);
    int unref_count = 0;
    ThreadLocalPtr tls2(unref);
    Params p(&mu, &cv, &unref_count, th, unref);
    p.tls2 = &tls2;

    for (int i = 0; i < p.total; ++i) {
      env_->StartThread(func1, static_cast<void*>(&p));
    }

    env_->WaitForJoin();

//n个线程x 2个线程退出时的本地实例清理
    ASSERT_EQ(unref_count, 2 * p.total);
  }

//案例2:threadlocal实例销毁触发的unref
  auto func2 = [](void* ptr) {
    auto& p = *static_cast<Params*>(ptr);

    p.mu->Lock();
    ++(p.started);
    p.cv->SignalAll();
    while (p.started != p.total) {
      p.cv->Wait();
    }
    p.mu->Unlock();

    ASSERT_TRUE(p.tls1.Get() == nullptr);
    ASSERT_TRUE(p.tls2->Get() == nullptr);

    p.tls1.Reset(ptr);
    p.tls2->Reset(ptr);

    p.tls1.Reset(ptr);
    p.tls2->Reset(ptr);

    p.mu->Lock();
    ++(p.completed);
    p.cv->SignalAll();

//等待指令退出线程
    while (p.completed != 0) {
      p.cv->Wait();
    }
    p.mu->Unlock();
  };

  for (int th = 1; th <= 128; th += th) {
    port::Mutex mu;
    port::CondVar cv(&mu);
    int unref_count = 0;
    Params p(&mu, &cv, &unref_count, th, unref);
    p.tls2 = new ThreadLocalPtr(unref);

    for (int i = 0; i < p.total; ++i) {
      env_->StartThread(func2, static_cast<void*>(&p));
    }

//等待所有线程使用参数完成
    mu.Lock();
    while (p.completed != p.total) {
      cv.Wait();
    }
    mu.Unlock();

//现在销毁一个线程本地实例
    delete p.tls2;
    p.tls2 = nullptr;
//n个线程的实例销毁
    ASSERT_EQ(unref_count, p.total);

//退出信号
    mu.Lock();
    p.completed = 0;
    cv.SignalAll();
    mu.Unlock();
    env_->WaitForJoin();
//对于左侧实例，其他n个线程退出UNREF
    ASSERT_EQ(unref_count, 2 * p.total);
  }
}

TEST_F(ThreadLocalTest, Swap) {
  ThreadLocalPtr tls;
  tls.Reset(reinterpret_cast<void*>(1));
  ASSERT_EQ(reinterpret_cast<int64_t>(tls.Swap(nullptr)), 1);
  ASSERT_TRUE(tls.Swap(reinterpret_cast<void*>(2)) == nullptr);
  ASSERT_EQ(reinterpret_cast<int64_t>(tls.Get()), 2);
  ASSERT_EQ(reinterpret_cast<int64_t>(tls.Swap(reinterpret_cast<void*>(3))), 2);
}

TEST_F(ThreadLocalTest, Scrape) {
  auto unref = [](void* ptr) {
    auto& p = *static_cast<Params*>(ptr);
    p.mu->Lock();
    ++(*p.unref);
    p.mu->Unlock();
  };

  auto func = [](void* ptr) {
    auto& p = *static_cast<Params*>(ptr);

    ASSERT_TRUE(p.tls1.Get() == nullptr);
    ASSERT_TRUE(p.tls2->Get() == nullptr);

    p.tls1.Reset(ptr);
    p.tls2->Reset(ptr);

    p.tls1.Reset(ptr);
    p.tls2->Reset(ptr);

    p.mu->Lock();
    ++(p.completed);
    p.cv->SignalAll();

//等待指令退出线程
    while (p.completed != 0) {
      p.cv->Wait();
    }
    p.mu->Unlock();
  };

  for (int th = 1; th <= 128; th += th) {
    port::Mutex mu;
    port::CondVar cv(&mu);
    int unref_count = 0;
    Params p(&mu, &cv, &unref_count, th, unref);
    p.tls2 = new ThreadLocalPtr(unref);

    for (int i = 0; i < p.total; ++i) {
      env_->StartThread(func, static_cast<void*>(&p));
    }

//等待所有线程使用参数完成
    mu.Lock();
    while (p.completed != p.total) {
      cv.Wait();
    }
    mu.Unlock();

    ASSERT_EQ(unref_count, 0);

//清除所有线程本地数据。线程上没有UNREF
//退出或线程本地指针销毁
    autovector<void*> ptrs;
    p.tls1.Scrape(&ptrs, nullptr);
    p.tls2->Scrape(&ptrs, nullptr);
    delete p.tls2;
//退出信号
    mu.Lock();
    p.completed = 0;
    cv.SignalAll();
    mu.Unlock();
    env_->WaitForJoin();

    ASSERT_EQ(unref_count, 0);
  }
}

TEST_F(ThreadLocalTest, Fold) {
  auto unref = [](void* ptr) {
    delete static_cast<std::atomic<int64_t>*>(ptr);
  };
  static const int kNumThreads = 16;
  static const int kItersPerThread = 10;
  port::Mutex mu;
  port::CondVar cv(&mu);
  Params params(&mu, &cv, nullptr, kNumThreads, unref);
  auto func = [](void* ptr) {
    auto& p = *static_cast<Params*>(ptr);
    ASSERT_TRUE(p.tls1.Get() == nullptr);
    p.tls1.Reset(new std::atomic<int64_t>(0));

    for (int i = 0; i < kItersPerThread; ++i) {
      static_cast<std::atomic<int64_t>*>(p.tls1.Get())->fetch_add(1);
    }

    p.mu->Lock();
    ++(p.completed);
    p.cv->SignalAll();

//等待指令退出线程
    while (p.completed != 0) {
      p.cv->Wait();
    }
    p.mu->Unlock();
  };

  for (int th = 0; th < params.total; ++th) {
    env_->StartThread(func, static_cast<void*>(&params));
  }

//等待所有线程使用参数完成
  mu.Lock();
  while (params.completed != params.total) {
    cv.Wait();
  }
  mu.Unlock();

//验证fold（）行为
  int64_t sum = 0;
  params.tls1.Fold(
      [](void* ptr, void* res) {
        auto sum_ptr = static_cast<int64_t*>(res);
        *sum_ptr += static_cast<std::atomic<int64_t>*>(ptr)->load();
      },
      &sum);
  ASSERT_EQ(sum, kNumThreads * kItersPerThread);

//退出信号
  mu.Lock();
  params.completed = 0;
  cv.SignalAll();
  mu.Unlock();
  env_->WaitForJoin();
}

TEST_F(ThreadLocalTest, CompareAndSwap) {
  ThreadLocalPtr tls;
  ASSERT_TRUE(tls.Swap(reinterpret_cast<void*>(1)) == nullptr);
  void* expected = reinterpret_cast<void*>(1);
//2互换
  ASSERT_TRUE(tls.CompareAndSwap(reinterpret_cast<void*>(2), expected));
  expected = reinterpret_cast<void*>(100);
//交换失败，仍为2
  ASSERT_TRUE(!tls.CompareAndSwap(reinterpret_cast<void*>(2), expected));
  ASSERT_EQ(expected, reinterpret_cast<void*>(2));
//3互换
  expected = reinterpret_cast<void*>(2);
  ASSERT_TRUE(tls.CompareAndSwap(reinterpret_cast<void*>(3), expected));
  ASSERT_EQ(tls.Get(), reinterpret_cast<void*>(3));
}

namespace {

void* AccessThreadLocal(void* arg) {
  TEST_SYNC_POINT("AccessThreadLocal:Start");
  ThreadLocalPtr tlp;
  tlp.Reset(new std::string("hello RocksDB"));
  TEST_SYNC_POINT("AccessThreadLocal:End");
  return nullptr;
}

}  //命名空间

//以下测试被禁用，因为运行它需要手动步骤
//正确地。
//
//目前，我们无法访问同步点而不访问asan错误，当
//子线程在主线程死后死亡。因此，如果手动启用
//此测试只在同步点上看到asan错误，这意味着您通过了
//测试。
TEST_F(ThreadLocalTest, DISABLED_MainThreadDiesFirst) {
  rocksdb::SyncPoint::GetInstance()->LoadDependency(
      {{"AccessThreadLocal:Start", "MainThreadDiesFirst:End"},
       {"PosixEnv::~PosixEnv():End", "AccessThreadLocal:End"}});

//触发单例初始化。
  Env::Default();

#ifndef ROCKSDB_LITE
  try {
#endif  //摇滚乐
    rocksdb::port::Thread th(&AccessThreadLocal, nullptr);
    th.detach();
    TEST_SYNC_POINT("MainThreadDiesFirst:End");
#ifndef ROCKSDB_LITE
  } catch (const std::system_error& ex) {
    std::cerr << "Start thread: " << ex.code() << std::endl;
    FAIL();
  }
#endif  //摇滚乐
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
