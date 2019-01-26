
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
//
//它是BlockBasedFilter和FullFilter的基类。
//这两个都用于blockbasedtable。第一个包含过滤器
//对于sst文件中的一部分密钥，第二个包含所有密钥的筛选器
//在SST文件中。

#pragma once

#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/table.h"
#include "util/hash.h"
#include "format.h"

namespace rocksdb {

const uint64_t kNotValid = ULLONG_MAX;
class FilterPolicy;

//filterBlockBuilder用于构造
//特殊表格。它生成一个字符串，存储为
//桌子上的一块特别的木块。
//
//对filterblockbuilder的调用序列必须与regexp匹配：
//（开始块添加*）*完成
//
//基于块的/完整的filterblock将以相同的方式调用。
class FilterBlockBuilder {
 public:
  explicit FilterBlockBuilder() {}
  virtual ~FilterBlockBuilder() {}

virtual bool IsBlockBased() = 0;                    //如果是基于块的筛选器
virtual void StartBlock(uint64_t block_offset) = 0;  //启动新的块筛选器
virtual void Add(const Slice& key) = 0;      //向当前筛选器添加键
Slice Finish() {                             //生成滤波器
    const BlockHandle empty_handle;
    Status dont_care_status;
    auto ret = Finish(empty_handle, &dont_care_status);
    assert(dont_care_status.ok());
    return ret;
  }
  virtual Slice Finish(const BlockHandle& tmp, Status* status) = 0;

 private:
//不允许复制
  FilterBlockBuilder(const FilterBlockBuilder&);
  void operator=(const FilterBlockBuilder&);
};

//filterblockreader用于从sst表分析筛选器。
//KeymayMatch和PrefixmayMatch将触发过滤器检查
//
//基于块的/完整的filterblock将以相同的方式调用。
class FilterBlockReader {
 public:
  explicit FilterBlockReader()
      : whole_key_filtering_(true), size_(0), statistics_(nullptr) {}
  explicit FilterBlockReader(size_t s, Statistics* stats,
                             bool _whole_key_filtering)
      : whole_key_filtering_(_whole_key_filtering),
        size_(s),
        statistics_(stats) {}
  virtual ~FilterBlockReader() {}

virtual bool IsBlockBased() = 0;  //如果是基于块的筛选器
  /*
   *如果未设置IO，则如果在没有
   *从磁盘读取数据。这在partitionedfilterblockreader中用于
   *避免读取已经不在块缓存中的分区
   *
   *通常过滤器只建立在用户密钥上，而InternalKey不是
   *查询需要。但是，partitionedfilterblockreader中的索引是
   *基于internalkey，运行时必须通过const-ikey-ptr提供。
   ＊查询。
   **/

  virtual bool KeyMayMatch(const Slice& key, uint64_t block_offset = kNotValid,
                           const bool no_io = false,
                           const Slice* const const_ikey_ptr = nullptr) = 0;
  /*
   *这里的no_io和const_ikey_ptr与keymaymatch中的含义相同。
   **/

  virtual bool PrefixMayMatch(const Slice& prefix,
                              uint64_t block_offset = kNotValid,
                              const bool no_io = false,
                              const Slice* const const_ikey_ptr = nullptr) = 0;
  virtual size_t ApproximateMemoryUsage() const = 0;
  virtual size_t size() const { return size_; }
  virtual Statistics* statistics() const { return statistics_; }

  bool whole_key_filtering() const { return whole_key_filtering_; }

//将此对象转换为人类可读的形式
  virtual std::string ToString() const {
    std::string error_msg("Unsupported filter \n");
    return error_msg;
  }

  virtual void CacheDependencies(bool pin) {}

 protected:
  bool whole_key_filtering_;

 private:
//不允许复制
  FilterBlockReader(const FilterBlockReader&);
  void operator=(const FilterBlockReader&);
  size_t size_;
  Statistics* statistics_;
  int level_ = -1;
};

}  //命名空间rocksdb
