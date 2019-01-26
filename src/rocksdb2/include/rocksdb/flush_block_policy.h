
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

#include <string>
#include "rocksdb/table.h"

namespace rocksdb {

class Slice;
class BlockBuilder;
struct Options;

//FlushBlockPolicy提供了一种可配置的方法来确定何时刷新
//在基于块的表中，
class FlushBlockPolicy {
 public:
//跟踪键/值序列并将布尔值返回到
//确定表生成器是否应刷新当前数据块。
  virtual bool Update(const Slice& key,
                      const Slice& value) = 0;

  virtual ~FlushBlockPolicy() { }
};

class FlushBlockPolicyFactory {
 public:
//返回刷新块策略的名称。
  virtual const char* Name() const = 0;

//返回按数据大小刷新数据块的新块刷新策略。
//FlushBlockPolicy可能需要访问数据块的元数据
//生成器确定何时刷新块。
//
//调用方必须在使用
//结果已关闭。
  virtual FlushBlockPolicy* NewFlushBlockPolicy(
      const BlockBasedTableOptions& table_options,
      const BlockBuilder& data_block_builder) const = 0;

  virtual ~FlushBlockPolicyFactory() { }
};

class FlushBlockBySizePolicyFactory : public FlushBlockPolicyFactory {
 public:
  FlushBlockBySizePolicyFactory() {}

  const char* Name() const override { return "FlushBlockBySizePolicyFactory"; }

  FlushBlockPolicy* NewFlushBlockPolicy(
      const BlockBasedTableOptions& table_options,
      const BlockBuilder& data_block_builder) const override;

  static FlushBlockPolicy* NewFlushBlockPolicy(
      const uint64_t size, const int deviation,
      const BlockBuilder& data_block_builder);
};

}  //罗克斯德
