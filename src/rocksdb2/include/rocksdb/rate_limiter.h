
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

#pragma once

#include "rocksdb/env.h"
#include "rocksdb/statistics.h"

namespace rocksdb {

class RateLimiter {
 public:
  enum class OpType {
//限制：我们目前只调用optype:：kread for的request（）。
//当设置dboptions:：new_table_reader_for_compaction_inputs时压缩
    kRead,
    kWrite,
  };
  enum class Mode {
    kReadsOnly,
    kWritesOnly,
    kAllIo,
  };

//对于API兼容性，默认为仅速率限制写入。
  explicit RateLimiter(Mode mode = Mode::kWritesOnly) : mode_(mode) {}

  virtual ~RateLimiter() {}

//此API允许用户每秒动态更改速率限制器的字节数。
//必需：字节/秒>0
  virtual void SetBytesPerSecond(int64_t bytes_per_second) = 0;

//不赞成的新的Ratelimiter派生类应重写
//请求（const int64_t，const env:：iopriority，statistics*）或
//请求（const int64_t，const env:：iopriority，statistics*，optype）
//相反。
//
//请求字节的令牌。如果无法满足此请求，则调用
//被封锁了。呼叫方负责确保
//字节<=GetSingleBurstBytes（）
  virtual void Request(const int64_t bytes, const Env::IOPriority pri) {
    assert(false);
  }

//请求令牌获取字节并可能更新统计信息。如果这样
//无法满足请求，呼叫被阻止。呼叫方负责
//确保bytes<=GetSingleBurstBytes（）。
  virtual void Request(const int64_t bytes, const Env::IOPriority pri,
                       /*Tistics*/*统计资料*/）
    //为了与API兼容，默认实现在
    //不支持哪些统计信息。
    请求（字节，pri）；
  }

  //请求令牌读取或写入字节，并可能更新统计信息。
  / /
  //如果不能满足此请求，则会阻止调用。来电者
  //负责确保bytes<=GetSingleBurstBytes（）。
  虚拟无效请求（const int64_t bytes，const env:：iopriority pri，
                       统计*统计，op type op_type）
    如果（IsrateLimited（Op_Type））
      请求（字节、pri、stats）；
    }
  }

  //请求令牌读取或写入字节，并可能更新统计信息。
  //考虑到getSingleBurstBytes（）和对齐（例如，在
  //direct i/o）分配适当数量的字节，可以减少
  //大于请求的字节数。
  虚拟大小请求令牌（大小字节，大小对齐，
                              env:：io priority io_priority，statistics*统计，
                              ratelimiter：：op type op_类型）；

  //单个突发中可以授予最大字节数
  virtual int64_t getsingleburstbytes（）常量=0；

  //通过速率限制器的总字节数
  虚拟int64_t gettotalbytesthrough（
      const env:：iopriority pri=env:：io_total）const=0；

  //通过速率限制器的请求总数
  虚拟Int64获取总请求（
      const env:：iopriority pri=env:：io_total）const=0；

  virtual int64_t getbytespersecond（）常量=0；

  虚拟布尔IsrateLimited（op type op_type）
    如果（（模式==ratelimiter:：模式：：kWriteOnly&&
         op_type==ratelimiter:：op type:：kread）
        （模式==ratelimiter:：模式：：kreadsonly&&
         op_type==ratelimiter:：op type:：kWrite））
      返回错误；
    }
    回归真实；
  }

 受保护的：
  模式getMode（）返回模式_

 私人：
  const模式
}；

//创建一个ratelimiter对象，该对象可以在rocksdb实例之间共享到
//控制刷新和压缩的写入速率。
//@rate_bytes_per_sec:这是您要设置的唯一参数
/时间。它控制压缩和刷新的总写入速率，单位为字节/
/秒。目前，RockSDB不强制执行任何其他利率限制
//而不是刷新和压缩，例如写入wal。
//@重新填充\u period \u us：这控制重新填充令牌的频率。例如，
//当“速率字节数/秒”设置为10MB/s且“重新填充周期”设置为
//100毫秒，然后每隔100毫秒在内部重新填充1 MB。较大的价值可能导致
//Burstier写入，而较小的值会带来更多的CPU开销。
//默认值在大多数情况下都有效。
//@fairness:ratelimiter接受高优先级请求和低优先级请求。
//低优先级请求通常会被阻止以支持高优先级请求。目前，
//rocksdb将低优先级分配给压缩请求，将高优先级分配给请求
//从冲洗。如果刷新请求进入，则低优先级请求可能会被阻止。
//连续。此公平性参数授予低优先级请求权限
//1/即使存在避免饥饿的高优先级请求，也有公平的机会。
//您应该将其保留为默认值10。
//@模式：模式指示哪些类型的操作对限制计数。
外部ratelimiter*新通用ratelimiter（
    Int64_t Rate_bytes_per_sec，Int64_t Refill_Period_us=100*1000，
    Int32公平性=10，
    ratelimiter：：模式模式=ratelimiter：：模式：：kWriteOnly）；

//命名空间rocksdb
