
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
//版权所有（c）2012 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rocksdb/filter_policy.h"

namespace rocksdb {

class Slice;

class FullFilterBitsBuilder : public FilterBitsBuilder {
 public:
  explicit FullFilterBitsBuilder(const size_t bits_per_key,
                                 const size_t num_probes);

  ~FullFilterBitsBuilder();

  virtual void AddKey(const Slice& key) override;

//为哈希[0，n-1]创建一个筛选器，该筛选器在此处分配
//创建过滤器时，确保
//总计\u位=num \u行*缓存\u行\u大小*8
//DST长度大于等于5，num_探针为1，num_测线为4
//然后总的\u位=（len-5）*8，可以计算缓存线的大小
//+———————————————————————————————————————+
//过滤总长度为_位/8_的数据
//+———————————————————————————————————————+
//_
//……γ
//_
//+———————————————————————————————————————+
//……num_probe:1 byte_num_lines:4 bytes_
//+———————————————————————————————————————+
  virtual Slice Finish(std::unique_ptr<const char[]>* buf) override;

//计算适合空间的条目数。
  virtual int CalculateNumEntry(const uint32_t space) override;

//计算新过滤器的空间。这与calculateNumEntry相反。
  uint32_t CalculateSpace(const int num_entry, uint32_t* total_bits,
                          uint32_t* num_lines);

 private:
  size_t bits_per_key_;
  size_t num_probes_;
  std::vector<uint32_t> hash_entries_;

//获取为CPU缓存线优化的totalbits
  uint32_t GetTotalBitsForLocality(uint32_t total_bits);

//为新筛选器保留空间
  char* ReserveSpace(const int num_entry, uint32_t* total_bits,
                     uint32_t* num_lines);

//假设对该函数进行单线程访问。
  void AddHash(uint32_t h, char* data, uint32_t num_lines, uint32_t total_bits);

//不允许复制
  FullFilterBitsBuilder(const FullFilterBitsBuilder&);
  void operator=(const FullFilterBitsBuilder&);
};

}  //命名空间rocksdb
