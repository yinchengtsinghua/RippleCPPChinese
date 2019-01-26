
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2017至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#include "format.h"

#include <algorithm>
#include <map>
#include <memory>

#include "utilities/cassandra/serialize.h"

namespace rocksdb {
namespace cassandra {
namespace {
const int32_t kDefaultLocalDeletionTime =
  std::numeric_limits<int32_t>::max();
const int64_t kDefaultMarkedForDeleteAt =
  std::numeric_limits<int64_t>::min();
}

ColumnBase::ColumnBase(int8_t mask, int8_t index)
  : mask_(mask), index_(index) {}

std::size_t ColumnBase::Size() const {
  return sizeof(mask_) + sizeof(index_);
}

int8_t ColumnBase::Mask() const {
  return mask_;
}

int8_t ColumnBase::Index() const {
  return index_;
}

void ColumnBase::Serialize(std::string* dest) const {
  rocksdb::cassandra::Serialize<int8_t>(mask_, dest);
  rocksdb::cassandra::Serialize<int8_t>(index_, dest);
}

std::shared_ptr<ColumnBase> ColumnBase::Deserialize(const char* src,
                                                    std::size_t offset) {
  int8_t mask = rocksdb::cassandra::Deserialize<int8_t>(src, offset);
  if ((mask & ColumnTypeMask::DELETION_MASK) != 0) {
    return Tombstone::Deserialize(src, offset);
  } else if ((mask & ColumnTypeMask::EXPIRATION_MASK) != 0) {
    return ExpiringColumn::Deserialize(src, offset);
  } else {
    return Column::Deserialize(src, offset);
  }
}

Column::Column(
  int8_t mask,
  int8_t index,
  int64_t timestamp,
  int32_t value_size,
  const char* value
) : ColumnBase(mask, index), timestamp_(timestamp),
  value_size_(value_size), value_(value) {}

int64_t Column::Timestamp() const {
  return timestamp_;
}

std::size_t Column::Size() const {
  return ColumnBase::Size() + sizeof(timestamp_) + sizeof(value_size_)
    + value_size_;
}

void Column::Serialize(std::string* dest) const {
  ColumnBase::Serialize(dest);
  rocksdb::cassandra::Serialize<int64_t>(timestamp_, dest);
  rocksdb::cassandra::Serialize<int32_t>(value_size_, dest);
  dest->append(value_, value_size_);
}

std::shared_ptr<Column> Column::Deserialize(const char *src,
                                            std::size_t offset) {
  int8_t mask = rocksdb::cassandra::Deserialize<int8_t>(src, offset);
  offset += sizeof(mask);
  int8_t index = rocksdb::cassandra::Deserialize<int8_t>(src, offset);
  offset += sizeof(index);
  int64_t timestamp = rocksdb::cassandra::Deserialize<int64_t>(src, offset);
  offset += sizeof(timestamp);
  int32_t value_size = rocksdb::cassandra::Deserialize<int32_t>(src, offset);
  offset += sizeof(value_size);
  return std::make_shared<Column>(
    mask, index, timestamp, value_size, src + offset);
}

ExpiringColumn::ExpiringColumn(
  int8_t mask,
  int8_t index,
  int64_t timestamp,
  int32_t value_size,
  const char* value,
  int32_t ttl
) : Column(mask, index, timestamp, value_size, value),
  ttl_(ttl) {}

std::size_t ExpiringColumn::Size() const {
  return Column::Size() + sizeof(ttl_);
}

void ExpiringColumn::Serialize(std::string* dest) const {
  Column::Serialize(dest);
  rocksdb::cassandra::Serialize<int32_t>(ttl_, dest);
}

std::chrono::time_point<std::chrono::system_clock> ExpiringColumn::TimePoint() const {
  return std::chrono::time_point<std::chrono::system_clock>(std::chrono::microseconds(Timestamp()));
}

std::chrono::seconds ExpiringColumn::Ttl() const {
  return std::chrono::seconds(ttl_);
}

bool ExpiringColumn::Expired() const {
  return TimePoint() + Ttl() < std::chrono::system_clock::now();
}

std::shared_ptr<Tombstone> ExpiringColumn::ToTombstone() const {
  auto expired_at = (TimePoint() + Ttl()).time_since_epoch();
  int32_t local_deletion_time = static_cast<int32_t>(
    std::chrono::duration_cast<std::chrono::seconds>(expired_at).count());
  int64_t marked_for_delete_at =
    std::chrono::duration_cast<std::chrono::microseconds>(expired_at).count();
  return std::make_shared<Tombstone>(
    ColumnTypeMask::DELETION_MASK,
    Index(),
    local_deletion_time,
    marked_for_delete_at);
}

std::shared_ptr<ExpiringColumn> ExpiringColumn::Deserialize(
    const char *src,
    std::size_t offset) {
  int8_t mask = rocksdb::cassandra::Deserialize<int8_t>(src, offset);
  offset += sizeof(mask);
  int8_t index = rocksdb::cassandra::Deserialize<int8_t>(src, offset);
  offset += sizeof(index);
  int64_t timestamp = rocksdb::cassandra::Deserialize<int64_t>(src, offset);
  offset += sizeof(timestamp);
  int32_t value_size = rocksdb::cassandra::Deserialize<int32_t>(src, offset);
  offset += sizeof(value_size);
  const char* value = src + offset;
  offset += value_size;
  int32_t ttl =  rocksdb::cassandra::Deserialize<int32_t>(src, offset);
  return std::make_shared<ExpiringColumn>(
    mask, index, timestamp, value_size, value, ttl);
}

Tombstone::Tombstone(
  int8_t mask,
  int8_t index,
  int32_t local_deletion_time,
  int64_t marked_for_delete_at
) : ColumnBase(mask, index), local_deletion_time_(local_deletion_time),
  marked_for_delete_at_(marked_for_delete_at) {}

int64_t Tombstone::Timestamp() const {
  return marked_for_delete_at_;
}

std::size_t Tombstone::Size() const {
  return ColumnBase::Size() + sizeof(local_deletion_time_)
    + sizeof(marked_for_delete_at_);
}

void Tombstone::Serialize(std::string* dest) const {
  ColumnBase::Serialize(dest);
  rocksdb::cassandra::Serialize<int32_t>(local_deletion_time_, dest);
  rocksdb::cassandra::Serialize<int64_t>(marked_for_delete_at_, dest);
}

std::shared_ptr<Tombstone> Tombstone::Deserialize(const char *src,
                                                  std::size_t offset) {
  int8_t mask = rocksdb::cassandra::Deserialize<int8_t>(src, offset);
  offset += sizeof(mask);
  int8_t index = rocksdb::cassandra::Deserialize<int8_t>(src, offset);
  offset += sizeof(index);
  int32_t local_deletion_time =
    rocksdb::cassandra::Deserialize<int32_t>(src, offset);
  offset += sizeof(int32_t);
  int64_t marked_for_delete_at =
    rocksdb::cassandra::Deserialize<int64_t>(src, offset);
  return std::make_shared<Tombstone>(
    mask, index, local_deletion_time, marked_for_delete_at);
}

RowValue::RowValue(int32_t local_deletion_time, int64_t marked_for_delete_at)
  : local_deletion_time_(local_deletion_time),
  marked_for_delete_at_(marked_for_delete_at), columns_(),
  last_modified_time_(0) {}

RowValue::RowValue(Columns columns,
                  int64_t last_modified_time)
  : local_deletion_time_(kDefaultLocalDeletionTime),
  marked_for_delete_at_(kDefaultMarkedForDeleteAt),
  columns_(std::move(columns)), last_modified_time_(last_modified_time) {}

std::size_t RowValue::Size() const {
  std::size_t size = sizeof(local_deletion_time_)
    + sizeof(marked_for_delete_at_);
  for (const auto& column : columns_) {
    size += column -> Size();
  }
  return size;
}

int64_t RowValue::LastModifiedTime() const {
  if (IsTombstone()) {
    return marked_for_delete_at_;
  } else {
    return last_modified_time_;
  }
}

bool RowValue::IsTombstone() const {
  return marked_for_delete_at_ > kDefaultMarkedForDeleteAt;
}

void RowValue::Serialize(std::string* dest) const {
  rocksdb::cassandra::Serialize<int32_t>(local_deletion_time_, dest);
  rocksdb::cassandra::Serialize<int64_t>(marked_for_delete_at_, dest);
  for (const auto& column : columns_) {
    column -> Serialize(dest);
  }
}

RowValue RowValue::PurgeTtl(bool* changed) const {
  *changed = false;
  Columns new_columns;
  for (auto& column : columns_) {
    if(column->Mask() == ColumnTypeMask::EXPIRATION_MASK) {
      std::shared_ptr<ExpiringColumn> expiring_column =
        std::static_pointer_cast<ExpiringColumn>(column);

      if(expiring_column->Expired()){
        *changed = true;
        continue;
      }
    }

    new_columns.push_back(column);
  }
  return RowValue(std::move(new_columns), last_modified_time_);
}

RowValue RowValue::ExpireTtl(bool* changed) const {
  *changed = false;
  Columns new_columns;
  for (auto& column : columns_) {
    if(column->Mask() == ColumnTypeMask::EXPIRATION_MASK) {
      std::shared_ptr<ExpiringColumn> expiring_column =
        std::static_pointer_cast<ExpiringColumn>(column);

      if(expiring_column->Expired()) {
        shared_ptr<Tombstone> tombstone = expiring_column->ToTombstone();
        new_columns.push_back(tombstone);
        *changed = true;
        continue;
      }
    }
    new_columns.push_back(column);
  }
  return RowValue(std::move(new_columns), last_modified_time_);
}

bool RowValue::Empty() const {
  return columns_.empty();
}

RowValue RowValue::Deserialize(const char *src, std::size_t size) {
  std::size_t offset = 0;
  assert(size >= sizeof(local_deletion_time_) + sizeof(marked_for_delete_at_));
  int32_t local_deletion_time =
    rocksdb::cassandra::Deserialize<int32_t>(src, offset);
  offset += sizeof(int32_t);
  int64_t marked_for_delete_at =
    rocksdb::cassandra::Deserialize<int64_t>(src, offset);
  offset += sizeof(int64_t);
  if (offset == size) {
    return RowValue(local_deletion_time, marked_for_delete_at);
  }

  assert(local_deletion_time == kDefaultLocalDeletionTime);
  assert(marked_for_delete_at == kDefaultMarkedForDeleteAt);
  Columns columns;
  int64_t last_modified_time = 0;
  while (offset < size) {
    auto c = ColumnBase::Deserialize(src, offset);
    offset += c -> Size();
    assert(offset <= size);
    last_modified_time = std::max(last_modified_time, c -> Timestamp());
    columns.push_back(std::move(c));
  }

  return RowValue(std::move(columns), last_modified_time);
}

//将多个行值合并为一个。
//对于具有相同索引的行中的每一列，我们选择最新的
//时间戳。我们还考虑了行墓碑，通过迭代
//每行的时间戳顺序都是相反的，当我们到达第一行时停止
//行墓碑。
RowValue RowValue::Merge(std::vector<RowValue>&& values) {
  assert(values.size() > 0);
  if (values.size() == 1) {
    return std::move(values[0]);
  }

//按上次修改的时间合并列，并在单击后跳过
//一排墓碑。
  std::sort(values.begin(), values.end(),
    [](const RowValue& r1, const RowValue& r2) {
      return r1.LastModifiedTime() > r2.LastModifiedTime();
    });

  std::map<int8_t, std::shared_ptr<ColumnBase>> merged_columns;
  int64_t tombstone_timestamp = 0;

  for (auto& value : values) {
    if (value.IsTombstone()) {
      if (merged_columns.size() == 0) {
        return std::move(value);
      }
      tombstone_timestamp = value.LastModifiedTime();
      break;
    }
    for (auto& column : value.columns_) {
      int8_t index = column->Index();
      if (merged_columns.find(index) == merged_columns.end()) {
        merged_columns[index] = column;
      } else {
        if (column->Timestamp() > merged_columns[index]->Timestamp()) {
          merged_columns[index] = column;
        }
      }
    }
  }

  int64_t last_modified_time = 0;
  Columns columns;
  for (auto& pair: merged_columns) {
//对于某些行，它的最后一个“修改时间>行逻辑删除时间戳”，但是
//它可能有时间戳比逻辑删除更有效的行，所以我们
//Ned过滤这些行。
    if (pair.second->Timestamp() <= tombstone_timestamp) {
      continue;
    }
    last_modified_time = std::max(last_modified_time, pair.second->Timestamp());
    columns.push_back(std::move(pair.second));
  }
  return RowValue(std::move(columns), last_modified_time);
}

} //名字卡桑德达
} //命名空间rocksdb
