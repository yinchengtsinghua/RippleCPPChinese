
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

#include "rocksdb/comparator.h"
#include "table/iterator_wrapper.h"

namespace rocksdb {

//当与std：：priority_queue一起使用时，此比较函数将
//最大键位于顶部的迭代器。
class MaxIteratorComparator {
 public:
  MaxIteratorComparator(const Comparator* comparator) :
    comparator_(comparator) {}

  bool operator()(IteratorWrapper* a, IteratorWrapper* b) const {
    return comparator_->Compare(a->key(), b->key()) < 0;
  }
 private:
  const Comparator* comparator_;
};

//当与std：：priority_queue一起使用时，此比较函数将
//迭代器，上面有最小键。
class MinIteratorComparator {
 public:
  MinIteratorComparator(const Comparator* comparator) :
    comparator_(comparator) {}

  bool operator()(IteratorWrapper* a, IteratorWrapper* b) const {
    return comparator_->Compare(a->key(), b->key()) > 0;
  }
 private:
  const Comparator* comparator_;
};

}  //命名空间rocksdb
