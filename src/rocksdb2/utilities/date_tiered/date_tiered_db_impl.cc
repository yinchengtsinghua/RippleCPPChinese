
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

#include "utilities/date_tiered/date_tiered_db_impl.h"

#include <limits>

#include "db/db_impl.h"
#include "db/db_iter.h"
#include "db/write_batch_internal.h"
#include "monitoring/instrumented_mutex.h"
#include "options/options_helper.h"
#include "rocksdb/convenience.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/utilities/date_tiered_db.h"
#include "table/merging_iterator.h"
#include "util/coding.h"
#include "util/filename.h"
#include "util/string_util.h"

namespace rocksdb {

//在datiereddbimpl内打开db，因为选项需要指向其ttl的指针
DateTieredDBImpl::DateTieredDBImpl(
    DB* db, Options options,
    const std::vector<ColumnFamilyDescriptor>& descriptors,
    const std::vector<ColumnFamilyHandle*>& handles, int64_t ttl,
    int64_t column_family_interval)
    : db_(db),
      cf_options_(ColumnFamilyOptions(options)),
      ioptions_(ImmutableCFOptions(options)),
      ttl_(ttl),
      column_family_interval_(column_family_interval),
      mutex_(options.statistics.get(), db->GetEnv(), DB_MUTEX_WAIT_MICROS,
             options.use_adaptive_mutex) {
  latest_timebound_ = std::numeric_limits<int64_t>::min();
  for (size_t i = 0; i < handles.size(); ++i) {
    const auto& name = descriptors[i].name;
    int64_t timestamp = 0;
    try {
      timestamp = ParseUint64(name);
    } catch (const std::invalid_argument&) {
//绕过不相关的列族，例如默认
      db_->DestroyColumnFamilyHandle(handles[i]);
      continue;
    }
    if (timestamp > latest_timebound_) {
      latest_timebound_ = timestamp;
    }
    handle_map_.insert(std::make_pair(timestamp, handles[i]));
  }
}

DateTieredDBImpl::~DateTieredDBImpl() {
  for (auto handle : handle_map_) {
    db_->DestroyColumnFamilyHandle(handle.second);
  }
  delete db_;
  db_ = nullptr;
}

Status DateTieredDB::Open(const Options& options, const std::string& dbname,
                          DateTieredDB** dbptr, int64_t ttl,
                          int64_t column_family_interval, bool read_only) {
  DBOptions db_options(options);
  ColumnFamilyOptions cf_options(options);
  std::vector<ColumnFamilyDescriptor> descriptors;
  std::vector<ColumnFamilyHandle*> handles;
  DB* db;
  Status s;

//获取列族
  std::vector<std::string> column_family_names;
  s = DB::ListColumnFamilies(db_options, dbname, &column_family_names);
  if (!s.ok()) {
//找不到列族。使用默认值
    s = DB::Open(options, dbname, &db);
    if (!s.ok()) {
      return s;
    }
  } else {
    for (auto name : column_family_names) {
      descriptors.emplace_back(ColumnFamilyDescriptor(name, cf_options));
    }

//开放式数据库
    if (read_only) {
      s = DB::OpenForReadOnly(db_options, dbname, descriptors, &handles, &db);
    } else {
      s = DB::Open(db_options, dbname, descriptors, &handles, &db);
    }
  }

  if (s.ok()) {
    *dbptr = new DateTieredDBImpl(db, options, descriptors, handles, ttl,
                                  column_family_interval);
  }
  return s;
}

//根据提供的TTL检查字符串是否过时
bool DateTieredDBImpl::IsStale(int64_t keytime, int64_t ttl, Env* env) {
  if (ttl <= 0) {
//如果TTL为非正值，则数据是新的
    return false;
  }
  int64_t curtime;
  if (!env->GetCurrentTime(&curtime).ok()) {
//如果无法获得当前时间，则将数据视为新数据
    return false;
  }
  return curtime >= keytime + ttl;
}

//当列族中的所有数据都过期时删除列族
//托多（JHLI）：可以做背景工作
Status DateTieredDBImpl::DropObsoleteColumnFamilies() {
  int64_t curtime;
  Status s;
  s = db_->GetEnv()->GetCurrentTime(&curtime);
  if (!s.ok()) {
    return s;
  }
  {
    InstrumentedMutexLock l(&mutex_);
    auto iter = handle_map_.begin();
    while (iter != handle_map_.end()) {
      if (iter->first <= curtime - ttl_) {
        s = db_->DropColumnFamily(iter->second);
        if (!s.ok()) {
          return s;
        }
        delete iter->second;
        iter = handle_map_.erase(iter);
      } else {
        break;
      }
    }
  }
  return Status::OK();
}

//从用户密钥获取时间戳
Status DateTieredDBImpl::GetTimestamp(const Slice& key, int64_t* result) {
  if (key.size() < kTSLength) {
    return Status::Corruption("Bad timestamp in key");
  }
  const char* pos = key.data() + key.size() - 8;
  int64_t timestamp = 0;
  if (port::kLittleEndian) {
    int bytes_to_fill = 8;
    for (int i = 0; i < bytes_to_fill; ++i) {
      timestamp |= (static_cast<uint64_t>(static_cast<unsigned char>(pos[i]))
                    << ((bytes_to_fill - i - 1) << 3));
    }
  } else {
    memcpy(&timestamp, pos, sizeof(timestamp));
  }
  *result = timestamp;
  return Status::OK();
}

Status DateTieredDBImpl::CreateColumnFamily(
    ColumnFamilyHandle** column_family) {
  int64_t curtime;
  Status s;
  mutex_.AssertHeld();
  s = db_->GetEnv()->GetCurrentTime(&curtime);
  if (!s.ok()) {
    return s;
  }
  int64_t new_timebound;
  if (handle_map_.empty()) {
    new_timebound = curtime + column_family_interval_;
  } else {
    new_timebound =
        latest_timebound_ +
        ((curtime - latest_timebound_) / column_family_interval_ + 1) *
            column_family_interval_;
  }
  std::string cf_name = ToString(new_timebound);
  latest_timebound_ = new_timebound;
  s = db_->CreateColumnFamily(cf_options_, cf_name, column_family);
  if (s.ok()) {
    handle_map_.insert(std::make_pair(new_timebound, *column_family));
  }
  return s;
}

Status DateTieredDBImpl::FindColumnFamily(int64_t keytime,
                                          ColumnFamilyHandle** column_family,
                                          bool create_if_missing) {
  *column_family = nullptr;
  {
    InstrumentedMutexLock l(&mutex_);
    auto iter = handle_map_.upper_bound(keytime);
    if (iter == handle_map_.end()) {
      if (!create_if_missing) {
        return Status::NotFound();
      } else {
        return CreateColumnFamily(column_family);
      }
    }
//移动到上一个元素以获取适当的时间窗口
    *column_family = iter->second;
  }
  return Status::OK();
}

Status DateTieredDBImpl::Put(const WriteOptions& options, const Slice& key,
                             const Slice& val) {
  int64_t timestamp = 0;
  Status s;
  s = GetTimestamp(key, &timestamp);
  if (!s.ok()) {
    return s;
  }
  DropObsoleteColumnFamilies();

//删除对过时数据的请求
  if (IsStale(timestamp, ttl_, db_->GetEnv())) {
    return Status::InvalidArgument();
  }

//决定要放入的列族（即时间窗口）
  ColumnFamilyHandle* column_family;
  /*findcolumnfamily（timestamp，&column_family，true/*如果_丢失，则创建_*/）；
  如果（！）S.O.（））{
    返回S；
  }

  //使用writebatch高效放置
  写入批处理；
  batch.put（columneu-family，key，val）；
  返回写入（选项和批处理）；
}

状态日期reddbimpl:：get（const readoptions&options，const slice&key，
                             标准：：字符串*值）
  int64_t timestamp=0；
  状态S；
  S=GetTimeStamp（键和时间戳）；
  如果（！）S.O.（））{
    返回S；
  }
  //删除对过时数据的请求
  if（isstale（timestage，ttl_，db_u->getenv（））
    返回状态：：NotFound（）；
  }

  //决定要从中获取的列族
  column系列handle*column_系列；
  s=findcolumnfamily（timestamp，&column_family，false/*如果不允许，则创建_*/);

  if (!s.ok()) {
    return s;
  }
  if (column_family == nullptr) {
//找不到列族
    return Status::NotFound();
  }

