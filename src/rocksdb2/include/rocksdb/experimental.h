
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

#include "rocksdb/db.h"
#include "rocksdb/status.h"

namespace rocksdb {
namespace experimental {

//仅用于平整压实
Status SuggestCompactRange(DB* db, ColumnFamilyHandle* column_family,
                           const Slice* begin, const Slice* end);
Status SuggestCompactRange(DB* db, const Slice* begin, const Slice* end);

//将所有l0文件移动到目标级跳过压缩。
//仅当l0中的文件具有不相交的范围时，此操作才成功；此
//例如，如果键插入到排序的
//秩序。此外，1和目标级别之间的所有级别都必须为空。
//如果违反上述任何条件，则invalidArgument将
//返回。
Status PromoteL0(DB* db, ColumnFamilyHandle* column_family,
                 int target_level = 1);

}  //命名空间实验
}  //命名空间rocksdb
