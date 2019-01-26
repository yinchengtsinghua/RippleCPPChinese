
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
#include <unordered_map>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"

namespace rocksdb {

#ifndef ROCKSDB_LITE
//以下功能集提供了一种构建rocksdb选项的方法
//从字符串或字符串到字符串的映射。以下是一般规则
//按类型设置字符串中的选项值。一些RocksDB类型也
//这些API支持。请参阅函数本身的注释
//有关如何配置这些rocksdb类型的更多信息。
//
//＊字符串：
//字符串将直接用作值，而不进行任何截断或
//修整。
//
//* Booleans：
//-“真”或“1”=>真
//-“假”或“0”=>假。
//[实例]：
//-“在getcolumn系列选项frommap中为_点击优化_过滤器”，“1”，或
//-“在getColumnFamilyOptionsFromString中为“hits=true”优化“过滤器”。
//
//*整数：
//除以下内容外，整数直接从字符串转换
//我们支持的单位：
//-“k”或“k”=>2^10
//-“m”或“m”=>2^20
//-“g”或“g”=>2^30
//-“t”或“t”=>2^40//仅适用于具有足够位的无符号int。
//[实例]：
//-“Arena_Block_Size”，“19g”in getColumn FamilyOptions FromMap，或
//-“Arena_Block_Size=19g”在getColumnFamilyOptions FromString中。
//
//*双精度/浮点：
//双精度/浮点直接从字符串转换。注意
//目前我们不支持部队。
//[实例]：
//-“硬速率限制”，“2.1”在getColumn系列选项FromMap中，或
//-“Hard_Rate_Limit=2.1”in getColumnFamilyOptions FromString.
//*数组/向量：
//数组由值列表指定，其中“：”用作
//分隔每个值的分隔符。
//[实例]：
//-“每_级压缩”，“knocompression:ksnappycompression”
//在getColumnFamilyOptions FromMap中，或
//-“压缩级别=knocompression:ksnappycompression”in
//getColumnFamilyOptions来自MapString
//* Enums：
//每个枚举的有效值与其常量的名称相同。
//[实例]：
//-compressionType:有效值为“knocompression”，
//“ksnappycompression”，“kzlibcompression”，“kbzip2 compression”，…
//-compactionStyle:有效值为“kCompactionStyleLevel”，
//“kCompactionStyleUniversal”、“kCompactionStyleFifo”和
//“kCompactionStylenone”。
//

//除了
//将选项名称的“opts-map”映射到选项值以构造新的
//columnfamilyOptions“新选项”。
//
//下面是如何配置某些非基元类型的说明
//ColumnFoOptions中的选项：
//
//*表工厂：
//可以使用自定义嵌套选项语法配置表工厂。
//
//选项_a=值_a；选项_b=值_b；选项_c=值_c；…}
//
//嵌套选项由两个大括号包围，其中
//多个选项分配。每个任务都是这样的
//“变量_name=value；”。
//
//目前我们支持以下类型的TableFactory：
//-基于块的表格工厂：
//使用名称“基于块的表工厂”初始化表工厂
//基于块的表工厂。其BlockBasedTableFactoryOptions可以是
//使用嵌套选项语法配置。
//[实例]：
//*“基于block_的_table_factory”，“block_cache=1m；block_size=4K；”
//相当于为表工厂分配BlockBasedTableFactory
//具有1M LRU块缓存，块大小等于4K：
//列系列选项cf_opt；
//基于块的表格选项BLKU OPT；
//blk_opt.block_cache=newlrucache（1*1024*1024）；
//BLKU OPT.BLOCKU SIZE=4*1024；
//cf_opt.table_factory.reset（newblockbasedTableFactory（blk_opt））；
//-平板工厂：
//使用名称“plain-table-factory”初始化table-factory
//平板工厂。其PlainTableFactoryOptions可以使用
//嵌套选项语法。
//[实例]：
//*“纯_table_factory”，“用户_key_len=66；bloom_bits_per_key=20；”
//
//*Memtable_工厂：
//使用“memtable”配置memtable_工厂。以下是支持的
//Memtable工厂：
//- SkipList：
//通过“skip-list:<lookahead>”将memtable配置为使用skip list，
//或者只需“跳过列表”即可使用默认的skip list。
//[实例]：
//*“memtable”，“skip_list:5”等于设置
//从Memtable到Skiplist工厂（5）。
//-前缀：
//将“prfix-hash:<hash-bucket-count>”传递到配置memtable
//使用prefix hash，或简单地“prefix_hash”使用默认值
//前缀。
//[实例]：
//*“memtable”，“前缀_hash:1000”等于设置
//到newhashskiplitrepFactory的memtable（散列桶计数）。
//-哈希链接列表：
//将“hash-linkedlist:<hash-bucket-count>”传递到配置memtable
//使用hash linkedlist，或者简单地使用“hash-linkedlist”来使用默认值
//HashLinkedList。
//[实例]：
//*“memtable”，“hash_linkedlist:1000”等于
//将memtable设置为newHashlinkListRepFactory（1000）。
//-VectorRepFactory公司：
//通过“vector:<count>”配置memtable以使用vectorrepfactory，
//或者简单地“vector”使用默认的vector memtable。
//[实例]：
//*“memtable”，“vector:1024”等于设置memtable
//至VectorRepFactory（1024）。
//-杜鹃花工厂：
//pass“布谷鸟：<write_buffer_size>”to use hash布谷鸟工厂with the
//指定的写缓冲区大小，或者简单地“布谷鸟”使用默认值
//杜鹃花工厂。
//[实例]：
//*“memtable”，“布谷鸟：1024”相当于设置memtable
//至Newhashcuckoorepfactory（1024）。
//
//*压缩选项：
//使用“压缩选项”配置压缩选项。值格式
//格式为“<window_bits>：<level>：<strategy>：<max_dict_bytes>”。
//[实例]：
//*“压缩选项”，“4:5:6:7”等于设置：
//列系列选项cf_opt；
//cf_opt.compression_opts.window_bits=4；
//cf_opt.compression_opts.level=5；
//cf_opt.compression_opts.strategy=6；
//cf_opt.compression_opts.max_dict_bytes=7；
//
//@param base_options输出“new_options”的默认选项。
//@param opts_将选项名映射到值映射，以指定“新选项”的方式。
//应该设置。
//@param new_options基于“base_options”的结果选项
//“opts_map”中指定的更改。
//@param input_strings_设为true时转义，每个转义字符
//将进一步转换opts映射值中前缀为'\'
//在分配给相关选项之前返回到原始字符串。
//@param ignore_unknown_options当设置为true时，将忽略未知选项
//而不是导致未知的选项错误。
//@return status:：ok（）成功。否则，非OK状态指示
//将返回错误，并将“新选项”设置为“基本选项”。
Status GetColumnFamilyOptionsFromMap(
    const ColumnFamilyOptions& base_options,
    const std::unordered_map<std::string, std::string>& opts_map,
    ColumnFamilyOptions* new_options, bool input_strings_escaped = false,
    bool ignore_unknown_options = false);

//除了
//将选项名称的“opts-map”映射到选项值以构造新的
//dboptions“新选项”。
//
//下面是如何配置某些非基元类型的说明
//dboptions中的选项：
//
//*速率限制器每秒字节数：
//RateLimiter可以通过指定其字节/秒来直接配置。
//[实例]：
//-通过“速率限制器_字节/秒”，“1024”等于
//将newGenericRatelimiter（1024）传递给速率限制器，以每秒\u字节。
//
//@param base_options输出“new_options”的默认选项。
//@param opts_将选项名映射到值映射，以指定“新选项”的方式。
//应该设置。
//@param new_options基于“base_options”的结果选项
//“opts_map”中指定的更改。
//@param input_strings_设为true时转义，每个转义字符
//将进一步转换opts映射值中前缀为'\'
//在分配给相关选项之前返回到原始字符串。
//@param ignore_unknown_options当设置为true时，将忽略未知选项
//而不是导致未知的选项错误。
//@return status:：ok（）成功。否则，非OK状态指示
//将返回错误，并将“新选项”设置为“基本选项”。
Status GetDBOptionsFromMap(
    const DBOptions& base_options,
    const std::unordered_map<std::string, std::string>& opts_map,
    DBOptions* new_options, bool input_strings_escaped = false,
    bool ignore_unknown_options = false);

//除了一个
//将选项名称的“opts-map”映射到选项值以构造新的
//BlockBasedTableOptions“新建表格选项”。
//
//下面是如何配置某些非基元类型的说明
//BlockBasedTableOptions中的选项：
//
//*过滤策略：
//我们目前只支持以下过滤器策略
//功能：
//-bloomfilter：使用“bloomfilter:[位\每\键]：[使用基于\块\的\生成器]”
//指定bloomfilter。上面的字符串等价于调用
//Newbloomfilterpolicy（每个密钥位，使用基于块的构建器）。
//[实例]：
//-pass“filter_policy”，“bloomfilter:4:true”in
//GetBlockBasedTableOptionsFromMap使用4位的BloomFilter
//每键并使用基于\块\的\生成器启用。
//
//*块缓存/块缓存已压缩：
//我们目前只支持getoptions api中的lru缓存。LRU
//可以通过直接指定缓存的大小来设置缓存。
//[实例]：
//-在getBlockBasedTableOptionsFromMap中传递“block_cache”，“1M”
//相当于使用newlrucache（1024*1024）设置块缓存。
//
//@param table_options输出“new_table_options”的默认选项。
//@param opts_将选项名映射到值映射以指定如何
//应设置“新的表格选项”。
//@param new_table_options基于“table_options”的结果选项
//更改在“opts_map”中指定。
//@param input_strings_设为true时转义，每个转义字符
//将进一步转换opts映射值中前缀为'\'
//在分配给相关选项之前返回到原始字符串。
//@param ignore_unknown_options当设置为true时，将忽略未知选项
//而不是导致未知的选项错误。
//@return status:：ok（）成功。否则，非OK状态指示
//将返回错误，并将“new_table_options”设置为
//“表格选项”。
Status GetBlockBasedTableOptionsFromMap(
    const BlockBasedTableOptions& table_options,
    const std::unordered_map<std::string, std::string>& opts_map,
    BlockBasedTableOptions* new_table_options,
    bool input_strings_escaped = false, bool ignore_unknown_options = false);

//除了
//将选项名称的“opts-map”映射到选项值以构造新的
//PlainTableOptions“新的表格选项”。
//
//@param table_options输出“new_table_options”的默认选项。
//@param opts_将选项名映射到值映射以指定如何
//应设置“新的表格选项”。
//@param new_table_options基于“table_options”的结果选项
//更改在“opts_map”中指定。
//@param input_strings_设为true时转义，每个转义字符
//将进一步转换opts映射值中前缀为'\'
//在分配给相关选项之前返回到原始字符串。
//@param ignore_unknown_options当设置为true时，将忽略未知选项
//而不是导致未知的选项错误。
//@return status:：ok（）成功。否则，非OK状态指示
//将返回错误，并将“new_table_options”设置为
//“表格选项”。
Status GetPlainTableOptionsFromMap(
    const PlainTableOptions& table_options,
    const std::unordered_map<std::string, std::string>& opts_map,
    PlainTableOptions* new_table_options, bool input_strings_escaped = false,
    bool ignore_unknown_options = false);

//获取选项名称和值的字符串表示形式，将它们应用到
//以\选项为基础，然后返回新选项。字符串具有
//以下格式：
//“写入缓冲区大小=1024；最大写入缓冲区数目=2”
//也可以进行嵌套选项配置。例如，您可以定义
//BlockBasedTableOptions作为基于块的表工厂的字符串的一部分：
//“写入_buffer_size=1024；块_based_table_factory=块_size=4K”
//“max_write_buffer_num=2”
Status GetColumnFamilyOptionsFromString(
    const ColumnFamilyOptions& base_options,
    const std::string& opts_str,
    ColumnFamilyOptions* new_options);

Status GetDBOptionsFromString(
    const DBOptions& base_options,
    const std::string& opts_str,
    DBOptions* new_options);

Status GetStringFromDBOptions(std::string* opts_str,
                              const DBOptions& db_options,
                              const std::string& delimiter = ";  ");

Status GetStringFromColumnFamilyOptions(std::string* opts_str,
                                        const ColumnFamilyOptions& cf_options,
                                        const std::string& delimiter = ";  ");

Status GetStringFromCompressionType(std::string* compression_str,
                                    CompressionType compression_type);

std::vector<CompressionType> GetSupportedCompressions();

Status GetBlockBasedTableOptionsFromString(
    const BlockBasedTableOptions& table_options,
    const std::string& opts_str,
    BlockBasedTableOptions* new_table_options);

Status GetPlainTableOptionsFromString(
    const PlainTableOptions& table_options,
    const std::string& opts_str,
    PlainTableOptions* new_table_options);

Status GetMemTableRepFactoryFromString(
    const std::string& opts_str,
    std::unique_ptr<MemTableRepFactory>* new_mem_factory);

Status GetOptionsFromString(const Options& base_options,
                            const std::string& opts_str, Options* new_options);

Status StringToMap(const std::string& opts_str,
                   std::unordered_map<std::string, std::string>* opts_map);

//请求停止后台工作，如果wait为true，则等待完成
void CancelAllBackgroundWork(DB* db, bool wait = false);

//删除完全在给定范围内的文件
//可能会在范围内保留一些键，这些键在文件中
//完全在范围内。
//删除前的快照可能看不到给定范围内的数据。
Status DeleteFilesInRange(DB* db, ColumnFamilyHandle* column_family,
                          const Slice* begin, const Slice* end);

//验证文件的校验和
Status VerifySstFileChecksum(const Options& options,
                             const EnvOptions& env_options,
                             const std::string& file_path);
#endif  //摇滚乐

}  //命名空间rocksdb
