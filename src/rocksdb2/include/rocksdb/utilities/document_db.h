
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
#ifndef ROCKSDB_LITE

#include <string>
#include <vector>

#include "rocksdb/utilities/stackable_db.h"
#include "rocksdb/utilities/json_document.h"
#include "rocksdb/db.h"

namespace rocksdb {

//重要提示：documentdb是一项正在进行的工作。它是不稳定的，我们可能
//在不发出警告的情况下更改API。在使用前与RockSDB团队交谈
//生产；

//documentdb是rocksdb之上的一个层，它提供了一个非常简单的JSON API。
//创建数据库时，指定要保留在数据库中的索引列表。
//数据。您可以将JSON文档插入数据库，数据库将自动
//索引的添加到数据库的每个文档都需要有“_id”字段，即
//自动索引，是唯一的主键。所有其他索引都是
//非唯一性。

//注意：JSON中的字段名不允许以“$”或
//包含“。”我们目前不强制执行该规则，但将开始执行
//糟透了。

//光标是执行查询后得到的结果。得到一切
//查询的结果，在valid（）返回true时对光标调用next（）。
class Cursor {
 public:
  Cursor() = default;
  virtual ~Cursor() {}

  virtual bool Valid() const = 0;
  virtual void Next() = 0;
//返回的jsondocument的生命周期是直到下一个（）调用
  virtual const JSONDocument& document() const = 0;
  virtual Status status() const = 0;

 private:
//不允许复制
  Cursor(const Cursor&);
  void operator=(const Cursor&);
};

struct DocumentDBOptions {
  int background_threads = 4;
uint64_t memtable_size = 128 * 1024 * 1024;    //128兆字节
uint64_t cache_size = 1 * 1024 * 1024 * 1024;  //1 GB
};

//todo（icanadi）将'jsondocument*info'参数添加到所有可以
//由调用方用于获取有关调用执行的更多信息（数字
//删除的记录数、更新的记录数等）
class DocumentDB : public StackableDB {
 public:
  struct IndexDescriptor {
//当前，只能在单个字段上定义索引。指定一个
//字段x上的索引，将索引描述设置为json“x:1”
//当前值需要为1，这意味着升序。
//将来，我们还计划支持多个键上的索引，其中
//可以将升序排序（1）与降序排序索引（1）混合使用。
    JSONDocument* description;
    std::string name;
  };

//打开具有指定索引的documentdb。索引列表必须
//完成，即包括数据库中存在的所有索引，主索引除外
//关键指标
//否则，open（）将返回一个错误
  static Status Open(const DocumentDBOptions& options, const std::string& name,
                     const std::vector<IndexDescriptor>& indexes,
                     DocumentDB** db, bool read_only = false);

  explicit DocumentDB(DB* db) : StackableDB(db) {}

//创建新索引。它将在调用期间停止所有写入操作。
//数据库中的所有当前文档都会被扫描并相应的索引项
//被创建
  virtual Status CreateIndex(const WriteOptions& write_options,
                             const IndexDescriptor& index) = 0;

//删除索引。客户负责确保索引
//由当前执行的查询使用
  virtual Status DropIndex(const std::string& name) = 0;

//向数据库中插入文档。文档需要有一个主键“_id”
//可以是字符串或整数。否则写入将失败
//带无效参数。
  virtual Status Insert(const WriteOptions& options,
                        const JSONDocument& document) = 0;

//自动删除与筛选器匹配的所有文档
  virtual Status Remove(const ReadOptions& read_options,
                        const WriteOptions& write_options,
                        const JSONDocument& query) = 0;

//此操作顺序是否：
//1。查找与筛选器匹配的所有文档
//2。对于所有文档，原子性地：
//2.1。应用更新运算符
//2.2。更新辅助索引
//
//目前只支持$set update operator。
//语法为：$set:key1:value1，key2:value2，等等…
//此操作员将文档的key1字段更改为value1，key2更改为
//值2等。即使文档没有条目，也将设置新值
//对于指定的键。
//
//不能更改文档的主键。
//
//更新示例：更新（id:$gt:5，$index:id，$set:启用：真）
  virtual Status Update(const ReadOptions& read_options,
                        const WriteOptions& write_options,
                        const JSONDocument& filter,
                        const JSONDocument& updates) = 0;

//查询必须是一个数组，其中每个元素都是一个运算符。目前
//只支持$filter运算符。$filter运算符的语法为：
//$filter:key1:条件1，key2:条件2等条件可以
//要么是：
//1）单一值，在这种情况下，条件是相等条件，或
//2）定义的运算符，如$gt:4，它将匹配
//键大于4。
//
//支持的运算符有：
//1）$gt——大于
//2）$gte——大于或等于
//3）$lt——小于
//4）$LTE——小于或等于
//如果希望筛选器使用索引，则需要这样指定它：
//$filter:…（条件）…$index:索引_name
//
//实例查询：
//*[$filter:name:john，age:$gte:18，$index:age]
//将返回年龄大于或等于18岁的所有约翰，并将使用
//索引“年龄”以满足查询。
  virtual Cursor* Query(const ReadOptions& read_options,
                        const JSONDocument& query) = 0;
};

}  //命名空间rocksdb
#endif  //摇滚乐
