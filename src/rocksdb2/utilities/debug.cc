
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

#include "rocksdb/utilities/debug.h"

#include "db/db_impl.h"

namespace rocksdb {

Status GetAllKeyVersions(DB* db, Slice begin_key, Slice end_key,
                         std::vector<KeyVersion>* key_versions) {
  assert(key_versions != nullptr);
  key_versions->clear();

  DBImpl* idb = static_cast<DBImpl*>(db->GetRootDB());
  auto icmp = InternalKeyComparator(idb->GetOptions().comparator);
  /*gedelaggregator range_del_agg（ICMP，/*快照*/）；
  竞技场竞技场；
  ScopedaRenaiterator ITER（IDB->NewInternalIterator（&Arena，&Range_del_agg））；

  如果（！）begin_key.empty（））
    Internalkey伊基；
    ikey.setmaxpossibleforuserkey（begin_key）；
    iter->seek（ikey.encode（））；
  }否则{
    iter->seektofirst（）；
  }

  for（；iter->valid（）；iter->next（））
    Parsedinteralkey伊基；
    如果（！）parseInternalKey（iter->key（），&ikey））
      返回状态：：损坏（“内部键[”+ITER->Key（）.ToString（）+
                                “]分析错误！”）；
    }

    如果（！）结束\key.empty（）&&
        icmp.user_comparator（）->比较（ikey.user_key，end_key）>0）
      断裂；
    }

    key_versions->emplace_back（ikey.user_key.toString（）/*_user_key*/,

                               /*r->value（）.toString（）/*\u value*/，
                               ikey.sequence/*序列*/,

                               /*tic_cast<int>（ikey.type）/*_type*/）；
  }
  返回状态：：OK（）；
}

//命名空间rocksdb

endif//rocksdb_lite
