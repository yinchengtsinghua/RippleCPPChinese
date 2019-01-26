
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
#pragma once
#include "db/version_edit.h"
#include "util/random.h"

namespace rocksdb {
static const uint32_t kFileReadSampleRate = 1024;
extern bool should_sample_file_read();
extern void sample_file_read_inc(FileMetaData*);

inline bool should_sample_file_read() {
  return (Random::GetTLSInstance()->Next() % kFileReadSampleRate == 307);
}

inline void sample_file_read_inc(FileMetaData* meta) {
  meta->stats.num_reads_sampled.fetch_add(kFileReadSampleRate,
                                          std::memory_order_relaxed);
}
}
