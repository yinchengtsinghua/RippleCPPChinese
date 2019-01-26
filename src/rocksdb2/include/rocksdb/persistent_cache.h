
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2013，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
#pragma once

#include <stdint.h>
#include <memory>
#include <string>

#include "rocksdb/env.h"
#include "rocksdb/slice.h"
#include "rocksdb/statistics.h"
#include "rocksdb/status.h"

namespace rocksdb {

//持久缓存
//
//用于在持久介质上缓存IO页的持久缓存接口。这个
//缓存接口专门为持久读缓存设计。
class PersistentCache {
 public:
  typedef std::vector<std::map<std::string, double>> StatsType;

  virtual ~PersistentCache() {}

//插入到页面缓存
//
//page_键标识符，用于在重新启动时唯一标识页面
//数据页数据
//页面大小
  virtual Status Insert(const Slice& key, const char* data,
                        const size_t size) = 0;

//按页标识符查找页缓存
//
//页\键页标识符
//应复制数据的buf缓冲区
//页面大小
  virtual Status Lookup(const Slice& key, std::unique_ptr<char[]>* data,
                        size_t* size) = 0;

//缓存是否存储未压缩的数据？
//
//如果缓存配置为存储未压缩的数据，则为true，否则为false
  virtual bool IsCompressed() = 0;

//返回属性作为字符串的映射，每层双
//
//持久缓存可以初始化为一层缓存。统计数据是
//轮胎自上而下
  virtual StatsType Stats() = 0;

  virtual std::string GetPrintableOptions() const = 0;
};

//创建新持久缓存的factor方法
Status NewPersistentCache(Env* const env, const std::string& path,
                          const uint64_t size,
                          const std::shared_ptr<Logger>& log,
                          const bool optimized_for_nvm,
                          std::shared_ptr<PersistentCache>* cache);
}  //命名空间rocksdb
