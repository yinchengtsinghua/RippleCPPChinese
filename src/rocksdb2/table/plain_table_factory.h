
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

#ifndef ROCKSDB_LITE
#include <memory>
#include <string>
#include <stdint.h>

#include "options/options_helper.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"

namespace rocksdb {

struct EnvOptions;

using std::unique_ptr;
class Status;
class RandomAccessFile;
class WritableFile;
class Table;
class TableBuilder;

//indexedTable需要固定长度的密钥，配置为构造函数
//工厂类的参数。输出文件格式：
//+-----------+-----------+
//版本用户密钥长度
//+——————++————————+————————+————————+——+<=key1偏移量
//编码键1值_大小_
//+————————+————————+——————————+——————+—
//值1_
//_
//+——————————————+——————————+——+<=key2偏移量
//编码键2值_大小_
//+————————+————————+——————————+——————+—
//值2_
//_
//..γ
//+————————+————————————+————————————+
//
//当密钥编码类型为kplain时。关键部分编码为：
//+-----------+-----------+
//[钥匙尺寸]内部钥匙
//+-----------+-----------+
//对于用户_key_len=kPlainTableVariableLength的情况，
//简单地说：
//+---------------------+
//内键
//+---------------------+
//对于用户\u key\u len！=kPlainTableVariableLength大小写。
//
//如果密钥编码类型为kprefix。密钥正以这种格式编码。
//有三种方法可以对密钥进行编码：
//（1）全键
//+——————+——————+————+——————————————+——————————+————————+
//全键标志全键大小全内键
//+——————+——————+————+——————————————+——————————+————————+
//它只是对一个完整的键进行编码
//
//（2）与前一个密钥共享相同前缀的密钥，编码为
//格式（1）。
//+————————+————————+——————————+——————————+————————+
//前缀标志前缀大小后缀标志后缀大小密钥后缀
//+————————+————————+——————————+——————————+————————+
//其中键是键的后缀部分，包括内部字节。
//实际的键将通过连接
//上一个键，这里是键的后缀部分，这里给出了大小。
//
//（3）与前一个密钥共享相同前缀的密钥，编码为
//（2）的格式。
//+————————+————————+————————+————————————+——————————+————————+——————+
//密钥后缀标志密钥后缀大小内部密钥后缀
//+————————+————————+————————+————————————+——————————+————————+——————+
//通过连接上一个键的前缀（即
//也是最后一个键以（1）的格式编码的前缀，并且
//这里给出了密钥。
//
//例如，我们用于以下键（前缀和后缀由
//空间）：
//0000 0001
//0000 00021
//0000 0002
//00011 00
//0002 0001
//将按如下方式编码：
//FK 8 0000000 1
//PF 4 SF 5 5
//SF 4 0002
//FK 7 0001100
//FK 8 00020001
//（其中fk表示全键标志，pf表示前缀标志，sf表示后缀标志）
//
//上面显示的所有“密钥标志+密钥大小”都是这种格式：
//第一个字节的8位：
//+---+---+---+---+---+---+---+---+---+---+
//类型尺寸
//+---+---+---+---+---+---+---+---+---+---+
//类型表示：全键、前缀或后缀。
//最后6位表示大小。如果大小位不是全部为1，则表示
//键的大小。否则，变量32在该字节之后读取。这个变量
//值+0x3f（所有1的值）将是密钥大小。
//
//例如，长度为16的完整密钥将编码为（二进制）：
//00 010000
//（00表示全键）
//100字节的前缀将编码为：
//01 111111 00100101
//（63）（37）
//（01表示键后缀）
//
//上面的所有内部密钥（包括kplan和kprefix）都编码在
//这种格式：
//有两种类型：
//（1）普通内部密钥格式
//+——————……--------+---+---+---+---+---+---+---+---+---+---+---+---+
//用户密钥类型序列号
//+——————……-------------+---+---+---+---+---+---+---+---+---+---+---+
//（2）序列ID为0且为值类型的键的特殊情况
//+——————……--------+---+
//用户密钥0x80
//+——————……--------+---+
//为序列ID=0的特殊情况保存7个字节。
//
//
class PlainTableFactory : public TableFactory {
 public:
  ~PlainTableFactory() {}
//user_key_len是用户密钥的长度。如果设置为
//kplainTableVariableLength，则表示可变长度。否则，所有
//键需要具有该值的固定长度。Bloom_Bits_per_键是
//每个键用于Bloom文件管理器的位数。哈希表比率为
//用于前缀散列的散列表的所需利用率。
//hash_table_ratio=哈希表中的前缀数/bucket
//hash_table_ratio=0表示跳过哈希表，但只回复二进制
//搜索。
//索引稀疏度决定键的索引间隔
//在同一个前缀内。它将是线性搜索的最大数目
//哈希和二进制搜索后必需。
//index_sparseness=0表示每个键的索引。
//巨大的页面大小决定是否从巨大的
//页面TLB和页面大小（如果从那里分配）。见评论
//arena:：allocateAligned（）了解详细信息。
  explicit PlainTableFactory(
      const PlainTableOptions& _table_options = PlainTableOptions())
      : table_options_(_table_options) {}

  const char* Name() const override { return "PlainTable"; }
  Status NewTableReader(const TableReaderOptions& table_reader_options,
                        unique_ptr<RandomAccessFileReader>&& file,
                        uint64_t file_size, unique_ptr<TableReader>* table,
                        bool prefetch_index_and_filter_in_cache) const override;

  TableBuilder* NewTableBuilder(
      const TableBuilderOptions& table_builder_options,
      uint32_t column_family_id, WritableFileWriter* file) const override;

  std::string GetPrintableTableOptions() const override;

  const PlainTableOptions& table_options() const;

  static const char kValueTypeSeqId0 = char(0xFF);

//清除指定的数据库选项。
  Status SanitizeOptions(const DBOptions& db_opts,
                         const ColumnFamilyOptions& cf_opts) const override {
    return Status::OK();
  }

  void* GetOptions() override { return &table_options_; }

  Status GetOptionString(std::string* opt_string,
                         const std::string& delimiter) const override {
    return Status::OK();
  }

 private:
  PlainTableOptions table_options_;
};

static std::unordered_map<std::string, OptionTypeInfo> plain_table_type_info = {
    {"user_key_len",
     {offsetof(struct PlainTableOptions, user_key_len), OptionType::kUInt32T,
      OptionVerificationType::kNormal, false, 0}},
    {"bloom_bits_per_key",
     {offsetof(struct PlainTableOptions, bloom_bits_per_key), OptionType::kInt,
      OptionVerificationType::kNormal, false, 0}},
    {"hash_table_ratio",
     {offsetof(struct PlainTableOptions, hash_table_ratio), OptionType::kDouble,
      OptionVerificationType::kNormal, false, 0}},
    {"index_sparseness",
     {offsetof(struct PlainTableOptions, index_sparseness), OptionType::kSizeT,
      OptionVerificationType::kNormal, false, 0}},
    {"huge_page_tlb_size",
     {offsetof(struct PlainTableOptions, huge_page_tlb_size),
      OptionType::kSizeT, OptionVerificationType::kNormal, false, 0}},
    {"encoding_type",
     {offsetof(struct PlainTableOptions, encoding_type),
      OptionType::kEncodingType, OptionVerificationType::kByName, false, 0}},
    {"full_scan_mode",
     {offsetof(struct PlainTableOptions, full_scan_mode), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"store_index_in_file",
     {offsetof(struct PlainTableOptions, store_index_in_file),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}}};

}  //命名空间rocksdb
#endif  //摇滚乐
