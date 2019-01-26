
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
#include <vector>

#include <stdint.h>
#include "rocksdb/slice.h"

namespace rocksdb {

class BlockBuilder {
 public:
  BlockBuilder(const BlockBuilder&) = delete;
  void operator=(const BlockBuilder&) = delete;

  explicit BlockBuilder(int block_restart_interval,
                        bool use_delta_encoding = true);

//重新设置内容，就像刚刚构造了BlockBuilder一样。
  void Reset();

//要求：自上次调用Reset（）以来未调用finish（）。
//需要：密钥大于任何以前添加的密钥
  void Add(const Slice& key, const Slice& value);

//完成构建块并返回引用
//阻止内容。返回的切片将对
//此生成器的生存期或直到调用Reset（）。
  Slice Finish();

//返回块当前（未压缩）大小的估计值
//我们正在建造。
  inline size_t CurrentSizeEstimate() const { return estimate_; }

//返回追加键和值后估计的块大小。
  size_t EstimateSizeAfterKV(const Slice& key, const Slice& value) const;

//返回真iff自上次重置（）以来未添加任何条目
  bool empty() const {
    return buffer_.empty();
  }

 private:
  const int          block_restart_interval_;
  const bool         use_delta_encoding_;

std::string           buffer_;    //目标缓冲区
std::vector<uint32_t> restarts_;  //再启动点
  size_t                estimate_;
int                   counter_;   //自重新启动后发出的条目数
bool                  finished_;  //是否已调用finish（）？
  std::string           last_key_;
};

}  //命名空间rocksdb
