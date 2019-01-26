
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

#include "rocksdb/utilities/leveldb_options.h"
#include "rocksdb/cache.h"
#include "rocksdb/comparator.h"
#include "rocksdb/env.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"

namespace rocksdb {

LevelDBOptions::LevelDBOptions()
    : comparator(BytewiseComparator()),
      create_if_missing(false),
      error_if_exists(false),
      paranoid_checks(false),
      env(Env::Default()),
      info_log(nullptr),
      write_buffer_size(4 << 20),
      max_open_files(1000),
      block_cache(nullptr),
      block_size(4096),
      block_restart_interval(16),
      compression(kSnappyCompression),
      filter_policy(nullptr) {}

Options ConvertOptions(const LevelDBOptions& leveldb_options) {
  Options options = Options();
  options.create_if_missing = leveldb_options.create_if_missing;
  options.error_if_exists = leveldb_options.error_if_exists;
  options.paranoid_checks = leveldb_options.paranoid_checks;
  options.env = leveldb_options.env;
  options.info_log.reset(leveldb_options.info_log);
  options.write_buffer_size = leveldb_options.write_buffer_size;
  options.max_open_files = leveldb_options.max_open_files;
  options.compression = leveldb_options.compression;

  BlockBasedTableOptions table_options;
  table_options.block_cache.reset(leveldb_options.block_cache);
  table_options.block_size = leveldb_options.block_size;
  table_options.block_restart_interval = leveldb_options.block_restart_interval;
  table_options.filter_policy.reset(leveldb_options.filter_policy);
  options.table_factory.reset(NewBlockBasedTableFactory(table_options));

  return options;
}

}  //命名空间rocksdb
