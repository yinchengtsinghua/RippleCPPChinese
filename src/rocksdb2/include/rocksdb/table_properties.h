
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
#pragma once

#include <stdint.h>
#include <map>
#include <string>
#include "rocksdb/status.h"
#include "rocksdb/types.h"

namespace rocksdb {

//--表属性
//除了基本表属性外，每个表还可以具有用户
//收集的属性。
//用户收集的属性值编码为原始字节--
//用户必须自己解释这些值。
//注意：要在“userCollectedProperties”中执行前缀查找/扫描，可以执行以下操作
//类似于：
//
//UserCollectedProperties属性props=…；
//for（auto pos=props.lower_bound（前缀）；
//销售时点情报系统！=props.end（）&&pos->first.compare（0，prefix.size（），prefix）==0；
//+POS）{
//…
//}
typedef std::map<std::string, std::string> UserCollectedProperties;

//属性块中表属性的人可读名称。
struct TablePropertiesNames {
  static const std::string kDataSize;
  static const std::string kIndexSize;
  static const std::string kIndexPartitions;
  static const std::string kTopLevelIndexSize;
  static const std::string kFilterSize;
  static const std::string kRawKeySize;
  static const std::string kRawValueSize;
  static const std::string kNumDataBlocks;
  static const std::string kNumEntries;
  static const std::string kFormatVersion;
  static const std::string kFixedKeyLen;
  static const std::string kFilterPolicy;
  static const std::string kColumnFamilyName;
  static const std::string kColumnFamilyId;
  static const std::string kComparator;
  static const std::string kMergeOperator;
  static const std::string kPrefixExtractorName;
  static const std::string kPropertyCollectors;
  static const std::string kCompression;
  static const std::string kCreationTime;
  static const std::string kOldestKeyTime;
};

extern const std::string kPropertiesBlock;
extern const std::string kCompressionDictBlock;
extern const std::string kRangeDelBlock;

enum EntryType {
  kEntryPut,
  kEntryDelete,
  kEntrySingleDelete,
  kEntryMerge,
  kEntryOther,
};

//`TablePropertiesCollector`为用户提供收集
//他们感兴趣的自己的财产。这门课本质上是
//将在表期间调用的回调函数集合
//建筑物。它由表格属性和集合界面组成。方法
//不需要是线程安全的，因为我们将创建一个
//每个表的TablePropertiesCollector对象，然后按顺序调用它
class TablePropertiesCollector {
 public:
  virtual ~TablePropertiesCollector() {}

//但是，Deprecate用户定义的收集器应实现addUserKey（）。
//由于向后兼容的原因，这个旧函数仍然可以工作。
//当新的键/值对插入到表中时，将调用add（）。
//@params key插入表中的用户键。
//@params value插入表中的值。
  /*实际状态添加（const slice&/*key*/，const slice&/*value*/）
    返回状态：：invalidArgument（
        “TablePropertiesCollector:：Add（）已弃用。”）；
  }

  //将新的键/值对插入到
  /表格。
  //@params key插入表中的用户密钥。
  //@params值插入到表中的值。
  虚拟状态adduserkey（const slice&key，const slice&value，
                            输入类型*/, SequenceNumber /*seq*/,

                            /*t64_t/*文件大小*/）
    //为了向后兼容。
    返回加法（键，值）；
  }

  //finish（）将在已生成表并准备就绪时调用
  //用于写入属性块。
  //@params properties用户将其收集的统计信息添加到
  //`属性`。
  虚拟状态完成（UserCollectedProperties*属性）=0；

  //返回人可读的属性，其中键是属性名和
  //该值是人类可读的值形式。
  虚拟用户CollectedProperties GetReadableProperties（）const=0；

  //属性收集器的名称可用于调试目的。
  虚拟常量char*name（）const=0；

  //实验返回是否进一步压缩输出文件
  virtual bool needcompact（）const返回false；
}；

//构造TablePropertiesCollector。内部创建新的
//每个新表的TablePropertiesCollector
类表属性CollectorFactory_
 公众：
  结构上下文
    uint32_t列_family_id；
    静态结构，家庭；
  }；

  虚拟~TablePropertiesCollectorFactory（）
  //必须是线程安全的
  虚拟表属性集合器*创建表属性集合器（
      TablePropertiesCollectorFactory:：Context Context）=0；

  //属性收集器的名称可用于调试目的。
  虚拟常量char*name（）const=0；
}；

//TableProperties包含一组与其关联的只读属性
/表格。
结构表属性
 公众：
  //所有数据块的总大小。
  uint64_t data_size=0；
  //索引块的大小。
  uint64_t index_size=0；
  //使用ktwolvelindexsearch时的索引分区总数
  uint64_t index_partitions=0；
  //如果使用ktwolvelindexsearch，则为顶级索引的大小
  uint64_t顶级_级_索引_大小=0；
  //过滤块的大小。
  uint64_t过滤器_尺寸=0；
  //原始密钥总大小
  uint64_t raw_key_size=0；
  //原始值总大小
  uint64_t原始_值_大小=0；
  //此表中的块数
  uint64_t num_data_blocks=0；
  //此表中的条目数
  uint64_t num_entries=0；
  //格式版本，保留向后兼容
  uint64_t格式_version=0；
  //如果为0，则键的长度可变。否则，每个键的字节数。
  
  //此sst文件的列族ID，对应于标识的cf
  //按列_family_name。
  uint64_t列_族_id=
      RocksDB:：TablePropertiesCollectorFactory:：Context:：KunknownColumnFamily；
  //创建sst文件的时间。
  //由于sst文件是不可变的，这相当于上次修改的时间。
  uint64创建时间=0；
  //最早键的时间戳。0表示未知。
  uint64_t最旧的_key_time=0；

  //与此sst文件关联的列族的名称。
  //如果列族未知，“column_family_name”将是空字符串。
  std：：字符串列_family_name；

  //此表中使用的筛选策略的名称。
  //如果不使用筛选策略，“filter_policy_name”将是空字符串。
  std:：string filter_policy_name；

  //此表中使用的比较器的名称。
  std：：字符串比较器名称；

  //此表中使用的合并运算符的名称。
  //如果不使用合并运算符，`merge_operator_name`将为“nullptr”。
  std：：字符串合并\运算符名称；

  //此表中使用的前缀提取程序的名称
  //如果不使用前缀提取器，`前缀提取器名称'将为“nullptr”。
  std：：字符串前缀提取程序名称；

  //此表中使用的属性收集器工厂的名称
  //用逗号分隔
  //收集器名称[1]，收集器名称[2]，收集器名称[3]..
  std：：字符串属性_collectors_name；

  //用于压缩sst文件的压缩算法。
  std：：字符串压缩名称；

  //用户收集的属性
  用户收集属性用户收集属性；
  用户收集的属性可读的属性；

  //文件中每个属性值的偏移量。
  std:：map<std:：string，uint64_t>properties_offsets；

  //将此对象转换为可读形式
  //@prop_delim:每个属性的分隔符。
  std:：string to字符串（const std:：string&prop delim=“；”，
                       const std：：字符串&kv_delim=“=”）const；

  //聚合指定的
  //表属性。
  void add（const tableproperties&tp）；
}；

//额外属性
//下面是数据库收集的非基本属性列表
/本身。特别是关于内部键的一些属性
“/”table`）未知。
外部uint64获取删除键（const usercollectedproperties&props）；
外部uint64获取合并操作数（const usercollectedproperties&props，
                                 bool*财产存在）；

//命名空间rocksdb
