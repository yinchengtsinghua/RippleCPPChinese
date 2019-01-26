
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

#include "rocksdb/utilities/option_change_migration.h"

#ifndef ROCKSDB_LITE
#include "rocksdb/db.h"

namespace rocksdb {
namespace {
//返回允许我们打开/写入数据库的选项“opts”的版本
//不会触发自动压缩或停止。这是有保证的
//通过禁用自动压缩并使用巨大的值来停止
//触发器。
Options GetNoCompactionOptions(const Options& opts) {
  Options ret_opts = opts;
  ret_opts.disable_auto_compactions = true;
  ret_opts.level0_slowdown_writes_trigger = 999999;
  ret_opts.level0_stop_writes_trigger = 999999;
  ret_opts.soft_pending_compaction_bytes_limit = 0;
  ret_opts.hard_pending_compaction_bytes_limit = 0;
  return ret_opts;
}

Status OpenDb(const Options& options, const std::string& dbname,
              std::unique_ptr<DB>* db) {
  db->reset();
  DB* tmpdb;
  Status s = DB::Open(options, dbname, &tmpdb);
  if (s.ok()) {
    db->reset(tmpdb);
  }
  return s;
}

Status CompactToLevel(const Options& options, const std::string& dbname,
                      int dest_level, bool need_reopen) {
  std::unique_ptr<DB> db;
  Options no_compact_opts = GetNoCompactionOptions(options);
  if (dest_level == 0) {
//L0对它的文件有严格的SequenceID要求。它更安全
//只放一个压缩文件到那里。
//这仅用于转换为通用压缩
//只有一个级别。在这种情况下，压缩到一个文件也是
//最优的。
    no_compact_opts.target_file_size_base = 999999999999999;
    no_compact_opts.max_compaction_bytes = 999999999999999;
  }
  Status s = OpenDb(no_compact_opts, dbname, &db);
  if (!s.ok()) {
    return s;
  }
  CompactRangeOptions cro;
  cro.change_level = true;
  cro.target_level = dest_level;
  if (dest_level == 0) {
    cro.bottommost_level_compaction = BottommostLevelCompaction::kForce;
  }
  db->CompactRange(cro, nullptr, nullptr);

  if (need_reopen) {
//需要重新启动数据库以重写清单文件。
//为了打开具有特定num_级别的数据库，清单文件应该
//不包含任何超过num_级别的记录。发行一个
//完全压缩会将所有数据移动到不超过
//num_级别，但清单可能仍包含以前提到的记录
//更高的层次。重新打开数据库将强制重写清单
//以便清除这些记录。
    db.reset();
    s = OpenDb(no_compact_opts, dbname, &db);
  }
  return s;
}

Status MigrateToUniversal(std::string dbname, const Options& old_opts,
                          const Options& new_opts) {
  if (old_opts.num_levels <= new_opts.num_levels ||
      old_opts.compaction_style == CompactionStyle::kCompactionStyleFIFO) {
    return Status::OK();
  } else {
    bool need_compact = false;
    {
      std::unique_ptr<DB> db;
      Options opts = GetNoCompactionOptions(old_opts);
      Status s = OpenDb(opts, dbname, &db);
      if (!s.ok()) {
        return s;
      }
      ColumnFamilyMetaData metadata;
      db->GetColumnFamilyMetaData(&metadata);
      if (!metadata.levels.empty() &&
          metadata.levels.back().level >= new_opts.num_levels) {
        need_compact = true;
      }
    }
    if (need_compact) {
      return CompactToLevel(old_opts, dbname, new_opts.num_levels - 1, true);
    }
    return Status::OK();
  }
}

Status MigrateToLevelBase(std::string dbname, const Options& old_opts,
                          const Options& new_opts) {
  if (!new_opts.level_compaction_dynamic_level_bytes) {
    if (old_opts.num_levels == 1) {
      return Status::OK();
    }
//将所有部件压缩到1级，以确保安全打开。
    Options opts = old_opts;
    opts.target_file_size_base = new_opts.target_file_size_base;
//虽然有时我们可以用新选项打开数据库而不会出错，
//我们仍然希望压缩文件以避免LSM树卡住
//形状不好。例如，如果用户更改了级别大小
//乘数从4到8，有相同的数据，我们将有更少的
//水平。除非我们发出完整的命令，否则LSM树可能会卡住
//它的级别比需要的多，并且不会自动恢复。
    return CompactToLevel(opts, dbname, 1, true);
  } else {
//把所有东西压缩到最后一层，以保证安全。
//开的。
    if (old_opts.num_levels == 1) {
      return Status::OK();
    } else if (new_opts.num_levels > old_opts.num_levels) {
//动态级别模式要求先将数据放入最后一个级别。
      return CompactToLevel(new_opts, dbname, new_opts.num_levels - 1, false);
    } else {
      Options opts = old_opts;
      opts.target_file_size_base = new_opts.target_file_size_base;
      return CompactToLevel(opts, dbname, new_opts.num_levels - 1, true);
    }
  }
}
}  //命名空间

Status OptionChangeMigration(std::string dbname, const Options& old_opts,
                             const Options& new_opts) {
  if (old_opts.compaction_style == CompactionStyle::kCompactionStyleFIFO) {
//由FIFO兼容生成的LSM可以通过任何压缩打开。
    return Status::OK();
  } else if (new_opts.compaction_style ==
             CompactionStyle::kCompactionStyleUniversal) {
    return MigrateToUniversal(dbname, old_opts, new_opts);
  } else if (new_opts.compaction_style ==
             CompactionStyle::kCompactionStyleLevel) {
    return MigrateToLevelBase(dbname, old_opts, new_opts);
  } else if (new_opts.compaction_style ==
             CompactionStyle::kCompactionStyleFIFO) {
    return CompactToLevel(old_opts, dbname, 0, true);
  } else {
    return Status::NotSupported(
        "Do not how to migrate to this compaction style");
  }
}
}  //命名空间rocksdb
#else
namespace rocksdb {
Status OptionChangeMigration(std::string dbname, const Options& old_opts,
                             const Options& new_opts) {
  return Status::NotSupported();
}
}  //命名空间rocksdb
#endif  //摇滚乐
