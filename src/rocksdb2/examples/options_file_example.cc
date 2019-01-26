
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
//此文件演示如何使用中定义的实用程序函数。
//rocksdb/utilities/options_util.h打开rocksdb数据库
//记住所有RocksDB选项。
#include <cstdio>
#include <string>
#include <vector>

#include "rocksdb/cache.h"
#include "rocksdb/compaction_filter.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/table.h"
#include "rocksdb/utilities/options_util.h"

using namespace rocksdb;

std::string kDBPath = "/tmp/rocksdb_options_file_example";

namespace {
//虚拟压实过滤器
class DummyCompactionFilter : public CompactionFilter {
 public:
  virtual ~DummyCompactionFilter() {}
  virtual bool Filter(int level, const Slice& key, const Slice& existing_value,
                      std::string* new_value, bool* value_changed) const {
    return false;
  }
  virtual const char* Name() const { return "DummyCompactionFilter"; }
};

}  //命名空间

int main() {
  DBOptions db_opt;
  db_opt.create_if_missing = true;

  std::vector<ColumnFamilyDescriptor> cf_descs;
  cf_descs.push_back({kDefaultColumnFamilyName, ColumnFamilyOptions()});
  cf_descs.push_back({"new_cf", ColumnFamilyOptions()});

//初始化BlockBasedTableOptions
  auto cache = NewLRUCache(1 * 1024 * 1024 * 1024);
  BlockBasedTableOptions bbt_opts;
  bbt_opts.block_size = 32 * 1024;
  bbt_opts.block_cache = cache;

//初始化列族选项
  std::unique_ptr<CompactionFilter> compaction_filter;
  compaction_filter.reset(new DummyCompactionFilter());
  cf_descs[0].options.table_factory.reset(NewBlockBasedTableFactory(bbt_opts));
  cf_descs[0].options.compaction_filter = compaction_filter.get();
  cf_descs[1].options.table_factory.reset(NewBlockBasedTableFactory(bbt_opts));

//销毁并打开数据库
  DB* db;
  Status s = DestroyDB(kDBPath, Options(db_opt, cf_descs[0].options));
  assert(s.ok());
  s = DB::Open(Options(db_opt, cf_descs[0].options), kDBPath, &db);
  assert(s.ok());

//创建列族，rocksdb将保留这些选项。
  ColumnFamilyHandle* cf;
  s = db->CreateColumnFamily(ColumnFamilyOptions(), "new_cf", &cf);
  assert(s.ok());

//关闭数据库
  delete cf;
  delete db;

//在下面的代码中，我们将使用
//存储在db目录中的选项文件。

//加载选项文件。
  DBOptions loaded_db_opt;
  std::vector<ColumnFamilyDescriptor> loaded_cf_descs;
  s = LoadLatestOptions(kDBPath, Env::Default(), &loaded_db_opt,
                        &loaded_cf_descs);
  assert(s.ok());
  assert(loaded_db_opt.create_if_missing == db_opt.create_if_missing);

//初始化每个列族的指针选项
  for (size_t i = 0; i < loaded_cf_descs.size(); ++i) {
    auto* loaded_bbt_opt = reinterpret_cast<BlockBasedTableOptions*>(
        loaded_cf_descs[0].options.table_factory->GetOptions());
//期望与将加载的BlockBasedTableOptions表单文件相同。
    assert(loaded_bbt_opt->block_size == bbt_opts.block_size);
//但是，块缓存需要按照文档手动初始化
//在rocksdb/utilities/options中
    loaded_bbt_opt->block_cache = cache;
  }
//此外，由于指针选项是用默认值初始化的，
//如果非defalut，我们需要正确初始化所有指针选项
//在调用db:：open（）之前使用值。
  assert(loaded_cf_descs[0].options.compaction_filter == nullptr);
  loaded_cf_descs[0].options.compaction_filter = compaction_filter.get();

//使用加载的选项重新打开数据库。
  std::vector<ColumnFamilyHandle*> handles;
  s = DB::Open(loaded_db_opt, kDBPath, loaded_cf_descs, &handles, &db);
  assert(s.ok());

//关闭数据库
  for (auto* handle : handles) {
    delete handle;
  }
  delete db;
}
