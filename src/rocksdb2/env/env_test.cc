
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

#ifndef OS_WIN
#include <sys/ioctl.h>
#endif

#ifdef ROCKSDB_MALLOC_USABLE_SIZE
#ifdef OS_FREEBSD
#include <malloc_np.h>
#else
#include <malloc.h>
#endif
#endif
#include <sys/types.h>

#include <iostream>
#include <unordered_set>
#include <atomic>
#include <list>

#ifdef OS_LINUX
#include <fcntl.h>
#include <linux/fs.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef ROCKSDB_FALLOCATE_PRESENT
#include <errno.h>
#endif

#include "env/env_chroot.h"
#include "port/port.h"
#include "rocksdb/env.h"
#include "util/coding.h"
#include "util/log_buffer.h"
#include "util/mutexlock.h"
#include "util/string_util.h"
#include "util/sync_point.h"
#include "util/testharness.h"
#include "util/testutil.h"

#ifdef OS_LINUX
static const size_t kPageSize = sysconf(_SC_PAGESIZE);
#else
static const size_t kPageSize = 4 * 1024;
#endif

namespace rocksdb {

static const int kDelayMicros = 100000;

struct Deleter {
  explicit Deleter(void (*fn)(void*)) : fn_(fn) {}

  void operator()(void* ptr) {
    assert(fn_);
    assert(ptr);
    (*fn_)(ptr);
  }

  void (*fn_)(void*);
};

std::unique_ptr<char, Deleter> NewAligned(const size_t size, const char ch) {
  char* ptr = nullptr;
#ifdef OS_WIN
  if (!(ptr = reinterpret_cast<char*>(_aligned_malloc(size, kPageSize)))) {
    return std::unique_ptr<char, Deleter>(nullptr, Deleter(_aligned_free));
  }
  std::unique_ptr<char, Deleter> uptr(ptr, Deleter(_aligned_free));
#else
  if (posix_memalign(reinterpret_cast<void**>(&ptr), kPageSize, size) != 0) {
    return std::unique_ptr<char, Deleter>(nullptr, Deleter(free));
  }
  std::unique_ptr<char, Deleter> uptr(ptr, Deleter(free));
#endif
  memset(uptr.get(), ch, size);
  return uptr;
}

class EnvPosixTest : public testing::Test {
 private:
  port::Mutex mu_;
  std::string events_;

 public:
  Env* env_;
  bool direct_io_;
  EnvPosixTest() : env_(Env::Default()), direct_io_(false) {}
};

class EnvPosixTestWithParam
    : public EnvPosixTest,
      public ::testing::WithParamInterface<std::pair<Env*, bool>> {
 public:
  EnvPosixTestWithParam() {
    std::pair<Env*, bool> param_pair = GetParam();
    env_ = param_pair.first;
    direct_io_ = param_pair.second;
  }

  void WaitThreadPoolsEmpty() {
//等待线程池为空。
    while (env_->GetThreadPoolQueueLen(Env::Priority::LOW) != 0) {
      Env::Default()->SleepForMicroseconds(kDelayMicros);
    }
    while (env_->GetThreadPoolQueueLen(Env::Priority::HIGH) != 0) {
      Env::Default()->SleepForMicroseconds(kDelayMicros);
    }
  }

  ~EnvPosixTestWithParam() { WaitThreadPoolsEmpty(); }
};

static void SetBool(void* ptr) {
  reinterpret_cast<std::atomic<bool>*>(ptr)->store(true);
}

TEST_F(EnvPosixTest, RunImmediately) {
  for (int pri = Env::BOTTOM; pri < Env::TOTAL; ++pri) {
    std::atomic<bool> called(false);
    env_->SetBackgroundThreads(1, static_cast<Env::Priority>(pri));
    env_->Schedule(&SetBool, &called, static_cast<Env::Priority>(pri));
    Env::Default()->SleepForMicroseconds(kDelayMicros);
    ASSERT_TRUE(called.load());
  }
}

TEST_P(EnvPosixTestWithParam, UnSchedule) {
  std::atomic<bool> called(false);
  env_->SetBackgroundThreads(1, Env::LOW);

  /*阻塞低优先级队列*/
  test::SleepingBackgroundTask sleeping_task, sleeping_task1;
  env_->Schedule(&test::SleepingBackgroundTask::DoSleepTask, &sleeping_task,
                 Env::Priority::LOW);

  /*安排其他任务*/
  env_->Schedule(&test::SleepingBackgroundTask::DoSleepTask, &sleeping_task1,
                 Env::Priority::LOW, &sleeping_task1);

  /*用另一个标签删除它*/
  ASSERT_EQ(0, env_->UnSchedule(&called, Env::Priority::LOW));

  /*用正确的标签将其从队列中删除*/
  ASSERT_EQ(1, env_->UnSchedule(&sleeping_task1, Env::Priority::LOW));

//取消阻止后台线程
  sleeping_task.WakeUp();

  /*安排其他任务*/
  env_->Schedule(&SetBool, &called);
  for (int i = 0; i < kDelayMicros; i++) {
    if (called.load()) {
      break;
    }
    Env::Default()->SleepForMicroseconds(1);
  }
  ASSERT_TRUE(called.load());

  ASSERT_TRUE(!sleeping_task.IsSleeping() && !sleeping_task1.IsSleeping());
  WaitThreadPoolsEmpty();
}

TEST_P(EnvPosixTestWithParam, RunMany) {
  std::atomic<int> last_id(0);

  struct CB {
std::atomic<int>* last_id_ptr;  //指向共享插槽的指针
int id;                         //执行此回调的顺序

    CB(std::atomic<int>* p, int i) : last_id_ptr(p), id(i) {}

    static void Run(void* v) {
      CB* cb = reinterpret_cast<CB*>(v);
      int cur = cb->last_id_ptr->load();
      ASSERT_EQ(cb->id - 1, cur);
      cb->last_id_ptr->store(cb->id);
    }
  };

//以不同于开始时间的顺序安排
  CB cb1(&last_id, 1);
  CB cb2(&last_id, 2);
  CB cb3(&last_id, 3);
  CB cb4(&last_id, 4);
  env_->Schedule(&CB::Run, &cb1);
  env_->Schedule(&CB::Run, &cb2);
  env_->Schedule(&CB::Run, &cb3);
  env_->Schedule(&CB::Run, &cb4);

  Env::Default()->SleepForMicroseconds(kDelayMicros);
  int cur = last_id.load(std::memory_order_acquire);
  ASSERT_EQ(4, cur);
  WaitThreadPoolsEmpty();
}

struct State {
  port::Mutex mu;
  int val;
  int num_running;
};

static void ThreadBody(void* arg) {
  State* s = reinterpret_cast<State*>(arg);
  s->mu.Lock();
  s->val += 1;
  s->num_running -= 1;
  s->mu.Unlock();
}

TEST_P(EnvPosixTestWithParam, StartThread) {
  State state;
  state.val = 0;
  state.num_running = 3;
  for (int i = 0; i < 3; i++) {
    env_->StartThread(&ThreadBody, &state);
  }
  while (true) {
    state.mu.Lock();
    int num = state.num_running;
    state.mu.Unlock();
    if (num == 0) {
      break;
    }
    Env::Default()->SleepForMicroseconds(kDelayMicros);
  }
  ASSERT_EQ(state.val, 3);
  WaitThreadPoolsEmpty();
}

TEST_P(EnvPosixTestWithParam, TwoPools) {
//向要运行的任务发出信号的数据结构。
  port::Mutex mutex;
  port::CondVar cv(&mutex);
  bool should_start = false;

  class CB {
   public:
    CB(const std::string& pool_name, int pool_size, port::Mutex* trigger_mu,
       port::CondVar* trigger_cv, bool* _should_start)
        : mu_(),
          num_running_(0),
          num_finished_(0),
          pool_size_(pool_size),
          pool_name_(pool_name),
          trigger_mu_(trigger_mu),
          trigger_cv_(trigger_cv),
          should_start_(_should_start) {}

    static void Run(void* v) {
      CB* cb = reinterpret_cast<CB*>(v);
      cb->Run();
    }

    void Run() {
      {
        MutexLock l(&mu_);
        num_running_++;
//确保我们没有超过池大小的作业在运行。
        ASSERT_LE(num_running_, pool_size_.load());
      }

      {
        MutexLock l(trigger_mu_);
        while (!(*should_start_)) {
          trigger_cv_->Wait();
        }
      }

      {
        MutexLock l(&mu_);
        num_running_--;
        num_finished_++;
      }
    }

    int NumFinished() {
      MutexLock l(&mu_);
      return num_finished_;
    }

    void Reset(int pool_size) {
      pool_size_.store(pool_size);
      num_finished_ = 0;
    }

   private:
    port::Mutex mu_;
    int num_running_;
    int num_finished_;
    std::atomic<int> pool_size_;
    std::string pool_name_;
    port::Mutex* trigger_mu_;
    port::CondVar* trigger_cv_;
    bool* should_start_;
  };

  const int kLowPoolSize = 2;
  const int kHighPoolSize = 4;
  const int kJobs = 8;

  CB low_pool_job("low", kLowPoolSize, &mutex, &cv, &should_start);
  CB high_pool_job("high", kHighPoolSize, &mutex, &cv, &should_start);

  env_->SetBackgroundThreads(kLowPoolSize);
  env_->SetBackgroundThreads(kHighPoolSize, Env::Priority::HIGH);

  ASSERT_EQ(0U, env_->GetThreadPoolQueueLen(Env::Priority::LOW));
  ASSERT_EQ(0U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));

//在每个池中安排相同数量的作业
  for (int i = 0; i < kJobs; i++) {
    env_->Schedule(&CB::Run, &low_pool_job);
    env_->Schedule(&CB::Run, &high_pool_job, Env::Priority::HIGH);
  }
//请稍等片刻，以便分派作业。
  int sleep_count = 0;
  while ((unsigned int)(kJobs - kLowPoolSize) !=
             env_->GetThreadPoolQueueLen(Env::Priority::LOW) ||
         (unsigned int)(kJobs - kHighPoolSize) !=
             env_->GetThreadPoolQueueLen(Env::Priority::HIGH)) {
    env_->SleepForMicroseconds(kDelayMicros);
    if (++sleep_count > 100) {
      break;
    }
  }

