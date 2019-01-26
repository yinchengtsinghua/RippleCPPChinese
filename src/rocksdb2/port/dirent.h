
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
//有关以下类型/功能的文档，请参阅port_example.h。

#ifndef STORAGE_LEVELDB_PORT_DIRENT_H_
#define STORAGE_LEVELDB_PORT_DIRENT_H_

#ifdef ROCKSDB_PLATFORM_POSIX
#include <dirent.h>
#include <sys/types.h>
#elif defined(OS_WIN)

namespace rocksdb {
namespace port {

struct dirent {
  /*r d_name[_max_path]；/*文件名*/
}；

结构；

dir*opendir（const char*名称）；

dirent*readdir（dir*dirp）；

int closedir（dir*dirp）；

//命名空间端口

使用端口：：dirent；
使用端口：：dir；
使用端口：：opendir；
使用端口：：readdir；
使用端口：：closedir；

//命名空间rocksdb

endif//操作系统赢

endif//存储级别数据库端口目录
