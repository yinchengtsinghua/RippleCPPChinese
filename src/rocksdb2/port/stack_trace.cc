
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
#include "port/stack_trace.h"

#if defined(ROCKSDB_LITE) || !(defined(ROCKSDB_BACKTRACE) || defined(OS_MACOSX)) || \
    defined(CYGWIN) || defined(OS_FREEBSD) || defined(OS_SOLARIS)

//诺普

namespace rocksdb {
namespace port {
void InstallStackTraceHandler() {}
void PrintStack(int first_frames_to_skip) {}
}  //命名空间端口
}  //命名空间rocksdb

#else

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cxxabi.h>

namespace rocksdb {
namespace port {

namespace {

#ifdef OS_LINUX
const char* GetExecutableName() {
  static char name[1024];

  char link[1024];
  snprintf(link, sizeof(link), "/proc/%d/exe", getpid());
  auto read = readlink(link, name, sizeof(name) - 1);
  if (-1 == read) {
    return nullptr;
  } else {
    name[read] = 0;
    return name;
  }
}

void PrintStackTraceLine(const char* symbol, void* frame) {
  static const char* executable = GetExecutableName();
  if (symbol) {
    fprintf(stderr, "%s ", symbol);
  }
  if (executable) {
//输出源到addr2行，用于地址转换
    const int kLineMax = 256;
    char cmd[kLineMax];
    snprintf(cmd, kLineMax, "addr2line %p -e %s -f -C 2>&1", frame, executable);
    auto f = popen(cmd, "r");
    if (f) {
      char line[kLineMax];
      while (fgets(line, sizeof(line), f)) {
line[strlen(line) - 1] = 0;  //删除换行符
        fprintf(stderr, "%s\t", line);
      }
      pclose(f);
    }
  } else {
    fprintf(stderr, " %p", frame);
  }

  fprintf(stderr, "\n");
}
#elif defined(OS_MACOSX)

void PrintStackTraceLine(const char* symbol, void* frame) {
  static int pid = getpid();
//输出源到ATOS，用于地址转换
  const int kLineMax = 256;
  char cmd[kLineMax];
  snprintf(cmd, kLineMax, "xcrun atos %p -p %d  2>&1", frame, pid);
  auto f = popen(cmd, "r");
  if (f) {
    char line[kLineMax];
    while (fgets(line, sizeof(line), f)) {
line[strlen(line) - 1] = 0;  //删除换行符
      fprintf(stderr, "%s\t", line);
    }
    pclose(f);
  } else if (symbol) {
    fprintf(stderr, "%s ", symbol);
  }

  fprintf(stderr, "\n");
}

#endif

}  //命名空间

void PrintStack(int first_frames_to_skip) {
  const int kMaxFrames = 100;
  void* frames[kMaxFrames];

  auto num_frames = backtrace(frames, kMaxFrames);
  auto symbols = backtrace_symbols(frames, num_frames);

  for (int i = first_frames_to_skip; i < num_frames; ++i) {
    fprintf(stderr, "#%-2d  ", i - first_frames_to_skip);
    PrintStackTraceLine((symbols != nullptr) ? symbols[i] : nullptr, frames[i]);
  }
  free(symbols);
}

static void StackTraceHandler(int sig) {
//重置为默认处理程序
  signal(sig, SIG_DFL);
  fprintf(stderr, "Received signal %d (%s)\n", sig, strsignal(sig));
//跳过前三个与信号处理程序相关的帧
  PrintStack(3);
//重新向默认处理程序发送信号（因此，如果需要，我们仍然会得到核心转储…）
  raise(sig);
}

void InstallStackTraceHandler() {
//只需使用简单而充分的旧信号
//对于这个用例
  signal(SIGILL, StackTraceHandler);
  signal(SIGSEGV, StackTraceHandler);
  signal(SIGBUS, StackTraceHandler);
  signal(SIGABRT, StackTraceHandler);
}

}  //命名空间端口
}  //命名空间rocksdb

#endif
