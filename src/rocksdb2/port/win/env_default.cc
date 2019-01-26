
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

#include <mutex>

#include <rocksdb/env.h>
#include "port/win/env_win.h"

namespace rocksdb {
namespace port {

//我们选择在堆上创建它，并使用std:：once进行以下操作
//原因
//1）当前可用的MS编译器没有实现原子C++ 11
//的初始化
//函数局部静力学
//2）我们选择不破坏env，因为从
//系统加载器
//它破坏静态（与从dllmain中相同）创建系统加载程序
//死锁。
//以这种方式，剩余的线程终止正常。
namespace {
  std::once_flag winenv_once_flag;
  Env* envptr;
};

}

Env* Env::Default() {
  using namespace port;
  std::call_once(winenv_once_flag, []() { envptr = new WinEnv(); });
  return envptr;
}

}

