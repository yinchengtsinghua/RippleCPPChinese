
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
//
//类，用于指定执行
//切片上的转换。不要求每片
//属于函数的域和/或范围。子类应该
//定义indomain和inrange以确定其中一个切片
//分别是这些集合。

#ifndef STORAGE_ROCKSDB_INCLUDE_SLICE_TRANSFORM_H_
#define STORAGE_ROCKSDB_INCLUDE_SLICE_TRANSFORM_H_

#include <string>

namespace rocksdb {

class Slice;

/*
 *slicletnform是转换一个字符串的通用可插拔方式。
 *另一个。它的主要用途是配置rocksdb
 *通过设置prefix提取器来存储前缀blooms
 *列系列选项。
 **/

class SliceTransform {
 public:
  virtual ~SliceTransform() {};

//返回此转换的名称。
  virtual const char* Name() const = 0;

//从指定的键中提取前缀。当
//在数据库中插入一个键，返回的切片用于
//创建一个Bloom过滤器。
  virtual Slice Transform(const Slice& key) const = 0;

//确定指定的键是否与逻辑兼容
//在转换方法中指定。此方法在
//插入到数据库中的键。如果此方法返回true，
//然后调用transform将密钥转换为其前缀并
//返回的前缀将插入Bloom过滤器。如果这样
//方法返回false，然后跳过对转换的调用，并
//Bloom过滤器中没有插入前缀。
//
//例如，如果transform方法在固定长度上操作
//前缀大小为4，然后调用Indomain（“abc”）返回
//错误，因为指定的密钥长度（3）小于
//前缀大小为4。
//
//此处显示wiki文档：
//https://github.com/facebook/rocksdb/wiki/prefix-seek-api-changes
//
  virtual bool InDomain(const Slice& key) const = 0;

//这是目前没有使用，并留在这里向后兼容。
  virtual bool InRange(const Slice& dst) const { return false; }

//transform（s）=transform（`prefix`），用于任何前缀为'prefix`的s。
//
//rocksdb不使用此功能，而是为用户使用。如果用户通过
//按字符串到rocksdb的选项，它们可能不知道什么前缀提取程序
//他们正在使用。此功能用于帮助用户确定：
//如果他们想重复所有键的前缀“prefix”，不管它是
//使用前缀bloom filter和seek to key`prefix`是安全的。
//如果此函数返回true，则意味着用户可以将（）查找到前缀
//使用布卢姆过滤器。否则，用户需要跳过Bloom过滤器
//通过设置readoptions.total_order_seek=true。
//
//下面是一个示例：假设我们实现一个返回
//使用分隔符“，”拆分字符串后的第一部分：
//1。sameresultwhenappended（“abc，”）应返回true。如果应用前缀
//使用它，所有匹配“abc:.*”的切片都将被提取。
//到“abc”，所以任何包含这些键的sst文件或memtable
//不会被过滤掉。
//2。sameresultwhenappended（“abc”）应返回false。用户不会
//保证看到所有匹配“abc.*”的键，如果用户寻求“abc”
//对具有相同设置的数据库。如果一个sst文件只包含
//“abcd，e”，文件可以被过滤掉，密钥将不可见。
//
//即，始终返回false的实现是安全的。
  virtual bool SameResultWhenAppended(const Slice& prefix) const {
    return false;
  }
};

extern const SliceTransform* NewFixedPrefixTransform(size_t prefix_len);

extern const SliceTransform* NewCappedPrefixTransform(size_t cap_len);

extern const SliceTransform* NewNoopTransform();

}

#endif  //存储块包括切片转换
