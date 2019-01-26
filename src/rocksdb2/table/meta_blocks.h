
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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "db/builder.h"
#include "db/table_properties_collector.h"
#include "util/kv_map.h"
#include "rocksdb/comparator.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "table/block_builder.h"
#include "table/format.h"

namespace rocksdb {

class BlockBuilder;
class BlockHandle;
class Env;
class Footer;
class Logger;
class RandomAccessFile;
struct TableProperties;
class InternalIterator;

class MetaIndexBuilder {
 public:
  MetaIndexBuilder(const MetaIndexBuilder&) = delete;
  MetaIndexBuilder& operator=(const MetaIndexBuilder&) = delete;

  MetaIndexBuilder();
  void Add(const std::string& key, const BlockHandle& handle);

//将所有添加的键/值对写入块并返回内容
//街区的
  Slice Finish();

 private:
//存储元块的排序键/句柄。
  stl_wrappers::KVMap meta_block_handles_;
  std::unique_ptr<BlockBuilder> meta_index_block_;
};

class PropertyBlockBuilder {
 public:
  PropertyBlockBuilder(const PropertyBlockBuilder&) = delete;
  PropertyBlockBuilder& operator=(const PropertyBlockBuilder&) = delete;

  PropertyBlockBuilder();

  void AddTableProperty(const TableProperties& props);
  void Add(const std::string& key, uint64_t value);
  void Add(const std::string& key, const std::string& value);
  void Add(const UserCollectedProperties& user_collected_properties);

//将所有添加的条目写入块并返回块内容
  Slice Finish();

 private:
  std::unique_ptr<BlockBuilder> properties_block_;
  stl_wrappers::KVMap props_;
};

//在用户定义的统计数据收集过程中是否遇到任何错误，
//我们将把警告消息写入信息日志。
void LogPropertiesCollectionError(
    Logger* info_log, const std::string& method, const std::string& name);

//实用程序函数帮助表生成器为用户触发批处理事件
//定义的属性收集器。
//返回值指示是否发生错误；如果发生错误，
//将记录警告消息。
//notifyCollectTableCollectorSonAdd（）为所有人触发“添加”事件
//财产收集者。
bool NotifyCollectTableCollectorsOnAdd(
    const Slice& key, const Slice& value, uint64_t file_size,
    const std::vector<std::unique_ptr<IntTblPropCollector>>& collectors,
    Logger* info_log);

//notifyCollectTableCollectorSonAdd（）为所有人触发“完成”事件
//财产收集者。收集的属性将添加到“builder”。
bool NotifyCollectTableCollectorsOnFinish(
    const std::vector<std::unique_ptr<IntTblPropCollector>>& collectors,
    Logger* info_log, PropertyBlockBuilder* builder);

//从表中读取属性。
//@返回一个状态，指示操作是否成功。关于成功，
//*表属性将指向堆分配的表属性
//对象，否则将不修改'table_properties'的值。
Status ReadProperties(const Slice& handle_value, RandomAccessFileReader* file,
                      FilePrefetchBuffer* prefetch_buffer, const Footer& footer,
                      const ImmutableCFOptions& ioptions,
                      TableProperties** table_properties);

//直接从普通表的属性块中读取属性。
//@返回一个状态，指示操作是否成功。关于成功，
//*表属性将指向堆分配的表属性
//对象，否则将不修改'table_properties'的值。
Status ReadTableProperties(RandomAccessFileReader* file, uint64_t file_size,
                           uint64_t table_magic_number,
                           const ImmutableCFOptions &ioptions,
                           TableProperties** properties);

//从元索引块中查找元块。
Status FindMetaBlock(InternalIterator* meta_index_iter,
                     const std::string& meta_block_name,
                     BlockHandle* block_handle);

//找到元块
Status FindMetaBlock(RandomAccessFileReader* file, uint64_t file_size,
                     uint64_t table_magic_number,
                     const ImmutableCFOptions &ioptions,
                     const std::string& meta_block_name,
                     BlockHandle* block_handle);

//读取名为meta_block_name的指定元块
//并用此块的内容初始化“contents”。
//返回状态：如果成功，则返回OK。
Status ReadMetaBlock(RandomAccessFileReader* file,
                     FilePrefetchBuffer* prefetch_buffer, uint64_t file_size,
                     uint64_t table_magic_number,
                     const ImmutableCFOptions& ioptions,
                     const std::string& meta_block_name,
                     BlockContents* contents);

}  //命名空间rocksdb