//用键获取值
  return db_->Get(options, column_family, key, value);
}

bool DateTieredDBImpl::KeyMayExist(const ReadOptions& options, const Slice& key,
                                   std::string* value, bool* value_found) {
  int64_t timestamp = 0;
  Status s;
  s = GetTimestamp(key, &timestamp);
  if (!s.ok()) {
//无法获取当前时间
    return false;
  }
//确定要从中获取的列族
  ColumnFamilyHandle* column_family;
  /*findcolumnfamily（timestamp，&column_family，false/*如果_丢失，则创建_*/）；
  如果（！）s.ok（）column_family==nullptr）
    //找不到列族
    返回错误；
  }
  if（isstale（timestage，ttl_，db_u->getenv（））
    返回错误；
  }
  返回db_u->keymayexist（选项、列_Family、键、值、找到的值）；
}

状态日期reddbimpl：：删除（const writeoptions&options，const slice&key）
  int64_t timestamp=0；
  状态S；
  S=GetTimeStamp（键和时间戳）；
  如果（！）S.O.（））{
    返回S；
  }
  DropoBosoleteColumnFamilies（）；
  //删除对过时数据的请求
  if（isstale（timestage，ttl_，db_u->getenv（））
    返回状态：：NotFound（）；
  }

  //决定要从中获取的列族
  column系列handle*column_系列；
  s=findcolumnfamily（timestamp，&column_family，false/*如果不允许，则创建_*/);

  if (!s.ok()) {
    return s;
  }
  if (column_family == nullptr) {
//找不到列族
    return Status::NotFound();
  }

