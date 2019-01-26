
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
#include "rocksdb/iterator.h"
#include "rocksdb/env.h"
#include "table/iterator_wrapper.h"

namespace rocksdb {

struct ReadOptions;
class InternalKeyComparator;
class Arena;

struct TwoLevelIteratorState {
  explicit TwoLevelIteratorState(bool _check_prefix_may_match)
      : check_prefix_may_match(_check_prefix_may_match) {}

  virtual ~TwoLevelIteratorState() {}
  virtual InternalIterator* NewSecondaryIterator(const Slice& handle) = 0;
  virtual bool PrefixMayMatch(const Slice& internal_key) = 0;
  virtual bool KeyReachedUpperBound(const Slice& internal_key) = 0;

//如果调用PrefixMayMatch（）。
  bool check_prefix_may_match;
};


//返回新的两级迭代器。两级迭代器包含
//其值指向一系列块的索引迭代器，其中
//每个块本身都是键、值对的序列。归还的人
//两级迭代器生成所有键/值对的串联
//按块的顺序。拥有“索引器”的所有权，并且
//不再需要时将其删除。
//
//使用提供的函数将索引值转换为
//对相应块内容的迭代器。
//竞技场：如果不为空，则竞技场用于分配迭代器。
//销毁迭代器时，析构函数将销毁
//所有的州，除了那些分配在竞技场的州。
//需要“自由”和“状态：自由”状态和“第一级”状态
//真的。否则，只需调用析构函数。
extern InternalIterator* NewTwoLevelIterator(
    TwoLevelIteratorState* state, InternalIterator* first_level_iter,
    Arena* arena = nullptr, bool need_free_iter_and_state = true);

}  //命名空间rocksdb
