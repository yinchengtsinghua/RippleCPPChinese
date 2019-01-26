
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2017至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#pragma once

#ifndef ROCKSDB_LITE

#include "rocksdb/db.h"
#include "rocksdb/types.h"

namespace rocksdb {

//与键的特定版本关联的数据。数据库可以在内部
//由于快照的原因，存储同一用户密钥的多个版本，而不是压缩
//正在发生，等等。
struct KeyVersion {
  KeyVersion() : user_key(""), value(""), sequence(0), type(0) {}

  KeyVersion(const std::string& _user_key, const std::string& _value,
             SequenceNumber _sequence, int _type)
      : user_key(_user_key), value(_value), sequence(_sequence), type(_type) {}

  std::string user_key;
  std::string value;
  SequenceNumber sequence;
//todo（ajkr）：我们应该提供一个助手函数，它将int转换为
//描述类型以便于调试的字符串。
  int type;
};

//返回所提供用户密钥范围内所有密钥版本的列表。
//范围包括在内，即[`begin_key`，`end_key`]。
//结果被插入到提供的向量中，`key_versions`。
Status GetAllKeyVersions(DB* db, Slice begin_key, Slice end_key,
                         std::vector<KeyVersion>* key_versions);

}  //命名空间rocksdb

#endif  //摇滚乐
