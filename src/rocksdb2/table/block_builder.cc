
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
//
//BlockBuilder在键被前缀压缩的位置生成块：
//
//当我们存储一个键时，我们删除与前一个键共享的前缀
//字符串。这有助于显著减少空间需求。
//此外，一旦每k个键，我们就不应用前缀
//压缩并存储整个密钥。我们称之为“重启”
//点“。块的尾端存储所有
//重新启动点，并可用于在查找时执行二进制搜索
//对于一个特定的键。值按原样存储（不压缩）
//紧接着相应的键。
//
//特定键值对的条目具有以下形式：
//共享字节：变量32
//非共享字节：变量32
//数值长度：变量32
//key_delta:char[非共享\u字节]
//值：char[值\长度]
//对于重新启动点，shared_bytes==0。
//
//块的拖车具有以下形式：
//重新启动：uint32[数字重新启动]
//num_重新启动：uint32
//restarts[i]包含第i个重新启动点块内的偏移量。

#include "table/block_builder.h"

#include <algorithm>
#include <assert.h>
#include "rocksdb/comparator.h"
#include "db/dbformat.h"
#include "util/coding.h"

namespace rocksdb {

BlockBuilder::BlockBuilder(int block_restart_interval, bool use_delta_encoding)
    : block_restart_interval_(block_restart_interval),
      use_delta_encoding_(use_delta_encoding),
      restarts_(),
      counter_(0),
      finished_(false) {
  assert(block_restart_interval_ >= 1);
restarts_.push_back(0);       //第一个重新启动点位于偏移量0处
  estimate_ = sizeof(uint32_t) + sizeof(uint32_t);
}

void BlockBuilder::Reset() {
  buffer_.clear();
  restarts_.clear();
restarts_.push_back(0);       //第一个重新启动点位于偏移量0处
  estimate_ = sizeof(uint32_t) + sizeof(uint32_t);
  counter_ = 0;
  finished_ = false;
  last_key_.clear();
}

size_t BlockBuilder::EstimateSizeAfterKV(const Slice& key, const Slice& value)
  const {
  size_t estimate = CurrentSizeEstimate();
  estimate += key.size() + value.size();
  if (counter_ >= block_restart_interval_) {
estimate += sizeof(uint32_t); //新的重新启动项。
  }

estimate += sizeof(int32_t); //共享前缀长度的变量。
estimate += VarintLength(key.size()); //键长度变量。
estimate += VarintLength(value.size()); //值长度变量。

  return estimate;
}

Slice BlockBuilder::Finish() {
//追加重新启动数组
  for (size_t i = 0; i < restarts_.size(); i++) {
    PutFixed32(&buffer_, restarts_[i]);
  }
  PutFixed32(&buffer_, static_cast<uint32_t>(restarts_.size()));
  finished_ = true;
  return Slice(buffer_);
}

void BlockBuilder::Add(const Slice& key, const Slice& value) {
  assert(!finished_);
  assert(counter_ <= block_restart_interval_);
size_t shared = 0;  //与上一个键共享的字节数
  if (counter_ >= block_restart_interval_) {
//重新启动压缩
    restarts_.push_back(static_cast<uint32_t>(buffer_.size()));
    estimate_ += sizeof(uint32_t);
    counter_ = 0;

    if (use_delta_encoding_) {
//更新状态
      last_key_.assign(key.data(), key.size());
    }
  } else if (use_delta_encoding_) {
    Slice last_key_piece(last_key_);
//查看与前一个字符串共享的内容
    shared = key.difference_offset(last_key_piece);

//更新状态
//我们以前只是在这里复制更改过的数据，但看起来
//更快地复制整个东西。
    last_key_.assign(key.data(), key.size());
  }

  const size_t non_shared = key.size() - shared;
  const size_t curr_size = buffer_.size();

//将“<shared><non-shared><value\u size>”添加到buffer\u
  PutVarint32Varint32Varint32(&buffer_, static_cast<uint32_t>(shared),
                              static_cast<uint32_t>(non_shared),
                              static_cast<uint32_t>(value.size()));

//将字符串增量添加到缓冲区中，后跟值
  buffer_.append(key.data() + shared, non_shared);
  buffer_.append(value.data(), value.size());

  counter_++;
  estimate_ += buffer_.size() - curr_size;
}

}  //命名空间rocksdb
