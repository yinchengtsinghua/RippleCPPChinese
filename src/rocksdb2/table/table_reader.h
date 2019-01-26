
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
#include <memory>
#include "table/internal_iterator.h"

namespace rocksdb {

class Iterator;
struct ParsedInternalKey;
class Slice;
class Arena;
struct ReadOptions;
struct TableProperties;
class GetContext;
class InternalIterator;

//表是从字符串到字符串的排序映射。表是
//不变的和持久的。可以从中安全地访问表
//多个线程没有外部同步。
class TableReader {
 public:
  virtual ~TableReader() {}

//返回表内容的新迭代器。
//newIterator（）的结果最初无效（调用方必须
//在使用迭代器之前调用其中一个seek方法）。
//竞技场：如果不为空，则需要使用竞技场来分配迭代器。
//销毁迭代器时，调用方不会调用“delete”
//但是迭代器：：~直接迭代器（）。毁灭者需要毁灭
//所有的州，除了那些分配在竞技场的州。
//跳过过滤器：禁用检查Bloom过滤器，即使它们存在。这个
//选项仅对基于块的表格式有效。
  virtual InternalIterator* NewIterator(const ReadOptions&,
                                        Arena* arena = nullptr,
                                        bool skip_filters = false) = 0;

  virtual InternalIterator* NewRangeTombstoneIterator(
      const ReadOptions& read_options) {
    return nullptr;
  }

//给定一个键，返回文件中的一个近似字节偏移量，其中
//该键的数据将开始（如果该键是
//存在于文件中）。返回值以文件形式表示
//字节，因此包括压缩基础数据等效果。
//例如，表中最后一个键的近似偏移量将
//接近文件长度。
  virtual uint64_t ApproximateOffsetOf(const Slice& key) = 0;

//设置压缩表。可能更改某些参数
//PosixFfDebug
  virtual void SetupForCompaction() = 0;

  virtual std::shared_ptr<const TableProperties> GetTableProperties() const = 0;

//准备在真正的get（）之前可以完成的工作
  virtual void Prepare(const Slice& target) {}

//报告内存使用量的近似值。
  virtual size_t ApproximateMemoryUsage() const = 0;

//重复调用get_Context->saveValue（），从
//在调用seek（key）之后找到的条目，直到返回false。
//如果筛选策略说密钥不存在，则不能进行此调用。
//
//当配置为
//仅内存，在块缓存中找不到密钥。
//
//read options是用于读取的选项
//关键是要搜索的关键
//跳过过滤器：禁用检查Bloom过滤器，即使它们存在。这个
//选项仅对基于块的表格式有效。
  virtual Status Get(const ReadOptions& readOptions, const Slice& key,
                     GetContext* get_context, bool skip_filters = false) = 0;

//预取与给定的键范围相对应的数据
//通常，此功能对于以下表实现是必需的：
//将数据保存在非易失性存储介质（如磁盘/SSD）上
  virtual Status Prefetch(const Slice* begin = nullptr,
                          const Slice* end = nullptr) {
    (void) begin;
    (void) end;
//默认实现是noop。
//子类应在适用时实现功能
    return Status::OK();
  }

//将数据库文件转换为可读形式
  virtual Status DumpTable(WritableFile* out_file) {
    return Status::NotSupported("DumpTable() not supported");
  }

//检查此数据库文件是否损坏
  virtual Status VerifyChecksum() {
    return Status::NotSupported("VerifyChecksum() not supported");
  }

  virtual void Close() {}
};

}  //命名空间rocksdb
