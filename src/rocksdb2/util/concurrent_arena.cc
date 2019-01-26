
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

#include "util/concurrent_arena.h"
#include <thread>
#include "port/port.h"
#include "util/random.h"

namespace rocksdb {

#ifdef ROCKSDB_SUPPORT_THREAD_LOCAL
__thread size_t ConcurrentArena::tls_cpuid = 0;
#endif

ConcurrentArena::ConcurrentArena(size_t block_size, AllocTracker* tracker,
                                 size_t huge_page_size)
    : shard_block_size_(block_size / 8),
      shards_(),
      arena_(block_size, tracker, huge_page_size) {
  Fixup();
}

ConcurrentArena::Shard* ConcurrentArena::Repick() {
  auto shard_and_index = shards_.AccessElementAndIndex();
#ifdef ROCKSDB_SUPPORT_THREAD_LOCAL
//即使我们是CPU 0，也要使用非零的tls_cpuid，这样我们就可以告诉我们
//重选
  tls_cpuid = shard_and_index.second | shards_.Size();
#endif
  return shard_and_index.first;
}

}  //命名空间rocksdb
