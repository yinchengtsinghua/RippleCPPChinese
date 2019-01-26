
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
//
//不能包含在任何.h文件中，以避免污染命名空间
//用宏。

#pragma once
#include "port/port.h"

//包含文件名和行号信息的helper宏
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define PREPEND_FILE_LINE(FMT) ("[" __FILE__ ":" TOSTRING(__LINE__) "] " FMT)

//不在标题级别包含文件/行信息
#define ROCKS_LOG_HEADER(LGR, FMT, ...) \
  rocksdb::Log(InfoLogLevel::HEADER_LEVEL, LGR, FMT, ##__VA_ARGS__)

#define ROCKS_LOG_DEBUG(LGR, FMT, ...)                                 \
  rocksdb::Log(InfoLogLevel::DEBUG_LEVEL, LGR, PREPEND_FILE_LINE(FMT), \
               ##__VA_ARGS__)

#define ROCKS_LOG_INFO(LGR, FMT, ...)                                 \
  rocksdb::Log(InfoLogLevel::INFO_LEVEL, LGR, PREPEND_FILE_LINE(FMT), \
               ##__VA_ARGS__)

#define ROCKS_LOG_WARN(LGR, FMT, ...)                                 \
  rocksdb::Log(InfoLogLevel::WARN_LEVEL, LGR, PREPEND_FILE_LINE(FMT), \
               ##__VA_ARGS__)

#define ROCKS_LOG_ERROR(LGR, FMT, ...)                                 \
  rocksdb::Log(InfoLogLevel::ERROR_LEVEL, LGR, PREPEND_FILE_LINE(FMT), \
               ##__VA_ARGS__)

#define ROCKS_LOG_FATAL(LGR, FMT, ...)                                 \
  rocksdb::Log(InfoLogLevel::FATAL_LEVEL, LGR, PREPEND_FILE_LINE(FMT), \
               ##__VA_ARGS__)

#define ROCKS_LOG_BUFFER(LOG_BUF, FMT, ...) \
  rocksdb::LogToBuffer(LOG_BUF, PREPEND_FILE_LINE(FMT), ##__VA_ARGS__)

#define ROCKS_LOG_BUFFER_MAX_SZ(LOG_BUF, MAX_LOG_SIZE, FMT, ...)      \
  rocksdb::LogToBuffer(LOG_BUF, MAX_LOG_SIZE, PREPEND_FILE_LINE(FMT), \
                       ##__VA_ARGS__)
