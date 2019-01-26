
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

#ifndef STORAGE_ROCKSDB_INCLUDE_TRANSACTION_LOG_ITERATOR_H_
#define STORAGE_ROCKSDB_INCLUDE_TRANSACTION_LOG_ITERATOR_H_

#include "rocksdb/status.h"
#include "rocksdb/types.h"
#include "rocksdb/write_batch.h"
#include <memory>
#include <vector>

namespace rocksdb {

class LogFile;
typedef std::vector<std::unique_ptr<LogFile>> VectorLogPtr;

enum  WalFileType {
  /*指示wal文件在存档目录中。wal文件从
   *当主数据库目录不存在时，将其存档并保留
   *在清理之前。根据存档大小清除文件
   （选项：：wal_-size_-limit_-mb）和上次清洗后的时间
   *（选项：：wal_ttl_秒）。
   **/

  kArchivedLogFile = 0,

  /*指示wal文件是活动的并驻留在主db目录中*/
  kAliveLogFile = 1
} ;

class LogFile {
 public:
  LogFile() {}
  virtual ~LogFile() {}

//返回日志文件相对于主db dir的路径名
//例如，对于实时日志文件=/000003.log
//对于存档日志文件=/archive/000003.log
  virtual std::string PathName() const = 0;


//日志文件的主标识符。
//这与日志文件的创建时间成正比。
  virtual uint64_t LogNumber() const = 0;

//日志文件可以是活动的或存档的
  virtual WalFileType Type() const = 0;

//此日志文件中写入的writebatch的起始序列号
  virtual SequenceNumber StartSequence() const = 0;

//磁盘上日志文件的大小（字节）
  virtual uint64_t SizeFileBytes() const = 0;
};

struct BatchResult {
  SequenceNumber sequence = 0;
  std::unique_ptr<WriteBatch> writeBatchPtr;

//为“五”规则添加空的“ctor”和“dtor”
//但是，保留原始语义并禁止复制
//因为唯一的指针成员不复制。
  BatchResult() {}

  ~BatchResult() {}

  BatchResult(const BatchResult&) = delete;

  BatchResult& operator=(const BatchResult&) = delete;

  BatchResult(BatchResult&& bResult)
      : sequence(std::move(bResult.sequence)),
        writeBatchPtr(std::move(bResult.writeBatchPtr)) {}

  BatchResult& operator=(BatchResult&& bResult) {
    sequence = std::move(bResult.sequence);
    writeBatchPtr = std::move(bResult.writeBatchPtr);
    return *this;
  }
};

//TransactionLogiterator用于迭代数据库中的事务。
//迭代器的一次运行是连续的，即迭代器将在
//序列中任何间隙的开始
class TransactionLogIterator {
 public:
  TransactionLogIterator() {}
  virtual ~TransactionLogIterator() {}

//迭代器定位在WRITEBATCH或无效。
//如果迭代器有效，则此方法返回true。
//无法从有效的迭代器读取数据。
  virtual bool Valid() = 0;

//将迭代器移动到下一个writebatch。
//要求：valid（）为true。
  virtual void Next() = 0;

//如果迭代器有效，则返回OK。
//当发生错误时返回错误。
  virtual Status status() = 0;

//如果有效返回是当前写入批和
//批处理中包含的最早事务。
//仅当valid（）为true且status（）为ok时使用。
  virtual BatchResult GetBatch() = 0;

//TransactionLogiterator的读取选项。
  struct ReadOptions {
//如果为true，则从基础存储中读取的所有数据将
//根据相应的校验和进行验证。
//默认值：真
    bool verify_checksums_;

    ReadOptions() : verify_checksums_(true) {}

    explicit ReadOptions(bool verify_checksums)
        : verify_checksums_(verify_checksums) {}
  };
};
} //命名空间rocksdb

#endif  //存储块包括事务日志迭代器
