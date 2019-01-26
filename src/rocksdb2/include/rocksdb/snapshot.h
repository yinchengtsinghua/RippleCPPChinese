
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

#include "rocksdb/types.h"

namespace rocksdb {

class DB;

//数据库特定状态的抽象句柄。
//快照是不可变的对象，因此可以安全地
//在没有任何外部同步的情况下从多个线程访问。
//
//要创建快照，请调用db:：getSnapshot（）。
//要销毁快照，请调用db:：releaseSnapshot（快照）。
class Snapshot {
 public:
//返回快照的序列号
  virtual SequenceNumber GetSequenceNumber() const = 0;

 protected:
  virtual ~Snapshot();
};

//快照的简单raii包装类。
//构造此对象将创建快照。毁灭意志
//释放快照。
class ManagedSnapshot {
 public:
  explicit ManagedSnapshot(DB* db);

//不创建快照，而是取得输入快照的所有权。
  ManagedSnapshot(DB* db, const Snapshot* _snapshot);

  ~ManagedSnapshot();

  const Snapshot* snapshot();

 private:
  DB* db_;
  const Snapshot* snapshot_;
};

}  //命名空间rocksdb
