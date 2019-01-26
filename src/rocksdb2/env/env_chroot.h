
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2016至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#pragma once

#if !defined(ROCKSDB_LITE) && !defined(OS_WIN)

#include <string>

#include "rocksdb/env.h"

namespace rocksdb {

//返回一个env，该env转换路径使根目录显示为
//是CeloTydir。chroot目录应该引用现有目录。
Env* NewChrootEnv(Env* base_env, const std::string& chroot_dir);

}  //命名空间rocksdb

#endif  //！定义（RocksDB-Lite）&&！定义（OsWin）
