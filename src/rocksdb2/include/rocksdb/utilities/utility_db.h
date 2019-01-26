
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#pragma once
#ifndef ROCKSDB_LITE
#include <vector>
#include <string>

#include "rocksdb/utilities/stackable_db.h"
#include "rocksdb/utilities/db_ttl.h"
#include "rocksdb/db.h"

namespace rocksdb {

//请不要用这门课。它被贬低了
class UtilityDB {
 public:
//此函数仅用于向后兼容。请使用
//dbwithttl中定义的函数（rocksdb/utilities/db_ttl.h）
//（弃用）
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((deprecated))
#elif _WIN32
   __declspec(deprecated)
#endif
    static Status OpenTtlDB(const Options& options,
                                                      const std::string& name,
                                                      StackableDB** dbptr,
                                                      int32_t ttl = 0,
                                                      bool read_only = false);
};

} //命名空间rocksdb
#endif  //摇滚乐
