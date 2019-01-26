
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
//writebatch保存一组更新，以原子方式应用于数据库。
//
//更新按添加顺序应用
//写入批处理。例如，“key”的值为“v3”
//在写入以下批之后：
//
//批量放置（“key”，“v1”）；
//批量删除（“key”）；
//批量放置（“key”，“v2”）；
//批量放置（“key”，“v3”）；
//
//多个线程可以在没有
//外部同步，但如果任何线程可以调用
//非常量方法，访问同一writebatch的所有线程都必须使用
//外部同步。

#ifndef STORAGE_ROCKSDB_INCLUDE_WRITE_BATCH_H_
#define STORAGE_ROCKSDB_INCLUDE_WRITE_BATCH_H_

#include <atomic>
#include <stack>
#include <string>
#include <stdint.h>
#include "rocksdb/status.h"
#include "rocksdb/write_batch_base.h"

namespace rocksdb {

class Slice;
class ColumnFamilyHandle;
struct SavePoints;
struct SliceParts;

struct SavePoint {
size_t size;  //RePig尺寸
int count;    //repu中的元素计数
  uint32_t content_flags;

  SavePoint() : size(0), count(0), content_flags(0) {}

  SavePoint(size_t _size, int _count, uint32_t _flags)
      : size(_size), count(_count), content_flags(_flags) {}

  void clear() {
    size = 0;
    count = 0;
    content_flags = 0;
  }

  bool is_cleared() const { return (size | count | content_flags) == 0; }
};

class WriteBatch : public WriteBatchBase {
 public:
  explicit WriteBatch(size_t reserved_bytes = 0, size_t max_bytes = 0);
  ~WriteBatch() override;

  using WriteBatchBase::Put;
//将映射“key->value”存储在数据库中。
  Status Put(ColumnFamilyHandle* column_family, const Slice& key,
             const Slice& value) override;
  Status Put(const Slice& key, const Slice& value) override {
    return Put(nullptr, key, value);
  }

//put（）的变体，用于收集输出，如writev（2）。关键和价值
//将写入数据库的是
//片。
  Status Put(ColumnFamilyHandle* column_family, const SliceParts& key,
             const SliceParts& value) override;
  Status Put(const SliceParts& key, const SliceParts& value) override {
    return Put(nullptr, key, value);
  }

  using WriteBatchBase::Delete;
//如果数据库包含“key”的映射，请将其删除。否则什么也不做。
  Status Delete(ColumnFamilyHandle* column_family, const Slice& key) override;
  Status Delete(const Slice& key) override { return Delete(nullptr, key); }

//采用sliceparts的变体
  Status Delete(ColumnFamilyHandle* column_family,
                const SliceParts& key) override;
  Status Delete(const SliceParts& key) override { return Delete(nullptr, key); }

  using WriteBatchBase::SingleDelete;
//db:：singledelete（）的writebatch实现。参见DB .H。
  Status SingleDelete(ColumnFamilyHandle* column_family,
                      const Slice& key) override;
  Status SingleDelete(const Slice& key) override {
    return SingleDelete(nullptr, key);
  }

//采用sliceparts的变体
  Status SingleDelete(ColumnFamilyHandle* column_family,
                      const SliceParts& key) override;
  Status SingleDelete(const SliceParts& key) override {
    return SingleDelete(nullptr, key);
  }

  using WriteBatchBase::DeleteRange;
//DB:：DeleteRange（）的WriteBatch实现。参见DB .H。
  Status DeleteRange(ColumnFamilyHandle* column_family, const Slice& begin_key,
                     const Slice& end_key) override;
  Status DeleteRange(const Slice& begin_key, const Slice& end_key) override {
    return DeleteRange(nullptr, begin_key, end_key);
  }

//采用sliceparts的变体
  Status DeleteRange(ColumnFamilyHandle* column_family,
                     const SliceParts& begin_key,
                     const SliceParts& end_key) override;
  Status DeleteRange(const SliceParts& begin_key,
                     const SliceParts& end_key) override {
    return DeleteRange(nullptr, begin_key, end_key);
  }

  using WriteBatchBase::Merge;
//将“value”与数据库中“key”的现有值合并。
//“key->merge（现有，值）”
  Status Merge(ColumnFamilyHandle* column_family, const Slice& key,
               const Slice& value) override;
  Status Merge(const Slice& key, const Slice& value) override {
    return Merge(nullptr, key, value);
  }

//采用sliceparts的变体
  Status Merge(ColumnFamilyHandle* column_family, const SliceParts& key,
               const SliceParts& value) override;
  Status Merge(const SliceParts& key, const SliceParts& value) override {
    return Merge(nullptr, key, value);
  }

  using WriteBatchBase::PutLogData;
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
  Status PutLogData(const Slice& blob) override;

  using WriteBatchBase::Clear;
//清除此批处理中缓冲的所有更新。
  void Clear() override;

//记录批处理的状态，以便将来调用RollbackToSavePoint（）。
//可以多次调用以设置多个保存点。
  void SetSavePoint() override;

//删除此批中的所有条目（Put、Merge、Delete、PutLogData），因为
//最近调用setsavepoint（）并删除最近的保存点。
//如果以前没有调用setsavepoint（），则状态：：NotFound（））
//将被退回。
//否则返回状态：：OK（）。
  Status RollbackToSavePoint() override;

//弹出最近的保存点。
//如果以前没有调用setsavepoint（），则状态：：NotFound（））
//将被退回。
//否则返回状态：：OK（）。
  Status PopSavePoint() override;

//支持对批内容进行迭代。
  class Handler {
   public:
    virtual ~Handler();
//此类中的所有处理程序函数都提供默认实现，因此
//当
//添加新成员函数。

//默认实现只调用不带列系列的Put for
//向后兼容。如果列族不是默认值，
//功能是noop
    virtual Status PutCF(uint32_t column_family_id, const Slice& key,
                         const Slice& value) {
      if (column_family_id == 0) {
//put（）历史上不返回状态。我们不想成为
//向后不兼容，所以我们没有更改返回状态
//（这是公共API）。我们执行一个普通的获取和返回状态：：OK（）。
        Put(key, value);
        return Status::OK();
      }
      return Status::InvalidArgument(
          "non-default column family and PutCF not implemented");
    }
    /*实际空置量（const slice&/*key*/，const slice&/*value*/）

