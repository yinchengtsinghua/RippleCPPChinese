
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//迭代器从源中生成一个键/值对序列。
//下面的类定义接口。多个实现
//由本图书馆提供。特别是，提供了迭代器
//访问表或数据库的内容。
//
//多个线程可以在迭代器上调用const方法，而不需要
//外部同步，但如果任何线程可以调用
//非常量方法，访问同一迭代器的所有线程都必须使用
//外部同步。

#ifndef STORAGE_ROCKSDB_INCLUDE_ITERATOR_H_
#define STORAGE_ROCKSDB_INCLUDE_ITERATOR_H_

#include <string>
#include "rocksdb/cleanable.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"

namespace rocksdb {

class Iterator : public Cleanable {
 public:
  Iterator() {}
  virtual ~Iterator() {}

//迭代器要么定位在键/值对上，要么
//无效。如果迭代器有效，则此方法返回true。
  virtual bool Valid() const = 0;

//在震源中的第一个键处定位。迭代器有效（）
//调用iff后，源不为空。
  virtual void SeekToFirst() = 0;

//定位在源中的最后一个键。迭代器是
//在此调用之后，如果源不是空的，则返回valid（）。
  virtual void SeekToLast() = 0;

//在源中的第一个键处定位，该键位于或超过目标
//在调用源包含的iff之后，迭代器是有效的（）。
//到达或超过目标的条目。
  virtual void Seek(const Slice& target) = 0;

//在源中最后一个键的位置
//在调用源包含的iff之后，迭代器是有效的（）。
//到达目标或在目标之前的条目。
  virtual void SeekForPrev(const Slice& target) {}

//移动到源中的下一个条目。在此调用之后，valid（）是
//如果迭代器未定位在源中的最后一个条目，则返回true。
//要求：有效（）
  virtual void Next() = 0;

//移动到源中的上一个条目。在此调用之后，valid（）是
//如果迭代器未定位在源中的第一个条目，则返回true。
//要求：有效（）
  virtual void Prev() = 0;

//返回当前条目的键。的基础存储
//返回的切片仅在下一次修改
//迭代器。
//要求：有效（）
  virtual Slice key() const = 0;

//返回当前条目的值。的基础存储
//返回的切片仅在下一次修改
//迭代器。
//要求：！（）启动（）
  virtual Slice value() const = 0;

//如果发生错误，请返回。否则返回OK状态。
//如果请求非阻塞IO，而此操作不能
//如果不做一些IO就满意了，那么这将返回status:：incomplete（）。
  virtual Status status() const = 0;

//如果支持，请更新迭代器以表示最新状态。这个
//迭代器将在调用后失效。不支持如果
//创建迭代器时提供readoptions.snapshot。
  virtual Status Refresh() {
    return Status::NotSupported("Refresh() is not supported");
  }

//属性“rocksdb.iterator.is key pinned”：
//如果返回“1”，则表示key（）返回的切片有效。
//只要迭代器没有被删除。
//如果
//-使用readoptions:：pin_data=true创建的迭代器
//-数据库表是用创建的
//blockBasedTableOptions：：使用_delta_encoding=false。
//属性“rocksdb.iterator.super version number”：
//迭代器使用的LSM版本。与db属性的格式相同
//kCurrentSuperVersionNumber（当前超版本号）。有关详细信息，请参阅其注释。
  virtual Status GetProperty(std::string prop_name, std::string* prop);

 private:
//不允许复制
  Iterator(const Iterator&);
  void operator=(const Iterator&);
};

//返回一个空的迭代器（不产生任何结果）。
extern Iterator* NewEmptyIterator();

//返回具有指定状态的空迭代器。
extern Iterator* NewErrorIterator(const Status& status);

}  //命名空间rocksdb

#endif  //存储块包括迭代器
