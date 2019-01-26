
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
#pragma once

#include <map>
#include <string>

#include "rocksdb/comparator.h"
#include "rocksdb/slice.h"
#include "util/coding.h"
#include "util/murmurhash.h"

namespace rocksdb {
namespace stl_wrappers {

struct LessOfComparator {
  explicit LessOfComparator(const Comparator* c = BytewiseComparator())
      : cmp(c) {}

  bool operator()(const std::string& a, const std::string& b) const {
    return cmp->Compare(Slice(a), Slice(b)) < 0;
  }
  bool operator()(const Slice& a, const Slice& b) const {
    return cmp->Compare(a, b) < 0;
  }

  const Comparator* cmp;
};

typedef std::map<std::string, std::string, LessOfComparator> KVMap;
}
}
