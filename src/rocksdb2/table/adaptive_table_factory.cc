
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

#ifndef ROCKSDB_LITE
#include "table/adaptive_table_factory.h"

#include "table/table_builder.h"
#include "table/format.h"
#include "port/port.h"

namespace rocksdb {

AdaptiveTableFactory::AdaptiveTableFactory(
    std::shared_ptr<TableFactory> table_factory_to_write,
    std::shared_ptr<TableFactory> block_based_table_factory,
    std::shared_ptr<TableFactory> plain_table_factory,
    std::shared_ptr<TableFactory> cuckoo_table_factory)
    : table_factory_to_write_(table_factory_to_write),
      block_based_table_factory_(block_based_table_factory),
      plain_table_factory_(plain_table_factory),
      cuckoo_table_factory_(cuckoo_table_factory) {
  if (!plain_table_factory_) {
    plain_table_factory_.reset(NewPlainTableFactory());
  }
  if (!block_based_table_factory_) {
    block_based_table_factory_.reset(NewBlockBasedTableFactory());
  }
  if (!cuckoo_table_factory_) {
    cuckoo_table_factory_.reset(NewCuckooTableFactory());
  }
  if (!table_factory_to_write_) {
    table_factory_to_write_ = block_based_table_factory_;
  }
}

extern const uint64_t kPlainTableMagicNumber;
extern const uint64_t kLegacyPlainTableMagicNumber;
extern const uint64_t kBlockBasedTableMagicNumber;
extern const uint64_t kLegacyBlockBasedTableMagicNumber;
extern const uint64_t kCuckooTableMagicNumber;

Status AdaptiveTableFactory::NewTableReader(
    const TableReaderOptions& table_reader_options,
    unique_ptr<RandomAccessFileReader>&& file, uint64_t file_size,
    unique_ptr<TableReader>* table,
    bool prefetch_index_and_filter_in_cache) const {
  Footer footer;
  /*o s=readfooterfromfile（file.get（），nullptr/*预取缓冲区*/，
                              文件大小和页脚）；
  如果（！）S.O.（））{
    返回S；
  }
  if（footer.table_magic_number（）==kPlainTablemagicNumber_
      footer.table_magic_number（）==klegacycplainTablemagicNumber）
    返回plain_table_factory_u->newTableReader（
        表阅读器选项，标准：：移动（文件），文件大小，表）；
  else if（footer.table_magic_number（）==kblockbasedTablemagicNumber_
      footer.table_magic_number（）==klegacycblockbasedTablemagicNumber）
    返回块基于表工厂的newTableReader（
        表阅读器选项，标准：：移动（文件），文件大小，表）；
  else if（footer.table_magic_number（）==kCuckooTablemagicNumber）
    返回布谷鸟表工厂新表阅读器（
        表阅读器选项，标准：：移动（文件），文件大小，表）；
  }否则{
    返回状态：：不支持（“未标识的表格式”）；
  }
}

TableBuilder*自适应表工厂：：NewTableBuilder（
    const tablebuilderOptions&table_builder_options，uint32_t column_family_id，
    writtablefilewriter*文件）const
  将table_factory_返回到_write_u->newTableBuilder（table_builder_选项，
                                                  列_family_id，file）；
}

std:：string adaptiveTableFactory:：GetPrintableTableOptions（）常量
  std：：字符串ret；
  重置储备（20000）；
  const int kbuffersize=200；
  字符缓冲区[KbufferSize]；

  if（表“工厂”到“写入”）；
    snprintf（buffer，kbuffersize，“写入工厂（%s）选项：\n%s\n”，
             （表“工厂”到“写入”->name（）？表\u工厂\u到\u写入\->name（）。
                                              ：“”，
             table_factory_to_write_u->getprintableTableOptions（）.c_str（））；
    ret.append（缓冲区）；
  }
  如果（普通的“桌子”工厂）
    snprintf（buffer，kbuffersize，“%s选项：\n%s\n”，
             普通表格工厂名称（）？plain_table_factory_u->name（）：“”，
             普通的_table_factory_u->getprintabletableoptions（）.c_str（））；
    ret.append（缓冲区）；
  }
  if（基于block_的_table_factory_
    SnPrTNF
        buffer，kbuffersize，“%s选项：\n%s\n”，
        （block_-based_-table_-factory_u->name（）？基于块的表工厂
                                            ：“”，
        block_-based_-table_-factory_u->getprintableTableOptions（）.c_str（））；
    ret.append（缓冲区）；
  }
  如果（布谷鸟桌工厂）
    snprintf（buffer，kbuffersize，“%s选项：\n%s\n”，
             布谷鸟表工厂名称（）？布谷鸟表工厂名称（）：“”，
             布谷鸟_table_factory_u->getprintabletableoptions（）.c_str（））；
    ret.append（缓冲区）；
  }
  返回RET；
}

外部表工厂*新的适应性表工厂（
    std:：shared_ptr<table factory>table_factory_to_write，
    std:：shared_ptr<table factory>block_-based_-table_-factory，
    std:：shared_ptr<table factory>plain_table_factory，
    std:：shared_ptr<table factory>布谷鸟_table_factory）
  返回新的adaptiveTableFactory（Table_Factory_to_write，
      布洛克洹台洹厂、素洹台洹厂、布谷洹台洹厂）；
}

//命名空间rocksdb
endif//rocksdb_lite
