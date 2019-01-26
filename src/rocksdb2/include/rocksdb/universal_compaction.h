
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

#ifndef STORAGE_ROCKSDB_UNIVERSAL_COMPACTION_OPTIONS_H
#define STORAGE_ROCKSDB_UNIVERSAL_COMPACTION_OPTIONS_H

#include <stdint.h>
#include <climits>
#include <vector>

namespace rocksdb {

//
//用于发出压缩请求的算法停止拾取新文件
//一次压实
//
enum CompactionStopStyle {
kCompactionStopStyleSimilarSize, //选取相似大小的文件
kCompactionStopStyleTotalSize    //已选取文件的总大小>下一个文件
};

class CompactionOptionsUniversal {
 public:

//比较文件大小时的灵活性百分比。如果候选文件
//大小比下一个文件的大小小1%，然后将下一个文件包含到
//这个候选集。/默认值：1
  unsigned int size_ratio;

//一次压缩运行中的最小文件数。默认值：2
  unsigned int min_merge_width;

//一次压缩运行中的最大文件数。默认值：uint_max
  unsigned int max_merge_width;

//尺寸放大定义为
//在数据库中存储单字节数据所需的附加存储。
//例如，2%的大小放大意味着
//包含100字节的用户数据最多可占用102字节
//物理存储。根据这个定义，完全压缩的数据库
//尺寸放大0%。RockSDB使用以下启发式方法
//计算大小放大：假设所有文件
//最早的文件有助于尺寸放大。
//默认值：200，这意味着100字节的数据库可能需要
//300字节的存储空间。
  unsigned int max_size_amplification_percent;

//如果此选项设置为-1（默认值），则所有输出文件
//将遵循指定的压缩类型。
//
//如果此选项不是负数，我们将尝试确保压缩
//大小刚好高于此值。在正常情况下，至少这个百分比
//数据将被压缩。
//当我们压缩到一个新文件时，以下是标准
//它需要压缩：假设这里是排序的文件列表
//按生成时间：
//A1…安B1…BM C1…CT
//其中a1是最新的，ct是最老的，我们要压缩
//b1…bm，我们将所有文件的总大小计算为总大小，如
//以及c1…ct的总大小作为total_c，压缩输出文件
//将被压缩iff
//总容量/总容量<这个百分比
//默认值：-1
  int compression_size_percent;

//用于停止在单个压缩运行中拾取文件的算法
//默认值：kCompactionStopStyleTotalSize
  CompactionStopStyle stop_style;

//通过启用
//非重叠文件的简单移动。
//默认：假
  bool allow_trivial_move;

//默认参数集
  CompactionOptionsUniversal()
      : size_ratio(1),
        min_merge_width(2),
        max_merge_width(UINT_MAX),
        max_size_amplification_percent(200),
        compression_size_percent(-1),
        stop_style(kCompactionStopStyleTotalSize),
        allow_trivial_move(false) {}
};

}  //命名空间rocksdb

#endif  //储存区