    虚拟状态删除cf（uint32_t column_family_id，const slice&key）
      if（列_family_id==0）
        删除（密钥）；
        返回状态：：OK（）；
      }
      返回状态：：invalidArgument（
          “非默认列族和删除未实现”）；
    }
    虚拟空隙删除（const slice&/*ke*/) {}


    virtual Status SingleDeleteCF(uint32_t column_family_id, const Slice& key) {
      if (column_family_id == 0) {
        SingleDelete(key);
        return Status::OK();
      }
      return Status::InvalidArgument(
          "non-default column family and SingleDeleteCF not implemented");
    }
    /*tual void singledelete（const slice&/*key*/）

    虚拟状态删除范围cf（uint32_t列_family_id，
                                 const slice&begin_key，const slice&end_key）
      返回状态：：InvalidArgument（“DeleteRangeCf Not Implemented”）；
    }

    虚拟状态合并cf（uint32_t column_family_id，const slice&key，
                           常量切片和值）
      if（列_family_id==0）
        合并（键，值）；
        返回状态：：OK（）；
      }
      返回状态：：invalidArgument（
          “非默认列族和未实现的mergecf”）；
    }
    虚拟空隙合并（常量切片&/*ke*/, const Slice& /*value*/) {}


    /*实际状态putblobindexcf（uint32_t/*列_family_id*/，
                                  常量切片&/*ke*/,

                                  /*st slice&/*值*/）
      返回状态：：InvalidArgument（“PutBloBindExcf未实现”）；
    }

    //logdata的默认实现不起作用。
    虚拟void logdata（const slice&blob）；

    虚拟状态markBeginPrepare（）
      返回状态：：InvalidArgument（“MarkBeginPrepare（）处理程序未定义”）；
    }

    虚拟状态markendprepare（const slice&xid）
      返回状态：：InvalidArgument（“markendPrepare（）处理程序未定义。”）；
    }

    虚拟状态标记回滚（const slice&xid）
      返回状态：：invalidArgument（
          “未定义markrollbackrepare（）处理程序。”）；
    }

    虚拟状态标记提交（const slice&xid）
      返回status:：invalidArgument（“markcommit（）处理程序未定义。”）；
    }

    //Continue由WRITEBATCH:：Iterate调用。如果返回错误，
    //迭代停止。否则，它将继续迭代。默认值
    //实现始终返回true。
    虚拟bool continue（）；
  }；
  状态迭代（handler*handler）const；

  //检索此批处理的序列化版本。
  const std:：string&data（）const返回rep_

  //检索批的数据大小。
  size_t getdatasize（）const返回rep_.size（）；

  //返回批处理中的更新数
  int count（）常量；

  //如果在迭代期间调用putcf，则返回true
  bool hasput（）常量；

  //如果在迭代期间调用deletecf，则返回true
  bool hasdelete（）常量；

  //如果在迭代期间调用singleDeleteCf，则返回true
  bool hassingledelete（）常量；

  //如果在迭代期间调用DeleteRangeCf，则返回true
  bool hasDeleteRange（）常量；

  //如果在迭代期间调用mergecf，则返回true
  bool hasmerge（）常量；

  //如果在迭代期间调用MarkBeginPrepare，则返回true
  bool hasbeginPrepare（）常量；

  //如果在迭代期间调用markendprepare，则返回true
  bool hasendpare（）常量；

  //如果在迭代期间调用markcommit，则返回trie
  bool hascmit（）常量；

  //如果在迭代期间调用markrollback，则返回trie
  bool hasrollback（）常量；

  使用WRITEBATCHBase:：GetWRITEBATCH；
  writebatch*getwritebatch（）重写返回此；

  //具有序列化字符串对象的构造函数
  显式writebatch（const std:：string&rep）；

  writebatch（const writebatch&src）；
  WRITEBATCH（WRITEBATCH和SRC）；
  writebatch&operator=（const writebatch&src）；
  writebatch&operator=（writebatch&src）；

  //将writebatch中的这一点标记为
  //如果启用了wal，则插入wal
  void markwalterminationpoint（）；
  const savepoint&getwalterminationpoint（）const返回wal-term u-point

  void setmaxbytes（size_t max_bytes）override max_bytes_=max_bytes；

 私人：
  友元类WriteBatchInternal；
  朋友类localsavepoint；
  保存点*保存点

  //通过writeimpl发送writebatch时，我们可能希望
  //指定只将批处理的前X条记录写入
  /沃尔。
  保存点Wal_Term_Point；

  //对于hasxyz。可变以允许延迟计算结果
  可变std:：atomic<uint32_t>content_flags_uu；

  //必要时执行内容标记的延迟计算
  uint32_t computeContentFlags（）常量；

  //代表的最大大小。
  大小\u t最大\u字节\；

 受保护的：
  std:：string rep_；//rep_的格式见write_batch.cc中的注释

  //有意复制
}；

//命名空间rocksdb

endif//存储\u rocksdb_包括\u write_batch_h_
