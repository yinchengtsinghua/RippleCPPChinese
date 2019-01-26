
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
#include <string>

#include "options/db_options.h"
#include "rocksdb/env.h"
#include "rocksdb/status.h"
#include "rocksdb/types.h"

namespace rocksdb {
//使用同步映射到选项。使用确定
//复制后将同步文件。
extern Status CopyFile(Env* env, const std::string& source,
                       const std::string& destination, uint64_t size,
                       bool use_fsync);

extern Status CreateFile(Env* env, const std::string& destination,
                         const std::string& contents);

extern Status DeleteSSTFile(const ImmutableDBOptions* db_options,
                            const std::string& fname, uint32_t path_id);

}  //命名空间rocksdb
