
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

#ifndef GFLAGS
#include <cstdio>
int main() {
  fprintf(stderr, "Please install gflags to run rocksdb tools\n");
  return 1;
}
#else

#include <gflags/gflags.h>

#include "monitoring/histogram.h"
#include "rocksdb/env.h"
#include "util/file_reader_writer.h"
#include "util/testharness.h"
#include "util/testutil.h"

using GFLAGS::ParseCommandLineFlags;
using GFLAGS::SetUsageMessage;

//模拟事务日志的简单基准

DEFINE_int32(num_records, 6000, "Number of records.");
DEFINE_int32(record_size, 249, "Size of each record.");
DEFINE_int32(record_interval, 10000, "Interval between records (microSec)");
DEFINE_int32(bytes_per_sync, 0, "bytes_per_sync parameter in EnvOptions");
DEFINE_bool(enable_sync, false, "sync after each write.");

namespace rocksdb {
void RunBenchmark() {
  std::string file_name = test::TmpDir() + "/log_write_benchmark.log";
  Env* env = Env::Default();
  EnvOptions env_options = env->OptimizeForLogWrite(EnvOptions());
  env_options.bytes_per_sync = FLAGS_bytes_per_sync;
  unique_ptr<WritableFile> file;
  env->NewWritableFile(file_name, &file, env_options);
  unique_ptr<WritableFileWriter> writer;
  writer.reset(new WritableFileWriter(std::move(file), env_options));

  std::string record;
  record.assign(FLAGS_record_size, 'X');

  HistogramImpl hist;

  uint64_t start_time = env->NowMicros();
  for (int i = 0; i < FLAGS_num_records; i++) {
    uint64_t start_nanos = env->NowNanos();
    writer->Append(record);
    writer->Flush();
    if (FLAGS_enable_sync) {
      writer->Sync(false);
    }
    hist.Add(env->NowNanos() - start_nanos);

    if (i % 1000 == 1) {
      fprintf(stderr, "Wrote %d records...\n", i);
    }

    int time_to_sleep =
        (i + 1) * FLAGS_record_interval - (env->NowMicros() - start_time);
    if (time_to_sleep > 0) {
      env->SleepForMicroseconds(time_to_sleep);
    }
  }

  fprintf(stderr, "Distribution of latency of append+flush: \n%s",
          hist.ToString().c_str());
}
}  //命名空间rocksdb

int main(int argc, char** argv) {
  SetUsageMessage(std::string("\nUSAGE:\n") + std::string(argv[0]) +
                  " [OPTIONS]...");
  ParseCommandLineFlags(&argc, &argv, true);

  rocksdb::RunBenchmark();
  return 0;
}

#endif  //GFLAGS