  ASSERT_EQ((unsigned int)(kJobs - kLowPoolSize),
            env_->GetThreadPoolQueueLen());
  ASSERT_EQ((unsigned int)(kJobs - kLowPoolSize),
            env_->GetThreadPoolQueueLen(Env::Priority::LOW));
  ASSERT_EQ((unsigned int)(kJobs - kHighPoolSize),
            env_->GetThreadPoolQueueLen(Env::Priority::HIGH));

//触发要运行的作业。
  {
    MutexLock l(&mutex);
    should_start = true;
    cv.SignalAll();
  }

//等待所有作业完成
  while (low_pool_job.NumFinished() < kJobs ||
         high_pool_job.NumFinished() < kJobs) {
    env_->SleepForMicroseconds(kDelayMicros);
  }

  ASSERT_EQ(0U, env_->GetThreadPoolQueueLen(Env::Priority::LOW));
  ASSERT_EQ(0U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));

//按计划安排工作；
  should_start = false;

//如果需要两个池，请调用incBackgroundThreads。一个增加的和
//另一个下降
  env_->IncBackgroundThreadsIfNeeded(kLowPoolSize - 1, Env::Priority::LOW);
  env_->IncBackgroundThreadsIfNeeded(kHighPoolSize + 1, Env::Priority::HIGH);
  high_pool_job.Reset(kHighPoolSize + 1);
  low_pool_job.Reset(kLowPoolSize);

//在每个池中安排相同数量的作业
  for (int i = 0; i < kJobs; i++) {
    env_->Schedule(&CB::Run, &low_pool_job);
    env_->Schedule(&CB::Run, &high_pool_job, Env::Priority::HIGH);
  }
//请稍等片刻，以便分派作业。
  sleep_count = 0;
  while ((unsigned int)(kJobs - kLowPoolSize) !=
             env_->GetThreadPoolQueueLen(Env::Priority::LOW) ||
         (unsigned int)(kJobs - (kHighPoolSize + 1)) !=
             env_->GetThreadPoolQueueLen(Env::Priority::HIGH)) {
    env_->SleepForMicroseconds(kDelayMicros);
    if (++sleep_count > 100) {
      break;
    }
  }
  ASSERT_EQ((unsigned int)(kJobs - kLowPoolSize),
            env_->GetThreadPoolQueueLen());
  ASSERT_EQ((unsigned int)(kJobs - kLowPoolSize),
            env_->GetThreadPoolQueueLen(Env::Priority::LOW));
  ASSERT_EQ((unsigned int)(kJobs - (kHighPoolSize + 1)),
            env_->GetThreadPoolQueueLen(Env::Priority::HIGH));

//触发要运行的作业。
  {
    MutexLock l(&mutex);
    should_start = true;
    cv.SignalAll();
  }

//等待所有作业完成
  while (low_pool_job.NumFinished() < kJobs ||
         high_pool_job.NumFinished() < kJobs) {
    env_->SleepForMicroseconds(kDelayMicros);
  }

  env_->SetBackgroundThreads(kHighPoolSize, Env::Priority::HIGH);
  WaitThreadPoolsEmpty();
}

