
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#ifndef STORAGE_LEVELDB_PORT_UTIL_LOGGER_H_
#define STORAGE_LEVELDB_PORT_UTIL_LOGGER_H_

//在下面包含适当的平台特定文件。如果你是
//移植到新平台，有关文档，请参阅“port_example.h”
//新的port platform>.h文件必须提供的内容。

#if defined(ROCKSDB_PLATFORM_POSIX)
#include "env/posix_logger.h"
#elif defined(OS_WIN)
#include "port/win/win_logger.h"
#endif

#endif  //存储级别B端口利用记录器
