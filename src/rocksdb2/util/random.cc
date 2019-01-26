
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

#include "util/random.h"

#include <stdint.h>
#include <string.h>
#include <thread>
#include <utility>

#include "port/likely.h"
#include "util/thread_local.h"

#ifdef ROCKSDB_SUPPORT_THREAD_LOCAL
#define STORAGE_DECL static __thread
#else
#define STORAGE_DECL static
#endif

namespace rocksdb {

Random* Random::GetTLSInstance() {
  STORAGE_DECL Random* tls_instance;
  STORAGE_DECL std::aligned_storage<sizeof(Random)>::type tls_instance_bytes;

  auto rv = tls_instance;
  if (UNLIKELY(rv == nullptr)) {
    size_t seed = std::hash<std::thread::id>()(std::this_thread::get_id());
    rv = new (&tls_instance_bytes) Random((uint32_t)seed);
    tls_instance = rv;
  }
  return rv;
}

}  //命名空间rocksdb
