
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

#include <memory>
#include <string>
#include <unordered_map>

#include "rocksdb/status.h"

namespace rocksdb {

class Env;
class Logger;

//sstfilemanager用于跟踪数据库中的sst文件并在其中进行控制。
//缺失率。
//所有sstfileManager公共函数都是线程安全的。
class SstFileManager {
 public:
  virtual ~SstFileManager() {}

//更新rocksdb应使用的最大允许空间，如果
//sst文件的总大小超过了允许的最大空间，写入到
//RockSDB将失败。
//
//将“允许的最大空间”设置为0将禁用此功能，允许的最大空间
//空间将是无限的（默认值）。
//
//线程安全。
  virtual void SetMaxAllowedSpaceUsage(uint64_t max_allowed_space) = 0;

//如果sst文件的总大小超过允许的最大值，则返回true
//空间使用。
//
//线程安全。
  virtual bool IsMaxAllowedSpaceReached() = 0;

//返回所有跟踪文件的总大小。
//线程安全
  virtual uint64_t GetTotalSize() = 0;

//返回包含所有跟踪文件和相应大小的映射。
//线程安全
  virtual std::unordered_map<std::string, uint64_t> GetTrackedFiles() = 0;

//返回删除速率限制（字节/秒）。
//线程安全
  virtual int64_t GetDeleteRateBytesPerSecond() = 0;

//更新删除速率限制（字节/秒）。
//零表示禁用删除速率限制并立即删除文件
//线程安全
  virtual void SetDeleteRateBytesPerSecond(int64_t delete_rate) = 0;
};

//创建可在多个RocksDB之间共享的新sstfileManager
//跟踪sst文件并控制删除率的实例。
//
//@param env:指向env对象的指针，请参见“rocksdb/env.h”。
//@param info_log:如果不是nullptr，将使用info_log记录错误。
//
//=删除率限制特定参数==
//@param trash_dir:将删除文件移动到的目录的路径
//在应用速率限制时在后台线程中删除。如果这样
//目录不存在，将创建该目录。此目录不应
//由任何其他进程或任何其他sstfileManager使用，设置为“”到
//禁用删除速率限制。
//@param rate_bytes_per_sec:如果
//此值设置为1024（1 KB/秒），我们删除了一个大小为4 KB的文件
//1秒钟后，我们将再等待3秒钟，然后删除其他
//文件，设置为0以禁用删除率限制。
//@param delete_existing_trash:如果设置为true，则新创建的
//sstfilemanager将删除垃圾桶目录中已存在的文件。
//@param status:如果不是nullptr，则status将包含发生的任何错误。
//创建丢失的垃圾桶目录或删除垃圾桶中的现有文件时。
extern SstFileManager* NewSstFileManager(
    Env* env, std::shared_ptr<Logger> info_log = nullptr,
    std::string trash_dir = "", int64_t rate_bytes_per_sec = 0,
    bool delete_existing_trash = true, Status* status = nullptr);

}  //命名空间rocksdb
