
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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "utilities/transactions/transaction_util.h"

#include <inttypes.h>
#include <string>
#include <vector>

#include "db/db_impl.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/write_batch_with_index.h"
#include "util/string_util.h"

namespace rocksdb {

Status TransactionUtil::CheckKeyForConflicts(DBImpl* db_impl,
                                             ColumnFamilyHandle* column_family,
                                             const std::string& key,
                                             SequenceNumber key_seq,
                                             bool cache_only) {
  Status result;

  auto cfh = reinterpret_cast<ColumnFamilyHandleImpl*>(column_family);
  auto cfd = cfh->cfd();
  SuperVersion* sv = db_impl->GetAndRefSuperVersion(cfd);

  if (sv == nullptr) {
    result = Status::InvalidArgument("Could not access column family " +
                                     cfh->GetName());
  }

  if (result.ok()) {
    SequenceNumber earliest_seq =
        db_impl->GetEarliestMemTableSequenceNumber(sv, true);

    result = CheckKey(db_impl, sv, earliest_seq, key_seq, key, cache_only);

    db_impl->ReturnAndCleanupSuperVersion(cfd, sv);
  }

  return result;
}

Status TransactionUtil::CheckKey(DBImpl* db_impl, SuperVersion* sv,
                                 SequenceNumber earliest_seq,
                                 SequenceNumber key_seq, const std::string& key,
                                 bool cache_only) {
  Status result;
  bool need_to_read_sst = false;

//由于检查SST文件的速度太慢，我们将只使用
//用于检查最近是否有任何写入的memtables
//在该事务中访问该密钥之后。但是如果
//memtables没有足够长的历史记录，我们必须使
//交易。
  if (earliest_seq == kMaxSequenceNumber) {
//这张记忆表的年代不详。不能依靠它来检查
//用于最近的写入。这种错误在实践中不应该经常发生，因为
//memtable应该有一个有效的最早序列号，除了
//角落案例（如恢复过程中的错误案例）。
    need_to_read_sst = true;

    if (cache_only) {
      result = Status::TryAgain(
          "Transaction ould not check for conflicts as the MemTable does not "
          "countain a long enough history to check write at SequenceNumber: ",
          ToString(key_seq));
    }
  } else if (key_seq < earliest_seq) {
    need_to_read_sst = true;

    if (cache_only) {
//此memtable的年龄太新，无法用于检查最近的
//写。
      char msg[300];
      snprintf(msg, sizeof(msg),
               "Transaction could not check for conflicts for operation at "
               "SequenceNumber %" PRIu64
               " as the MemTable only contains changes newer than "
               "SequenceNumber %" PRIu64
               ".  Increasing the value of the "
               "max_write_buffer_number_to_maintain option could reduce the "
               "frequency "
               "of this error.",
               key_seq, earliest_seq);
      result = Status::TryAgain(msg);
    }
  }

  if (result.ok()) {
    SequenceNumber seq = kMaxSequenceNumber;
    bool found_record_for_key = false;

    Status s = db_impl->GetLatestSequenceForKey(sv, key, !need_to_read_sst,
                                                &seq, &found_record_for_key);

    if (!(s.ok() || s.IsNotFound() || s.IsMergeInProgress())) {
      result = s;
    } else if (found_record_for_key && (seq > key_seq)) {
//写冲突
      result = Status::Busy();
    }
  }

  return result;
}

Status TransactionUtil::CheckKeysForConflicts(DBImpl* db_impl,
                                              const TransactionKeyMap& key_map,
                                              bool cache_only) {
  Status result;

  for (auto& key_map_iter : key_map) {
    uint32_t cf_id = key_map_iter.first;
    const auto& keys = key_map_iter.second;

    SuperVersion* sv = db_impl->GetAndRefSuperVersion(cf_id);
    if (sv == nullptr) {
      result = Status::InvalidArgument("Could not access column family " +
                                       ToString(cf_id));
      break;
    }

    SequenceNumber earliest_seq =
        db_impl->GetEarliestMemTableSequenceNumber(sv, true);

//对于此事务中的每个键，检查是否有人
//自事务启动后写入此密钥。
    for (const auto& key_iter : keys) {
      const auto& key = key_iter.first;
      const SequenceNumber key_seq = key_iter.second.seq;

      result = CheckKey(db_impl, sv, earliest_seq, key_seq, key, cache_only);

      if (!result.ok()) {
        break;
      }
    }

    db_impl->ReturnAndCleanupSuperVersion(cf_id, sv);

    if (!result.ok()) {
      break;
    }
  }

  return result;
}


}  //命名空间rocksdb

#endif  //摇滚乐
