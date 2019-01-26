
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
#include <unordered_map>

#ifndef ROCKSDB_LITE
namespace rocksdb {
//此枚举定义rocksdb选项的健全级别。
enum OptionsSanityCheckLevel : unsigned char {
//完全不执行健全性检查。
  kSanityLevelNone = 0x00,
//执行最小检查以确保rocksdb实例
//打开时不会损坏/错误解释数据。
  kSanityLevelLooselyCompatible = 0x01,
//执行精确的匹配健全性检查。
  kSanityLevelExactMatch = 0xFF,
};

//数据库选项的健全性检查级别
static const std::unordered_map<std::string, OptionsSanityCheckLevel>
    sanity_level_db_options {};

//列族选项的健全性检查级别
static const std::unordered_map<std::string, OptionsSanityCheckLevel>
    sanity_level_cf_options = {
        {"comparator", kSanityLevelLooselyCompatible},
        {"table_factory", kSanityLevelLooselyCompatible},
        {"merge_operator", kSanityLevelLooselyCompatible}};

//基于块的表选项的健全性检查级别
static const std::unordered_map<std::string, OptionsSanityCheckLevel>
    sanity_level_bbt_options {};

OptionsSanityCheckLevel DBOptionSanityCheckLevel(
    const std::string& options_name);
OptionsSanityCheckLevel CFOptionSanityCheckLevel(
    const std::string& options_name);
OptionsSanityCheckLevel BBTOptionSanityCheckLevel(
    const std::string& options_name);

}  //命名空间rocksdb

#endif  //！摇滚乐
