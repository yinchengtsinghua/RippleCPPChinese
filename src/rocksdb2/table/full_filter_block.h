
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

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/slice_transform.h"
#include "db/dbformat.h"
#include "util/hash.h"
#include "table/filter_block.h"

namespace rocksdb {

class FilterPolicy;
class FilterBitsBuilder;
class FilterBitsReader;

//FullFilterBlockBuilder用于为
//特殊表格。它生成一个字符串，存储为
//桌子上的一块特别的木块。
//完全过滤块的格式为：
//+———————————————————————————————————————+
//sst文件中所有密钥的完全筛选
//+———————————————————————————————————————+
//全滤器可能非常大。最后，我们把
//num_probes:bloom过滤器中使用了多少散列函数
//
class FullFilterBlockBuilder : public FilterBlockBuilder {
 public:
  explicit FullFilterBlockBuilder(const SliceTransform* prefix_extractor,
                                  bool whole_key_filtering,
                                  FilterBitsBuilder* filter_bits_builder);
//位生成器是在筛选器策略中创建的，应在此处传递
//直接。在这里删除
  ~FullFilterBlockBuilder() {}

  virtual bool IsBlockBased() override { return false; }
  virtual void StartBlock(uint64_t block_offset) override {}
  virtual void Add(const Slice& key) override;
  virtual Slice Finish(const BlockHandle& tmp, Status* status) override;
  using FilterBlockBuilder::Finish;

 protected:
  virtual void AddKey(const Slice& key);
  std::unique_ptr<FilterBitsBuilder> filter_bits_builder_;

 private:
//重要提示：所有这些可能指向无效地址
//在销毁该滤块时。析构函数
//不应取消引用它们。
  const SliceTransform* prefix_extractor_;
  bool whole_key_filtering_;

  uint32_t num_added_;
  std::unique_ptr<const char[]> filter_data_;

  void AddPrefix(const Slice& key);

//不允许复制
  FullFilterBlockBuilder(const FullFilterBlockBuilder&);
  void operator=(const FullFilterBlockBuilder&);
};

//filterblockreader用于从sst表分析筛选器。
//KeymayMatch和PrefixmayMatch将触发过滤器检查
class FullFilterBlockReader : public FilterBlockReader {
 public:
//要求：“内容”和过滤器读卡器必须保持活动状态
//而*这是活的。
  explicit FullFilterBlockReader(const SliceTransform* prefix_extractor,
                                 bool whole_key_filtering,
                                 const Slice& contents,
                                 FilterBitsReader* filter_bits_reader,
                                 Statistics* statistics);
  explicit FullFilterBlockReader(const SliceTransform* prefix_extractor,
                                 bool whole_key_filtering,
                                 BlockContents&& contents,
                                 FilterBitsReader* filter_bits_reader,
                                 Statistics* statistics);

//位读卡器是在筛选器策略中创建的，应在此处传递
//直接。在这里删除
  ~FullFilterBlockReader() {}

  virtual bool IsBlockBased() override { return false; }
  virtual bool KeyMayMatch(
      const Slice& key, uint64_t block_offset = kNotValid,
      const bool no_io = false,
      const Slice* const const_ikey_ptr = nullptr) override;
  virtual bool PrefixMayMatch(
      const Slice& prefix, uint64_t block_offset = kNotValid,
      const bool no_io = false,
      const Slice* const const_ikey_ptr = nullptr) override;
  virtual size_t ApproximateMemoryUsage() const override;

 private:
  const SliceTransform* prefix_extractor_;
  Slice contents_;
  std::unique_ptr<FilterBitsReader> filter_bits_reader_;
  BlockContents block_contents_;
  std::unique_ptr<const char[]> filter_data_;

//不允许复制
  FullFilterBlockReader(const FullFilterBlockReader&);
  bool MayMatch(const Slice& entry);
  void operator=(const FullFilterBlockReader&);
};

}  //命名空间rocksdb
