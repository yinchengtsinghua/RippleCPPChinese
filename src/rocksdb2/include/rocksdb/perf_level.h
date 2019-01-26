
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

#ifndef INCLUDE_ROCKSDB_PERF_LEVEL_H_
#define INCLUDE_ROCKSDB_PERF_LEVEL_H_

#include <stdint.h>
#include <string>

namespace rocksdb {

//要收集多少性能统计信息。影响性能上下文和iostats上下文。
enum PerfLevel : unsigned char {
kUninitialized = 0,             //未知设置
kDisable = 1,                   //禁用性能状态
kEnableCount = 2,               //仅启用计数统计
kEnableTimeExceptForMutex = 3,  //除了计数统计之外，还启用时间
//除互斥体之外的状态
kEnableTime = 4,                //启用计数和时间统计
kOutOfBounds = 5                //注意：必须始终是最后一个值！
};

//设置当前线程的性能统计级别
void SetPerfLevel(PerfLevel level);

//获取当前线程的当前性能统计级别
PerfLevel GetPerfLevel();

}  //命名空间rocksdb

#endif  //包括RocksDB性能级别
