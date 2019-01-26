
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
#include <memory>

#include "rocksdb/table_properties.h"

namespace rocksdb {

//创建标记SST的表属性收集器的工厂
//当至少观察到“d”删除时，按需压缩文件
//任何“n”连续实体中的条目。
//
//@param sliding_window_size“n”。注意这个号码是
//四舍五入到128的最小倍数
//大于指定的大小。
//@param deletion_触发器“d”。注意，即使“n”被更改，
//“d”的指定数字将不会更改。
extern std::shared_ptr<TablePropertiesCollectorFactory>
    NewCompactOnDeletionCollectorFactory(
        size_t sliding_window_size,
        size_t deletion_trigger);
}  //命名空间rocksdb

#endif  //！摇滚乐
