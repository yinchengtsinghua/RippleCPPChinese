
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

#pragma once

#include <cstddef>

namespace rocksdb {

class Slice;
class Status;
class ColumnFamilyHandle;
class WriteBatch;
struct SliceParts;

//定义写入批处理的基本接口的抽象基类。
//有关基本实现，请参阅writebatch，对于
//索引实施。
class WriteBatchBase {
 public:
  virtual ~WriteBatchBase() {}

//将映射“key->value”存储在数据库中。
  virtual Status Put(ColumnFamilyHandle* column_family, const Slice& key,
                     const Slice& value) = 0;
  virtual Status Put(const Slice& key, const Slice& value) = 0;

//put（）的变体，用于收集输出，如writev（2）。关键和价值
//将写入数据库的是
//片。
  virtual Status Put(ColumnFamilyHandle* column_family, const SliceParts& key,
                     const SliceParts& value);
  virtual Status Put(const SliceParts& key, const SliceParts& value);

//将“value”与数据库中“key”的现有值合并。
//“key->merge（现有，值）”
  virtual Status Merge(ColumnFamilyHandle* column_family, const Slice& key,
                       const Slice& value) = 0;
  virtual Status Merge(const Slice& key, const Slice& value) = 0;

//采用sliceparts的变体
  virtual Status Merge(ColumnFamilyHandle* column_family, const SliceParts& key,
                       const SliceParts& value);
  virtual Status Merge(const SliceParts& key, const SliceParts& value);

//如果数据库包含“key”的映射，请将其删除。否则什么也不做。
  virtual Status Delete(ColumnFamilyHandle* column_family,
                        const Slice& key) = 0;
  virtual Status Delete(const Slice& key) = 0;

//采用sliceparts的变体
  virtual Status Delete(ColumnFamilyHandle* column_family,
                        const SliceParts& key);
  virtual Status Delete(const SliceParts& key);

//如果数据库包含“key”的映射，请将其删除。希望
//未覆盖密钥。否则什么也不做。
  virtual Status SingleDelete(ColumnFamilyHandle* column_family,
                              const Slice& key) = 0;
  virtual Status SingleDelete(const Slice& key) = 0;

//采用sliceparts的变体
  virtual Status SingleDelete(ColumnFamilyHandle* column_family,
                              const SliceParts& key);
  virtual Status SingleDelete(const SliceParts& key);

//如果数据库包含范围[“begin_key”，“end_key”]内的映射，
//擦掉它们。否则什么也不做。
  virtual Status DeleteRange(ColumnFamilyHandle* column_family,
                             const Slice& begin_key, const Slice& end_key) = 0;
  virtual Status DeleteRange(const Slice& begin_key, const Slice& end_key) = 0;

//采用sliceparts的变体
  virtual Status DeleteRange(ColumnFamilyHandle* column_family,
                             const SliceParts& begin_key,
                             const SliceParts& end_key);
  virtual Status DeleteRange(const SliceParts& begin_key,
                             const SliceParts& end_key);

//将任意大小的blob附加到此批处理中的记录。斑点将
//存储在事务日志中，但不存储在任何其他文件中。特别地，
//它不会被保存到sst文件中。当遍历此时
//WRITEBATCH、WRITEBATCH:：Handler:：LogData将与内容一起调用
//所遇到的blob。Blob、Puts、Deletes和Merges将
//按插入顺序遇到。斑点将
//不使用序列号并且不会增加批的计数
//
//示例应用程序：将时间戳添加到事务日志中以在中使用
//复制。
  virtual Status PutLogData(const Slice& blob) = 0;

//清除此批处理中缓冲的所有更新。
  virtual void Clear() = 0;

//将此批转换为writebatch。这是一种抽象的
//将任何writebatchbase（例如writebatchwithindex）转换为基本
//写入批处理。
  virtual WriteBatch* GetWriteBatch() = 0;

//记录批处理的状态，以便将来调用RollbackToSavePoint（）。
//可以多次调用以设置多个保存点。
  virtual void SetSavePoint() = 0;

//删除此批中的所有条目（Put、Merge、Delete、PutLogData），因为
//最近调用setsavepoint（）并删除最近的保存点。
//
//清除（）
  virtual Status RollbackToSavePoint() = 0;

//弹出最近的保存点。
//如果以前没有调用setsavepoint（），则状态：：NotFound（））
//将被退回。
//否则返回状态：：OK（）。
  virtual Status PopSavePoint() = 0;

//以字节为单位设置写入批的最大大小。0表示无限制。
  virtual void SetMaxBytes(size_t max_bytes) = 0;
};

}  //命名空间rocksdb
