
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

//此文件包含rocksdb选项的实用程序功能。
#pragma once

#ifndef ROCKSDB_LITE

#include <string>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"

namespace rocksdb {
//通过加载
//存储在指定rocksdb数据库中的最新rocksdb选项文件。
//
//请注意，所有指针选项（表工厂除外，它将
//将使用默认值初始化
//价值观。在这个函数调用之后，开发人员可以进一步初始化它们。
//下面是将初始化的指针选项的示例列表
//
//＊Env
//*Memtable_工厂
//*压实过滤器工厂
//*前缀提取程序
//*比较器
//*合并运算符
//*压实过滤器
//
//对于表工厂，此函数进一步支持反序列化
//BlockBasedTableFactory及其BlockBasedTable选项，除了
//blockbasedtableoptions的指针选项（flush块策略工厂，
//块缓存和块缓存已压缩），将用初始化
//默认值。开发人员可以通过以下方式进一步指定这三个选项：
//将TableFactory:：GetOptions（）的返回值强制转换为
//BlockBasedTableOptions并进行必要的更改。
//
//如果要忽略选项，可以将ignore_unknown_选项设置为true
//这是来自更新版本的数据库，主要用于转发
//兼容性。
//
//examples/options_file_example.cc演示如何使用此函数
//打开RockSDB实例。
//
//@RETURN函数成功运行时返回OK状态。如果
//指定的“dbpath”不包含任何选项文件，然后
//将返回状态：：NotFound。返回值，而不是
//状态：：确定或状态：：未找到表示存在一些与错误相关的
//到选项文件本身。
//
//@从文件中查看加载选项
Status LoadLatestOptions(const std::string& dbpath, Env* env,
                         DBOptions* db_options,
                         std::vector<ColumnFamilyDescriptor>* cf_descs,
                         bool ignore_unknown_options = false);

//与loadLatestOptions类似，此函数构造dbOptions
//和基于指定RockSDB选项文件的ColumnFamilyDescriptor。
//
//@参见加载延迟停止
Status LoadOptionsFromFile(const std::string& options_file_name, Env* env,
                           DBOptions* db_options,
                           std::vector<ColumnFamilyDescriptor>* cf_descs,
                           bool ignore_unknown_options = false);

//返回指定db路径下的最新选项文件名。
Status GetLatestOptionsFileName(const std::string& dbpath, Env* env,
                                std::string* options_file_name);

//如果输入dboptions和columnFamilyDescriptor
//与存储在指定数据库路径中的最新选项兼容。
//
//如果返回状态为非OK，则表示指定的rocksdb实例
//可能无法用选项的输入集正确打开。目前，
//更改以下选项之一将使兼容性检查失败：
//
//*比较器
//*前缀提取程序
//*表工厂
//*合并运算符
Status CheckOptionsCompatibility(
    const std::string& dbpath, Env* env, const DBOptions& db_options,
    const std::vector<ColumnFamilyDescriptor>& cf_descs,
    bool ignore_unknown_options = false);

}  //命名空间rocksdb
#endif  //！摇滚乐
