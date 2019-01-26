
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
//版权所有（c）2012 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//筛选块存储在表文件末尾附近。它包含
//表中所有数据块的过滤器（如Bloom过滤器）组合在一起
//变成一个滤块。

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <vector>
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/slice_transform.h"
#include "table/filter_block.h"
#include "util/hash.h"

namespace rocksdb {


//BlockBasedFilterBlockBuilder用于构造
//特殊表格。它生成一个字符串，存储为
//桌子上的一块特别的木块。
//
//对BlockBasedFilterBlockBuilder的调用序列必须与regexp匹配：
//（开始块添加*）*完成
class BlockBasedFilterBlockBuilder : public FilterBlockBuilder {
 public:
  BlockBasedFilterBlockBuilder(const SliceTransform* prefix_extractor,
      const BlockBasedTableOptions& table_opt);

  virtual bool IsBlockBased() override { return true; }
  virtual void StartBlock(uint64_t block_offset) override;
  virtual void Add(const Slice& key) override;
  virtual Slice Finish(const BlockHandle& tmp, Status* status) override;
  using FilterBlockBuilder::Finish;

 private:
  void AddKey(const Slice& key);
  void AddPrefix(const Slice& key);
  void GenerateFilter();

//重要提示：所有这些可能指向无效地址
//在销毁该滤块时。析构函数
//不应取消引用它们。
  const FilterPolicy* policy_;
  const SliceTransform* prefix_extractor_;
  bool whole_key_filtering_;

size_t prev_prefix_start_;        //最后一个附加前缀的位置
//“进入”。
size_t prev_prefix_size_;         //最后一个附加前缀的长度
//“入门”。
std::string entries_;             //扁平条目内容
std::vector<size_t> start_;       //在每个条目的条目中开始索引
std::string result_;              //过滤到目前为止计算的数据
std::vector<Slice> tmp_entries_;  //policy_u->createfilter（）参数
  std::vector<uint32_t> filter_offsets_;

//不允许复制
  BlockBasedFilterBlockBuilder(const BlockBasedFilterBlockBuilder&);
  void operator=(const BlockBasedFilterBlockBuilder&);
};

//filterblockreader用于从sst表分析筛选器。
//KeymayMatch和PrefixmayMatch将触发过滤器检查
class BlockBasedFilterBlockReader : public FilterBlockReader {
 public:
//要求：“内容”和*策略在*这是活动状态时必须保持活动状态。
  BlockBasedFilterBlockReader(const SliceTransform* prefix_extractor,
                              const BlockBasedTableOptions& table_opt,
                              bool whole_key_filtering,
                              BlockContents&& contents, Statistics* statistics);
  virtual bool IsBlockBased() override { return true; }
  virtual bool KeyMayMatch(
      const Slice& key, uint64_t block_offset = kNotValid,
      const bool no_io = false,
      const Slice* const const_ikey_ptr = nullptr) override;
  virtual bool PrefixMayMatch(
      const Slice& prefix, uint64_t block_offset = kNotValid,
      const bool no_io = false,
      const Slice* const const_ikey_ptr = nullptr) override;
  virtual size_t ApproximateMemoryUsage() const override;

//将此对象转换为人类可读的形式
  std::string ToString() const override;

 private:
  const FilterPolicy* policy_;
  const SliceTransform* prefix_extractor_;
const char* data_;    //指向筛选数据的指针（在块开始处）
const char* offset_;  //指向偏移数组开始的指针（在块结束处）
size_t num_;          //偏移数组中的条目数
size_t base_lg_;      //编码参数（参见.cc文件中的kfilterbaselg）
  BlockContents contents_;

  bool MayMatch(const Slice& entry, uint64_t block_offset);

//不允许复制
  BlockBasedFilterBlockReader(const BlockBasedFilterBlockReader&);
  void operator=(const BlockBasedFilterBlockReader&);
};
}  //命名空间rocksdb
