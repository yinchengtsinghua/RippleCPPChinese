
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
//这个文件实现了Java和C++之间的回调“桥”。
//RocksDB:统计

#ifndef JAVA_ROCKSJNI_STATISTICSJNI_H_
#define JAVA_ROCKSJNI_STATISTICSJNI_H_

#include <memory>
#include <set>
#include <string>
#include "rocksdb/statistics.h"
#include "monitoring/statistics.h"

namespace rocksdb {

  class StatisticsJni : public StatisticsImpl {
   public:
     StatisticsJni(std::shared_ptr<Statistics> stats);
     StatisticsJni(std::shared_ptr<Statistics> stats,
         const std::set<uint32_t> ignore_histograms);
     virtual bool HistEnabledForType(uint32_t type) const override;

   private:
     const std::set<uint32_t> m_ignore_histograms;
 };

}  //命名空间rocksdb

#endif  //爪哇Rocksjni_统计学家jni_h_
