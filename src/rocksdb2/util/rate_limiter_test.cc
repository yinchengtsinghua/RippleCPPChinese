
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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "util/rate_limiter.h"
#include <inttypes.h>
#include <limits>
#include "rocksdb/env.h"
#include "util/random.h"
#include "util/sync_point.h"
#include "util/testharness.h"

namespace rocksdb {

//托多：当我们并行运行测试时，速率将不准确。
class RateLimiterTest : public testing::Test {};

TEST_F(RateLimiterTest, OverflowRate) {
  GenericRateLimiter limiter(port::kMaxInt64, 1000, 10);
  ASSERT_GT(limiter.GetSingleBurstBytes(), 1000000000ll);
}

TEST_F(RateLimiterTest, StartStop) {
  std::unique_ptr<RateLimiter> limiter(NewGenericRateLimiter(100, 100, 10));
}

TEST_F(RateLimiterTest, Modes) {
  for (auto mode : {RateLimiter::Mode::kWritesOnly,
                    RateLimiter::Mode::kReadsOnly, RateLimiter::Mode::kAllIo}) {
    /*ericratelimiter限制器（2000/*速率字节/秒*/，
                               1000*1000/*补充期*/,

                               /*/*公平性*/，模式）；
    限制器请求（1000/*字节*/, Env::IO_HIGH, nullptr /* stats */,

                    RateLimiter::OpType::kRead);
    if (mode == RateLimiter::Mode::kWritesOnly) {
      ASSERT_EQ(0, limiter.GetTotalBytesThrough(Env::IO_HIGH));
    } else {
      ASSERT_EQ(1000, limiter.GetTotalBytesThrough(Env::IO_HIGH));
    }

    /*iter.request（1000/*字节*/，env:：io_高，nullptr/*状态*/，
                    ratelimiter:：optype:：kwrite）；
    如果（模式==ratelimiter:：mode:：kallio）
      断言_Eq（2000，limiter.gettotalbytesthrough（env:：io_high））；
    }否则{
      断言_Eq（1000，limiter.gettotalbytesthrough（env:：io_high））；
    }
  }
}

如果有的话！（已定义（travis）和已定义（os_macosx）
测试_f（Ratelimitterest，Rate）
  auto*env=env:：default（）；
  结构ARG{
    arg（Int32_t_目标_率，Int_突发）
        ：限制器（新通用速率限制（_目标速率，100*1000，10）），
          请求大小（目标速率/10）
          突发（突发）
    std:：unique_ptr<ratelimiter>限制器；
    Int32请求大小；
    突发；
  }；

  自动编写器=[]（void*p）
    auto*thread_env=env:：default（）；
    auto*arg=static_cast<arg*>（p）；
    //测试2秒
    auto until=thread_env->nowmicros（）+2*1000000；
    随机R（（uint32_t）（thread_env->nownanos（）%
                        std:：numeric_limits<uint32_t>：：max（））；
    while（thread_env->nowmicros（）<until）
      对于（int i=0；i<static_cast<int>（r.skated（arg->burst）+1）；++i）
        arg->limiter->request（r.uniform（arg->request_size-1）+1，
                              env:：io_high，nullptr/*状态*/,

                              RateLimiter::OpType::kWrite);
      }
      arg->limiter->Request(r.Uniform(arg->request_size - 1) + 1, Env::IO_LOW,
                            /*lptr/*stats*/，ratelimiter:：optype:：kWrite）；
    }
  }；

  对于（int i=1；i<=16；i*=2）
    Int32_t目标=i*1024*10；
    arg arg（目标，i/4+1）；
    int64_t old_total_bytes_through=0；
    对于（int iter=1；iter<=2；+iter）
      //第二次迭代动态更改目标
      如果（iter==2）
        目标*＝2；
        arg.limiter->setbytespersecond（目标）；
      }
      自动启动=env->nowmicros（）；
      对于（int t=0；t<i；+t）
        env->startthread（writer和arg）；
      }
      env->waitForJoin（）；

      auto elapsed=env->nowmicros（）-启动；
      倍率=
          （arg.limiter->gettotalbytesthrough（）-旧的总字节数*
          1000000.0/已用；
      旧的_total_bytes_through=arg.limiter->gettotalbytesthrough（）；
      FPrTNF（STDER）
              “请求大小[1-%”prii32“]，限制百分比”prii32
              “kb/秒，实际速率：%lf kb/秒，已用%.2lf秒\n”，
              arg.request-1，目标/1024，速率/1024，
              已用/1000000.0）；

      断言“GE”（比率/目标，0.80）；
      断言（比率/目标，1.25）；
    }
  }
}
第二节

测试F（Ratelimitterest，LimitChangeTest）
  //当极限值变小时的饥饿试验
  Int64加注周期=1000*1000；
  auto*env=env:：default（）；
  rocksdb:：syncpoint:：getInstance（）->启用处理（）；
  结构ARG{
    arg（Int32_t_请求大小，env:：iopriority_pri，
        std:：shared-ptr<ratelimiter>u limiter）
        ：请求_大小（_请求_大小）、pri（_pri）、限制器（_限制器）
    Int32请求大小；
    环境：IOpriority pri；
    std:：shared_ptr<ratelimiter>limiter；
  }；

  自动编写器=[]（void*p）
    auto*arg=static_cast<arg*>（p）；
    arg->limiter->request（arg->request_size，arg->pri，nullptr/*状态*/,

                          RateLimiter::OpType::kWrite);
  };

  for (uint32_t i = 1; i <= 16; i <<= 1) {
    int32_t target = i * 1024 * 10;
//每秒重新加注
    for (int iter = 0; iter < 2; iter++) {
      std::shared_ptr<RateLimiter> limiter =
          std::make_shared<GenericRateLimiter>(target, refill_period, 10);
      rocksdb::SyncPoint::GetInstance()->LoadDependency(
          {{"GenericRateLimiter::Request",
            "RateLimiterTest::LimitChangeTest:changeLimitStart"},
           {"RateLimiterTest::LimitChangeTest:changeLimitEnd",
            "GenericRateLimiter::Refill"}});
      Arg arg(target, Env::IO_HIGH, limiter);
//背后的想法是先启动一个请求，然后再重新填充，
//将限制更新为其他值（2x/0.5x）。不应该挨饿
//在任何情况下都有保证
//TODO（Lightmark）：欢迎使用更多的测试用例。
      env->StartThread(writer, &arg);
      int32_t new_limit = (target << 1) >> (iter << 1);
      TEST_SYNC_POINT("RateLimiterTest::LimitChangeTest:changeLimitStart");
      arg.limiter->SetBytesPerSecond(new_limit);
      TEST_SYNC_POINT("RateLimiterTest::LimitChangeTest:changeLimitEnd");
      env->WaitForJoin();
      fprintf(stderr,
              "[COMPLETE] request size %" PRIi32 " KB, new limit %" PRIi32
              "KB/sec, refill period %" PRIi64 " ms\n",
              target / 1024, new_limit / 1024, refill_period / 1000);
    }
  }
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
