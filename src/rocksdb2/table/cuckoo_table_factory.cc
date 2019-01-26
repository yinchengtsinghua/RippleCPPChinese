
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

#ifndef ROCKSDB_LITE
#include "table/cuckoo_table_factory.h"

#include "db/dbformat.h"
#include "table/cuckoo_table_builder.h"
#include "table/cuckoo_table_reader.h"

namespace rocksdb {

Status CuckooTableFactory::NewTableReader(
    const TableReaderOptions& table_reader_options,
    unique_ptr<RandomAccessFileReader>&& file, uint64_t file_size,
    std::unique_ptr<TableReader>* table,
    bool prefetch_index_and_filter_in_cache) const {
  std::unique_ptr<CuckooTableReader> new_reader(new CuckooTableReader(
      table_reader_options.ioptions, std::move(file), file_size,
      table_reader_options.internal_comparator.user_comparator(), nullptr));
  Status s = new_reader->status();
  if (s.ok()) {
    *table = std::move(new_reader);
  }
  return s;
}

TableBuilder* CuckooTableFactory::NewTableBuilder(
    const TableBuilderOptions& table_builder_options, uint32_t column_family_id,
    WritableFileWriter* file) const {
//忽略skipfilters标志。不适用于此文件格式
//

//TODO:将生成器更改为采用选项结构
  return new CuckooTableBuilder(
      file, table_options_.hash_table_ratio, 64,
      table_options_.max_search_depth,
      table_builder_options.internal_comparator.user_comparator(),
      table_options_.cuckoo_block_size, table_options_.use_module_hash,
      /*le_options_uuIdentity_as_first_hash，nullptr/*get_slice_hash*/，
      列\u family_id，表\u builder_options.column_family_name）；
}

std:：string布谷表工厂：：getPrintableTableOptions（）常量
  std：：字符串ret；
  ret.reserve（2000年）；
  const int kbuffersize=200；
  字符缓冲区[KbufferSize]；

  snprintf（buffer，kbuffersize，“哈希表比率：%lf\n”，
           表\选项\哈希\表\比率）；
  ret.append（缓冲区）；
  snprintf（buffer，kbuffersize，“最大搜索深度：%u\n”，
           表\选项\最大搜索深度）；
  ret.append（缓冲区）；
  snprintf（buffer，kbuffersize，“布谷鸟块大小：%u\n”，
           表“布谷鸟块大小”选项；
  ret.append（缓冲区）；
  snprintf（buffer，kbuffersize，“标识\u作为\u第一个哈希：%d\n”，
           表\选项\标识\作为\第一个\散列）；
  ret.append（缓冲区）；
  返回RET；
}

TableFactory*新布谷鸟表工厂（const布谷鸟表选项和表选项）
  返回新布谷台工厂（表选项）；
}

//命名空间rocksdb
endif//rocksdb_lite
