
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//部分版权所有（c）2011至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

//借来
//http://www.crazygaze.com/blog/2016/03/24/portable-c-timer-queue/
//计时器队列
//
//许可证
//
//本文中的源代码是在CC0许可下获得许可的，所以请感觉
//自由的
//复制、修改、共享、做任何你想做的事情。
//不需要归因，但如果你这样做，我会很高兴。
//cc0许可证

//将工作与本契约关联的人已将工作奉献给
//通过放弃他或她在世界范围内的所有工作权利而获得的公共领域
//根据版权法，包括与
//法律允许的范围。您可以复制、修改、分发和执行
//工作，甚至
//商业目的，均未经许可。查看其他信息
//下面。
//

#include "util/timer_queue.h"
#include <future>

namespace Timing {

using Clock = std::chrono::high_resolution_clock;
double now() {
  static auto start = Clock::now();
  return std::chrono::duration<double, std::milli>(Clock::now() - start)
      .count();
}

}  //命名空间计时

int main() {
  TimerQueue q;

  double tnow = Timing::now();

  q.add(10000, [tnow](bool aborted) mutable {
    printf("T 1: %d, Elapsed %4.2fms\n", aborted, Timing::now() - tnow);
    return std::make_pair(false, 0);
  });
  q.add(10001, [tnow](bool aborted) mutable {
    printf("T 2: %d, Elapsed %4.2fms\n", aborted, Timing::now() - tnow);
    return std::make_pair(false, 0);
  });

  q.add(1000, [tnow](bool aborted) mutable {
    printf("T 3: %d, Elapsed %4.2fms\n", aborted, Timing::now() - tnow);
    return std::make_pair(!aborted, 1000);
  });

  auto id = q.add(2000, [tnow](bool aborted) mutable {
    printf("T 4: %d, Elapsed %4.2fms\n", aborted, Timing::now() - tnow);
    return std::make_pair(!aborted, 2000);
  });

  (void)id;
//auto ret=q.取消（id）；
//断言（ret==1）；
//Q.

  return 0;
}
///////////////////////////////////
