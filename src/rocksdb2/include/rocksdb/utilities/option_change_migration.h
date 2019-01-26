
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

#include <string>
#include "rocksdb/options.h"
#include "rocksdb/status.h"

namespace rocksdb {
//尝试迁移使用旧选项创建的数据库以使用新选项。
//不支持多个列族。
//这是最好的努力。没有成功的保证。
//可以执行完全压缩。
Status OptionChangeMigration(std::string dbname, const Options& old_opts,
                             const Options& new_opts);
}  //命名空间rocksdb
