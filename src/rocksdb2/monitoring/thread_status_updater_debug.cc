
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

#include <mutex>

#include "db/column_family.h"
#include "monitoring/thread_status_updater.h"

namespace rocksdb {

#ifndef NDEBUG
#ifdef ROCKSDB_USING_THREAD_STATUS
void ThreadStatusUpdater::TEST_VerifyColumnFamilyInfoMap(
    const std::vector<ColumnFamilyHandle*>& handles,
    bool check_exist) {
  std::unique_lock<std::mutex> lock(thread_list_mutex_);
  if (check_exist) {
    assert(cf_info_map_.size() == handles.size());
  }
  for (auto* handle : handles) {
    auto* cfd = reinterpret_cast<ColumnFamilyHandleImpl*>(handle)->cfd();
    auto iter __attribute__((unused)) = cf_info_map_.find(cfd);
    if (check_exist) {
      assert(iter != cf_info_map_.end());
      assert(iter->second);
      assert(iter->second->cf_name == cfd->GetName());
    } else {
      assert(iter == cf_info_map_.end());
    }
  }
}

#else

void ThreadStatusUpdater::TEST_VerifyColumnFamilyInfoMap(
    const std::vector<ColumnFamilyHandle*>& handles,
    bool check_exist) {
}

#endif  //使用线程状态的rocksdb_
#endif  //！调试程序


}  //命名空间rocksdb