TEST_P(EnvPosixTestWithParam, DecreaseNumBgThreads) {
  std::vector<test::SleepingBackgroundTask> tasks(10);

//首先将线程数设置为1。
  env_->SetBackgroundThreads(1, Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);

//安排3项任务。0正在运行；任务1、2正在等待。
  for (size_t i = 0; i < 3; i++) {
    env_->Schedule(&test::SleepingBackgroundTask::DoSleepTask, &tasks[i],
                   Env::Priority::HIGH);
    Env::Default()->SleepForMicroseconds(kDelayMicros);
  }
  ASSERT_EQ(2U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  ASSERT_TRUE(tasks[0].IsSleeping());
  ASSERT_TRUE(!tasks[1].IsSleeping());
  ASSERT_TRUE(!tasks[2].IsSleeping());

//增加到2个螺纹。任务0，1正在运行；2正在等待
  env_->SetBackgroundThreads(2, Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_EQ(1U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  ASSERT_TRUE(tasks[0].IsSleeping());
  ASSERT_TRUE(tasks[1].IsSleeping());
  ASSERT_TRUE(!tasks[2].IsSleeping());

//缩回到1个螺纹。静止任务0，1正在运行，2正在等待
  env_->SetBackgroundThreads(1, Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_EQ(1U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  ASSERT_TRUE(tasks[0].IsSleeping());
  ASSERT_TRUE(tasks[1].IsSleeping());
  ASSERT_TRUE(!tasks[2].IsSleeping());

//最后一项任务完成。任务0正在运行，2正在等待。
  tasks[1].WakeUp();
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_EQ(1U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  ASSERT_TRUE(tasks[0].IsSleeping());
  ASSERT_TRUE(!tasks[1].IsSleeping());
  ASSERT_TRUE(!tasks[2].IsSleeping());

//增加到5个螺纹。任务0和2正在运行。
  env_->SetBackgroundThreads(5, Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_EQ((unsigned int)0, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  ASSERT_TRUE(tasks[0].IsSleeping());
  ASSERT_TRUE(tasks[2].IsSleeping());

//在没有足够的线程数的情况下，将线程数更改几次
//任务。
  env_->SetBackgroundThreads(7, Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  tasks[2].WakeUp();
  ASSERT_EQ(0U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  env_->SetBackgroundThreads(3, Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_EQ(0U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  env_->SetBackgroundThreads(4, Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_EQ(0U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  env_->SetBackgroundThreads(5, Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_EQ(0U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  env_->SetBackgroundThreads(4, Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_EQ(0U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));

  Env::Default()->SleepForMicroseconds(kDelayMicros * 50);

//再排队5个任务。线程池大小现在为4。
//任务0、3、4、5正在运行；6、7正在等待。
  for (size_t i = 3; i < 8; i++) {
    env_->Schedule(&test::SleepingBackgroundTask::DoSleepTask, &tasks[i],
                   Env::Priority::HIGH);
  }
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_EQ(2U, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  ASSERT_TRUE(tasks[3].IsSleeping());
  ASSERT_TRUE(tasks[4].IsSleeping());
  ASSERT_TRUE(tasks[5].IsSleeping());
  ASSERT_TRUE(!tasks[6].IsSleeping());
  ASSERT_TRUE(!tasks[7].IsSleeping());

//唤醒任务0、3和4。任务5、6、7运行。
  tasks[0].WakeUp();
  tasks[3].WakeUp();
  tasks[4].WakeUp();

  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_EQ((unsigned int)0, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  for (size_t i = 5; i < 8; i++) {
    ASSERT_TRUE(tasks[i].IsSleeping());
  }

//缩回到1个螺纹。仍在运行任务5、6、7
  env_->SetBackgroundThreads(1, Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_TRUE(tasks[5].IsSleeping());
  ASSERT_TRUE(tasks[6].IsSleeping());
  ASSERT_TRUE(tasks[7].IsSleeping());

//唤醒任务6。任务5，7运行
  tasks[6].WakeUp();
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_TRUE(tasks[5].IsSleeping());
  ASSERT_TRUE(!tasks[6].IsSleeping());
  ASSERT_TRUE(tasks[7].IsSleeping());

//唤醒线程7。任务5运行
  tasks[7].WakeUp();
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_TRUE(!tasks[7].IsSleeping());

//使线程8和9排队。任务5正在运行；8、9中的一个可能正在运行。
  env_->Schedule(&test::SleepingBackgroundTask::DoSleepTask, &tasks[8],
                 Env::Priority::HIGH);
  env_->Schedule(&test::SleepingBackgroundTask::DoSleepTask, &tasks[9],
                 Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_GT(env_->GetThreadPoolQueueLen(Env::Priority::HIGH), (unsigned int)0);
  ASSERT_TRUE(!tasks[8].IsSleeping() || !tasks[9].IsSleeping());

//增加到4个螺纹。任务5、8、9运行。
  env_->SetBackgroundThreads(4, Env::Priority::HIGH);
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_EQ((unsigned int)0, env_->GetThreadPoolQueueLen(Env::Priority::HIGH));
  ASSERT_TRUE(tasks[8].IsSleeping());
  ASSERT_TRUE(tasks[9].IsSleeping());

//收缩到1个螺纹
  env_->SetBackgroundThreads(1, Env::Priority::HIGH);

//唤醒线程9。
  tasks[9].WakeUp();
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_TRUE(!tasks[9].IsSleeping());
  ASSERT_TRUE(tasks[8].IsSleeping());

//唤醒线程8
  tasks[8].WakeUp();
  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_TRUE(!tasks[8].IsSleeping());

//唤醒最后一个线程
  tasks[5].WakeUp();

  Env::Default()->SleepForMicroseconds(kDelayMicros);
  ASSERT_TRUE(!tasks[5].IsSleeping());
  WaitThreadPoolsEmpty();
}

#if (defined OS_LINUX || defined OS_WIN)
//Travis不支持任何文件的fallocate或获取唯一ID
//原因。
#ifndef TRAVIS

namespace {
bool IsSingleVarint(const std::string& s) {
  Slice slice(s);

  uint64_t v;
  if (!GetVarint64(&slice, &v)) {
    return false;
  }

  return slice.size() == 0;
}

bool IsUniqueIDValid(const std::string& s) {
  return !s.empty() && !IsSingleVarint(s);
}

const size_t MAX_ID_SIZE = 100;
char temp_id[MAX_ID_SIZE];


}  //命名空间

//确定我们是否可以使用fs-ioc-getversion ioctl
//在目录dir中的文件上。在其中创建临时文件，
//尝试应用ioctl（保存结果）、cleanup和
//返回结果。如果支持，则返回true，并且
//如果失败，则返回false。
//注意，这个函数“知道”刚刚创建了dir
//并且是空的，所以我们创建一个简单命名的测试文件：“f”。
bool ioctl_support__FS_IOC_GETVERSION(const std::string& dir) {
#ifdef OS_WIN
  return true;
#else
  const std::string file = dir + "/f";
  int fd;
  do {
    fd = open(file.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
  } while (fd < 0 && errno == EINTR);
  long int version;
  bool ok = (fd >= 0 && ioctl(fd, FS_IOC_GETVERSION, &version) >= 0);

  close(fd);
  unlink(file.c_str());

  return ok;
#endif
}

//为了确保env:：getuniqueid相关测试正常工作，文件
//应存储在常规存储中，如“硬盘”或“闪存设备”，
//而不是在tmpfs文件系统上（如某些系统上的/dev/shm和/tmp）。
//否则，我们无法获得正确的ID。
//
//此函数用于替换test:：tmpdir（），它可能是
//自定义为位于不适用于getUniqueID（）的文件系统上。

class IoctlFriendlyTmpdir {
 public:
  explicit IoctlFriendlyTmpdir() {
    char dir_buf[100];

    const char *fmt = "%s/rocksdb.XXXXXX";
    const char *tmp = getenv("TEST_IOCTL_FRIENDLY_TMPDIR");

#ifdef OS_WIN
#define rmdir _rmdir
    if(tmp == nullptr) {
      tmp = getenv("TMP");
    }

    snprintf(dir_buf, sizeof dir_buf, fmt, tmp);
    auto result = _mktemp(dir_buf);
    assert(result != nullptr);
    BOOL ret = CreateDirectory(dir_buf, NULL);
    assert(ret == TRUE);
    dir_ = dir_buf;
#else
    std::list<std::string> candidate_dir_list = {"/var/tmp", "/tmp"};

//如果$test-ioctl-friendly-tmpdir/rocksdb.xxxx适合，使用
//$test_ioctl_friendly_tmpdir；将“%s”减去2，然后
//为尾随的nul字节添加1。
    if (tmp && strlen(tmp) + strlen(fmt) - 2 + 1 <= sizeof dir_buf) {
//使用$test_ioctl_friendly_tmpdir值
      candidate_dir_list.push_front(tmp);
    }

    for (const std::string& d : candidate_dir_list) {
      snprintf(dir_buf, sizeof dir_buf, fmt, d.c_str());
      if (mkdtemp(dir_buf)) {
        if (ioctl_support__FS_IOC_GETVERSION(dir_buf)) {
          dir_ = dir_buf;
          return;
        } else {
//仅在以下情况下诊断IOCTL相关故障：
//通过该envvar指定的目录。
          if (tmp && tmp == d) {
            fprintf(stderr, "TEST_IOCTL_FRIENDLY_TMPDIR-specified directory is "
                    "not suitable: %s\n", d.c_str());
          }
rmdir(dir_buf);  //忽略失败
        }
      } else {
//mkdtemp失败：诊断它，但不要放弃。
        fprintf(stderr, "mkdtemp(%s/...) failed: %s\n", d.c_str(),
                strerror(errno));
      }
    }

    fprintf(stderr, "failed to find an ioctl-friendly temporary directory;"
            " specify one via the TEST_IOCTL_FRIENDLY_TMPDIR envvar\n");
    std::abort();
#endif
}

  ~IoctlFriendlyTmpdir() {
    rmdir(dir_.c_str());
  }

  const std::string& name() const {
    return dir_;
  }

 private:
  std::string dir_;
};

#ifndef ROCKSDB_LITE
TEST_F(EnvPosixTest, PositionedAppend) {
  unique_ptr<WritableFile> writable_file;
  EnvOptions options;
  options.use_direct_writes = true;
  options.use_mmap_writes = false;
  IoctlFriendlyTmpdir ift;
  ASSERT_OK(env_->NewWritableFile(ift.name() + "/f", &writable_file, options));
  const size_t kBlockSize = 4096;
  const size_t kPageSize = 4096;
  const size_t kDataSize = kPageSize;
//写一页“A”
  auto data_ptr = NewAligned(kDataSize, 'a');
  Slice data_a(data_ptr.get(), kDataSize);
  ASSERT_OK(writable_file->PositionedAppend(data_a, 0U));
//在第一个扇区后写一个值为“B”的页面
  data_ptr = NewAligned(kDataSize, 'b');
  Slice data_b(data_ptr.get(), kDataSize);
  ASSERT_OK(writable_file->PositionedAppend(data_b, kBlockSize));
  ASSERT_OK(writable_file->Close());
//文件现在有1个扇区值A，后面是一个页面值B。

//验证以上内容
  unique_ptr<SequentialFile> seq_file;
  ASSERT_OK(env_->NewSequentialFile(ift.name() + "/f", &seq_file, options));
  char scratch[kPageSize * 2];
  Slice result;
  ASSERT_OK(seq_file->Read(sizeof(scratch), &result, scratch));
  ASSERT_EQ(kPageSize + kBlockSize, result.size());
  ASSERT_EQ('a', result[kBlockSize - 1]);
  ASSERT_EQ('b', result[kBlockSize]);
}
#endif  //！摇滚乐

//仅适用于Linux平台
TEST_P(EnvPosixTestWithParam, RandomAccessUniqueID) {
//创建文件。
  if (env_ == Env::Default()) {
    EnvOptions soptions;
    soptions.use_direct_reads = soptions.use_direct_writes = direct_io_;
    IoctlFriendlyTmpdir ift;
    std::string fname = ift.name() + "/testfile";
    unique_ptr<WritableFile> wfile;
    ASSERT_OK(env_->NewWritableFile(fname, &wfile, soptions));

    unique_ptr<RandomAccessFile> file;

//获取唯一ID
    ASSERT_OK(env_->NewRandomAccessFile(fname, &file, soptions));
    size_t id_size = file->GetUniqueId(temp_id, MAX_ID_SIZE);
    ASSERT_TRUE(id_size > 0);
    std::string unique_id1(temp_id, id_size);
    ASSERT_TRUE(IsUniqueIDValid(unique_id1));

//再次获取唯一ID
    ASSERT_OK(env_->NewRandomAccessFile(fname, &file, soptions));
    id_size = file->GetUniqueId(temp_id, MAX_ID_SIZE);
    ASSERT_TRUE(id_size > 0);
    std::string unique_id2(temp_id, id_size);
    ASSERT_TRUE(IsUniqueIDValid(unique_id2));

//等待一段时间后再次获取唯一ID。
    env_->SleepForMicroseconds(1000000);
    ASSERT_OK(env_->NewRandomAccessFile(fname, &file, soptions));
    id_size = file->GetUniqueId(temp_id, MAX_ID_SIZE);
    ASSERT_TRUE(id_size > 0);
    std::string unique_id3(temp_id, id_size);
    ASSERT_TRUE(IsUniqueIDValid(unique_id3));

//检查ID是否相同。
    ASSERT_EQ(unique_id1, unique_id2);
    ASSERT_EQ(unique_id2, unique_id3);

//删除文件
    env_->DeleteFile(fname);
  }
}

//仅适用于Linux平台
#ifdef ROCKSDB_FALLOCATE_PRESENT
TEST_P(EnvPosixTestWithParam, AllocateTest) {
  if (env_ == Env::Default()) {
    IoctlFriendlyTmpdir ift;
    std::string fname = ift.name() + "/preallocate_testfile";

//尝试在文件中释放，以查看目标文件系统是否支持
//它。
//如果不支持fallocate，则跳过测试。
    std::string fname_test_fallocate = ift.name() + "/preallocate_testfile_2";
    int fd = -1;
    do {
      fd = open(fname_test_fallocate.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    } while (fd < 0 && errno == EINTR);
    ASSERT_GT(fd, 0);

    int alloc_status = fallocate(fd, 0, 0, 1);

    int err_number = 0;
    if (alloc_status != 0) {
      err_number = errno;
      fprintf(stderr, "Warning: fallocate() fails, %s\n", strerror(err_number));
    }
    close(fd);
    ASSERT_OK(env_->DeleteFile(fname_test_fallocate));
    if (alloc_status != 0 && err_number == EOPNOTSUPP) {
//包含该文件的文件系统不支持fallocate
      return;
    }

    EnvOptions soptions;
    soptions.use_mmap_writes = false;
    soptions.use_direct_reads = soptions.use_direct_writes = direct_io_;
    unique_ptr<WritableFile> wfile;
    ASSERT_OK(env_->NewWritableFile(fname, &wfile, soptions));

//分配100兆字节
    size_t kPreallocateSize = 100 * 1024 * 1024;
    size_t kBlockSize = 512;
    size_t kPageSize = 4096;
    size_t kDataSize = 1024 * 1024;
    auto data_ptr = NewAligned(kDataSize, 'A');
    Slice data(data_ptr.get(), kDataSize);
    wfile->SetPreallocationBlockSize(kPreallocateSize);
    wfile->PrepareWrite(wfile->GetFileSize(), kDataSize);
    ASSERT_OK(wfile->Append(data));
    ASSERT_OK(wfile->Flush());

    struct stat f_stat;
    ASSERT_EQ(stat(fname.c_str(), &f_stat), 0);
    ASSERT_EQ((unsigned int)kDataSize, f_stat.st_size);
//验证块是否已预先分配
//请注意，我们不检查预先分配的块的确切数量--
//我们只要求分配的块的数量至少是我们
//期待。
//看起来有些FS给了我们更多的模块。那很好。
//这可能值得进一步调查。
    ASSERT_LE((unsigned int)(kPreallocateSize / kBlockSize), f_stat.st_blocks);

//关闭文件，如果释放块
    wfile.reset();

    stat(fname.c_str(), &f_stat);
    ASSERT_EQ((unsigned int)kDataSize, f_stat.st_size);
//验证是否在文件关闭时解除分配了预分配的块
//因为fs可能会给我们更多的块，所以我们添加了一个完整的页面
//并期望块的数目小于或等于这个数。
    ASSERT_GE((f_stat.st_size + kPageSize + kBlockSize - 1) / kBlockSize,
              (unsigned int)f_stat.st_blocks);
  }
}
#endif  //岩石出现

//如果ss中的任何字符串是另一个字符串的前缀，则返回true。
bool HasPrefix(const std::unordered_set<std::string>& ss) {
  for (const std::string& s: ss) {
    if (s.empty()) {
      return true;
    }
    for (size_t i = 1; i < s.size(); ++i) {
      if (ss.count(s.substr(0, i)) != 0) {
        return true;
      }
    }
  }
  return false;
}

//仅适用于Linux和Win平台
TEST_P(EnvPosixTestWithParam, RandomAccessUniqueIDConcurrent) {
  if (env_ == Env::Default()) {
//检查一组并发存在的文件是否具有唯一的ID。
    EnvOptions soptions;
    soptions.use_direct_reads = soptions.use_direct_writes = direct_io_;

//创建文件
    IoctlFriendlyTmpdir ift;
    std::vector<std::string> fnames;
    for (int i = 0; i < 1000; ++i) {
      fnames.push_back(ift.name() + "/" + "testfile" + ToString(i));

//创建文件。
      unique_ptr<WritableFile> wfile;
      ASSERT_OK(env_->NewWritableFile(fnames[i], &wfile, soptions));
    }

//收集并检查ID是否唯一。
    std::unordered_set<std::string> ids;
    for (const std::string fname : fnames) {
      unique_ptr<RandomAccessFile> file;
      std::string unique_id;
      ASSERT_OK(env_->NewRandomAccessFile(fname, &file, soptions));
      size_t id_size = file->GetUniqueId(temp_id, MAX_ID_SIZE);
      ASSERT_TRUE(id_size > 0);
      unique_id = std::string(temp_id, id_size);
      ASSERT_TRUE(IsUniqueIDValid(unique_id));

      ASSERT_TRUE(ids.count(unique_id) == 0);
      ids.insert(unique_id);
    }

//删除文件
    for (const std::string fname : fnames) {
      ASSERT_OK(env_->DeleteFile(fname));
    }

    ASSERT_TRUE(!HasPrefix(ids));
  }
}

//仅适用于Linux和Win平台
TEST_P(EnvPosixTestWithParam, RandomAccessUniqueIDDeletes) {
  if (env_ == Env::Default()) {
    EnvOptions soptions;
    soptions.use_direct_reads = soptions.use_direct_writes = direct_io_;

    IoctlFriendlyTmpdir ift;
    std::string fname = ift.name() + "/" + "testfile";

//检查删除文件后，在新的
//文件。
    std::unordered_set<std::string> ids;
    for (int i = 0; i < 1000; ++i) {
//创建文件。
      {
        unique_ptr<WritableFile> wfile;
        ASSERT_OK(env_->NewWritableFile(fname, &wfile, soptions));
      }

//获取唯一ID
      std::string unique_id;
      {
        unique_ptr<RandomAccessFile> file;
        ASSERT_OK(env_->NewRandomAccessFile(fname, &file, soptions));
        size_t id_size = file->GetUniqueId(temp_id, MAX_ID_SIZE);
        ASSERT_TRUE(id_size > 0);
        unique_id = std::string(temp_id, id_size);
      }

      ASSERT_TRUE(IsUniqueIDValid(unique_id));
      ASSERT_TRUE(ids.count(unique_id) == 0);
      ids.insert(unique_id);

//删除文件
      ASSERT_OK(env_->DeleteFile(fname));
    }

    ASSERT_TRUE(!HasPrefix(ids));
  }
}

//仅适用于Linux平台
#ifdef OS_WIN
TEST_P(EnvPosixTestWithParam, DISABLED_InvalidateCache) {
#else
TEST_P(EnvPosixTestWithParam, InvalidateCache) {
#endif
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();
    EnvOptions soptions;
    soptions.use_direct_reads = soptions.use_direct_writes = direct_io_;
    std::string fname = test::TmpDir(env_) + "/" + "testfile";

    const size_t kSectorSize = 512;
    auto data = NewAligned(kSectorSize, 0);
    Slice slice(data.get(), kSectorSize);

//创建文件。
    {
      unique_ptr<WritableFile> wfile;
#if !defined(OS_MACOSX) && !defined(OS_WIN) && !defined(OS_SOLARIS) && !defined(OS_AIX)
      if (soptions.use_direct_writes) {
        soptions.use_direct_writes = false;
      }
#endif
      ASSERT_OK(env_->NewWritableFile(fname, &wfile, soptions));
      ASSERT_OK(wfile->Append(slice));
      ASSERT_OK(wfile->InvalidateCache(0, 0));
      ASSERT_OK(wfile->Close());
    }

//随机读取
    {
      unique_ptr<RandomAccessFile> file;
      auto scratch = NewAligned(kSectorSize, 0);
      Slice result;
#if !defined(OS_MACOSX) && !defined(OS_WIN) && !defined(OS_SOLARIS) && !defined(OS_AIX)
      if (soptions.use_direct_reads) {
        soptions.use_direct_reads = false;
      }
#endif
      ASSERT_OK(env_->NewRandomAccessFile(fname, &file, soptions));
      ASSERT_OK(file->Read(0, kSectorSize, &result, scratch.get()));
      ASSERT_EQ(memcmp(scratch.get(), data.get(), kSectorSize), 0);
      ASSERT_OK(file->InvalidateCache(0, 11));
      ASSERT_OK(file->InvalidateCache(0, 0));
    }

//顺序读取
    {
      unique_ptr<SequentialFile> file;
      auto scratch = NewAligned(kSectorSize, 0);
      Slice result;
#if !defined(OS_MACOSX) && !defined(OS_WIN) && !defined(OS_SOLARIS) && !defined(OS_AIX)
      if (soptions.use_direct_reads) {
        soptions.use_direct_reads = false;
      }
#endif
      ASSERT_OK(env_->NewSequentialFile(fname, &file, soptions));
      if (file->use_direct_io()) {
        ASSERT_OK(file->PositionedRead(0, kSectorSize, &result, scratch.get()));
      } else {
        ASSERT_OK(file->Read(kSectorSize, &result, scratch.get()));
      }
      ASSERT_EQ(memcmp(scratch.get(), data.get(), kSectorSize), 0);
      ASSERT_OK(file->InvalidateCache(0, 11));
      ASSERT_OK(file->InvalidateCache(0, 0));
    }
//删除文件
    ASSERT_OK(env_->DeleteFile(fname));
  rocksdb::SyncPoint::GetInstance()->ClearTrace();
}
#endif  //不是特拉维斯
#endif  //操作系统Linux

class TestLogger : public Logger {
 public:
  using Logger::Logv;
  virtual void Logv(const char* format, va_list ap) override {
    log_count++;

    char new_format[550];
    std::fill_n(new_format, sizeof(new_format), '2');
    {
      va_list backup_ap;
      va_copy(backup_ap, ap);
      int n = vsnprintf(new_format, sizeof(new_format) - 1, format, backup_ap);
//额外信息48字节+分配的字节

//当我们有n=-1时，不需要终止零
#ifdef OS_WIN
      if (n < 0) {
        char_0_count++;
      }
#endif

      if (new_format[0] == '[') {
//[调试]
        ASSERT_TRUE(n <= 56 + (512 - static_cast<int>(sizeof(struct timeval))));
      } else {
        ASSERT_TRUE(n <= 48 + (512 - static_cast<int>(sizeof(struct timeval))));
      }
      va_end(backup_ap);
    }

    for (size_t i = 0; i < sizeof(new_format); i++) {
      if (new_format[i] == 'x') {
        char_x_count++;
      } else if (new_format[i] == '\0') {
        char_0_count++;
      }
    }
  }
  int log_count;
  int char_x_count;
  int char_0_count;
};

TEST_P(EnvPosixTestWithParam, LogBufferTest) {
  TestLogger test_logger;
  test_logger.SetInfoLogLevel(InfoLogLevel::INFO_LEVEL);
  test_logger.log_count = 0;
  test_logger.char_x_count = 0;
  test_logger.char_0_count = 0;
  LogBuffer log_buffer(InfoLogLevel::INFO_LEVEL, &test_logger);
  LogBuffer log_buffer_debug(DEBUG_LEVEL, &test_logger);

  char bytes200[200];
  std::fill_n(bytes200, sizeof(bytes200), '1');
  bytes200[sizeof(bytes200) - 1] = '\0';
  char bytes600[600];
  std::fill_n(bytes600, sizeof(bytes600), '1');
  bytes600[sizeof(bytes600) - 1] = '\0';
  char bytes9000[9000];
  std::fill_n(bytes9000, sizeof(bytes9000), '1');
  bytes9000[sizeof(bytes9000) - 1] = '\0';

  ROCKS_LOG_BUFFER(&log_buffer, "x%sx", bytes200);
  ROCKS_LOG_BUFFER(&log_buffer, "x%sx", bytes600);
  ROCKS_LOG_BUFFER(&log_buffer, "x%sx%sx%sx", bytes200, bytes200, bytes200);
  ROCKS_LOG_BUFFER(&log_buffer, "x%sx%sx", bytes200, bytes600);
  ROCKS_LOG_BUFFER(&log_buffer, "x%sx%sx", bytes600, bytes9000);

  ROCKS_LOG_BUFFER(&log_buffer_debug, "x%sx", bytes200);
  test_logger.SetInfoLogLevel(DEBUG_LEVEL);
  ROCKS_LOG_BUFFER(&log_buffer_debug, "x%sx%sx%sx", bytes600, bytes9000,
                   bytes200);

  ASSERT_EQ(0, test_logger.log_count);
  log_buffer.FlushBufferToLog();
  log_buffer_debug.FlushBufferToLog();
  ASSERT_EQ(6, test_logger.log_count);
  ASSERT_EQ(6, test_logger.char_0_count);
  ASSERT_EQ(10, test_logger.char_x_count);
}

class TestLogger2 : public Logger {
 public:
  explicit TestLogger2(size_t max_log_size) : max_log_size_(max_log_size) {}
  using Logger::Logv;
  virtual void Logv(const char* format, va_list ap) override {
    char new_format[2000];
    std::fill_n(new_format, sizeof(new_format), '2');
    {
      va_list backup_ap;
      va_copy(backup_ap, ap);
      int n = vsnprintf(new_format, sizeof(new_format) - 1, format, backup_ap);
//额外信息48字节+分配的字节
      ASSERT_TRUE(
          n <= 48 + static_cast<int>(max_log_size_ - sizeof(struct timeval)));
      ASSERT_TRUE(n > static_cast<int>(max_log_size_ - sizeof(struct timeval)));
      va_end(backup_ap);
    }
  }
  size_t max_log_size_;
};

TEST_P(EnvPosixTestWithParam, LogBufferMaxSizeTest) {
  char bytes9000[9000];
  std::fill_n(bytes9000, sizeof(bytes9000), '1');
  bytes9000[sizeof(bytes9000) - 1] = '\0';

  for (size_t max_log_size = 256; max_log_size <= 1024;
       max_log_size += 1024 - 256) {
    TestLogger2 test_logger(max_log_size);
    test_logger.SetInfoLogLevel(InfoLogLevel::INFO_LEVEL);
    LogBuffer log_buffer(InfoLogLevel::INFO_LEVEL, &test_logger);
    ROCKS_LOG_BUFFER_MAX_SZ(&log_buffer, max_log_size, "%s", bytes9000);
    log_buffer.FlushBufferToLog();
  }
}

TEST_P(EnvPosixTestWithParam, Preallocation) {
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();
    const std::string src = test::TmpDir(env_) + "/" + "testfile";
    unique_ptr<WritableFile> srcfile;
    EnvOptions soptions;
    soptions.use_direct_reads = soptions.use_direct_writes = direct_io_;
#if !defined(OS_MACOSX) && !defined(OS_WIN) && !defined(OS_SOLARIS) && !defined(OS_AIX)
    if (soptions.use_direct_writes) {
      rocksdb::SyncPoint::GetInstance()->SetCallBack(
          "NewWritableFile:O_DIRECT", [&](void* arg) {
            int* val = static_cast<int*>(arg);
            *val &= ~O_DIRECT;
          });
    }
#endif
    ASSERT_OK(env_->NewWritableFile(src, &srcfile, soptions));
    srcfile->SetPreallocationBlockSize(1024 * 1024);

//没有写入意味着没有预分配
    size_t block_size, last_allocated_block;
    srcfile->GetPreallocationStatus(&block_size, &last_allocated_block);
    ASSERT_EQ(last_allocated_block, 0UL);

//小写应该预先分配一个块
    size_t kStrSize = 4096;
    auto data = NewAligned(kStrSize, 'A');
    Slice str(data.get(), kStrSize);
    srcfile->PrepareWrite(srcfile->GetFileSize(), kStrSize);
    srcfile->Append(str);
    srcfile->GetPreallocationStatus(&block_size, &last_allocated_block);
    ASSERT_EQ(last_allocated_block, 1UL);

//写一个完整的预分配块，确保我们增加了两个。
    {
      auto buf_ptr = NewAligned(block_size, ' ');
      Slice buf(buf_ptr.get(), block_size);
      srcfile->PrepareWrite(srcfile->GetFileSize(), block_size);
      srcfile->Append(buf);
      srcfile->GetPreallocationStatus(&block_size, &last_allocated_block);
      ASSERT_EQ(last_allocated_block, 2UL);
    }

//一次再写五个块，确保我们在需要的地方。
    {
      auto buf_ptr = NewAligned(block_size * 5, ' ');
      Slice buf = Slice(buf_ptr.get(), block_size * 5);
      srcfile->PrepareWrite(srcfile->GetFileSize(), buf.size());
      srcfile->Append(buf);
      srcfile->GetPreallocationStatus(&block_size, &last_allocated_block);
      ASSERT_EQ(last_allocated_block, 7UL);
    }
  rocksdb::SyncPoint::GetInstance()->ClearTrace();
}

//测试获取子文件属性的两种方法（批量或
//个人）行为始终如一。
TEST_P(EnvPosixTestWithParam, ConsistentChildrenAttributes) {
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();
    EnvOptions soptions;
    soptions.use_direct_reads = soptions.use_direct_writes = direct_io_;
    const int kNumChildren = 10;

    std::string data;
    for (int i = 0; i < kNumChildren; ++i) {
      std::ostringstream oss;
      oss << test::TmpDir(env_) << "/testfile_" << i;
      const std::string path = oss.str();
      unique_ptr<WritableFile> file;
#if !defined(OS_MACOSX) && !defined(OS_WIN) && !defined(OS_SOLARIS) && !defined(OS_AIX)
      if (soptions.use_direct_writes) {
        rocksdb::SyncPoint::GetInstance()->SetCallBack(
            "NewWritableFile:O_DIRECT", [&](void* arg) {
              int* val = static_cast<int*>(arg);
              *val &= ~O_DIRECT;
            });
      }
#endif
      ASSERT_OK(env_->NewWritableFile(path, &file, soptions));
      auto buf_ptr = NewAligned(data.size(), 'T');
      Slice buf(buf_ptr.get(), data.size());
      file->Append(buf);
      data.append(std::string(4096, 'T'));
    }

    std::vector<Env::FileAttributes> file_attrs;
    ASSERT_OK(env_->GetChildrenFileAttributes(test::TmpDir(env_), &file_attrs));
    for (int i = 0; i < kNumChildren; ++i) {
      std::ostringstream oss;
      oss << "testfile_" << i;
      const std::string name = oss.str();
      const std::string path = test::TmpDir(env_) + "/" + name;

      auto file_attrs_iter = std::find_if(
          file_attrs.begin(), file_attrs.end(),
          [&name](const Env::FileAttributes& fm) { return fm.name == name; });
      ASSERT_TRUE(file_attrs_iter != file_attrs.end());
      uint64_t size;
      ASSERT_OK(env_->GetFileSize(path, &size));
      ASSERT_EQ(size, 4096 * i);
      ASSERT_EQ(size, file_attrs_iter->size_bytes);
    }
    rocksdb::SyncPoint::GetInstance()->ClearTrace();
}

//测试所有writablefilewrapper是否将所有调用转发到writablefile。
TEST_P(EnvPosixTestWithParam, WritableFileWrapper) {
  class Base : public WritableFile {
   public:
    mutable int *step_;

    void inc(int x) const {
      EXPECT_EQ(x, (*step_)++);
    }

    explicit Base(int* step) : step_(step) {
      inc(0);
    }

    Status Append(const Slice& data) override { inc(1); return Status::OK(); }
    Status Truncate(uint64_t size) override { return Status::OK(); }
    Status Close() override { inc(2); return Status::OK(); }
    Status Flush() override { inc(3); return Status::OK(); }
    Status Sync() override { inc(4); return Status::OK(); }
    Status Fsync() override { inc(5); return Status::OK(); }
    void SetIOPriority(Env::IOPriority pri) override { inc(6); }
    uint64_t GetFileSize() override { inc(7); return 0; }
    void GetPreallocationStatus(size_t* block_size,
                                size_t* last_allocated_block) override {
      inc(8);
    }
    size_t GetUniqueId(char* id, size_t max_size) const override {
      inc(9);
      return 0;
    }
    Status InvalidateCache(size_t offset, size_t length) override {
      inc(10);
      return Status::OK();
    }

   protected:
    Status Allocate(uint64_t offset, uint64_t len) override {
      inc(11);
      return Status::OK();
    }
    Status RangeSync(uint64_t offset, uint64_t nbytes) override {
      inc(12);
      return Status::OK();
    }

   public:
    ~Base() {
      inc(13);
    }
  };

  class Wrapper : public WritableFileWrapper {
   public:
    explicit Wrapper(WritableFile* target) : WritableFileWrapper(target) {}

    void CallProtectedMethods() {
      Allocate(0, 0);
      RangeSync(0, 0);
    }
  };

  int step = 0;

  {
    Base b(&step);
    Wrapper w(&b);
    w.Append(Slice());
    w.Close();
    w.Flush();
    w.Sync();
    w.Fsync();
    w.SetIOPriority(Env::IOPriority::IO_HIGH);
    w.GetFileSize();
    w.GetPreallocationStatus(nullptr, nullptr);
    w.GetUniqueId(nullptr, 0);
    w.InvalidateCache(0, 0);
    w.CallProtectedMethods();
  }

  EXPECT_EQ(14, step);
}

TEST_P(EnvPosixTestWithParam, PosixRandomRWFile) {
  const std::string path = test::TmpDir(env_) + "/random_rw_file";

  env_->DeleteFile(path);

  std::unique_ptr<RandomRWFile> file;
  ASSERT_OK(env_->NewRandomRWFile(path, &file, EnvOptions()));

  char buf[10000];
  Slice read_res;

  ASSERT_OK(file->Write(0, "ABCD"));
  ASSERT_OK(file->Read(0, 10, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "ABCD");

  ASSERT_OK(file->Write(2, "XXXX"));
  ASSERT_OK(file->Read(0, 10, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "ABXXXX");

  ASSERT_OK(file->Write(10, "ZZZ"));
  ASSERT_OK(file->Read(10, 10, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "ZZZ");

  ASSERT_OK(file->Write(11, "Y"));
  ASSERT_OK(file->Read(10, 10, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "ZYZ");

  ASSERT_OK(file->Write(200, "FFFFF"));
  ASSERT_OK(file->Read(200, 10, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "FFFFF");

  ASSERT_OK(file->Write(205, "XXXX"));
  ASSERT_OK(file->Read(200, 10, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "FFFFFXXXX");

  ASSERT_OK(file->Write(5, "QQQQ"));
  ASSERT_OK(file->Read(0, 9, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "ABXXXQQQQ");

  ASSERT_OK(file->Read(2, 4, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "XXXQ");

//关闭文件并重新打开
  file->Close();
  ASSERT_OK(env_->NewRandomRWFile(path, &file, EnvOptions()));

  ASSERT_OK(file->Read(0, 9, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "ABXXXQQQQ");

  ASSERT_OK(file->Read(10, 3, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "ZYZ");

  ASSERT_OK(file->Read(200, 9, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "FFFFFXXXX");

  ASSERT_OK(file->Write(4, "TTTTTTTTTTTTTTTT"));
  ASSERT_OK(file->Read(0, 10, &read_res, buf));
  ASSERT_EQ(read_res.ToString(), "ABXXTTTTTT");

//清理
  env_->DeleteFile(path);
}

class RandomRWFileWithMirrorString {
 public:
  explicit RandomRWFileWithMirrorString(RandomRWFile* _file) : file_(_file) {}

  void Write(size_t offset, const std::string& data) {
//写入镜像字符串
    StringWrite(offset, data);

//写入文件
    Status s = file_->Write(offset, data);
    ASSERT_OK(s) << s.ToString();
  }

  void Read(size_t offset = 0, size_t n = 1000000) {
    Slice str_res(nullptr, 0);
    if (offset < file_mirror_.size()) {
      size_t str_res_sz = std::min(file_mirror_.size() - offset, n);
      str_res = Slice(file_mirror_.data() + offset, str_res_sz);
      StopSliceAtNull(&str_res);
    }

    Slice file_res;
    Status s = file_->Read(offset, n, &file_res, buf_);
    ASSERT_OK(s) << s.ToString();
    StopSliceAtNull(&file_res);

    ASSERT_EQ(str_res.ToString(), file_res.ToString()) << offset << " " << n;
  }

  void SetFile(RandomRWFile* _file) { file_ = _file; }

 private:
  void StringWrite(size_t offset, const std::string& src) {
    if (offset + src.size() > file_mirror_.size()) {
      file_mirror_.resize(offset + src.size(), '\0');
    }

    char* pos = const_cast<char*>(file_mirror_.data() + offset);
    memcpy(pos, src.data(), src.size());
  }

  void StopSliceAtNull(Slice* slc) {
    for (size_t i = 0; i < slc->size(); i++) {
      if ((*slc)[i] == '\0') {
        *slc = Slice(slc->data(), i);
        break;
      }
    }
  }

  char buf_[10000];
  RandomRWFile* file_;
  std::string file_mirror_;
};

TEST_P(EnvPosixTestWithParam, PosixRandomRWFileRandomized) {
  const std::string path = test::TmpDir(env_) + "/random_rw_file_rand";
  env_->DeleteFile(path);

  unique_ptr<RandomRWFile> file;
  ASSERT_OK(env_->NewRandomRWFile(path, &file, EnvOptions()));
  RandomRWFileWithMirrorString file_with_mirror(file.get());

  Random rnd(301);
  std::string buf;
  for (int i = 0; i < 10000; i++) {
//性别随机数据
    test::RandomString(&rnd, 10, &buf);

//为写入选择随机偏移量
    size_t write_off = rnd.Next() % 1000;
    file_with_mirror.Write(write_off, buf);

//选取随机偏移进行读取
    size_t read_off = rnd.Next() % 1000;
    size_t read_sz = rnd.Next() % 20;
    file_with_mirror.Read(read_off, read_sz);

    if (i % 500 == 0) {
//每500个iter重新打开文件
      ASSERT_OK(env_->NewRandomRWFile(path, &file, EnvOptions()));
      file_with_mirror.SetFile(file.get());
    }
  }

//清理
  env_->DeleteFile(path);
}

INSTANTIATE_TEST_CASE_P(DefaultEnvWithoutDirectIO, EnvPosixTestWithParam,
                        ::testing::Values(std::pair<Env*, bool>(Env::Default(),
                                                                false)));
#if !defined(ROCKSDB_LITE)
INSTANTIATE_TEST_CASE_P(DefaultEnvWithDirectIO, EnvPosixTestWithParam,
                        ::testing::Values(std::pair<Env*, bool>(Env::Default(),
                                                                true)));
#endif  //！已定义（RocksDB-Lite）

#if !defined(ROCKSDB_LITE) && !defined(OS_WIN)
static unique_ptr<Env> chroot_env(NewChrootEnv(Env::Default(),
                                               test::TmpDir(Env::Default())));
INSTANTIATE_TEST_CASE_P(
    ChrootEnvWithoutDirectIO, EnvPosixTestWithParam,
    ::testing::Values(std::pair<Env*, bool>(chroot_env.get(), false)));
INSTANTIATE_TEST_CASE_P(
    ChrootEnvWithDirectIO, EnvPosixTestWithParam,
    ::testing::Values(std::pair<Env*, bool>(chroot_env.get(), true)));
#endif  //！定义（RocksDB-Lite）&&！定义（OsWin）

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