//用键获取值
  return db_->Delete(options, column_family, key);
}

Status DateTieredDBImpl::Merge(const WriteOptions& options, const Slice& key,
                               const Slice& value) {
//确定要从中获取的列族
  int64_t timestamp = 0;
  Status s;
  s = GetTimestamp(key, &timestamp);
  if (!s.ok()) {
//无法获取当前时间
    return s;
  }
  ColumnFamilyHandle* column_family;
  /*findcolumnfamily（timestamp，&column_family，true/*如果_丢失，则创建_*/）；
  如果（！）S.O.（））{
    返回S；
  }
  写入批处理；
  batch.merge（列\族，键，值）；
  返回写入（选项和批处理）；
}

状态日期reddbimpl:：write（const writeoptions&opts，writebatch*更新）
  类处理程序：public writebatch:：handler_
   公众：
    显式处理程序（）
    writebatch更新\ttl；
    状态批改写状态；
    虚拟状态putcf（uint32_t column_family_id，const slice&key，
                         常量切片和值）重写
      writebatchInternal:：Put（&updates_ttl，column_family_id，key，value）；
      返回状态：：OK（）；
    }
    虚拟状态合并cf（uint32_t column_family_id，const slice&key，
                           常量切片和值）重写
      writebatchInternal:：Merge（&updates_ttl，column_family_id，key，value）；
      返回状态：：OK（）；
    }
    虚拟状态删除cf（uint32_t列_family_id，
                            const slice&key）覆盖
      writebatchInternal：：删除（&updates_ttl，column_family_id，key）；
      返回状态：：OK（）；
    }
    虚拟void logdata（const slice&blob）覆盖
      更新ttl.putlogdata（blob）；
    }
  }；
  经办人；
  更新->迭代（&handler）；
  如果（！）handler.batch_rewrite_status.ok（））
    返回handler.batch_rewrite_status；
  }否则{
    返回db_u->write（opts，&（handler.updates_ttl））；
  }
}

迭代器*日期reddbimpl:：new迭代器（const readoptions&opts）
  if（handle_map_.empty（））
    返回newEmptyIterator（）；
  }

  db impl*db_impl=reinterpret_cast<db impl*>（db_uuu）；

  auto-db-iter=newarenawrappedBiterator（
      数据库impl->getenv（），opts，ioptions，kmaxSequenceNumber，
      cf_u options_u.max_sequential_skip_in_iterations，0）；

  auto arena=db_iter->getarena（）；
  mergeiteratorbuilder builder构建器（cf_u options_u.comparator，arena）；
  for（auto&item:handle_map_）
    自动处理=item.second；
    builder.additerator（db_impl->newInternalIterator（
        竞技场，db-iter->getRangeDelaggregator（），handle））；
  }
  auto internal_iter=builder.finish（）；
  db iter->setiterunderdbiter（内部iter）；
  返回数据库；
}
//命名空间rocksdb
endif//rocksdb_lite
