
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

#include <sstream>
#include "monitoring/iostats_context_imp.h"
#include "rocksdb/env.h"
#include "util/thread_local.h"

namespace rocksdb {

#ifdef ROCKSDB_SUPPORT_THREAD_LOCAL
__thread IOStatsContext iostats_context;
#endif

IOStatsContext* get_iostats_context() {
#ifdef ROCKSDB_SUPPORT_THREAD_LOCAL
  return &iostats_context;
#else
  return nullptr;
#endif
}

void IOStatsContext::Reset() {
  thread_pool_id = Env::Priority::TOTAL;
  bytes_read = 0;
  bytes_written = 0;
  open_nanos = 0;
  allocate_nanos = 0;
  write_nanos = 0;
  read_nanos = 0;
  range_sync_nanos = 0;
  prepare_write_nanos = 0;
  fsync_nanos = 0;
  logger_nanos = 0;
}

#define IOSTATS_CONTEXT_OUTPUT(counter)         \
  if (!exclude_zero_counters || counter > 0) {  \
    ss << #counter << " = " << counter << ", "; \
  }

std::string IOStatsContext::ToString(bool exclude_zero_counters) const {
  std::ostringstream ss;
  IOSTATS_CONTEXT_OUTPUT(thread_pool_id);
  IOSTATS_CONTEXT_OUTPUT(bytes_read);
  IOSTATS_CONTEXT_OUTPUT(bytes_written);
  IOSTATS_CONTEXT_OUTPUT(open_nanos);
  IOSTATS_CONTEXT_OUTPUT(allocate_nanos);
  IOSTATS_CONTEXT_OUTPUT(write_nanos);
  IOSTATS_CONTEXT_OUTPUT(read_nanos);
  IOSTATS_CONTEXT_OUTPUT(range_sync_nanos);
  IOSTATS_CONTEXT_OUTPUT(fsync_nanos);
  IOSTATS_CONTEXT_OUTPUT(prepare_write_nanos);
  IOSTATS_CONTEXT_OUTPUT(logger_nanos);

  return ss.str();
}

}  //命名空间rocksdb
