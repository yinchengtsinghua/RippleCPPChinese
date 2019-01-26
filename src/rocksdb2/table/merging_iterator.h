
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

#pragma once

#include "rocksdb/types.h"

namespace rocksdb {

class Comparator;
class InternalIterator;
class Env;
class Arena;

//返回一个迭代器，该迭代器在
//儿童[0，n-1]。取得子迭代器的所有权，并且
//将在结果迭代器被删除时删除它们。
//
//结果没有重复抑制。也就是说，如果
//密钥存在于k子迭代器中，将生成k次。
//
//要求：n>=0
extern InternalIterator* NewMergingIterator(const Comparator* comparator,
                                            InternalIterator** children, int n,
                                            Arena* arena = nullptr,
                                            bool prefix_seek_mode = false);

class MergingIterator;

//一个生成器类，通过逐个添加迭代器来构建合并迭代器。
class MergeIteratorBuilder {
 public:
//比较器：用于合并比较器的比较器。
//竞技场：需要从中分配合并迭代器的地方。
  explicit MergeIteratorBuilder(const Comparator* comparator, Arena* arena,
                                bool prefix_seek_mode = false);
  ~MergeIteratorBuilder() {}

//将ITER添加到合并迭代器。
  void AddIterator(InternalIterator* iter);

//获取用于构建合并迭代器的Arena。它被称为一个孩子
//需要分配迭代器。
  Arena* GetArena() { return arena; }

//返回合并结果迭代器。
  InternalIterator* Finish();

 private:
  MergingIterator* merge_iter;
  InternalIterator* first_iter;
  bool use_merging_iter;
  Arena* arena;
};

}  //命名空间rocksdb
