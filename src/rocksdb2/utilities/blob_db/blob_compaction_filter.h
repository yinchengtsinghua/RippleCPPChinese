
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
#pragma once
#ifndef ROCKSDB_LITE

#include "rocksdb/compaction_filter.h"
#include "rocksdb/env.h"
#include "utilities/blob_db/blob_index.h"

namespace rocksdb {
namespace blob_db {

//CompactionFilter从基数据库中删除过期的blob索引。
class BlobIndexCompactionFilter : public CompactionFilter {
 public:
  explicit BlobIndexCompactionFilter(uint64_t current_time)
      : current_time_(current_time) {}

  virtual const char* Name() const override {
    return "BlobIndexCompactionFilter";
  }

//筛选过期的blob索引，而不考虑快照。
  virtual bool IgnoreSnapshots() const override { return true; }

  /*tual decision filterv2（int/*level*/，const slice&/*key*/，
                            值类型值\类型，常量切片和值，
                            标准：：字符串*/*新值*/,

                            /*：：string*/*skip_until*/）const override_
    如果（值类型！= KBLBN DEX）{
      退货决定：kkeep；
    }
    blob index blob_索引；
    状态s=blob_index.decodeFrom（值）；
    如果（！）S.O.（））{
      //无法解码blob索引。保持价值。
      退货决定：kkeep；
    }
    if（blob_index.hasttl（）&&blob_index.expiration（）<=current_time_
      /过期
      退货决定：kremove；
    }
    退货决定：kkeep；
  }

 私人：
  计算电流时间；
}；

类blobindexcompactionfilterfactory:public compactionfilterfactory_
 公众：
  显式blobindexcompactionfilterfactory（env*env）：env_

  virtual const char*name（）const override_
    返回“blobindexcompactionfilterfactory”；
  }

  虚拟std:：unique_ptr<compactionfilter>createCompletionfilter（
      const compactionfilter：：上下文&/*上下文*/) override {

    int64_t current_time = 0;
    Status s = env_->GetCurrentTime(&current_time);
    if (!s.ok()) {
      return nullptr;
    }
    assert(current_time >= 0);
    return std::unique_ptr<CompactionFilter>(
        new BlobIndexCompactionFilter(static_cast<uint64_t>(current_time)));
  }

 private:
  Env* env_;
};

}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //摇滚乐
